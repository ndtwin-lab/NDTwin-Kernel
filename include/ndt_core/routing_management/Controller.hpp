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
#include "ndt_core/routing_management/FlowDispatcher.hpp"

class FlowRoutingManager;

/**
 * @brief Owns long-lived control-plane components and exposes a shared FlowDispatcher.
 *
 * Controller wires together the FlowRoutingManager with a single FlowDispatcher instance
 * that is shared across all sessions/requests. The dispatcher is constructed with a sender
 * callback that ultimately applies FlowJobs through the FlowRoutingManager.
 *
 * Lifetime / ordering:
 *  - m_flowRoutingManager is declared before dispatcher_ to ensure it is constructed first
 *    and remains valid for the lifetime of dispatcher_ (the dispatcher may capture/use it).
 */
class Controller
{
  public:
    /**
     * @brief Construct the controller.
     *
     * @param flowRoutingManager Shared routing manager used by the dispatcher sender callback
     *                           to install/modify/delete flow rules.
     */
    Controller(std::shared_ptr<FlowRoutingManager> flowRoutingManager);

    /**
     * @brief Destroy the controller and release owned resources.
     *
     * Ensures the dispatcher is destroyed after the FlowRoutingManager pointer remains valid
     * for its lifetime (member destruction happens in reverse declaration order).
     */
    ~Controller();

    /**
     * @brief Access the shared FlowDispatcher.
     *
     * Returned reference is valid as long as this Controller instance lives.
     */
    FlowDispatcher& dispatcher()
    {
        return dispatcher_;
    }

  private:
    // Declare m_flowRoutingManager BEFORE dispatcher_ so it's constructed first
    std::shared_ptr<FlowRoutingManager> m_flowRoutingManager;

    FlowDispatcher dispatcher_; // long-lived, shared by all sessions
};
