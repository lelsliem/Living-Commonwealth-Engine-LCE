//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      SurfaceTest.cpp
//
// Purpose:
//
//      The surface-stability guard — the teeth of the 0.8.4 API Freeze.
//      HeaderMapTest freezes the FILE map (every path a consumer can
//      include). This suite goes one level deeper and freezes the
//      DECLARATIONS inside those headers:
//
//        * enum ordinals        — the adapter's co-save writes raw
//                                 ordinals; a reorder or an insertion
//                                 in the middle corrupts saves. Every
//                                 enumerator is pinned to its value.
//        * struct field types   — every public field of every public
//                                 struct is pinned to its exact type.
//                                 A rename, a retype, or a reordering
//                                 of an existing field fails here.
//        * member / free-function signatures — every non-template
//                                 member and every public free function
//                                 is pinned to its exact signature,
//                                 noexcept included.
//
//      The checks are static_asserts: the freeze is enforced at
//      COMPILE time — the harness cannot even build against a drifted
//      surface, so a breaking change is caught before it reaches the
//      adapter or a modder's build.
//
//      What the freeze does NOT pin (deliberately):
//        * Version values       — they are meant to change every
//                                 release. The functions are pinned;
//                                 the numbers are not.
//        * Field additions to ABI-dependent structs — where a struct
//                                 contains std::string (whose size
//                                 differs across toolchains), a size
//                                 guard would fail spuriously, so
//                                 those structs are pinned by field
//                                 type only. Additive change is the
//                                 CompatPolicy's append-only rule
//                                 (Docs/Design/CompatPolicy.md);
//                                 size guards ARE pinned where the
//                                 ABI is stable (every member is a
//                                 fixed-size type — Intent, Outcome,
//                                 Need, Goal, Relationship,
//                                 WorldTime).
//
//      Updating this file is a changelog-worthy event: the freeze
//      exists to force every surface change to be deliberate and
//      documented.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "SurfaceTest.h"

#include "LCE/Config/Configuration.h"
#include "LCE/Events/Event.h"
#include "LCE/Events/EventBus.h"
#include "LCE/Logging/Logger.h"
#include "LCE/Runtime/ServiceRegistry.h"
#include "LCE/Scheduling/Scheduler.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/SimulationEvents.h"
#include "LCE/Simulation/Society/Traits.h"
#include "LCE/Tasks/Task.h"
#include "LCE/Time/Clock.h"
#include "LCE/Version/Version.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace LCE::Tests
{
    // The failure message names the drifted declaration — a surface
    // change compiles to an error that says exactly what moved. The
    // macro is variadic because a condition may contain commas at
    // angle-bracket depth (std::is_same_v<X, int>), which would split
    // a fixed-arity macro argument list; __VA_ARGS__ re-joins them.
#define LCE_SURFACE(...) static_assert(__VA_ARGS__)

    namespace SurfaceDetail
    {
        // A complete type for exercising the ServiceRegistry templates.
        struct FakeService
        {
        };
    }

    //============================================================================
    // LCE/Version/Version.h — values change per release; shapes do not.
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            std::is_same_v<
                std::remove_cv_t<decltype(LCE::Version::MajorValue)>, int>,
            "Version::MajorValue is not an int");
        LCE_SURFACE(
            std::is_same_v<
                std::remove_cv_t<decltype(LCE::Version::MinorValue)>, int>,
            "Version::MinorValue is not an int");
        LCE_SURFACE(
            std::is_same_v<
                std::remove_cv_t<decltype(LCE::Version::PatchValue)>, int>,
            "Version::PatchValue is not an int");
        LCE_SURFACE(
            std::is_same_v<
                std::remove_cv_t<decltype(LCE::Version::VersionString)>,
                std::string_view>,
            "Version::VersionString is not a string_view");
        LCE_SURFACE(
            std::is_same_v<
                std::remove_cv_t<decltype(LCE::Version::EngineName)>,
                std::string_view>,
            "Version::EngineName is not a string_view");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Version::Major),
                int (*)() noexcept>,
            "Version::Major signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Version::Minor),
                int (*)() noexcept>,
            "Version::Minor signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Version::Patch),
                int (*)() noexcept>,
            "Version::Patch signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Version::String),
                std::string_view (*)() noexcept>,
            "Version::String signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Version::Name),
                std::string_view (*)() noexcept>,
            "Version::Name signature changed");
    }

    //============================================================================
    // LCE/Logging/LogLevel.h
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            static_cast<int>(LCE::Logging::LogLevel::Trace) == 0,
            "LogLevel::Trace ordinal");
        LCE_SURFACE(
            static_cast<int>(LCE::Logging::LogLevel::Debug) == 1,
            "LogLevel::Debug ordinal");
        LCE_SURFACE(
            static_cast<int>(LCE::Logging::LogLevel::Info) == 2,
            "LogLevel::Info ordinal");
        LCE_SURFACE(
            static_cast<int>(LCE::Logging::LogLevel::Warning) == 3,
            "LogLevel::Warning ordinal");
        LCE_SURFACE(
            static_cast<int>(LCE::Logging::LogLevel::Error) == 4,
            "LogLevel::Error ordinal");
        LCE_SURFACE(
            static_cast<int>(LCE::Logging::LogLevel::Critical) == 5,
            "LogLevel::Critical ordinal");
        LCE_SURFACE(
            sizeof(LCE::Logging::LogLevel) == 4,
            "LogLevel is not 4 bytes");
    }

    //============================================================================
    // LCE/Logging/Logger.h
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Initialize),
                void (*)() noexcept>,
            "Logging::Initialize signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Shutdown),
                void (*)() noexcept>,
            "Logging::Shutdown signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Flush),
                void (*)() noexcept>,
            "Logging::Flush signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Write),
                void (*)(LCE::Logging::LogLevel, std::string_view) noexcept>,
            "Logging::Write signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Trace),
                void (*)(std::string_view) noexcept>,
            "Logging::Trace signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Debug),
                void (*)(std::string_view) noexcept>,
            "Logging::Debug signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Info),
                void (*)(std::string_view) noexcept>,
            "Logging::Info signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Warning),
                void (*)(std::string_view) noexcept>,
            "Logging::Warning signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Error),
                void (*)(std::string_view) noexcept>,
            "Logging::Error signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Logging::Critical),
                void (*)(std::string_view) noexcept>,
            "Logging::Critical signature changed");
    }

    //============================================================================
    // LCE/Events/Event.h and LCE/Events/EventBus.h
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            std::is_polymorphic_v<LCE::Events::Event>,
            "Event is not polymorphic");
        LCE_SURFACE(
            std::has_virtual_destructor_v<LCE::Events::Event>,
            "Event lost its virtual destructor");

        using BusHandler = LCE::Events::EventBus::EventHandler;
        LCE_SURFACE(
            std::is_same_v<
                BusHandler,
                std::function<void(const LCE::Events::Event&)>>,
            "EventBus::EventHandler type changed");
        LCE_SURFACE(
            std::is_copy_constructible_v<LCE::Events::EventBus> == false,
            "EventBus became copyable");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Events::EventBus::Subscribe),
                void (LCE::Events::EventBus::*)(
                    std::type_index, BusHandler)>,
            "EventBus::Subscribe signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Events::EventBus::Publish),
                void (LCE::Events::EventBus::*)(const LCE::Events::Event&)>,
            "EventBus::Publish signature changed");
    }

    //============================================================================
    // LCE/Config/Configuration.h
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Config::Configuration::Set),
                void (LCE::Config::Configuration::*)(
                    std::string_view, std::string_view)>,
            "Configuration::Set signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Config::Configuration::Has),
                bool (LCE::Config::Configuration::*)(
                    std::string_view) const noexcept>,
            "Configuration::Has signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Config::Configuration::Get),
                std::string_view (LCE::Config::Configuration::*)(
                    std::string_view) const noexcept>,
            "Configuration::Get signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<const LCE::Config::Configuration&>()
                    .ForEach([](std::string_view, std::string_view) {})),
                void>,
            "Configuration::ForEach signature changed");
    }

    //============================================================================
    // LCE/Runtime/ServiceRegistry.h
    //============================================================================

    namespace
    {
        using FakeService = SurfaceDetail::FakeService;

        LCE_SURFACE(
            std::is_copy_constructible_v<LCE::Runtime::ServiceRegistry> == false,
            "ServiceRegistry became copyable");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<LCE::Runtime::ServiceRegistry&>()
                    .Register<FakeService>(
                        std::declval<std::shared_ptr<FakeService>>())),
                void>,
            "ServiceRegistry::Register signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<const LCE::Runtime::ServiceRegistry&>()
                    .Has<FakeService>()),
                bool>,
            "ServiceRegistry::Has signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<const LCE::Runtime::ServiceRegistry&>()
                    .Get<FakeService>()),
                std::shared_ptr<FakeService>>,
            "ServiceRegistry::Get signature changed");
    }

    //============================================================================
    // LCE/Scheduling/Scheduler.h
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            std::is_same_v<
                LCE::Scheduling::Scheduler::Duration,
                std::chrono::duration<double>>,
            "Scheduler::Duration type changed");
        LCE_SURFACE(
            std::is_same_v<
                LCE::Scheduling::Scheduler::Callback,
                std::function<void()>>,
            "Scheduler::Callback type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Scheduling::Scheduler::Update),
                void (LCE::Scheduling::Scheduler::*)(
                    LCE::Scheduling::Scheduler::Duration) noexcept>,
            "Scheduler::Update signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Scheduling::Scheduler::Schedule),
                void (LCE::Scheduling::Scheduler::*)(
                    LCE::Scheduling::Scheduler::Duration,
                    LCE::Scheduling::Scheduler::Callback)>,
            "Scheduler::Schedule signature changed");
    }

    //============================================================================
    // LCE/Tasks/Task.h and LCE/Time/Clock.h
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            std::is_same_v<
                LCE::Tasks::Task::Callback,
                std::function<void()>>,
            "Task::Callback type changed");
        LCE_SURFACE(
            std::is_constructible_v<
                LCE::Tasks::Task,
                LCE::Tasks::Task::Callback>,
            "Task lost its Callback constructor");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Tasks::Task::Execute),
                void (LCE::Tasks::Task::*)()>,
            "Task::Execute signature changed");

        LCE_SURFACE(
            std::is_same_v<
                LCE::Time::Clock::Duration,
                std::chrono::duration<double>>,
            "Clock::Duration type changed");
        LCE_SURFACE(
            std::is_default_constructible_v<LCE::Time::Clock>,
            "Clock lost its default constructor");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Time::Clock::Elapsed),
                LCE::Time::Clock::Duration (LCE::Time::Clock::*)()
                    const noexcept>,
            "Clock::Elapsed signature changed");
    }

    //============================================================================
    // LCE/Simulation/Entity/EntityId.h
    //============================================================================

    namespace
    {
        using EntityId = LCE::Simulation::EntityId;

        LCE_SURFACE(
            std::is_same_v<EntityId::ValueType, std::uint64_t>,
            "EntityId::ValueType changed");
        LCE_SURFACE(
            EntityId::InvalidValue == 0,
            "EntityId::InvalidValue changed");
        LCE_SURFACE(
            sizeof(EntityId) == 8,
            "EntityId is not 8 bytes");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityId::Value),
                EntityId::ValueType (EntityId::*)() const noexcept>,
            "EntityId::Value signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityId::Index),
                std::uint32_t (EntityId::*)() const noexcept>,
            "EntityId::Index signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityId::Generation),
                std::uint32_t (EntityId::*)() const noexcept>,
            "EntityId::Generation signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityId::IsValid),
                bool (EntityId::*)() const noexcept>,
            "EntityId::IsValid signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityId::Make),
                EntityId (*)(std::uint32_t, std::uint32_t) noexcept>,
            "EntityId::Make signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<EntityId>() == std::declval<EntityId>()),
                bool>,
            "EntityId::operator== changed");
        LCE_SURFACE(
            std::is_invocable_r_v<
                std::size_t, std::hash<EntityId>, EntityId>,
            "std::hash<EntityId> specialization lost");
    }

    //============================================================================
    // LCE/Simulation/Society/Groups.h
    //============================================================================

    namespace
    {
        using GroupId = LCE::Simulation::GroupId;

        LCE_SURFACE(
            std::is_same_v<GroupId::ValueType, std::uint64_t>,
            "GroupId::ValueType changed");
        LCE_SURFACE(
            GroupId::InvalidValue == 0,
            "GroupId::InvalidValue changed");
        LCE_SURFACE(
            sizeof(GroupId) == 8,
            "GroupId is not 8 bytes");
        LCE_SURFACE(
            std::is_constructible_v<GroupId, GroupId::ValueType>,
            "GroupId lost its value constructor");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&GroupId::Value),
                GroupId::ValueType (GroupId::*)() const noexcept>,
            "GroupId::Value signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&GroupId::IsValid),
                bool (GroupId::*)() const noexcept>,
            "GroupId::IsValid signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<GroupId>() == std::declval<GroupId>()),
                bool>,
            "GroupId::operator== changed");
        LCE_SURFACE(
            std::is_invocable_r_v<
                std::size_t, std::hash<GroupId>, GroupId>,
            "std::hash<GroupId> specialization lost");

        LCE_SURFACE(
            std::is_same_v<
                decltype(LCE::Simulation::Groups::Memberships),
                std::vector<GroupId>>,
            "Groups::Memberships type changed");
    }

    //============================================================================
    // LCE/Simulation/Substrate/Rng.h
    //============================================================================

    namespace
    {
        using Rng = LCE::Simulation::Rng;

        LCE_SURFACE(
            std::is_constructible_v<Rng, std::uint64_t>,
            "Rng lost its seed constructor");
        LCE_SURFACE(
            sizeof(Rng) == 16,
            "Rng state changed size (must stay one seed + one state word)");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&Rng::Next),
                std::uint64_t (Rng::*)() noexcept>,
            "Rng::Next signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(static_cast<float (Rng::*)() noexcept>(&Rng::NextFloat)),
                float (Rng::*)() noexcept>,
            "Rng::NextFloat() signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(static_cast<float (Rng::*)(float, float) noexcept>(&Rng::NextFloat)),
                float (Rng::*)(float, float) noexcept>,
            "Rng::NextFloat(min,max) signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&Rng::State),
                std::uint64_t (Rng::*)() const noexcept>,
            "Rng::State signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&Rng::SetState),
                void (Rng::*)(std::uint64_t) noexcept>,
            "Rng::SetState signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&Rng::Derive),
                Rng (Rng::*)(std::uint64_t) const noexcept>,
            "Rng::Derive signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&Rng::StableDerive),
                Rng (Rng::*)(std::uint64_t) const noexcept>,
            "Rng::StableDerive signature changed");
    }

    //============================================================================
    // LCE/Simulation/Substrate/WorldTime.h
    //============================================================================

    namespace
    {
        LCE_SURFACE(
            std::is_same_v<
                decltype(LCE::Simulation::WorldTime::Day),
                std::uint64_t>,
            "WorldTime::Day type changed");
        LCE_SURFACE(
            sizeof(LCE::Simulation::WorldTime) == 8,
            "WorldTime is not 8 bytes");

        using Season = LCE::Simulation::Season;
        LCE_SURFACE(
            static_cast<int>(Season::Spring) == 0,
            "Season::Spring ordinal");
        LCE_SURFACE(
            static_cast<int>(Season::Summer) == 1,
            "Season::Summer ordinal");
        LCE_SURFACE(
            static_cast<int>(Season::Autumn) == 2,
            "Season::Autumn ordinal");
        LCE_SURFACE(
            static_cast<int>(Season::Winter) == 3,
            "Season::Winter ordinal");
        LCE_SURFACE(
            sizeof(Season) == 4,
            "Season is not 4 bytes");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::SeasonOf),
                Season (*)(std::uint64_t) noexcept>,
            "SeasonOf signature changed");
    }

    //============================================================================
    // LCE/Simulation/Mind/Memory.h
    //============================================================================

    namespace
    {
        using InteractionKind = LCE::Simulation::InteractionKind;

        LCE_SURFACE(
            static_cast<int>(InteractionKind::Trade) == 0,
            "InteractionKind::Trade ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::Combat) == 1,
            "InteractionKind::Combat ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::Aid) == 2,
            "InteractionKind::Aid ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::Social) == 3,
            "InteractionKind::Social ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::Wronged) == 4,
            "InteractionKind::Wronged ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::WeatherClear) == 5,
            "InteractionKind::WeatherClear ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::WeatherOvercast) == 6,
            "InteractionKind::WeatherOvercast ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::WeatherRain) == 7,
            "InteractionKind::WeatherRain ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::WeatherFog) == 8,
            "InteractionKind::WeatherFog ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::WeatherMisty) == 9,
            "InteractionKind::WeatherMisty ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::WeatherRadstorm) == 10,
            "InteractionKind::WeatherRadstorm ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::Death) == 11,
            "InteractionKind::Death ordinal (co-save critical)");
        LCE_SURFACE(
            static_cast<int>(InteractionKind::Fact) == 12,
            "InteractionKind::Fact ordinal (co-save critical — append-only)");
        LCE_SURFACE(
            sizeof(InteractionKind) == 4,
            "InteractionKind is not 4 bytes");

        using MemoryEvent = LCE::Simulation::MemoryEvent;
        LCE_SURFACE(
            std::is_same_v<decltype(MemoryEvent::Other), EntityId>,
            "MemoryEvent::Other type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(MemoryEvent::Kind), InteractionKind>,
            "MemoryEvent::Kind type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(MemoryEvent::Weight), float>,
            "MemoryEvent::Weight type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(MemoryEvent::Day), std::uint64_t>,
            "MemoryEvent::Day type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(MemoryEvent::Label), std::string>,
            "MemoryEvent::Label type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(LCE::Simulation::Memory::Events),
                std::vector<MemoryEvent>>,
            "Memory::Events type changed");
    }

    //============================================================================
    // LCE/Simulation/Mind/Needs.h
    //============================================================================

    namespace
    {
        using NeedType = LCE::Simulation::NeedType;
        LCE_SURFACE(
            static_cast<int>(NeedType::Hunger) == 0,
            "NeedType::Hunger ordinal");
        LCE_SURFACE(
            static_cast<int>(NeedType::Fatigue) == 1,
            "NeedType::Fatigue ordinal");
        LCE_SURFACE(
            static_cast<int>(NeedType::Social) == 2,
            "NeedType::Social ordinal");
        LCE_SURFACE(
            static_cast<int>(NeedType::Safety) == 3,
            "NeedType::Safety ordinal");
        LCE_SURFACE(
            static_cast<int>(NeedType::Comfort) == 4,
            "NeedType::Comfort ordinal");
        LCE_SURFACE(
            sizeof(NeedType) == 4,
            "NeedType is not 4 bytes");

        using Need = LCE::Simulation::Need;
        LCE_SURFACE(
            std::is_same_v<decltype(Need::Type), NeedType>,
            "Need::Type type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Need::Value), float>,
            "Need::Value type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Need::DecayRate), float>,
            "Need::DecayRate type changed");
        LCE_SURFACE(
            sizeof(Need) == 12,
            "Need size changed (x64 ABI)");
        LCE_SURFACE(
            std::is_same_v<
                decltype(LCE::Simulation::Needs::List),
                std::vector<Need>>,
            "Needs::List type changed");
    }

    //============================================================================
    // LCE/Simulation/Mind/Goals.h
    //============================================================================

    namespace
    {
        using GoalType = LCE::Simulation::GoalType;
        LCE_SURFACE(
            static_cast<int>(GoalType::AcquireFood) == 0,
            "GoalType::AcquireFood ordinal");
        LCE_SURFACE(
            static_cast<int>(GoalType::ReachSafety) == 1,
            "GoalType::ReachSafety ordinal");
        LCE_SURFACE(
            static_cast<int>(GoalType::Socialize) == 2,
            "GoalType::Socialize ordinal");
        LCE_SURFACE(
            static_cast<int>(GoalType::Prosper) == 3,
            "GoalType::Prosper ordinal");
        LCE_SURFACE(
            sizeof(GoalType) == 4,
            "GoalType is not 4 bytes");

        using Goal = LCE::Simulation::Goal;
        LCE_SURFACE(
            std::is_same_v<decltype(Goal::Type), GoalType>,
            "Goal::Type type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Goal::Urgency), float>,
            "Goal::Urgency type changed");
        LCE_SURFACE(
            sizeof(Goal) == 8,
            "Goal size changed (x64 ABI)");
        LCE_SURFACE(
            std::is_same_v<
                decltype(LCE::Simulation::Goals::Active),
                std::optional<Goal>>,
            "Goals::Active type changed");
    }

    //============================================================================
    // LCE/Simulation/Mind/Relationships.h
    //============================================================================

    namespace
    {
        using Relationship = LCE::Simulation::Relationship;
        LCE_SURFACE(
            std::is_same_v<decltype(Relationship::Disposition), float>,
            "Relationship::Disposition type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Relationship::Trust), float>,
            "Relationship::Trust type changed");
        LCE_SURFACE(
            sizeof(Relationship) == 8,
            "Relationship size changed (x64 ABI)");
        LCE_SURFACE(
            std::is_same_v<
                decltype(LCE::Simulation::Relationships::ByEntity),
                std::unordered_map<EntityId, Relationship>>,
            "Relationships::ByEntity type changed");
    }

    //============================================================================
    // LCE/Simulation/Society/Traits.h
    //============================================================================

    namespace
    {
        using TraitValue = LCE::Simulation::TraitValue;
        LCE_SURFACE(
            std::is_same_v<decltype(TraitValue::Name), std::string>,
            "TraitValue::Name type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TraitValue::Value), float>,
            "TraitValue::Value type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(LCE::Simulation::Traits::List),
                std::vector<TraitValue>>,
            "Traits::List type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::JitteredTraits),
                LCE::Simulation::Traits (*)(
                    const LCE::Simulation::Traits&,
                    EntityId,
                    const Rng*,
                    float)>,
            "JitteredTraits signature changed");
    }

    //============================================================================
    // LCE/Simulation/Decision/Behaviour.h
    //============================================================================

    namespace
    {
        using ActionType = LCE::Simulation::ActionType;
        LCE_SURFACE(
            static_cast<int>(ActionType::MoveTo) == 0,
            "ActionType::MoveTo ordinal");
        LCE_SURFACE(
            static_cast<int>(ActionType::Rest) == 1,
            "ActionType::Rest ordinal");
        LCE_SURFACE(
            static_cast<int>(ActionType::Socialize) == 2,
            "ActionType::Socialize ordinal");
        LCE_SURFACE(
            static_cast<int>(ActionType::Explore) == 3,
            "ActionType::Explore ordinal");
        LCE_SURFACE(
            static_cast<int>(ActionType::Work) == 4,
            "ActionType::Work ordinal");
        LCE_SURFACE(
            static_cast<int>(ActionType::Flee) == 5,
            "ActionType::Flee ordinal");
        LCE_SURFACE(
            sizeof(ActionType) == 4,
            "ActionType is not 4 bytes");

        using Intent = LCE::Simulation::Intent;
        LCE_SURFACE(
            std::is_same_v<decltype(Intent::Action), ActionType>,
            "Intent::Action type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Intent::Target), EntityId>,
            "Intent::Target type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Intent::Confidence), float>,
            "Intent::Confidence type changed");
        LCE_SURFACE(
            sizeof(Intent) == 24,
            "Intent size changed (x64 ABI)");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::Decide),
                std::optional<Intent> (*)(
                    const LCE::Simulation::EntityRegistry&,
                    EntityId,
                    const Rng*,
                    float)>,
            "Decide signature changed");
    }

    //============================================================================
    // LCE/Simulation/Decision/Outcome.h
    //============================================================================

    namespace
    {
        using OutcomeResult = LCE::Simulation::OutcomeResult;
        LCE_SURFACE(
            static_cast<int>(OutcomeResult::Success) == 0,
            "OutcomeResult::Success ordinal");
        LCE_SURFACE(
            static_cast<int>(OutcomeResult::Partial) == 1,
            "OutcomeResult::Partial ordinal");
        LCE_SURFACE(
            static_cast<int>(OutcomeResult::Failure) == 2,
            "OutcomeResult::Failure ordinal");
        LCE_SURFACE(
            sizeof(OutcomeResult) == 4,
            "OutcomeResult is not 4 bytes");

        using Outcome = LCE::Simulation::Outcome;
        LCE_SURFACE(
            std::is_same_v<decltype(Outcome::Other), EntityId>,
            "Outcome::Other type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Outcome::Kind), InteractionKind>,
            "Outcome::Kind type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Outcome::Result), OutcomeResult>,
            "Outcome::Result type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(Outcome::Weight), float>,
            "Outcome::Weight type changed");
        LCE_SURFACE(
            sizeof(Outcome) == 24,
            "Outcome size changed (x64 ABI)");
    }

    //============================================================================
    // LCE/Simulation/Decision/Legacy.h
    //============================================================================

    namespace
    {
        using LegacyFact = LCE::Simulation::LegacyFact;
        LCE_SURFACE(
            std::is_same_v<decltype(LegacyFact::Owner), EntityId>,
            "LegacyFact::Owner type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(LegacyFact::Day), std::uint64_t>,
            "LegacyFact::Day type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(LegacyFact::Name), std::string>,
            "LegacyFact::Name type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(LegacyFact::Weight), float>,
            "LegacyFact::Weight type changed");

        using LegacyStore = LCE::Simulation::LegacyStore;
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LegacyStore::Leave),
                void (LegacyStore::*)(LegacyFact)>,
            "LegacyStore::Leave signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LegacyStore::Read),
                std::optional<LegacyFact> (LegacyStore::*)(
                    std::string_view) const>,
            "LegacyStore::Read signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LegacyStore::Forget),
                void (LegacyStore::*)(std::string_view)>,
            "LegacyStore::Forget signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LegacyStore::Empty),
                bool (LegacyStore::*)() const noexcept>,
            "LegacyStore::Empty signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LegacyStore::Serialize),
                std::optional<LCE::Simulation::ComponentBlob> (LegacyStore::*)()
                    const>,
            "LegacyStore::Serialize signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LegacyStore::Deserialize),
                void (LegacyStore::*)(const LCE::Simulation::ComponentBlob&)>,
            "LegacyStore::Deserialize signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LegacyStore::Clear),
                void (LegacyStore::*)() noexcept>,
            "LegacyStore::Clear signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<LegacyStore&>().SetSerializer(
                    LCE::Simulation::ComponentSerializer<
                        std::unordered_map<std::string, LegacyFact>>{})),
                void>,
            "LegacyStore::SetSerializer signature changed");
    }

    //============================================================================
    // LCE/Simulation/Entity/RegistrySnapshot.h
    //============================================================================

    namespace
    {
        using namespace LCE::Simulation;

        LCE_SURFACE(
            std::is_same_v<
                std::remove_cv_t<decltype(kSnapshotVersion)>,
                std::uint32_t>,
            "kSnapshotVersion is not a uint32");
        LCE_SURFACE(
            kSnapshotVersion == 2,
            "kSnapshotVersion changed (co-save contract — deliberate + changelog)");
        LCE_SURFACE(
            std::is_same_v<ComponentBlob, std::vector<std::byte>>,
            "ComponentBlob type changed");

        using Serializer = ComponentSerializer<LCE::Simulation::Needs>;
        LCE_SURFACE(
            std::is_same_v<
                decltype(Serializer::Serialize),
                std::function<ComponentBlob(const LCE::Simulation::Needs&)>>,
            "ComponentSerializer::Serialize type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(Serializer::Deserialize),
                std::function<LCE::Simulation::Needs(const ComponentBlob&)>>,
            "ComponentSerializer::Deserialize type changed");

        LCE_SURFACE(
            std::is_same_v<decltype(SnapshotComponent::Type), std::type_index>,
            "SnapshotComponent::Type type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SnapshotComponent::Data), ComponentBlob>,
            "SnapshotComponent::Data type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SnapshotEntity::Id), EntityId>,
            "SnapshotEntity::Id type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(SnapshotEntity::Components),
                std::vector<SnapshotComponent>>,
            "SnapshotEntity::Components type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(RegistrySnapshot::Version),
                std::uint32_t>,
            "RegistrySnapshot::Version type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(RegistrySnapshot::Entities),
                std::vector<SnapshotEntity>>,
            "RegistrySnapshot::Entities type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(RegistrySnapshot::Legacy),
                std::optional<ComponentBlob>>,
            "RegistrySnapshot::Legacy type changed");
    }

    //============================================================================
    // LCE/Simulation/Entity/EntityRegistry.h
    //============================================================================

    namespace
    {
        using EntityRegistry = LCE::Simulation::EntityRegistry;
        using Needs = LCE::Simulation::Needs;

        LCE_SURFACE(
            std::is_copy_constructible_v<EntityRegistry> == false,
            "EntityRegistry became copyable");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::SetEventSink),
                void (EntityRegistry::*)(LCE::Events::EventBus*) noexcept>,
            "EntityRegistry::SetEventSink signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::CreateEntity),
                EntityId (EntityRegistry::*)()>,
            "EntityRegistry::CreateEntity signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::DestroyEntity),
                void (EntityRegistry::*)(EntityId)>,
            "EntityRegistry::DestroyEntity signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::IsAlive),
                bool (EntityRegistry::*)(EntityId) const>,
            "EntityRegistry::IsAlive signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<EntityRegistry&>()
                    .AddComponent<Needs>(std::declval<EntityId>(), Needs{})),
                void>,
            "EntityRegistry::AddComponent signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<EntityRegistry&>()
                    .RemoveComponent<Needs>(std::declval<EntityId>())),
                void>,
            "EntityRegistry::RemoveComponent signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<const EntityRegistry&>()
                    .HasComponent<Needs>(std::declval<EntityId>())),
                bool>,
            "EntityRegistry::HasComponent signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<const EntityRegistry&>()
                    .GetComponent<Needs>(std::declval<EntityId>())),
                std::shared_ptr<Needs>>,
            "EntityRegistry::GetComponent signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<EntityRegistry&>()
                    .ForEachWithComponent<Needs>([](EntityId, Needs&) {})),
                void>,
            "EntityRegistry::ForEachWithComponent signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<const EntityRegistry&>()
                    .QueryWhere<Needs>(
                        [](EntityId, const Needs&) { return true; })),
                std::vector<EntityId>>,
            "EntityRegistry::QueryWhere signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(std::declval<EntityRegistry&>()
                    .RegisterSerializer<Needs>(
                        ComponentSerializer<Needs>{})),
                void>,
            "EntityRegistry::RegisterSerializer signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::Capture),
                RegistrySnapshot (EntityRegistry::*)() const>,
            "EntityRegistry::Capture signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::Restore),
                void (EntityRegistry::*)(const RegistrySnapshot&)>,
            "EntityRegistry::Restore signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::Clear),
                void (EntityRegistry::*)()>,
            "EntityRegistry::Clear signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::LeaveLegacy),
                void (EntityRegistry::*)(LegacyFact)>,
            "EntityRegistry::LeaveLegacy signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::ReadLegacy),
                std::optional<LegacyFact> (EntityRegistry::*)(
                    std::string_view) const>,
            "EntityRegistry::ReadLegacy signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::ForgetLegacy),
                void (EntityRegistry::*)(std::string_view)>,
            "EntityRegistry::ForgetLegacy signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&EntityRegistry::RegisterLegacySerializer),
                void (EntityRegistry::*)(
                    ComponentSerializer<
                        std::unordered_map<std::string, LegacyFact>>)>,
            "EntityRegistry::RegisterLegacySerializer signature changed");
    }

    //============================================================================
    // LCE/Simulation/SimulationEvents.h
    //============================================================================

    namespace
    {
        using namespace LCE::Simulation;

        LCE_SURFACE(
            std::is_base_of_v<
                LCE::Events::Event, EntityCreatedEvent>,
            "EntityCreatedEvent lost its Event base");
        LCE_SURFACE(
            std::is_constructible_v<EntityCreatedEvent, EntityId>,
            "EntityCreatedEvent lost its constructor");
        LCE_SURFACE(
            std::is_same_v<decltype(EntityCreatedEvent::Id), EntityId>,
            "EntityCreatedEvent::Id type changed");

        LCE_SURFACE(
            std::is_base_of_v<
                LCE::Events::Event, IntentProducedEvent>,
            "IntentProducedEvent lost its Event base");
        LCE_SURFACE(
            std::is_constructible_v<
                IntentProducedEvent, EntityId, Intent>,
            "IntentProducedEvent lost its constructor");
        LCE_SURFACE(
            std::is_same_v<decltype(IntentProducedEvent::Id), EntityId>,
            "IntentProducedEvent::Id type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(IntentProducedEvent::Intent), Intent>,
            "IntentProducedEvent::Intent type changed");

        LCE_SURFACE(
            std::is_base_of_v<
                LCE::Events::Event, OutcomeRecordedEvent>,
            "OutcomeRecordedEvent lost its Event base");
        LCE_SURFACE(
            std::is_constructible_v<
                OutcomeRecordedEvent, EntityId, Outcome>,
            "OutcomeRecordedEvent lost its constructor");
        LCE_SURFACE(
            std::is_same_v<decltype(OutcomeRecordedEvent::Id), EntityId>,
            "OutcomeRecordedEvent::Id type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(OutcomeRecordedEvent::Outcome), Outcome>,
            "OutcomeRecordedEvent::Outcome type changed");

        LCE_SURFACE(
            std::is_base_of_v<
                LCE::Events::Event, RelationshipChangedEvent>,
            "RelationshipChangedEvent lost its Event base");
        LCE_SURFACE(
            std::is_constructible_v<
                RelationshipChangedEvent,
                EntityId, EntityId, float, float, std::string, std::uint64_t>,
            "RelationshipChangedEvent lost its constructor");
        LCE_SURFACE(
            std::is_same_v<decltype(RelationshipChangedEvent::Subject), EntityId>,
            "RelationshipChangedEvent::Subject type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(RelationshipChangedEvent::Other), EntityId>,
            "RelationshipChangedEvent::Other type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(RelationshipChangedEvent::Disposition), float>,
            "RelationshipChangedEvent::Disposition type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(RelationshipChangedEvent::Trust), float>,
            "RelationshipChangedEvent::Trust type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(RelationshipChangedEvent::Threshold), std::string>,
            "RelationshipChangedEvent::Threshold type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(RelationshipChangedEvent::Day), std::uint64_t>,
            "RelationshipChangedEvent::Day type changed");
    }

    //============================================================================
    // LCE/Simulation/Simulation.h
    //============================================================================

    namespace
    {
        using namespace LCE::Simulation;

        using BondThreshold = LCE::Simulation::BondThreshold;
        LCE_SURFACE(
            std::is_same_v<decltype(BondThreshold::Name), std::string>,
            "BondThreshold::Name type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(BondThreshold::Value), float>,
            "BondThreshold::Value type changed");

        using SimulationTuning = LCE::Simulation::SimulationTuning;
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::MemoryFadeRate), float>,
            "SimulationTuning::MemoryFadeRate type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::ForgetThreshold), float>,
            "SimulationTuning::ForgetThreshold type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::DriftRate), float>,
            "SimulationTuning::DriftRate type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::GoalUrgencyRate), float>,
            "SimulationTuning::GoalUrgencyRate type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::TrustGain), float>,
            "SimulationTuning::TrustGain type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::DispositionGain), float>,
            "SimulationTuning::DispositionGain type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::DispositionLoss), float>,
            "SimulationTuning::DispositionLoss type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::NeedJitter), float>,
            "SimulationTuning::NeedJitter type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::GroupInheritance), float>,
            "SimulationTuning::GroupInheritance type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::HungerDesperate), float>,
            "SimulationTuning::HungerDesperate type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::MemoryCap), std::size_t>,
            "SimulationTuning::MemoryCap type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::BequestFloor), float>,
            "SimulationTuning::BequestFloor type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::InheritanceScale), float>,
            "SimulationTuning::InheritanceScale type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(SimulationTuning::LegacyMaxAgeDays), std::uint64_t>,
            "SimulationTuning::LegacyMaxAgeDays type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(SimulationTuning::BondThresholds),
                std::vector<BondThreshold>>,
            "SimulationTuning::BondThresholds type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&SimulationTuning::FromConfiguration),
                SimulationTuning (*)(const LCE::Config::Configuration&)>,
            "SimulationTuning::FromConfiguration signature changed");

        using TickReport = LCE::Simulation::TickReport;
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::Entities), std::uint64_t>,
            "TickReport::Entities type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::MemoryEvents), std::uint64_t>,
            "TickReport::MemoryEvents type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::Relationships), std::uint64_t>,
            "TickReport::Relationships type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::NeedsMs), double>,
            "TickReport::NeedsMs type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::MemoryMs), double>,
            "TickReport::MemoryMs type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::RelationshipsMs), double>,
            "TickReport::RelationshipsMs type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::GoalsMs), double>,
            "TickReport::GoalsMs type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::DecideMs), double>,
            "TickReport::DecideMs type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(TickReport::TotalMs), double>,
            "TickReport::TotalMs type changed");

        using FixedStep = LCE::Simulation::FixedStep;
        LCE_SURFACE(
            std::is_same_v<decltype(FixedStep::Remaining), double>,
            "FixedStep::Remaining type changed");
        LCE_SURFACE(
            std::is_same_v<decltype(FixedStep::Step), double>,
            "FixedStep::Step type changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&FixedStep::Advance),
                std::size_t (FixedStep::*)(
                    double,
                    EntityRegistry&,
                    const SimulationTuning&,
                    LCE::Events::EventBus*,
                    const Rng*,
                    TickReport*)>,
            "FixedStep::Advance signature changed");

        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::Update),
                void (*)(
                    EntityRegistry&,
                    double,
                    const SimulationTuning&,
                    LCE::Events::EventBus*,
                    const Rng*,
                    TickReport*)>,
            "Update signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::Remember),
                void (*)(
                    EntityRegistry&,
                    EntityId,
                    const MemoryEvent&,
                    const SimulationTuning&,
                    WorldTime,
                    LCE::Events::EventBus*)>,
            "Remember signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::ReportOutcome),
                void (*)(
                    EntityRegistry&,
                    EntityId,
                    const Outcome&,
                    const SimulationTuning&,
                    LCE::Events::EventBus*,
                    WorldTime)>,
            "ReportOutcome signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::InheritGroupAttitudes),
                void (*)(EntityRegistry&, EntityId, GroupId)>,
            "InheritGroupAttitudes signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::Bequeath),
                std::size_t (*)(
                    EntityRegistry&,
                    EntityId,
                    std::span<const EntityId>,
                    const SimulationTuning&)>,
            "Bequeath signature changed");
        LCE_SURFACE(
            std::is_same_v<
                decltype(&LCE::Simulation::InheritMemory),
                std::size_t (*)(
                    EntityRegistry&,
                    EntityId,
                    EntityId,
                    const SimulationTuning&,
                    WorldTime,
                    bool (*)(const MemoryEvent&))>,
            "InheritMemory signature changed");
    }

#undef LCE_SURFACE

    bool SurfaceTest()
    {
        // The freeze is enforced at compile time by the static_asserts
        // above. Reaching this line means every pinned declaration still
        // matches — the suite passes by construction.
        return true;
    }
}
