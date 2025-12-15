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

// ndt_core/collection/TopologyAndFlowMonitor.hpp
#pragma once

#include "common_types/GraphTypes.hpp" // for Graph
#include "common_types/SFlowType.hpp"  // for FlowKey, Path
#include "utils/Utils.hpp"             // for DeploymentMode
#include <array>                       // for array
#include <atomic>                      // for atomic
#include <cstdint>                     // for uint32_t, uint64_t, int64_t
#include <map>                         // for map
#include <memory>                      // for shared_ptr
#include <mutex>                       // for mutex
#include <nlohmann/json.hpp>           // for json
#include <optional>                    // for optional
#include <set>                         // for set
#include <shared_mutex>                // for shared_mutex
#include <string>                      // for string, allocator
#include <thread>                      // for thread
#include <unordered_map>               // for unordered_map
#include <utility>                     // for pair
#include <vector>                      // for vector
#include "utils/AppConfig.hpp"

// We don't need to use constexpr std::string_view
// Cuz the files are used for ifstream (which needs std::string as the param type)
// static const std::string TOPOLOGY_FILE = "../StaticNetworkTopology_ipAlias4_10Switches.json";
static const std::string TOPOLOGY_FILE = AppConfig::TOPOLOGY_FILE;
    
static const std::string TOPOLOGY_FILE_MININET = "../StaticNetworkTopologyMininet_10Switches.json";

static constexpr uint64_t EMPTY_LINK_THRESHOLD = 700000000;
static constexpr uint64_t MICE_FLOW_UNDER_THRESHOLD = 10000000;

using json = nlohmann::json;

class EventBus;

class TopologyAndFlowMonitor
{
  public:
    TopologyAndFlowMonitor(std::shared_ptr<Graph> graph,
                           std::shared_ptr<std::shared_mutex> graphMutex,
                           std::shared_ptr<EventBus> eventBus,
                           int mode);
    ~TopologyAndFlowMonitor();

    /**
     * @brief Starts the topology and flow monitoring services.
     *
     * This function sets the running flag to true and spawns two background threads:
     * 1. The main monitoring thread (run) for fetching topology data.
     * 2. The flow flushing thread (flushEdgeFlowLoop) for cleaning up stale flows.
     */
    void start();

    /**
     * @brief Destructor for the TopologyAndFlowMonitor class.
     *
     * Calls the stop() method to terminate the monitoring thread and the edge flow
     * flushing thread, ensuring a clean shutdown and proper resource release.
     */
    void stop();

    /**
     * @brief Loads and constructs the static network topology from a JSON configuration file.
     *
     * This function parses the specified JSON file to populate the internal graph structure
     * (`m_graph`). The process involves two main stages:
     * 1. **Nodes Parsing**: Iterates through the "nodes" array to create vertices (Switches or
     * Hosts), setting properties like DPID, MAC, IP, and device metadata. Special handling is
     * applied if the deployment mode is set to MININET (e.g., reading `bridge_name`).
     * 2. **Edges Parsing**: Iterates through the "edges" array to create connections between
     * vertices. Endpoints are resolved using DPID for switches or IP addresses for hosts.
     *
     * @param path The file system path to the JSON topology file.
     *
     * @note If the file cannot be opened, an error is logged and the function returns without
     * modifying the graph.
     * @note If an edge's source or destination cannot be resolved in the graph, the edge is skipped
     * and a warning is logged.
     */
    void logGraph();

    void updateLinkInfo(std::pair<uint32_t, uint32_t> agentIpAndPort,
                        uint64_t leftIn,
                        uint64_t leftOut,
                        uint64_t interfaceSpeed);

    void updateLinkInfoLeftLinkBandwidth(std::pair<uint32_t, uint32_t> agentIpAndPort,
                                         uint64_t estimatedIn);

    std::optional<Graph::vertex_descriptor> findVertexByIp(uint32_t ip) const;
    std::optional<Graph::vertex_descriptor> findVertexByIpNoLock(uint32_t ip) const;

    std::optional<Graph::edge_descriptor> findEdgeByAgentIpAndPort(
        const std::pair<uint32_t, uint32_t>& agentIpAndPort) const;
    std::optional<Graph::edge_descriptor> findEdgeByAgentIpAndPortNoLock(
        const std::pair<uint32_t, uint32_t>& agentIpAndPort) const;

    std::optional<Graph::edge_descriptor> findReverseEdgeByAgentIpAndPort(
        const std::pair<uint32_t, uint32_t>& agentIpAndPort) const;
    std::optional<Graph::edge_descriptor> findReverseEdgeByAgentIpAndPortNoLock(
        const std::pair<uint32_t, uint32_t>& agentIpAndPort) const;

    std::optional<std::pair<uint32_t, uint32_t>> getAgentKeyFromTheOtherSide(
        const std::pair<uint32_t, uint32_t>& agentIpAndPort) const;
    std::optional<std::pair<uint32_t, uint32_t>> getAgentKeyFromTheOtherSideNoLock(
        const std::pair<uint32_t, uint32_t>& agentIpAndPort) const;

    std::optional<Graph::edge_descriptor> findEdgeByDpidAndPort(
        std::pair<uint64_t, uint32_t> dpidAndPort) const;
    std::optional<Graph::edge_descriptor> findEdgeByDpidAndPortNoLock(
        std::pair<uint64_t, uint32_t> dpidAndPort) const;

    std::optional<Graph::edge_descriptor> findEdgeBySrcAndDstDpid(
        std::pair<uint64_t, uint64_t> srcDpidAndDstDpid) const;
    std::optional<Graph::edge_descriptor> findEdgeBySrcAndDstDpidNoLock(
        std::pair<uint64_t, uint64_t> srcDpidAndDstDpid) const;

    std::optional<Graph::edge_descriptor> findEdgeByHostIp(uint32_t hostIp) const;
    std::optional<Graph::edge_descriptor> findEdgeByHostIpNoLock(uint32_t hostIp) const;
    std::optional<Graph::edge_descriptor> findEdgeByHostIp(std::vector<uint32_t> hostIp) const;
    std::optional<Graph::edge_descriptor> findEdgeByHostIpNoLock(
        std::vector<uint32_t> hostIp) const;
    std::optional<Graph::edge_descriptor> findReverseEdgeByHostIp(uint32_t hostIp) const;
    std::optional<Graph::edge_descriptor> findReverseEdgeByHostIp(
        std::vector<uint32_t> hostIp) const;

    std::optional<Graph::edge_descriptor> findEdgeBySrcAndDstIp(uint32_t srcIp,
                                                                uint32_t dstIp) const;
    std::optional<Graph::edge_descriptor> findEdgeBySrcAndDstIpNoLock(uint32_t srcIp,
                                                                      uint32_t dstIp) const;

    void setEdgeDown(Graph::edge_descriptor e);
    void setEdgeDownNoLock(Graph::edge_descriptor e);
    void setEdgeUp(Graph::edge_descriptor e);
    void setEdgeUpNoLock(Graph::edge_descriptor e);
    void setEdgeEnable(Graph::edge_descriptor e);
    void setEdgeEnableNoLock(Graph::edge_descriptor e);
    void setEdgeDisable(Graph::edge_descriptor e);
    void setEdgeDisableNoLock(Graph::edge_descriptor e);
    void setVertexDown(Graph::vertex_descriptor v);
    void setVertexUp(Graph::vertex_descriptor v);
    bool getVertexIsUp(Graph::vertex_descriptor v);
    void setVertexEnable(Graph::vertex_descriptor v);
    void setVertexDisable(Graph::vertex_descriptor v);
    std::pair<uint64_t, uint32_t> getEdgeStats(Graph::edge_descriptor e) const;
    std::pair<uint64_t, uint32_t> getEdgeStatsNoLock(Graph::edge_descriptor e) const;
    // void setEdgeFlow(Graph::edge_descriptor e, sflow::FlowKey key, bool isAlive);
    // void setEdgeFlowNoLock(Graph::edge_descriptor e, sflow::FlowKey key, bool isAlive);
    std::set<sflow::FlowKey> getEdgeFlowSet(Graph::edge_descriptor e) const;
    std::set<sflow::FlowKey> getEdgeFlowSetNoLock(Graph::edge_descriptor e) const;
    int getEdgeElephantFlowCount(Graph::edge_descriptor e) const;
    Graph getGraph() const;
    void setVertexDeviceName(Graph::vertex_descriptor v, std::string name);
    void setVertexNickname(Graph::vertex_descriptor v, std::string name);
    bool getVertexIsEnabled(Graph::vertex_descriptor v);
    void setMininetBridgePorts(Graph::vertex_descriptor v, std::vector<std::string> ports);
    std::vector<std::string> getMininetBridgePorts(Graph::vertex_descriptor v);
    double getAvgLinkUsage(const Graph& g) const;

    std::optional<Graph::vertex_descriptor> findSwitchByDpid(uint64_t dpid) const;
    std::optional<Graph::vertex_descriptor> findSwitchByDpidNoLock(uint64_t dpid) const;

    std::optional<Graph::vertex_descriptor> findSwitchByIp(uint32_t ip) const;
    std::optional<Graph::vertex_descriptor> findSwitchByIpNoLock(uint32_t ip) const;

    std::optional<Graph::vertex_descriptor> findVertexByMac(uint64_t mac) const;
    std::optional<Graph::vertex_descriptor> findVertexByMacNoLock(uint64_t mac) const;

    std::optional<Graph::vertex_descriptor> findVertexByMininetBridgeName(
        const std::string& name) const;
    std::optional<Graph::vertex_descriptor> findVertexByMininetBridgeNameNoLock(
        const std::string& name) const;

    std::optional<Graph::vertex_descriptor> findVertexByDeviceName(const std::string& name) const;
    std::optional<Graph::vertex_descriptor> findVertexByDeviceNameNoLock(
        const std::string& name) const;

    std::map<uint64_t, std::string> m_dpidToIpStrMap;
    std::map<std::string, std::string> m_dpidStrToIpStrMap;
    std::map<std::string, uint64_t> m_ipStrToDpidMap;
    std::map<std::string, std::string> m_ipStrToDpidStrMap;

    std::vector<sflow::Path> getAllPathsBetweenTwoHosts(sflow::FlowKey flowKey,
                                                        uint64_t swDpid,
                                                        uint64_t dstSwDpid);

    void disableSwitchAndEdges(uint64_t dpid);
    void enableSwitchAndEdges(uint64_t dpid);

    std::vector<sflow::Path> bfsAllPathsToDst(
        const Graph& g,
        Graph::vertex_descriptor dstSwitch,
        const uint32_t& dstIp,
        const std::vector<uint32_t>& allHostIps,
        std::unordered_map<uint64_t, std::vector<std::pair<uint32_t, uint32_t>>>&
            newOpenflowTables);

    json getStaticTopologyJson();

    // for llm
    json getLinkBandwidthBetweenSwitches(const std::string& dpid1, const std::string& dpid2);
    json getTopKCongestedLinksJson(int k);
    // for llm

    bool touchEdgeFlow(Graph::edge_descriptor e, const sflow::FlowKey& key);

  private:
    std::mutex m_configurationFileMutex;
    void run();

    void fetchAndUpdateTopologyData();
    void updateSwitches(const std::string& topologyData);
    void updateHosts(const std::string& topologyData);
    void updateLinks(const std::string& topologyData);
    void updateGraph(const std::string&, const std::string&, const std::string&);

    void loadStaticTopologyFromFile(const std::string& path);
    void initializeMappingsFromGraph();
    void flushEdgeFlowLoop();

    uint64_t hashDstIp(const std::string& str);

    std::array<std::string, 3> m_ryuUrl;

    std::atomic<bool> m_running{false};

    std::thread m_thread;
    std::thread m_flushEdgeFlowLoop;

    std::shared_ptr<Graph> m_graph;
    std::shared_ptr<std::shared_mutex> m_graphMutex;
    std::shared_ptr<EventBus> m_eventBus;

    utils::DeploymentMode m_mode;
};
