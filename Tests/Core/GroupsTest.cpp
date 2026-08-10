//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      GroupsTest.cpp
//
// Purpose:
//
//      Verifies the Society stone (0.6.0 stone 09): the Groups component
//      and its deterministic query; the vicarious echo — trust is earned
//      personally, disposition travels to group-mates ("they wronged my
//      brother"); and InheritGroupAttitudes — feelings inherit from the
//      group's experiences, then diverge. Plus the proof: one wrong,
//      many minds — every member of a settlement turns cold, and the
//      bond lines fire for each of them.
//
// SPDX-License-Identifier: MIT
//
//=============================================================================//

#include "LCE/Config/Configuration.h"
#include "LCE/Events/EventBus.h"
#include "LCE/Simulation/Simulation.h"
#include "LCE/Simulation/SimulationEvents.h"

#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

namespace LCE::Tests
{
    namespace
    {
        //-------------------------------------------------------------------------
        // Captures the (subject, threshold) pairs of every crossing, to
        // prove the echo reaches the right minds and fires the right lines.
        //-------------------------------------------------------------------------
        struct CrossingRecord
        {
            int Count = 0;
            std::vector<Simulation::EntityId> Subjects;
            std::vector<std::string> Names;

            void Subscribe(LCE::Events::EventBus& bus)
            {
                bus.Subscribe(
                    std::type_index(typeid(Simulation::RelationshipChangedEvent)),
                    [this](const LCE::Events::Event& event)
                    {
                        const auto& e =
                            static_cast<const Simulation::RelationshipChangedEvent&>(event);
                        ++Count;
                        Subjects.push_back(e.Subject);
                        Names.push_back(e.Threshold);
                    });
            }
        };

        Simulation::SimulationTuning WithEcho(float inheritance)
        {
            Config::Configuration config;

            config.Set("sim.group.inheritance", std::to_string(inheritance));

            return Simulation::SimulationTuning::FromConfiguration(config);
        }
    }

    bool GroupsTest()
    {
        //-------------------------------------------------------------------------
        // 1. Membership: the Groups component and its deterministic
        //    query — the two members of group 7, in ascending id order.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto outsider = registry.CreateEntity();
            const auto first = registry.CreateEntity();
            const auto second = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                first, Simulation::Groups{ { Simulation::GroupId{ 7 } } });
            registry.AddComponent<Simulation::Groups>(
                second, Simulation::Groups{
                    { Simulation::GroupId{ 7 }, Simulation::GroupId{ 3 } } });

            const auto members = registry.QueryWhere<Simulation::Groups>(
                [](Simulation::EntityId, const Simulation::Groups& groups)
                {
                    for (const auto group : groups.Memberships)
                    {
                        if (group == Simulation::GroupId{ 7 })
                        {
                            return true;
                        }
                    }

                    return false;
                });

            if (members.size() != 2
                || members[0] != first
                || members[1] != second)
            {
                return false;   // exactly the members, in ascending order
            }

            (void)outsider;
        }

        //-------------------------------------------------------------------------
        // 2. The echo, cold: a wrong done to one settler cools every
        //    member of their settlement toward the wrongdoer — at
        //    GroupInheritance strength, fainter than the subject's own.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto subject = registry.CreateEntity();
            const auto mate = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                subject, Simulation::Groups{ { Simulation::GroupId{ 1 } } });
            registry.AddComponent<Simulation::Groups>(
                mate, Simulation::Groups{ { Simulation::GroupId{ 1 } } });

            const auto tuning = WithEcho(0.5f);

            Simulation::ReportOutcome(
                registry, subject,
                { merchant, Simulation::InteractionKind::Wronged,
                  Simulation::OutcomeResult::Success },
                tuning);

            const auto subjectRelationships =
                registry.GetComponent<Simulation::Relationships>(subject);
            const auto mateRelationships =
                registry.GetComponent<Simulation::Relationships>(mate);

            if (!subjectRelationships || !mateRelationships)
            {
                return false;
            }

            const auto subjectIterator =
                subjectRelationships->ByEntity.find(merchant);
            const auto mateIterator =
                mateRelationships->ByEntity.find(merchant);

            if (subjectIterator == subjectRelationships->ByEntity.end()
                || subjectIterator->second.Disposition > -0.249f)   // full loss
            {
                return false;
            }

            if (mateIterator == mateRelationships->ByEntity.end()
                || mateIterator->second.Disposition > -0.124f   // half, the echo
                || mateIterator->second.Disposition < -0.126f)
            {
                return false;   // "they wronged my brother" — fainter, but felt
            }
        }

        //-------------------------------------------------------------------------
        // 3. The echo, warm: kindness reaches the settlement too.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto subject = registry.CreateEntity();
            const auto mate = registry.CreateEntity();
            const auto healer = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                subject, Simulation::Groups{ { Simulation::GroupId{ 1 } } });
            registry.AddComponent<Simulation::Groups>(
                mate, Simulation::Groups{ { Simulation::GroupId{ 1 } } });

            const auto tuning = WithEcho(0.5f);

            Simulation::ReportOutcome(
                registry, subject,
                { healer, Simulation::InteractionKind::Aid,
                  Simulation::OutcomeResult::Success },
                tuning);

            const auto mateRelationships =
                registry.GetComponent<Simulation::Relationships>(mate);

            if (!mateRelationships)
            {
                return false;
            }

            const auto iterator = mateRelationships->ByEntity.find(healer);

            if (iterator == mateRelationships->ByEntity.end()
                || iterator->second.Disposition < 0.049f
                || iterator->second.Disposition > 0.051f)
            {
                return false;   // +0.1 × 0.5 — the settlement warms
            }
        }

        //-------------------------------------------------------------------------
        // 4. Trust is earned personally: a trade's trust does NOT echo.
        //    The mate is left untouched — reliability is evidence, not
        //    gossip.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto subject = registry.CreateEntity();
            const auto mate = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                subject, Simulation::Groups{ { Simulation::GroupId{ 1 } } });
            registry.AddComponent<Simulation::Groups>(
                mate, Simulation::Groups{ { Simulation::GroupId{ 1 } } });

            const auto tuning = WithEcho(0.5f);

            Simulation::ReportOutcome(
                registry, subject,
                { merchant, Simulation::InteractionKind::Trade,
                  Simulation::OutcomeResult::Success },
                tuning);

            if (registry.HasComponent<Simulation::Relationships>(mate))
            {
                return false;   // the echo never touched the mate
            }
        }

        //-------------------------------------------------------------------------
        // 5. No groups, no echo — the world without Society behaves
        //    exactly as before (backward compatibility).
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto subject = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            const auto tuning = WithEcho(0.5f);

            Simulation::ReportOutcome(
                registry, subject,
                { merchant, Simulation::InteractionKind::Wronged,
                  Simulation::OutcomeResult::Success },
                tuning);

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(subject);

            if (!relationships
                || relationships->ByEntity.at(merchant).Disposition > -0.249f)
            {
                return false;   // the direct effect, and nothing else
            }
        }

        //-------------------------------------------------------------------------
        // 6. The proof: one wrong, many minds. The settlement's members
        //    each cross the enemy line — RelationshipChanged fires for the
        //    mate as well as the subject. One outcome, two minds, two
        //    crossings.
        //-------------------------------------------------------------------------
        {
            LCE::Events::EventBus bus;
            Simulation::EntityRegistry registry;

            const auto subject = registry.CreateEntity();
            const auto mate = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                subject, Simulation::Groups{ { Simulation::GroupId{ 1 } } });
            registry.AddComponent<Simulation::Groups>(
                mate, Simulation::Groups{ { Simulation::GroupId{ 1 } } });

            Config::Configuration config;

            config.Set("sim.group.inheritance", "1.0");
            config.Set("sim.bond.threshold.enemy", "-0.1");

            const auto tuning = Simulation::SimulationTuning::FromConfiguration(config);

            CrossingRecord record;
            record.Subscribe(bus);

            Simulation::ReportOutcome(
                registry, subject,
                { merchant, Simulation::InteractionKind::Wronged,
                  Simulation::OutcomeResult::Success },
                tuning, &bus);

            if (record.Count != 2
                || record.Names[0] != "enemy"
                || record.Names[1] != "enemy"
                || (record.Subjects[0] != subject && record.Subjects[0] != mate)
                || (record.Subjects[1] != subject && record.Subjects[1] != mate)
                || record.Subjects[0] == record.Subjects[1])
            {
                return false;   // both minds crossed; both are news
            }
        }

        //-------------------------------------------------------------------------
        // 7. Inheritance: a newcomer's disposition toward everyone the
        //    group collectively knows becomes the group's mean — +0.4 and
        //    +0.8 mean +0.6.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto memberA = registry.CreateEntity();
            const auto memberB = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();
            const auto newcomer = registry.CreateEntity();

            for (const auto member : { memberA, memberB })
            {
                registry.AddComponent<Simulation::Groups>(
                    member, Simulation::Groups{ { Simulation::GroupId{ 9 } } });
            }

            registry.AddComponent<Simulation::Relationships>(
                memberA,
                Simulation::Relationships{
                    { { merchant, Simulation::Relationship{ 0.4f, 0.0f } } } });
            registry.AddComponent<Simulation::Relationships>(
                memberB,
                Simulation::Relationships{
                    { { merchant, Simulation::Relationship{ 0.8f, 0.0f } } } });

            registry.AddComponent<Simulation::Groups>(
                newcomer, Simulation::Groups{ { Simulation::GroupId{ 9 } } });

            Simulation::InheritGroupAttitudes(
                registry, newcomer, Simulation::GroupId{ 9 });

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(newcomer);

            if (!relationships)
            {
                return false;
            }

            const auto iterator = relationships->ByEntity.find(merchant);

            if (iterator == relationships->ByEntity.end()
                || iterator->second.Disposition < 0.59f
                || iterator->second.Disposition > 0.61f)
            {
                return false;   // the mean of the settlement's experience
            }

            // Trust was never inherited — the newcomer trusts no one yet.
            if (iterator->second.Trust != 0.0f)
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------
        // 8. Personal knowledge beats inherited: a newcomer who already
        //    knows the merchant keeps their own feeling — divergence has
        //    already begun.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto member = registry.CreateEntity();
            const auto merchant = registry.CreateEntity();
            const auto newcomer = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                member, Simulation::Groups{ { Simulation::GroupId{ 9 } } });
            registry.AddComponent<Simulation::Relationships>(
                member,
                Simulation::Relationships{
                    { { merchant, Simulation::Relationship{ 0.9f, 0.0f } } } });

            registry.AddComponent<Simulation::Groups>(
                newcomer, Simulation::Groups{ { Simulation::GroupId{ 9 } } });
            registry.AddComponent<Simulation::Relationships>(
                newcomer,
                Simulation::Relationships{
                    { { merchant, Simulation::Relationship{ -0.3f, 0.0f } } } });

            Simulation::InheritGroupAttitudes(
                registry, newcomer, Simulation::GroupId{ 9 });

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(newcomer);

            if (!relationships
                || relationships->ByEntity.at(merchant).Disposition > -0.29f)
            {
                return false;   // the newcomer's own feeling stands
            }
        }

        //-------------------------------------------------------------------------
        // 9. No self-knowledge is inherited: a member who knows the
        //    newcomer does not make the newcomer know themself.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto member = registry.CreateEntity();
            const auto newcomer = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                member, Simulation::Groups{ { Simulation::GroupId{ 9 } } });
            registry.AddComponent<Simulation::Relationships>(
                member,
                Simulation::Relationships{
                    { { newcomer, Simulation::Relationship{ 0.5f, 0.0f } } } });

            registry.AddComponent<Simulation::Groups>(
                newcomer, Simulation::Groups{ { Simulation::GroupId{ 9 } } });

            Simulation::InheritGroupAttitudes(
                registry, newcomer, Simulation::GroupId{ 9 });

            const auto relationships =
                registry.GetComponent<Simulation::Relationships>(newcomer);

            if (!relationships
                || relationships->ByEntity.contains(newcomer))
            {
                return false;   // no relationship with the self
            }
        }

        //-------------------------------------------------------------------------
        // 10. A world fact names no one — there is no relationship to
        //     echo, so the settlement hears nothing.
        //-------------------------------------------------------------------------
        {
            Simulation::EntityRegistry registry;

            const auto subject = registry.CreateEntity();
            const auto mate = registry.CreateEntity();

            registry.AddComponent<Simulation::Groups>(
                subject, Simulation::Groups{ { Simulation::GroupId{ 1 } } });
            registry.AddComponent<Simulation::Groups>(
                mate, Simulation::Groups{ { Simulation::GroupId{ 1 } } });

            const auto tuning = WithEcho(0.5f);

            Simulation::Remember(
                registry, subject,
                { Simulation::EntityId{}, Simulation::InteractionKind::Trade, 1.0f },
                tuning);

            if (registry.HasComponent<Simulation::Relationships>(mate))
            {
                return false;   // nothing to feel about nobody
            }
        }

        return true;
    }
}
