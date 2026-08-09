#include "LCE/Events/EventBus.h"

#include <utility>

namespace LCE::Events
{
    void EventBus::Subscribe(
        std::type_index eventType,
        EventHandler handler)
    {
        // Handlers are keyed by std::type_index: the bus never needs to know
        // what an event IS, only that it has a stable identity (typeid).
        // This is type erasure — the bus stores and dispatches without ever
        // naming a concrete event type.
        m_Handlers[eventType].push_back(std::move(handler));
    }

    void EventBus::Publish(const Event& event)
    {
        const auto iterator = m_Handlers.find(
            std::type_index(typeid(event)));

        if (iterator == m_Handlers.end())
        {
            return;
        }

        // Dispatch is exact-type: a handler registered for a base type does
        // not receive derived events. Predictable and O(1) per lookup.
        //
        // The handler list is copied before delivery: a handler may call
        // Subscribe() while an event is being published, and push_back can
        // reallocate the very vector being iterated. Delivering from a
        // snapshot keeps Publish safe. A handler that subscribes mid-publish
        // misses the current event and receives the next one.
        const auto handlers = iterator->second;

        for (const auto& handler : handlers)
        {
            handler(event);
        }
    }
}