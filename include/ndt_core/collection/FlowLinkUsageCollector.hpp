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

namespace ndtClassifier
{
class Classifier;
struct FlowKey;
}

namespace sflow
{

#define SFLOW_PORT 6343
#define BUFFER_SIZE 65535
#define FLOW_IDLE_TIMEOUT 15000 // milliseconds

/**
 * @brief Collects sFlow samples and derives per-flow / per-link usage and paths.
 *
 * FlowLinkUsageCollector listens for sFlow datagrams (flow samples + counter samples),
 * maintains an in-memory flow table (m_flowInfoTable) and per-interface counter snapshots
 * (m_counterReports), and periodically computes average sending rates and other statistics.
 *
 * It also maintains a (src,dst) -> Path mapping and switch-count metadata, and can refresh
 * these paths after routing changes (e.g., OpenFlow entry modifications).
 *
 * Concurrency:
 *  - start()/stop() control background threads.
 *  - m_flowInfoTable is protected by a shared mutex (readers/writers).
 *  - counter report map and IF-index mapping are updated internally; callers should treat
 *    returned data as snapshots.
 *
 * Deployment modes:
 *  - Behavior may differ depending on m_mode (e.g., MININET vs TESTBED), such as how
 *    sampling rate or counter conversion is handled.
 */
class FlowLinkUsageCollector
{
  public:
    FlowLinkUsageCollector(std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
                           std::shared_ptr<FlowRoutingManager> flowRoutingManager,
                           std::shared_ptr<DeviceConfigurationAndPowerManager> deviceManager,
                           std::shared_ptr<EventBus> eventBusm,
                           int mode,
                           std::shared_ptr<ndtClassifier::Classifier> classifier);
    ~FlowLinkUsageCollector();

    /**
     * @brief Start sFlow reception and background maintenance threads.
     *
     * Creates/opens the UDP socket (SFLOW_PORT) and launches worker threads:
     *  - packet receive loop
     *  - periodic average-rate calculation
     *  - idle-flow purge loop
     *  - optional debug/testing tasks (if enabled)
     *
     */
    void start();
    /**
     * @brief Stop all worker threads and close the sFlow socket.
     *
     * Signals threads to exit (m_running=false), joins them, and releases socket resources.
     * Safe to call during shutdown.
     */
    void stop();

    // TODO: Prevent to get the whole flow table
    /**
     * @brief Return a snapshot of the current flow table.
     *
     * The returned map is a copy of internal state, keyed by FlowKey with FlowInfo values.
     * Intended for external consumers (e.g., REST handlers) to query current flow statistics.
     *
     * @note Copying the whole table can be expensive for large workloads.
     *       Consider adding query filters or a "top-K" interface for production use.
     */
    std::unordered_map<FlowKey, FlowInfo, FlowKeyHash> getFlowInfoTable();

    nlohmann::json getFlowInfoJson();
    nlohmann::json getTopKFlowInfoJson(int k);

    /**
     * @brief Replace the entire (src,dst)->Path map using a vector of paths.
     *
     * Typically called after the topology monitor or routing manager produces updated
     * end-to-end paths.
     *
     * @param allPathsVector List of paths to be loaded into the internal map.
     */
    void setAllPaths(std::vector<sflow::Path> allPathsVector);
    std::map<std::pair<uint32_t, uint32_t>, Path> getAllPaths();
    /**
     * @brief Update the path for one specific (srcIp,dstIp) pair.
     *
     * @param ipPair (srcIp, dstIp)
     * @param path   Full path for that pair
     */
    void setAllPath(std::pair<uint32_t, uint32_t> ipPair, Path path);
    /**
     * @brief Return all host IPs known to the collector / path map.
     *
     * Used by higher layers to enumerate endpoints for querying paths/stats.
     */
    std::vector<uint32_t> getAllHostIps();
    void printAllPathMap();

    /**
     * @brief Get the number of switches on the path between a (src,dst) pair.
     *
     * @param ipPair (srcIp,dstIp)
     * @return Optional switch count if the path is known; std::nullopt otherwise.
     */
    std::optional<size_t> getSwitchCount(std::pair<uint32_t, uint32_t> ipPair);
    /**
     * @brief Return a snapshot of all computed switch counts for all (src,dst) pairs.
     *
     * @return Map from (srcIp,dstIp) to switch count.
     */
    std::map<std::pair<uint32_t, uint32_t>, size_t> getAllSwitchCounts();

    json getPathBetweenHostsJson(const std::string& srcHostName, const std::string& dstHostName);

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
    void calFlowPathByQueried();

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
    std::thread m_calFlowPathByQueried;

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

    std::shared_ptr<ndtClassifier::Classifier> m_classifier;
};

} // namespace sflow
