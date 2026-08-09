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

        return callCount == 1;
    }
}