/*
 * Copyright (c) 2025-present
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * The NDTwin Authors and Contributors:
 *     Prof. Shie-Yuan Wang <National Yang Ming Chiao Tung University; CITI, Academia Sinica>
 *     Ms. Xiang-Ling Lin <CITI, Academia Sinica>
 *     Mr. Po-Yu Juan <CITI, Academia Sinica>
 */

#pragma once

#include <any>           // for any
#include <functional>    // for function
#include <mutex>         // for unique_lock
#include <shared_mutex>  // for shared_lock, shared_mutex
#include <unordered_map> // for unordered_map, operator==, _Node_const_iter...
#include <utility>       // for move, pair
#include <vector>        // for vector

// Define supported event types
enum class EventType
{
    FlowAdded,            // 1. A new flow is added (triggered by Ryu request)
    LinkFailureDetected,  // 2. A link failure has been detected in the topology (triggered by Ryu
                          // request)
    IdleFlowPurged,       // 3. An idle flow has been removed
    LinkRecoveryDetected, // 4. A link recovery has been detected in the topology (triggered by Ryu
                          // request)
    SwitchEntered,
    SwitchExited
};

// Event structure containing type and payload
struct Event
{
    EventType type;
    std::any payload; // Can hold any payload data, e.g., PacketInPayload
};

class EventBus
{
  public:
    using Handler = std::function<void(const Event&)>;

    // Register a handler for a specific event type
    void registerHandler(EventType type, Handler handler)
    {
        std::unique_lock lock(m_mutex);
        m_handlers[type].push_back(std::move(handler));
    }

    // Emit an event (synchronously calls all registered handlers)
    void emit(const Event& event) const
    {
        std::shared_lock lock(m_mutex);
        auto it = m_handlers.find(event.type);
        if (it != m_handlers.end())
        {
            for (const auto& handler : it->second)
            {
                handler(event); // Call all handlers for this event
            }
        }
    }

  private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<EventType, std::vector<Handler>> m_handlers;
};
