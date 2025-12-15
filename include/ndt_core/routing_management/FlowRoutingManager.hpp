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

// ndt_core/routing_management/FlowRoutingManager.hpp
#pragma once

#include "common_types/SFlowType.hpp" // for FlowKey (ptr only), Path
#include <memory>                     // for shared_ptr, unique_ptr
#include <nlohmann/json.hpp>          // for json
#include <optional>                   // for optional
#include <stdint.h>                   // for uint64_t
#include <string>                     // for string
class EventBus;                       // lines 44-44
class TopologyAndFlowMonitor;         // lines 36-36

namespace sflow
{
class FlowLinkUsageCollector;
} // namespace sflow
struct FlowAddedEventPayload;

using json = nlohmann::json;
using namespace std;

class FlowRoutingManager
{
  public:
    FlowRoutingManager(std::string apiUrl,
                       std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
                       std::shared_ptr<sflow::FlowLinkUsageCollector> collector,
                       std::shared_ptr<EventBus> eventBus);
    ~FlowRoutingManager();


    void deleteAnEntry(uint64_t dpid, json match);
    void installAnEntry(uint64_t dpid, int priority, json match, json action, int idleTimeout = 0);
    void modifyAnEntry(uint64_t dpid, int priority, json match, json action);

    void installAGroupEntry(json j);
    void deleteAGroupEntry(json j);
    void modifyAGroupEntry(json j);

    void installAMeterEntry(json j);
    void deleteAMeterEntry(json j);
    void modifyAMeterEntry(json j);

  private:

    std::shared_ptr<EventBus> m_eventBus;

    std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
    std::shared_ptr<sflow::FlowLinkUsageCollector> m_flowLinkUsageCollector;
};