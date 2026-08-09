#include "LCE/Events/EventBus.h"

#include <utility>

namespace LCE::Events
{
    void EventBus::Subscribe(
        std::type_index eventType,
        EventHandler handler)
    {
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

        for (const auto& handler : iterator->second)
        {
            handler(event);
        }
    }
}