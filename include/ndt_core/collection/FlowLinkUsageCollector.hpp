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

// ndt_core/collection/FlowLinkUsageCollector.hpp
#pragma once

#include "common_types/SFlowType.hpp" // for Path, CounterInfo, FlowInfo
#include "utils/Utils.hpp"            // for DeploymentMode
#include <atomic>                     // for atomic
#include <cstdint>                    // for uint32_t
#include <map>                        // for map
#include <memory>                     // for shared_ptr
#include <mutex>                      // for mutex
#include <nlohmann/json.hpp>          // for json
#include <shared_mutex>               // for shared_mutex
#include <string>                     // for string
#include <thread>                     // for thread
#include <unordered_map>              // for unordered_map
#include <utility>                    // for pair
#include <vector>                     // for vector

class DeviceConfigurationAndPowerManager; // lines 48-48
class EventBus;                           // lines 47-47
class FlowRoutingManager;                 // lines 46-46
class TopologyAndFlowMonitor;             // lines 45-45

namespace sflow
{

#define SAMPLING_RATE 1000
#define SFLOW_PORT 6343
#define BUFFER_SIZE 65535
#define POLLING_INTERVAL 4      // seconds
#define FLOW_IDLE_TIMEOUT 15000 // milliseconds

class FlowLinkUsageCollector
{
  public:
    FlowLinkUsageCollector(std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
                           std::shared_ptr<FlowRoutingManager> flowRoutingManager,
                           std::shared_ptr<DeviceConfigurationAndPowerManager> deviceManager,
                           std::shared_ptr<EventBus> eventBusm,
                           int mode);
    ~FlowLinkUsageCollector();

    void start();
    void stop();

    // TODO: Prevent to get the whole flow table
    std::unordered_map<FlowKey, FlowInfo, FlowKeyHash> getFlowInfoTable();

    nlohmann::json getFlowInfoJson();
    nlohmann::json getTopKFlowInfoJson(int k);

    void setAllPaths(std::vector<sflow::Path> allPathsVector);
    std::map<std::pair<uint32_t, uint32_t>, Path> getAllPaths();
    void setAllPath(std::pair<uint32_t, uint32_t> ipPair, Path path);
    std::vector<uint32_t> getAllHostIps();
    void printAllPathMap();
    void updateAllPathMapAfterModOpenflowEntry(
        std::vector<std::pair<uint32_t, uint32_t>> affectedFlows,
        uint32_t dstIp);
    void updateAllPathMapAfterModOpenflowEntries(
        std::vector<std::pair<std::vector<std::pair<uint32_t, uint32_t>>, uint32_t>>
            affectedFlowsAndDstIpForEachModifiedEntry);

    std::optional<size_t> getSwitchCount(std::pair<uint32_t, uint32_t> ipPair);
    std::map<std::pair<uint32_t, uint32_t>, size_t> getAllSwitchCounts();

    // LLM
    json getPathBetweenHostsJson(const std::string& srcHostName, const std::string& dstHostName);
    // LLM

  private:
    inline std::string ourIpToString(uint32_t ipFront, uint32_t ipBack);
    inline uint32_t ipFromFrontBack(uint32_t ipFront, uint32_t ipBack);
    void calAvgFlowSendingRatesPeriodically();
    void calAvgFlowSendingRatesImmediately();
    void testCalAvgFlowSendingRatesRandomly();
    void run();
    void handlePacket(char* buffer);
    void purgeIdleFlows();
    void fetchAllDestinationPaths();

    std::unordered_map<FlowKey, FlowInfo, FlowKeyHash> m_flowInfoTable;

    // key -> agent_ip and port
    // value -> last_report_time, last_received_input_octets and
    // last_received_output_octets, ...
    std::map<std::pair<uint32_t, uint32_t>, CounterInfo> m_counterReports;

    std::atomic<int> m_sockfd{-1};
    std::atomic<bool> m_running{false};

    std::thread m_pktRcvThread;
    std::thread m_calAvgFlowSendingRateThreadPeriodically;
    std::thread m_testCalAvgFlowSendingRatesRandomly;
    std::thread m_purgeThread;

    mutable std::shared_mutex m_flowInfoTableMutex;

    std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
    std::shared_ptr<FlowRoutingManager> m_flowRoutingManager;
    std::shared_ptr<DeviceConfigurationAndPowerManager> m_deviceConfigurationAndPowerManager;
    std::shared_ptr<EventBus> m_eventBus;

    utils::DeploymentMode m_mode;

    void populateIfIndexToOfportMap();
    std::unordered_map<uint32_t, uint32_t> m_ifIndexToOfportMap;
    std::mutex m_ifIndexMapMutex; // To protect the map during population and access

    // key -> (src ip, dst ip), value -> full path
    std::map<std::pair<uint32_t, uint32_t>, Path> m_allPathMap;

    // calc the count of switch
    std::map<std::pair<uint32_t, uint32_t>, size_t> m_switchCountMap;
    mutable std::shared_mutex m_switchCountMapMutex;
};

} // namespace sflow
