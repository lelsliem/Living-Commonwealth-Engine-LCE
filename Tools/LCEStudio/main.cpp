//=============================================================================//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │
// │                       ██╗      ██████╗███████╗
// │                       ██║     ██╔════╝██╔════╝
// │                       ██║     ██║     █████╗
// │                       ██║     ██║     ██╔══╝
// │                       ███████╗╚██████╗███████╗
// │                       ╚══════╝ ╚═════╝╚══════╝
// │
// │            Building living worlds through simulation.
// │
// │          “The core never knows; the window only watches the bus.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      main.cpp
//
// Purpose:
//
//      LCE Studio (0.8.9) — the observation window, scoped to be a beta
//      companion: a live event feed, an entity table, a mind inspector,
//      and a tuning cockpit, attached to the core through the public
//      API ONLY — exactly the shape the bench proved, with a page.
//
//      The GUI is deliberately zero-dependency: a tiny HTTP server on
//      127.0.0.1 (std sockets, ~70 lines with a platform shim) and ONE
//      embedded HTML page — the browser is the window. No windowing
//      library, no rendering loop, nothing the core must know about.
//      The same shape scales to a live in-game attach later (the page
//      talks to a socket; the socket is whoever listens).
//
//      Threading, taught: the sim ticks on one thread under a mutex;
//      the HTTP server reads the world under the same mutex; the feed
//      is a bounded ring buffer behind its own mutex. One mutex makes
//      the world safe to watch.
//
//      Endpoints:
//        /                  the page
//        /api/entities      every mind: needs, intent, counts
//        /api/mind?id=N     one mind, everything
//        /api/events?since=N  the feed after a cursor
//        /api/tuning        GET the sim.* knobs; POST key=value to turn
//
//      CLI:  LCE.Studio [--port N] [--selftest]
//      --selftest starts on an ephemeral port, makes one internal
//      request, and exits 0 — CI's smoke that the tool runs.
//
// Project:
//
//      Living Commonwealth Engine (LCE)
//
// License:
//
//      MIT License
//
// SPDX-License-Identifier: MIT
//
// Copyright:
//
//      (c) 2026-present LCE Contributors
//=============================================================================//

#include "LCE/Events/EventBus.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/SimulationEvents.h"
#include "LCE/Version/Version.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <typeindex>
#include <vector>

//------------------------------------------------------------------------------
// The platform shim — winsock on Windows, POSIX elsewhere. Everything
// below uses the shim, so the tool is one source on every toolchain.
//------------------------------------------------------------------------------
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace
{
    using namespace LCE::Simulation;
    using namespace LCE::Events;

    //--------------------------------------------------------------------------
    // Sockets.
    //--------------------------------------------------------------------------
#ifdef _WIN32
    using Socket = SOCKET;
    constexpr Socket kInvalidSocket = INVALID_SOCKET;
#else
    using Socket = int;
    constexpr Socket kInvalidSocket = -1;
#endif

    void InitSockets()
    {
#ifdef _WIN32
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
#endif
    }

    int Send(Socket client, const char* data, int length)
    {
#ifdef _WIN32
        return ::send(client, data, length, 0);
#else
        // MSG_NOSIGNAL: a closed browser tab must not kill the tool.
        return static_cast<int>(::send(client, data, length, MSG_NOSIGNAL));
#endif
    }

    int Recv(Socket client, char* buffer, int length)
    {
        return ::recv(client, buffer, length, 0);
    }

    void CloseSocket(Socket client)
    {
#ifdef _WIN32
        closesocket(client);
#else
        close(client);
#endif
    }

    struct Server
    {
        Socket Listener = kInvalidSocket;

        bool Listen(std::uint16_t port)
        {
#ifdef _WIN32
            Listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
            Listener = ::socket(AF_INET, SOCK_STREAM, 0);
#endif

            if (Listener == kInvalidSocket)
            {
                return false;
            }

            int yes = 1;
            setsockopt(
                Listener, SOL_SOCKET, SO_REUSEADDR,
                reinterpret_cast<const char*>(&yes), sizeof(yes));

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = htons(port);

            if (bind(
                    Listener, reinterpret_cast<sockaddr*>(&address),
                    sizeof(address)) != 0)
            {
                return false;
            }

            if (listen(Listener, 8) != 0)
            {
                return false;
            }

            return true;
        }

        // The actually-bound port (needed when 0 = ephemeral).
        std::uint16_t BoundPort() const
        {
            sockaddr_in address{};
            socklen_t length = sizeof(address);
            getsockname(
                Listener, reinterpret_cast<sockaddr*>(&address), &length);
            return ntohs(address.sin_port);
        }

        Socket Accept()
        {
            sockaddr_in client{};
            socklen_t length = sizeof(client);
            return ::accept(
                Listener, reinterpret_cast<sockaddr*>(&client), &length);
        }
    };

    bool SendAll(Socket client, const char* data, std::size_t length)
    {
        std::size_t sent = 0;

        while (sent < length)
        {
            const auto n = Send(
                client, data + sent,
                static_cast<int>(length - sent));

            if (n <= 0)
            {
                return false;
            }

            sent += static_cast<std::size_t>(n);
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // JSON, hand-rolled — the payloads are numeric and the labels are
    // few. Escape the quotes a world-chosen label could carry; nothing
    // else is worth a dependency.
    //--------------------------------------------------------------------------
    std::string JsonEscape(const std::string& text)
    {
        std::string out;

        for (const char c : text)
        {
            if (c == '"' || c == '\\')
            {
                out.push_back('\\');
            }

            out.push_back(c);
        }

        return out;
    }

    std::string TrimFloat(float value)
    {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(value));
        return buffer;
    }

    const char* NeedName(NeedType type)
    {
        switch (type)
        {
        case NeedType::Hunger:
            return "hunger";
        case NeedType::Fatigue:
            return "fatigue";
        case NeedType::Social:
            return "social";
        case NeedType::Safety:
            return "safety";
        case NeedType::Comfort:
            return "comfort";
        }

        return "?";
    }

    const char* ActionName(ActionType action)
    {
        switch (action)
        {
        case ActionType::MoveTo:
            return "MoveTo";
        case ActionType::Rest:
            return "Rest";
        case ActionType::Socialize:
            return "Socialize";
        case ActionType::Explore:
            return "Explore";
        case ActionType::Work:
            return "Work";
        case ActionType::Flee:
            return "Flee";
        }

        return "?";
    }

    const char* KindName(InteractionKind kind)
    {
        switch (kind)
        {
        case InteractionKind::Trade:
            return "Trade";
        case InteractionKind::Combat:
            return "Combat";
        case InteractionKind::Aid:
            return "Aid";
        case InteractionKind::Social:
            return "Social";
        case InteractionKind::Wronged:
            return "Wronged";
        case InteractionKind::Death:
            return "Death";
        case InteractionKind::Fact:
            return "Fact";
        default:
            return "Weather";
        }
    }

    const char* ResultName(OutcomeResult result)
    {
        switch (result)
        {
        case OutcomeResult::Success:
            return "Success";
        case OutcomeResult::Partial:
            return "Partial";
        case OutcomeResult::Failure:
            return "Failure";
        }

        return "?";
    }

    //--------------------------------------------------------------------------
    // The world — everything the HTTP server can look at, and the mutex
    // that makes it safe to watch while the sim ticks.
    //--------------------------------------------------------------------------
    struct World
    {
        // The sim thread ticks under this; the HTTP reads take it too.
        std::mutex Mutex;
        EntityRegistry Registry;
        // Rng has no default constructor — seed the village's stream
        // explicitly, so the same Studio session replays identically.
        Rng Rng{ 0x5EEDC0DEull };
        EventBus Bus;
        SimulationTuning Tuning;

        // The feed ring buffer — its own mutex, so the bus handler
        // (running inside the sim's tick) never contends with the page.
        std::mutex FeedMutex;
        std::vector<std::string> Feed;   // bounded, oldest evicted
        std::uint64_t FeedCount = 0;     // monotonic ids
    };

    void PushFeed(World& world, std::string json)
    {
        std::lock_guard lock(world.FeedMutex);

        if (world.Feed.size() >= 200)
        {
            world.Feed.erase(world.Feed.begin());
        }

        world.Feed.push_back(std::move(json));
        ++world.FeedCount;
    }

    // A village to watch: one trader, thirty settlers with needs,
    // memories of the trader, and relationships to prove the tables.
    void SeedVillage(World& world)
    {
        const auto trader = world.Registry.CreateEntity();

        for (int i = 0; i < 30; ++i)
        {
            const auto mind = world.Registry.CreateEntity();

            world.Registry.AddComponent<Needs>(
                mind,
                Needs{
                    { Need{ NeedType::Hunger, 0.9f, 0.02f },
                      Need{ NeedType::Fatigue, 0.9f, 0.015f },
                      Need{ NeedType::Social, 0.9f, 0.01f } } });

            Remember(
                world.Registry, mind,
                { trader, InteractionKind::Trade, 1.0f });
            Remember(
                world.Registry, mind,
                { trader, InteractionKind::Trade, 0.8f });
            Remember(
                world.Registry, mind,
                { trader, InteractionKind::Social, 0.6f });
        }
    }

    // The bus subscriptions — what the feed is made of. Subscribe once,
    // before the sim starts; every event below rides the public API.
    void WireFeed(World& world)
    {
        world.Bus.Subscribe(
            std::type_index(typeid(EntityCreatedEvent)),
            [&world](const LCE::Events::Event& event)
            {
                const auto& created =
                    static_cast<const EntityCreatedEvent&>(event);
                PushFeed(
                    world,
                    R"({"kind":"EntityCreated","entity":)"
                        + std::to_string(created.Id.Value()) + "}");
            });

        world.Bus.Subscribe(
            std::type_index(typeid(IntentProducedEvent)),
            [&world](const LCE::Events::Event& event)
            {
                const auto& produced =
                    static_cast<const IntentProducedEvent&>(event);
                PushFeed(
                    world,
                    R"({"kind":"Intent","entity":)"
                        + std::to_string(produced.Id.Value())
                        + R"(,"action":")" + ActionName(produced.Intent.Action)
                        + R"(","confidence":)"
                        + TrimFloat(produced.Intent.Confidence)
                        + R"(,"target":)"
                        + std::to_string(produced.Intent.Target.Value()) + "}");
            });

        world.Bus.Subscribe(
            std::type_index(typeid(OutcomeRecordedEvent)),
            [&world](const LCE::Events::Event& event)
            {
                const auto& recorded =
                    static_cast<const OutcomeRecordedEvent&>(event);
                PushFeed(
                    world,
                    R"({"kind":"Outcome","entity":)"
                        + std::to_string(recorded.Id.Value())
                        + R"(,"kind":")" + KindName(recorded.Outcome.Kind)
                        + R"(","result":")"
                        + ResultName(recorded.Outcome.Result) + "\"}");
            });

        world.Bus.Subscribe(
            std::type_index(typeid(RelationshipChangedEvent)),
            [&world](const LCE::Events::Event& event)
            {
                const auto& crossed =
                    static_cast<const RelationshipChangedEvent&>(event);
                PushFeed(
                    world,
                    R"({"kind":"Bond","subject":)"
                        + std::to_string(crossed.Subject.Value())
                        + R"(,"other":)"
                        + std::to_string(crossed.Other.Value())
                        + R"(,"threshold":")" + JsonEscape(crossed.Threshold)
                        + R"(","disposition":)"
                        + TrimFloat(crossed.Disposition) + "}");
            });
    }

    // The sim thread: tick at 20 Hz under the world mutex. FixedStep
    // keeps the cadence exact whatever the scheduler does to us.
    void SimLoop(World& world, const std::atomic<bool>& running)
    {
        FixedStep step;
        step.Step = 0.05;

        while (running.load())
        {
            {
                std::lock_guard lock(world.Mutex);
                step.Advance(
                    0.05, world.Registry, world.Tuning, &world.Bus, &world.Rng);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    //--------------------------------------------------------------------------
    // The endpoints.
    //--------------------------------------------------------------------------
    std::string EntityJson(const EntityRegistry& registry, EntityId id)
    {
        const auto needs = registry.GetComponent<Needs>(id);
        const auto memory = registry.GetComponent<Memory>(id);
        const auto relationships = registry.GetComponent<Relationships>(id);
        const auto intent = registry.GetComponent<Intent>(id);

        std::string out =
            R"({"id":)" + std::to_string(id.Value()) + R"(,"needs":{)";

        bool first = true;

        if (needs)
        {
            for (const auto& need : needs->List)
            {
                if (!first)
                {
                    out += ",";
                }

                first = false;
                out += R"(")" + std::string(NeedName(need.Type))
                    + R"(":)" + TrimFloat(need.Value);
            }
        }

        out += "}";

        out += R"(,"intent":)";

        if (intent)
        {
            out += R"({"action":")" + std::string(ActionName(intent->Action))
                + R"(","confidence":)" + TrimFloat(intent->Confidence)
                + R"(,"target":)"
                + std::to_string(intent->Target.Value()) + "}";
        }
        else
        {
            out += "null";
        }

        out += R"(,"memories":)"
            + std::to_string(memory ? memory->Events.size() : 0);
        out += R"(,"relationships":)"
            + std::to_string(
                relationships ? relationships->ByEntity.size() : 0);
        out += "}";

        return out;
    }

    std::string EntitiesJson(World& world)
    {
        std::lock_guard lock(world.Mutex);

        const auto ids = world.Registry.QueryWhere<Needs>(
            [](EntityId, const Needs&) { return true; });

        std::string out = "[";
        bool first = true;

        for (const auto id : ids)
        {
            if (!first)
            {
                out += ",";
            }

            first = false;
            out += EntityJson(world.Registry, id);
        }

        out += "]";
        return out;
    }

    std::string MindJson(World& world, EntityId id)
    {
        std::lock_guard lock(world.Mutex);

        if (!world.Registry.IsAlive(id))
        {
            return R"({"error":"no such entity"})";
        }

        const auto needs = world.Registry.GetComponent<Needs>(id);
        const auto memory = world.Registry.GetComponent<Memory>(id);
        const auto relationships = world.Registry.GetComponent<Relationships>(id);
        const auto intent = world.Registry.GetComponent<Intent>(id);
        const auto goals = world.Registry.GetComponent<Goals>(id);

        std::string out =
            R"({"id":)" + std::to_string(id.Value()) + R"(,"needs":[)";

        bool first = true;

        if (needs)
        {
            for (const auto& need : needs->List)
            {
                if (!first)
                {
                    out += ",";
                }

                first = false;
                out += R"({"type":")" + std::string(NeedName(need.Type))
                    + R"(","value":)" + TrimFloat(need.Value)
                    + R"(,"decay":)" + TrimFloat(need.DecayRate) + "}";
            }
        }

        out += R"(],"memories":[)";
        first = true;

        if (memory)
        {
            for (const auto& event : memory->Events)
            {
                if (!first)
                {
                    out += ",";
                }

                first = false;
                out += R"({"kind":")" + std::string(KindName(event.Kind))
                    + R"(","other":)" + std::to_string(event.Other.Value())
                    + R"(,"weight":)" + TrimFloat(event.Weight)
                    + R"(,"day":)" + std::to_string(event.Day)
                    + R"(,"label":")" + JsonEscape(event.Label) + "\"}";
            }
        }

        out += R"(],"relationships":[)";
        first = true;

        if (relationships)
        {
            for (const auto& [other, relationship] : relationships->ByEntity)
            {
                if (!first)
                {
                    out += ",";
                }

                first = false;
                out += R"({"other":)" + std::to_string(other.Value())
                    + R"(,"disposition":)" + TrimFloat(relationship.Disposition)
                    + R"(,"trust":)" + TrimFloat(relationship.Trust) + "}";
            }
        }

        out += "],";

        out += R"("intent":)";

        if (intent)
        {
            out += R"({"action":")" + std::string(ActionName(intent->Action))
                + R"(","confidence":)" + TrimFloat(intent->Confidence)
                + R"(,"target":)" + std::to_string(intent->Target.Value()) + "}";
        }
        else
        {
            out += "null";
        }

        out += R"(,"goal":)";

        if (goals && goals->Active)
        {
            out += R"({"urgency":)" + TrimFloat(goals->Active->Urgency) + "}";
        }
        else
        {
            out += "null";
        }

        out += "}";
        return out;
    }

    std::string EventsJson(World& world, std::uint64_t since)
    {
        std::lock_guard lock(world.FeedMutex);

        std::size_t keep = 0;

        if (world.FeedCount > since)
        {
            const auto wanted = world.FeedCount - since;
            keep = static_cast<std::size_t>(
                (std::min)(wanted, static_cast<std::uint64_t>(world.Feed.size())));
        }

        std::string out =
            R"({"since":)" + std::to_string(world.FeedCount) + R"(,"events":[)";

        const auto begin = world.Feed.size() - keep;

        for (std::size_t i = begin; i < world.Feed.size(); ++i)
        {
            if (i != begin)
            {
                out += ",";
            }

            out += world.Feed[i];
        }

        out += "]}";
        return out;
    }

    // The knobs the page can turn — the modder's sim.* keys, a subset
    // that maps cleanly to sliders.
    bool ApplyTuning(SimulationTuning& tuning, const std::string& body)
    {
        const auto eq = body.find('=');

        if (eq == std::string::npos)
        {
            return false;
        }

        const auto key = body.substr(0, eq);
        float value = 0.0f;

        try
        {
            value = std::stof(body.substr(eq + 1));
        }
        catch (...)
        {
            return false;
        }

        if (key == "sim.memory.fade")
        {
            tuning.MemoryFadeRate = value;
        }
        else if (key == "sim.memory.forget")
        {
            tuning.ForgetThreshold = value;
        }
        else if (key == "sim.drift.rate")
        {
            tuning.DriftRate = value;
        }
        else if (key == "sim.jitter")
        {
            tuning.NeedJitter = value;
        }
        else if (key == "sim.trust.gain")
        {
            tuning.TrustGain = value;
        }
        else
        {
            return false;
        }

        return true;
    }

    std::string TuningJson(World& world)
    {
        std::lock_guard lock(world.Mutex);

        std::string out = "{";
        out += R"("sim.memory.fade":)" + TrimFloat(world.Tuning.MemoryFadeRate);
        out += R"(,"sim.memory.forget":)" + TrimFloat(world.Tuning.ForgetThreshold);
        out += R"(,"sim.drift.rate":)" + TrimFloat(world.Tuning.DriftRate);
        out += R"(,"sim.jitter":)" + TrimFloat(world.Tuning.NeedJitter);
        out += R"(,"sim.trust.gain":)" + TrimFloat(world.Tuning.TrustGain);
        out += "}";
        return out;
    }

    //--------------------------------------------------------------------------
    // HTTP.
    //--------------------------------------------------------------------------
    std::string StudioPage();   // defined below; the page is the window

    struct Request
    {
        std::string Method;
        std::string Path;   // without the query
        std::string Query;
        std::string Body;
    };

    bool ParseRequest(const std::string& text, Request& out)
    {
        const auto lineEnd = text.find("\r\n");

        if (lineEnd == std::string::npos)
        {
            return false;
        }

        std::istringstream line{ text.substr(0, lineEnd) };
        line >> out.Method >> out.Path;

        if (out.Path.empty())
        {
            return false;
        }

        const auto query = out.Path.find('?');

        if (query != std::string::npos)
        {
            out.Query = out.Path.substr(query + 1);
            out.Path = out.Path.substr(0, query);
        }

        const auto bodyStart = text.find("\r\n\r\n");

        if (bodyStart != std::string::npos)
        {
            out.Body = text.substr(bodyStart + 4);
        }

        return true;
    }

    std::uint64_t QueryValue(
        const std::string& query,
        const std::string& key,
        std::uint64_t fallback)
    {
        const auto at = query.find(key + "=");

        if (at == std::string::npos)
        {
            return fallback;
        }

        const auto start = at + key.size() + 1;
        const auto end = query.find('&', start);

        try
        {
            return std::stoull(query.substr(start, end - start));
        }
        catch (...)
        {
            return fallback;
        }
    }

    std::string HttpResponse(
        const std::string& body,
        const char* contentType,
        bool notFound = false)
    {
        std::string response =
            notFound ? "HTTP/1.1 404 Not Found\r\n" : "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: " + std::string(contentType) + "\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += body;
        return response;
    }

    void HandleConnection(World& world, Socket client)
    {
        std::string text;
        char buffer[4096];

        // Read until the header terminator.
        for (;;)
        {
            const auto got = Recv(client, buffer, static_cast<int>(sizeof(buffer)));

            if (got <= 0)
            {
                CloseSocket(client);
                return;
            }

            text.append(buffer, static_cast<std::size_t>(got));

            if (text.find("\r\n\r\n") != std::string::npos)
            {
                break;
            }
        }

        Request request;

        if (!ParseRequest(text, request))
        {
            CloseSocket(client);
            return;
        }

        // The POST body follows the headers. The page's tuning POSTs are
        // tiny, but a body can arrive in a later segment than its header
        // — read until we hold the declared Content-Length, or the peer
        // closes.
        const auto lengthMarker = text.find("Content-Length:");

        if (lengthMarker != std::string::npos)
        {
            std::size_t contentLength = 0;
            std::istringstream value{ text.substr(lengthMarker + 15) };
            value >> contentLength;

            const auto headerEnd = text.find("\r\n\r\n") + 4;

            while (text.size() < headerEnd + contentLength)
            {
                const auto got = Recv(
                    client, buffer, static_cast<int>(sizeof(buffer)));

                if (got <= 0)
                {
                    break;
                }

                text.append(buffer, static_cast<std::size_t>(got));
            }
        }

        std::string body;
        const char* type = "text/html";

        if (request.Path == "/" || request.Path == "/index.html")
        {
            body = StudioPage();
        }
        else if (request.Path == "/api/entities")
        {
            body = EntitiesJson(world);
            type = "application/json";
        }
        else if (request.Path == "/api/mind")
        {
            body = MindJson(
                world, EntityId{ QueryValue(request.Query, "id", 0) });
            type = "application/json";
        }
        else if (request.Path == "/api/events")
        {
            body = EventsJson(
                world, QueryValue(request.Query, "since", 0));
            type = "application/json";
        }
        else if (request.Path == "/api/tuning")
        {
            if (request.Method == "POST")
            {
                std::lock_guard lock(world.Mutex);
                ApplyTuning(world.Tuning, request.Body);
            }

            body = TuningJson(world);
            type = "application/json";
        }
        else
        {
            const auto response = HttpResponse(
                "LCE Studio: no such endpoint", "text/plain", true);
            SendAll(client, response.data(), response.size());
            CloseSocket(client);
            return;
        }

        const auto response = HttpResponse(body, type);
        SendAll(client, response.data(), response.size());
        CloseSocket(client);
    }

    //--------------------------------------------------------------------------
    // The page — one embedded HTML document; the browser is the window.
    //--------------------------------------------------------------------------
    std::string StudioPage()
    {
        return R"HTML(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>LCE Studio — the observation window</title>
<style>
  body { background:#0f1115; color:#d7dae0; font-family:'Consolas','Menlo',monospace; margin:0; }
  header { padding:12px 20px; border-bottom:1px solid #2a2f3a; }
  header h1 { font-size:16px; margin:0; color:#7ee787; }
  header p { margin:2px 0 0; font-size:12px; color:#8b949e; }
  main { display:flex; flex-wrap:wrap; gap:12px; padding:12px 20px; }
  .panel { background:#161a22; border:1px solid #2a2f3a; border-radius:6px; padding:10px; min-width:280px; flex:1; }
  .panel h2 { font-size:12px; color:#58a6ff; margin:0 0 8px; text-transform:uppercase; letter-spacing:1px; }
  #feed { height:320px; overflow-y:auto; font-size:12px; }
  .ev { margin:2px 0; color:#c9d1d9; word-break:break-all; }
  .ev .t { color:#8b949e; }
  table { width:100%; border-collapse:collapse; font-size:12px; }
  th, td { text-align:left; padding:3px 6px; border-bottom:1px solid #21262d; }
  th { color:#8b949e; font-weight:normal; }
  tr:hover { background:#1c2129; cursor:pointer; }
  .low { color:#f85149; }
  .mid { color:#d29922; }
  .ok { color:#7ee787; }
  label { display:block; margin:6px 0; font-size:12px; color:#c9d1d9; }
  input[type=range] { width:100%; }
  pre { font-size:12px; white-space:pre-wrap; word-break:break-all; }
</style>
</head>
<body>
<header>
  <h1>LCE Studio — the observation window</h1>
  <p>The core never knows; the window only watches what the bus says. v0.8.9</p>
</header>
<main>
  <div class="panel">
    <h2>Event feed</h2>
    <div id="feed"></div>
  </div>
  <div class="panel">
    <h2>Settlers <span id="count"></span></h2>
    <table id="entities">
      <thead><tr><th>id</th><th>hunger</th><th>fatigue</th><th>social</th><th>intent</th><th>conf</th></tr></thead>
      <tbody></tbody>
    </table>
  </div>
  <div class="panel">
    <h2>Mind inspector <span id="mindId"></span></h2>
    <pre id="mind">select a settler</pre>
  </div>
  <div class="panel">
    <h2>Tuning (sim.*)</h2>
    <div id="tuning"></div>
  </div>
</main>
<script>
let since = 0;
const feedEl = document.getElementById('feed');
const esc = s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');

async function refreshEvents() {
  try {
    const j = await (await fetch('/api/events?since=' + since)).json();
    for (const e of j.events) {
      const line = document.createElement('div');
      line.className = 'ev';
      line.innerHTML = '<span class="t">' + (e.id != null ? '#' + e.id : '') + '</span> ' + esc(JSON.stringify(e));
      feedEl.appendChild(line);
    }
    while (feedEl.childNodes.length > 200) feedEl.removeChild(feedEl.firstChild);
    feedEl.scrollTop = feedEl.scrollHeight;
    since = j.since;
  } catch (e) {}
}

function valClass(v) { return v < 0.3 ? 'low' : v < 0.6 ? 'mid' : 'ok'; }

async function refreshEntities() {
  try {
    const j = await (await fetch('/api/entities')).json();
    document.getElementById('count').textContent = '(' + j.length + ')';
    const tb = document.querySelector('#entities tbody');
    tb.innerHTML = '';
    for (const e of j) {
      const tr = document.createElement('tr');
      const n = e.needs;
      tr.innerHTML = '<td>' + e.id + '</td>' +
        '<td class="' + valClass(n.hunger) + '">' + n.hunger + '</td>' +
        '<td class="' + valClass(n.fatigue) + '">' + n.fatigue + '</td>' +
        '<td class="' + valClass(n.social) + '">' + n.social + '</td>' +
        '<td>' + (e.intent ? e.intent.action : '-') + '</td>' +
        '<td>' + (e.intent ? e.intent.confidence : '-') + '</td>';
      tr.onclick = () => showMind(e.id);
      tb.appendChild(tr);
    }
  } catch (e) {}
}

async function showMind(id) {
  document.getElementById('mindId').textContent = '#' + id;
  try {
    const j = await (await fetch('/api/mind?id=' + id)).json();
    document.getElementById('mind').textContent = JSON.stringify(j, null, 2);
  } catch (e) {}
}

async function refreshTuning() {
  try {
    const j = await (await fetch('/api/tuning')).json();
    const box = document.getElementById('tuning');
    box.innerHTML = '';
    for (const [key, val] of Object.entries(j)) {
      const lab = document.createElement('label');
      lab.textContent = key + ' = ' + val;
      const input = document.createElement('input');
      input.type = 'range';
      input.min = '0';
      input.max = '1';
      input.step = '0.01';
      input.value = val;
      input.onchange = async () => {
        await fetch('/api/tuning', { method:'POST', body: key + '=' + input.value });
        refreshTuning();
      };
      lab.appendChild(input);
      box.appendChild(lab);
    }
  } catch (e) {}
}

setInterval(refreshEvents, 300);
setInterval(refreshEntities, 600);
setInterval(refreshTuning, 2000);
refreshEvents(); refreshEntities(); refreshTuning();
</script>
</body>
</html>
)HTML";
    }

    //--------------------------------------------------------------------------
    // The selftest client — one internal request proves the server.
    //--------------------------------------------------------------------------
    std::string FetchLocal(std::uint16_t port, const std::string& path)
    {
#ifdef _WIN32
        const auto client = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
        const auto client = ::socket(AF_INET, SOCK_STREAM, 0);
#endif

        if (client == kInvalidSocket)
        {
            return {};
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(port);

        if (connect(
                client, reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) != 0)
        {
            CloseSocket(client);
            return {};
        }

        const std::string request =
            "GET " + path
            + " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";

        SendAll(client, request.data(), request.size());

        std::string response;
        char buffer[8192];

        for (;;)
        {
            const auto got = Recv(client, buffer, static_cast<int>(sizeof(buffer)));

            if (got <= 0)
            {
                break;
            }

            response.append(buffer, static_cast<std::size_t>(got));
        }

        CloseSocket(client);

        const auto bodyStart = response.find("\r\n\r\n");

        return bodyStart == std::string::npos
            ? response
            : response.substr(bodyStart + 4);
    }
}

int main(int argc, char** argv)
{
    std::uint16_t port = 8471;
    bool selftest = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];

        if (argument == "--selftest")
        {
            selftest = true;
        }
        else if (argument == "--port" && i + 1 < argc)
        {
            port = static_cast<std::uint16_t>(std::atoi(argv[++i]));
        }
    }

    InitSockets();

    World world;

    // The registry publishes its observation events to the bus, and the
    // feed subscribes BEFORE the village is born — so the seed's births
    // land in the feed, and the window opens on a world that already
    // has history.
    world.Registry.SetEventSink(&world.Bus);
    WireFeed(world);
    SeedVillage(world);

    // The modder's knob, with the same defaults the rest of the engine
    // carries — a Studio session is a normal tick, tunable live.
    world.Tuning.NeedJitter = 0.15f;

    Server server;

    if (!server.Listen(selftest ? 0 : port))
    {
        std::printf(
            "LCE Studio: could not listen on 127.0.0.1:%u\n",
            static_cast<unsigned>(port));
        return 1;
    }

    const auto bound = server.BoundPort();

    std::atomic<bool> running{ true };
    std::thread sim(SimLoop, std::ref(world), std::ref(running));

    std::printf("LCE Studio — the observation window (%s)\n",
        std::string(LCE::Version::String()).c_str());
    std::printf("  world: 30 settlers + trader, ticking at 20 Hz\n");
    std::printf("  open:  http://127.0.0.1:%u\n",
        static_cast<unsigned>(bound));

    if (selftest)
    {
        // One connection, handled, verified — then out.
        std::string body;

        std::thread fetch(
            [&] { body = FetchLocal(bound, "/api/entities"); });

        const auto client = server.Accept();

        if (client != kInvalidSocket)
        {
            HandleConnection(world, client);
        }

        fetch.join();

        const bool ok =
            !body.empty() && body.find("\"id\"") != std::string::npos;

        std::printf("  selftest: %s\n", ok ? "OK" : "FAIL");

        running = false;
        sim.join();

        return ok ? 0 : 1;
    }

    // Serve until the process is stopped — Ctrl+C, or the window closes.
    for (;;)
    {
        const auto client = server.Accept();

        if (client == kInvalidSocket)
        {
            continue;
        }

        HandleConnection(world, client);
    }
}
