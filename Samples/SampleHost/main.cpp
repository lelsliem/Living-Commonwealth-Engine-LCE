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
// │      “A demo that teaches is worth a thousand pages of docs.”
// │
// └─────────────────────────────────────────────────────────────────────────┘
//
// Living Commonwealth Engine (LCE) — 0.5.0 SDK · The Sample Host
//
// The proof any game can embed LCE. A runnable, non-game host that
// drives the complete living loop — decide → act → observe → remember —
// with nothing but the public API. Read it top to bottom: it is the
// course, the README of every modder's first week.
//
// What this host demonstrates, section by section:
//
//   1. Tuning      — SimulationTuning::FromConfiguration: one text file
//                    sets the world's personality.
//   2. Observation — EventBus: EntityCreated, IntentProduced,
//                    OutcomeRecorded — push, not poll.
//   3. Serializers — the co-save pattern: how components become bytes
//                    and back, so the world can ride inside a save.
//   4. The toy world — a fake "game": names, market hours, merchants.
//                    (In Fallout 4 this is the adapter; here it is ~60
//                    lines, because the game is not the point.)
//   5. The loop    — Update → execute → ReportOutcome, per tick.
//   6. The story   — a farmer learns which merchant to trust. No script.
//   7. Save/load   — Capture → Restore mid-story: the world remembers
//                    where it left off, Rng stream and all.
//
// Build with LCE_BUILD_SAMPLES=ON. Run from anywhere; it reads host.ini
// from the working directory when present, else uses its built-in
// defaults (the file ships next to the executable as a reference).
//
// SPDX-License-Identifier: MIT
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Events/EventBus.h"
#include "LCE/Simulation/Behaviour.h"
#include "LCE/Simulation/EntityRegistry.h"
#include "LCE/Simulation/Goals.h"
#include "LCE/Simulation/Memory.h"
#include "LCE/Simulation/Needs.h"
#include "LCE/Simulation/Outcome.h"
#include "LCE/Simulation/Relationships.h"
#include "LCE/Simulation/Rng.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/SimulationEvents.h"
#include "LCE/Simulation/WorldTime.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace LCE::Simulation;

    //-------------------------------------------------------------------------
    // A one-line divider and a section header — the transcript's rhythm.
    //-------------------------------------------------------------------------
    void Rule()  { std::printf("──────────────────────────────────────────────────\n"); }
    void Header(const char* title)
    {
        Rule();
        std::printf("%s\n", title);
        Rule();
    }

    //-------------------------------------------------------------------------
    // SECTION 1 — Tuning: the modder's knob.
    //
    // The world's personality is data. The host tries host.ini from the
    // working directory (the file ships next to the executable); when it
    // is absent, the SAME lines are built from a string below — so the
    // host runs anywhere, and a developer who drops a host.ini beside
    // their run gets the identical result, editable without recompiling.
    //
    // The rates are tuned for this host's clock (one tick = one
    // simulation second, delta = 1.0). Memory fade is slow on purpose:
    // a weight-1.0 memory must outlive the whole demo, not fade in
    // five seconds. A game adapter tunes for its own clock — that is
    // the whole point of the knob.
    //-------------------------------------------------------------------------
    LCE::Config::Configuration LoadTuning()
    {
        std::stringstream source;
        std::ifstream file{ "host.ini" };

        if (file.is_open())
        {
            std::printf("  tuning: read host.ini from the working directory\n");
            source << file.rdbuf();
        }
        else
        {
            std::printf("  tuning: no host.ini found — built-in defaults stand\n");
            source
                // The market door: a weight-1.0 fact (faded at this rate)
                // dips below the 0.1 forget threshold after ~11 ticks —
                // close at 20:00, reopen by 08:00. The host's clock is
                // one tick = one simulation second.
                << "sim.memory.fade = 0.08\n"     // the door fades open again
                << "sim.drift.rate = 0.001\n"     // feelings drift slowly
                << "sim.goal.urgency = 0.05\n"    // goals grow with patience
                << "sim.trust.gain = 0.15\n"      // a fair trade proves reliable
                << "sim.jitter = 0.15\n"          // no two minds alike
                << "host.hunger.decay = 0.025\n"  // the host's own keys —
                << "host.fatigue.decay = 0.04\n"  //   the core ignores them
                << "host.social.decay = 0.01\n"   //   (unknown keys, Law 1)
                << "host.meal.food = 0.90\n";
        }

        LCE::Config::Configuration config;

        std::string line;

        while (std::getline(source, line))
        {
            // INI-ish: key = value, # comments, blank lines ignored.
            if (line.empty() || line[0] == '#')
            {
                continue;
            }

            const auto eq = line.find('=');

            if (eq == std::string::npos)
            {
                continue;
            }

            auto key = line.substr(0, eq);
            auto value = line.substr(eq + 1);

            // Trim whitespace around both sides.
            while (!key.empty() && (key.front() == ' ' || key.front() == '\t'))
            {
                key.erase(key.begin());
            }
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
            {
                key.pop_back();
            }
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
            {
                value.erase(value.begin());
            }
            while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
            {
                value.pop_back();
            }

            config.Set(key, value);
        }

        return config;
    }    //-------------------------------------------------------------------------
    // SECTION 2 — Observation: push, not poll.
    //
    // The game does not ask "is anything new?" — the bus tells it.
    // EntityCreated prints inline (three minds waking). The other two
    // channels COUNT into a ledger instead of shouting every tick — the
    // transcript stays a story, and the summary proves the host never
    // polled once: every decision and every outcome arrived on the bus.
    //-------------------------------------------------------------------------
    struct ObservationLedger
    {
        std::uint64_t Intents = 0;
        std::uint64_t Outcomes = 0;
    };

    void SubscribeObservations(LCE::Events::EventBus& bus, ObservationLedger& ledger)
    {
        bus.Subscribe(
            std::type_index(typeid(EntityCreatedEvent)),
            [](const LCE::Events::Event& raw)
            {
                const auto& event = static_cast<const EntityCreatedEvent&>(raw);
                std::printf("  [event] entity created #%llu\n",
                    static_cast<unsigned long long>(event.Id.Value()));
            });

        bus.Subscribe(
            std::type_index(typeid(IntentProducedEvent)),
            [&ledger](const LCE::Events::Event&)
            {
                ++ledger.Intents;
            });

        bus.Subscribe(
            std::type_index(typeid(OutcomeRecordedEvent)),
            [&ledger](const LCE::Events::Event&)
            {
                ++ledger.Outcomes;
            });
    }

    //-------------------------------------------------------------------------
    // SECTION 3 — Serializers: the co-save pattern.
    //
    // The core snapshot is pure data: component types become bytes and
    // back through serializers the host registers once at startup. This
    // is exactly what the Fallout 4 adapter does — the same five types,
    // the same idea, a fraction of the code. The core never interprets
    // the bytes; the serializer gives them meaning (type erasure, third
    // face).
    //-------------------------------------------------------------------------
    namespace Codec
    {
        // The smallest honest byte codec: append and read little-endian.
        struct Writer
        {
            std::vector<std::byte> Bytes;

            void U8(std::uint8_t value)
            {
                Bytes.push_back(static_cast<std::byte>(value));
            }

            void U32(std::uint32_t value)
            {
                for (int i = 0; i < 4; ++i)
                {
                    Bytes.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFu));
                }
            }

            void U64(std::uint64_t value)
            {
                for (int i = 0; i < 8; ++i)
                {
                    Bytes.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFull));
                }
            }

            void F(float value)
            {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &value, sizeof(bits));
                U32(bits);
            }
        };

        struct Reader
        {
            const ComponentBlob& Bytes;
            std::size_t Position = 0;

            std::uint8_t U8()
            {
                return std::to_integer<std::uint8_t>(Bytes[Position++]);
            }

            std::uint32_t U32()
            {
                std::uint32_t value = 0;

                for (int i = 0; i < 4; ++i)
                {
                    value |= std::to_integer<std::uint32_t>(Bytes[Position++]) << (8 * i);
                }

                return value;
            }

            std::uint64_t U64()
            {
                std::uint64_t value = 0;

                for (int i = 0; i < 8; ++i)
                {
                    value |= std::to_integer<std::uint64_t>(Bytes[Position++]) << (8 * i);
                }

                return value;
            }

            float F()
            {
                const std::uint32_t bits = U32();
                float value = 0.0f;
                std::memcpy(&value, &bits, sizeof(value));
                return value;
            }
        };
    }

    void RegisterSerializers(EntityRegistry& registry)
    {
        registry.RegisterSerializer<Needs>({
            [](const Needs& needs)
            {
                Codec::Writer writer;
                writer.U32(static_cast<std::uint32_t>(needs.List.size()));

                for (const auto& need : needs.List)
                {
                    writer.U32(static_cast<std::uint32_t>(need.Type));
                    writer.F(need.Value);
                    writer.F(need.DecayRate);
                }

                return writer.Bytes;
            },
            [](const ComponentBlob& blob)
            {
                Codec::Reader reader{ blob };
                Needs needs;
                const auto count = reader.U32();

                for (std::uint32_t i = 0; i < count; ++i)
                {
                    needs.List.push_back(Need{
                        static_cast<NeedType>(reader.U32()),
                        reader.F(),
                        reader.F() });
                }

                return needs;
            }
        });

        registry.RegisterSerializer<Memory>({
            [](const Memory& memory)
            {
                Codec::Writer writer;
                writer.U32(static_cast<std::uint32_t>(memory.Events.size()));

                for (const auto& event : memory.Events)
                {
                    writer.U64(event.Other.Value());
                    writer.U32(static_cast<std::uint32_t>(event.Kind));
                    writer.F(event.Weight);
                    writer.U64(event.Day);
                }

                return writer.Bytes;
            },
            [](const ComponentBlob& blob)
            {
                Codec::Reader reader{ blob };
                Memory memory;
                const auto count = reader.U32();

                for (std::uint32_t i = 0; i < count; ++i)
                {
                    memory.Events.push_back(MemoryEvent{
                        EntityId{ reader.U64() },
                        static_cast<InteractionKind>(reader.U32()),
                        reader.F(),
                        reader.U64() });
                }

                return memory;
            }
        });

        registry.RegisterSerializer<Relationships>({
            [](const Relationships& relationships)
            {
                Codec::Writer writer;
                writer.U32(static_cast<std::uint32_t>(relationships.ByEntity.size()));

                for (const auto& [other, relationship] : relationships.ByEntity)
                {
                    writer.U64(other.Value());
                    writer.F(relationship.Disposition);
                    writer.F(relationship.Trust);
                }

                return writer.Bytes;
            },
            [](const ComponentBlob& blob)
            {
                Codec::Reader reader{ blob };
                Relationships relationships;
                const auto count = reader.U32();

                for (std::uint32_t i = 0; i < count; ++i)
                {
                    const auto other = EntityId{ reader.U64() };
                    relationships.ByEntity[other] = Relationship{
                        reader.F(),
                        reader.F() };
                }

                return relationships;
            }
        });

        registry.RegisterSerializer<Goals>({
            [](const Goals& goals)
            {
                Codec::Writer writer;
                writer.U8(goals.Active ? 1 : 0);

                if (goals.Active)
                {
                    writer.U32(static_cast<std::uint32_t>(goals.Active->Type));
                    writer.F(goals.Active->Urgency);
                }

                return writer.Bytes;
            },
            [](const ComponentBlob& blob)
            {
                Codec::Reader reader{ blob };
                Goals goals;

                if (reader.U8() != 0)
                {
                    goals.Active = Goal{
                        static_cast<GoalType>(reader.U32()),
                        reader.F() };
                }

                return goals;
            }
        });

        registry.RegisterSerializer<Intent>({
            [](const Intent& intent)
            {
                Codec::Writer writer;
                writer.U32(static_cast<std::uint32_t>(intent.Action));
                writer.U64(intent.Target.Value());
                writer.F(intent.Confidence);
                return writer.Bytes;
            },
            [](const ComponentBlob& blob)
            {
                Codec::Reader reader{ blob };
                return Intent{
                    static_cast<ActionType>(reader.U32()),
                    EntityId{ reader.U64() },
                    reader.F() };
            }
        });
    }

    //-------------------------------------------------------------------------
    // SECTION 4 — The toy world: the fake game the host drives.
    //
    // This is the "adapter" in miniature. It owns everything the core
    // must never know: names, market hours, merchant honesty. The core
    // reasons over needs, memory, and trust; this code turns intents
    // into (fake) game events and reports how they went.
    //-------------------------------------------------------------------------

    // The market's trading hours, in a 24-hour day. The host's clock is
    // a single hour counter; the market is open 08:00–20:00.
    constexpr int kMarketOpenHour = 8;
    constexpr int kMarketCloseHour = 20;

    struct ToyWorld
    {
        // Names are game knowledge — the core never sees them.
        std::unordered_map<EntityId, std::string> Names;

        // Which visits of which merchant cheat the buyer. Aldous (A)
        // cheats on his 2nd and 3rd visits; Bellamy (B) never does.
        // Fixed so the story always teaches the lesson, no luck needed.
        std::unordered_map<EntityId, std::uint32_t> VisitCount;
        std::unordered_map<EntityId, std::vector<std::uint32_t>> CheatOnVisit;

        const char* Name(EntityId id) const
        {
            const auto iterator = Names.find(id);

            if (iterator == Names.end())
            {
                return "someone";
            }

            return iterator->second.c_str();
        }

        void CountVisit(EntityId merchant)
        {
            ++VisitCount[merchant];
        }

        bool Cheats(EntityId merchant) const
        {
            const auto visit = VisitCount.find(merchant);
            const auto cheat = CheatOnVisit.find(merchant);

            if (visit == VisitCount.end() || cheat == CheatOnVisit.end())
            {
                return false;
            }

            for (const auto cheatVisit : cheat->second)
            {
                if (visit->second == cheatVisit)
                {
                    return true;
                }
            }

            return false;
        }

        // The market's door. Open hours: push the stalls' Trade-kind
        // events (Decide's hunger branch finds food through them),
        // refreshed every open tick so they never fade away. At the
        // closing bell (20:00) exactly once: push { invalid, Trade } —
        // the world-fact gate that shuts the door WHILE it is
        // remembered. The host never refreshes the fact; it fades on its
        // own, and the tuning is chosen so a weight-1.0 fact dips below
        // the forget threshold around 08:00 — the door reopens because
        // the fact died of age. No script opened it.
        //
        // Note the pushes, not Remember(): Remember is for EXPERIENCES
        // — it also raises trust (+TrustGain per call), which is right
        // for a real trade but wrong for hourly market knowledge: the
        // inflation would bury the lesson this demo teaches. Knowledge
        // goes into memory directly — exactly what the Fallout 4
        // adapter's SeedMarketMemory does.
        void RefreshMarketState(
            EntityRegistry& registry,
            EntityId farmer,
            EntityId merchantA,
            EntityId merchantB,
            int hour,
            WorldTime time)
        {
            auto memory = registry.GetComponent<Memory>(farmer);

            if (!memory)
            {
                registry.AddComponent<Memory>(farmer, Memory{});
                memory = registry.GetComponent<Memory>(farmer);
            }

            const bool open = hour >= kMarketOpenHour && hour < kMarketCloseHour;

            if (open)
            {
                memory->Events.push_back(MemoryEvent{
                    merchantA, InteractionKind::Trade, 1.0f, time.Day });
                memory->Events.push_back(MemoryEvent{
                    merchantB, InteractionKind::Trade, 1.0f, time.Day });
            }
            else if (hour == kMarketCloseHour)
            {
                memory->Events.push_back(MemoryEvent{
                    EntityId{}, InteractionKind::Trade, 1.0f, time.Day });
            }
        }
    };

    // The fed write-through: the meal restores the farmer. The core
    // never sets need values — the game does, exactly like the adapter.
    void Feed(EntityRegistry& registry, EntityId id, float food)
    {
        const auto needs = registry.GetComponent<Needs>(id);

        if (!needs)
        {
            return;
        }

        for (auto& need : needs->List)
        {
            if (need.Type == NeedType::Hunger)
            {
                need.Value = food;
            }
        }
    }

    //-------------------------------------------------------------------------
    // SECTION 5 — The loop: decide → act → observe → remember.
    //
    // One tick = one simulation second. Update decays needs, fades
    // memory, drifts relationships, grows goals, and decides an Intent
    // for every mind (publishing IntentProducedEvent). The host then
    // executes those intents in the toy world and reports outcomes —
    // the observe leg that closes the loop into learning.
    //-------------------------------------------------------------------------
    void ExecuteIntents(
        EntityRegistry& registry,
        ToyWorld& world,
        const SimulationTuning& tuning,
        LCE::Events::EventBus& bus,
        WorldTime time,
        int hour)
    {
        // The plan: every mind's current intent, collected after the
        // tick (the adapter's Update → plan → execute rhythm).
        const auto minds = registry.QueryWhere<Needs>([](EntityId, const Needs&) {
            return true;
        });

        for (const auto id : minds)
        {
            const auto intent = registry.GetComponent<Intent>(id);

            if (!intent)
            {
                continue;
            }

            switch (intent->Action)
            {
            case ActionType::MoveTo:
            {
                // The farmer walks to the merchant's stall — one step
                // in the toy world, a navmesh walk in Fallout 4.
                std::printf("  %s walks to %s…\n",
                    world.Name(id), world.Name(intent->Target));

                world.CountVisit(intent->Target);
                const bool cheated = world.Cheats(intent->Target);

                if (cheated)
                {
                    std::printf("  …%s takes the caps and short-changes %s.\n",
                        world.Name(intent->Target), world.Name(id));
                }
                else
                {
                    std::printf("  …%s weighs the caps, and the meal is fair.\n",
                        world.Name(intent->Target));
                }

                Feed(registry, id, 0.90f);

                ReportOutcome(registry, id,
                    { intent->Target, InteractionKind::Trade,
                      cheated ? OutcomeResult::Failure : OutcomeResult::Success,
                      1.0f },
                    tuning, &bus, time);
                break;
            }

            case ActionType::Rest:
                std::printf("  %s rests. Fatigue fades.\n", world.Name(id));

                if (const auto needs = registry.GetComponent<Needs>(id))
                {
                    for (auto& need : needs->List)
                    {
                        if (need.Type == NeedType::Fatigue)
                        {
                            need.Value = 1.0f;
                        }
                    }
                }
                break;

            case ActionType::Socialize:
                std::printf("  %s chats with %s.\n",
                    world.Name(id), world.Name(intent->Target));
                ReportOutcome(registry, id,
                    { intent->Target, InteractionKind::Social,
                      OutcomeResult::Success, 1.0f },
                    tuning, &bus, time);
                break;

            case ActionType::Explore:
                // Explore means one of two doors: the market's (the
                // world-fact gate) or the mind's (no known target). The
                // host can tell them apart — the game knows the hour.
                if (hour < kMarketOpenHour || hour >= kMarketCloseHour)
                {
                    std::printf("  the market is shut — %s waits.\n",
                        world.Name(id));
                }
                else
                {
                    std::printf("  %s wanders, unhurried.\n", world.Name(id));
                }
                break;

            case ActionType::Work:
                std::printf("  %s works the fields.\n", world.Name(id));
                break;

            case ActionType::Flee:
                std::printf("  %s flees from %s!\n",
                    world.Name(id), world.Name(intent->Target));
                break;
            }
        }
    }

    //-------------------------------------------------------------------------
    // The story's summary: what the farmer's memory and trust look like
    // at the end, and the trades that shaped them. The lesson, in data.
    //-------------------------------------------------------------------------
    void Summarize(
        const EntityRegistry& registry,
        EntityId farmer,
        EntityId merchantA,
        EntityId merchantB,
        const ToyWorld& world)
    {
        Header("Summary — the farmer's ledger");

        const auto memory = registry.GetComponent<Memory>(farmer);
        const auto relationships = registry.GetComponent<Relationships>(farmer);

        std::printf("  trades: Aldous %u visits, Bellamy %u visits\n",
            world.VisitCount.count(merchantA) ? world.VisitCount.at(merchantA) : 0,
            world.VisitCount.count(merchantB) ? world.VisitCount.at(merchantB) : 0);

        if (relationships)
        {
            const auto trust = [&relationships](EntityId other) -> float
            {
                const auto iterator = relationships->ByEntity.find(other);
                return iterator == relationships->ByEntity.end()
                    ? 0.0f
                    : iterator->second.Trust;
            };

            std::printf("  trust: Aldous %.2f, Bellamy %.2f\n",
                trust(merchantA), trust(merchantB));
            std::printf("  the farmer prefers: %s\n",
                trust(merchantA) < trust(merchantB)
                    ? "Bellamy — Aldous proved unreliable"
                    : "Aldous — a steady hand");
        }

        if (memory)
        {
            std::printf("  memories: %zu events carried in the mind\n",
                memory->Events.size());
        }

        std::printf("\n  No quest script wrote this. The farmer decided,\n");
        std::printf("  acted, observed, remembered — and learned.\n");
    }
}

//=============================================================================//
// main
//=============================================================================//

int main()
{
    using namespace LCE::Simulation;

    Header("Living Commonwealth Engine — Sample Host");
    std::printf("  The proof any game can embed LCE.\n");
    std::printf("  One farmer, two merchants, zero scripts.\n");

    //-------------------------------------------------------------------------
    // SECTION 1 — Tuning.
    //-------------------------------------------------------------------------
    Header("Section 1 — Tuning: the modder's knob");
    const auto config = LoadTuning();
    const auto tuning = SimulationTuning::FromConfiguration(config);

    std::printf("  memory fade %.4f/s · goal urgency %.2f/s · jitter %.2f\n",
        tuning.MemoryFadeRate, tuning.GoalUrgencyRate, tuning.NeedJitter);
    std::printf("  the host's own keys (host.*) ride in the same file —\n");
    std::printf("  the core ignored them (unknown keys are not its business).\n");

    //-------------------------------------------------------------------------
    // SECTION 2 — Observation.
    //-------------------------------------------------------------------------
    Header("Section 2 — Observation: push, not poll");
    LCE::Events::EventBus bus;
    ObservationLedger ledger;
    SubscribeObservations(bus, ledger);

    //-------------------------------------------------------------------------
    // SECTION 3 — Serializers (the co-save pattern) + the world.
    //-------------------------------------------------------------------------
    EntityRegistry registry;
    registry.SetEventSink(&bus);
    RegisterSerializers(registry);

    // The seeded Rng — one word of state; Derive gives every mind its
    // own order-independent noise. Persist State() to resume the world.
    Rng rng{ 0xC0FFEEull };

    const auto farmer = registry.CreateEntity();
    const auto merchantA = registry.CreateEntity();
    const auto merchantB = registry.CreateEntity();

    ToyWorld world;
    world.Names[farmer] = "the farmer";
    world.Names[merchantA] = "Aldous";
    world.Names[merchantB] = "Bellamy";
    world.CheatOnVisit[merchantA] = { 3, 4 };   // fair twice, then cheats
    // twice — the farmer's lesson needs two good trades to earn trust
    // and two cheats to lose it (the drift gently pulls feelings toward
    // neutral between visits, so the numbers stay honest)

    // The farmer is a mind: hungry, a little tired, sociable. The
    // merchants are locations — no Needs, so they are never minds. The
    // decay rates come from the host's own tuning keys (host.*), read
    // through the same Configuration the core reads — game knowledge
    // riding beside the engine's.
    const auto rate = [&config](const char* key, float fallback) -> float
    {
        const auto raw = config.Get(key);

        if (raw.empty())
        {
            return fallback;
        }

        try
        {
            return std::stof(std::string{ raw });
        }
        catch (...)
        {
            return fallback;   // a broken line never breaks the world
        }
    };

    const float hungerDecay = rate("host.hunger.decay", 0.025f);
    const float fatigueDecay = rate("host.fatigue.decay", 0.04f);
    const float socialDecay = rate("host.social.decay", 0.01f);

    registry.AddComponent<Needs>(farmer, Needs{
        { Need{ NeedType::Hunger,  0.30f, hungerDecay },
          Need{ NeedType::Fatigue, 0.95f, fatigueDecay },
          Need{ NeedType::Social,  0.90f, socialDecay },
          Need{ NeedType::Safety,  1.00f, 0.00f },
          Need{ NeedType::Comfort, 1.00f, 0.00f } }
    });

    registry.AddComponent<Goals>(farmer, Goals{
        Goal{ GoalType::AcquireFood, 0.4f } });

    Header("Section 3 — The world wakes");
    std::printf("  %s is hungry (%.2f) and remembers no merchant yet.\n",
        world.Name(farmer), 0.30f);

    //-------------------------------------------------------------------------
    // SECTION 6 — The story. Two days of ticks; one sim-second each.
    // WorldTime is the calendar the memories get stamped with.
    //-------------------------------------------------------------------------
    Header("Section 6 — The story (eight days, tick by tick)");

    constexpr int kTotalTicks = 192;        // eight days of 24 hours
    constexpr int kTicksPerDay = 24;
    constexpr int kSaveTick = 120;          // the world is saved mid-story

    int savedAtDay = 0;
    std::uint64_t savedRng = 0;
    std::uint32_t savedComponents = 0;

    for (int tick = 0; tick < kTotalTicks; ++tick)
    {
        const int hour = tick % kTicksPerDay;
        const WorldTime time{ static_cast<std::uint64_t>(tick / kTicksPerDay) };

        // The calendar printed at the top of each day.
        if (hour == 8)
        {
            const char* season = "spring";

            switch (SeasonOf(time.Day))
            {
            case Season::Spring: season = "spring"; break;
            case Season::Summer: season = "summer"; break;
            case Season::Autumn: season = "autumn"; break;
            case Season::Winter: season = "winter"; break;
            }

            std::printf("\n  — Day %llu, %s — the market opens —\n",
                static_cast<unsigned long long>(time.Day), season);
        }

        // The market door: open hours remember the stalls; closed hours
        // remember the shutting fact.
        world.RefreshMarketState(registry, farmer, merchantA, merchantB,
            hour, time);

        // The heartbeat: one Update per tick, fed the seeded Rng.
        Update(registry, 1.0, tuning, &bus, &rng);

        // The executor: act on what the minds decided, report outcomes.
        ExecuteIntents(registry, world, tuning, bus, time, hour);

        //-------------------------------------------------------------------------
        // SECTION 7 — Save/load, mid-story: the world remembers.
        //-------------------------------------------------------------------------
        if (tick == kSaveTick)
        {
            Header("Section 7 — Save: the world remembers where it left off");
            const auto snapshot = registry.Capture();
            savedAtDay = tick / kTicksPerDay;
            savedRng = rng.State();

            std::uint32_t componentCount = 0;

            for (const auto& entity : snapshot.Entities)
            {
                componentCount += static_cast<std::uint32_t>(entity.Components.size());
            }

            savedComponents = componentCount;

            std::printf("  captured %zu entities, %u components, Rng state 0x%llX\n",
                snapshot.Entities.size(), componentCount,
                static_cast<unsigned long long>(savedRng));

            // A brand-new registry — a fresh game session that never saw
            // the first one. Serializers registered again, sink re-set.
            EntityRegistry restored;
            restored.SetEventSink(&bus);
            RegisterSerializers(restored);
            restored.Restore(snapshot);

            // The Rng resumes its exact stream — same seed, same world.
            rng.SetState(savedRng);

            std::printf("  restored into a fresh registry: the farmer's\n");
            std::printf("  trust survived the round-trip.\n");

            if (const auto relationships =
                    restored.GetComponent<Relationships>(farmer))
            {
                const auto trustA = relationships->ByEntity.find(merchantA);
                const auto trustB = relationships->ByEntity.find(merchantB);

                std::printf("  trust after load — Aldous %.2f, Bellamy %.2f\n",
                    trustA == relationships->ByEntity.end() ? 0.0f : trustA->second.Trust,
                    trustB == relationships->ByEntity.end() ? 0.0f : trustB->second.Trust);
            }
        }
    }

    //-------------------------------------------------------------------------
    // The end of the story.
    //-------------------------------------------------------------------------
    Summarize(registry, farmer, merchantA, merchantB, world);

    Header("The bus — push, not poll");
    std::printf("  %llu intent events and %llu outcome events arrived on\n",
        static_cast<unsigned long long>(ledger.Intents),
        static_cast<unsigned long long>(ledger.Outcomes));
    std::printf("  the bus — the host never polled once.");

    std::printf("\n  That is the living loop, end to end: decide → act →\n");
    std::printf("  observe → remember → decide. Embedded in a host with\n");
    std::printf("  no game at all — any engine can do what this did.\n");
    Rule();

    return 0;
}
