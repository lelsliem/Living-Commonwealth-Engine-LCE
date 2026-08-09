//=============================================================================//
//
// Living Commonwealth Engine (LCE)
// Building living worlds through simulation.
//
// File:
//
//      EventBusTest.cpp
//
// Purpose:
//
//      Verifies the Event Bus delivers events to registered handlers and
//      stays safe when a handler subscribes while an event is being
//      published (reentrancy).
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

namespace
{
    class TestEvent : public LCE::Events::Event
    {
    };
}

namespace LCE::Tests
{
    bool EventBusTest()
    {
        LCE::Events::EventBus eventBus;

        int callCount = 0;

        eventBus.Subscribe(
            std::type_index(typeid(TestEvent)),
            [&callCount](const LCE::Events::Event&)
            {
                ++callCount;
            });

        TestEvent event;

        eventBus.Publish(event);

        if (callCount != 1)
        {
            return false;
        }

        // Reentrancy: a handler that subscribes while an event is being
        // published must not corrupt the bus, and must not receive the
        // current event — delivery happens from a snapshot.
        int lateCount = 0;
        bool subscribed = false;

        auto lateHandler =
            [&eventBus, &lateCount, &subscribed](const LCE::Events::Event&)
            {
                if (!subscribed)
                {
                    subscribed = true;

                    eventBus.Subscribe(
                        std::type_index(typeid(TestEvent)),
                        [&lateCount](const LCE::Events::Event&)
                        {
                            ++lateCount;
                        });
                }
            };

        eventBus.Subscribe(
            std::type_index(typeid(TestEvent)),
            lateHandler);

        eventBus.Publish(event);

        if (lateCount != 0)
        {
            return false;   // the mid-publish subscriber missed this event
        }

        eventBus.Publish(event);

        if (lateCount != 1)
        {
            return false;   // ... and receives the next one
        }

        return true;
    }
}
