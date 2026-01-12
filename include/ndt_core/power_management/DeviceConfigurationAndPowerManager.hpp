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
 * NDTwin core contributors (as of January 15, 2026):
 *     Prof. Shie-Yuan Wang <National Yang Ming Chiao Tung University; CITI, Academia Sinica>
 *     Ms. Xiang-Ling Lin <CITI, Academia Sinica>
 *     Mr. Po-Yu Juan <CITI, Academia Sinica>
 *     Mr. Tsu-Li Mou <CITI, Academia Sinica>
 *     Mr. Zhen-Rong Wu <National Taiwan Normal University>
 *     Mr. Ting-En Chang <University of Wisconsin, Milwaukee>
 *     Mr. Yu-Cheng Chen <National Yang Ming Chiao Tung University>
 */

#pragma once

#include "utils/Utils.hpp"   // for DeploymentMode
#include <atomic>            // for atomic
#include <memory>            // for shared_ptr
#include <nlohmann/json.hpp> // for json
#include <shared_mutex>
#include <stdint.h>           // for uint32_t, uint64_t
#include <string>             // for string, basic_string
#include <thread>             // for thread
#include <tuple>              // for tuple
#include <unordered_map>      // for unordered_map
#include <vector>             // for vector
class TopologyAndFlowMonitor; // lines 34-34

using json = nlohmann::json;

namespace ndtClassifier
{
class Classifier;
}

/**
 * @brief Mapping between a switch management IP and its smart plug control endpoint.
 *
 * Used in TESTBED mode to power on/off a physical switch through a smart plug relay.
 */
struct SwitchInfo
{
    std::string switchIp;
    std::string plugIp;
    int plugIdx;
};

// This should align with real-world smart plug configuration

/**
 * @brief Central manager for switch power control and device status telemetry.
 *
 * DeviceConfigurationAndPowerManager provides a unified interface to:
 *  - query and toggle switch power state (TESTBED: via smart plug / gateway; MININET: via
 * simulator),
 *  - periodically collect and cache device health metrics (power, CPU, memory, temperature),
 *  - fetch and cache OpenFlow table snapshots for switches, and
 *  - expose results in JSON form for REST/API handlers.
 *
 * The class runs background worker threads when start() is called:
 *  - a ping worker to track reachability (optional/implementation-defined),
 *  - a status update worker that refreshes cached telemetry,
 *  - an OpenFlow table update worker that refreshes cached flow tables.
 *
 * Concurrency:
 *  - Cached status JSON is protected by m_statusMutex (shared_mutex).
 *  - Cached OpenFlow tables are protected by m_openflowTablesMutex.
 *  - Public getters return snapshots from the cache.
 *
 * Deployment:
 *  - TESTBED mode controls real hardware using the smart plug table and GW_IP gateway URL.
 *  - MININET mode derives state from the simulator (e.g., OVS/Mininet topology).
 */
class DeviceConfigurationAndPowerManager
{
  public:
    /**
     * @brief Construct the device manager.
     *
     * @param topoMonitor Topology monitor used for mapping/lookup (e.g., IP<->switch).
     * @param mode        Deployment mode (TESTBED / MININET).
     * @param gwUrl       Gateway URL/IP used to reach the testbed control service (TESTBED).
     */
    DeviceConfigurationAndPowerManager(std::shared_ptr<TopologyAndFlowMonitor> topoMonitor,
                                       int mode,
                                       std::string gwUrl,
                                       std::shared_ptr<ndtClassifier::Classifier> classifier);

    /**
     * @brief Query power state for one or more switches.
     *
     * Parses the "ip" query parameter from @p target. If ip is omitted or indicates
     * "all" (implementation-defined), it returns states for all known switches.
     *
     * @param target Full request target, e.g. "/ndt/get_switches_power_state?ip=10.10.10.12".
     * @return JSON mapping switch IP -> state string (e.g., "ON", "OFF", "error").
     * @throws std::runtime_error if the requested IP is unknown or not resolvable.
     */
    json getSwitchesPowerState(const std::string& target);

    /**
     * @brief Toggle power for a specific switch using a provided SwitchInfo record.
     *
     * Primarily intended for TESTBED mode where the plug endpoint is already known.
     *
     * @param ip     Switch IP address (typically matches si.switch_ip).
     * @param action "on" or "off".
     * @param si     Smart plug mapping for this switch.
     * @return true on success; false on failure.
     */
    bool setSwitchPowerState(std::string ip, std::string action, SwitchInfo si);

    /**
     * @brief Toggle power for a switch using the appropriate backend.
     *
     * @param ip     Switch IP address.
     * @param action "on" or "off" (case handling is implementation-defined).
     * @return true on success; false if the IP is unknown or the operation fails.
     */
    bool setSwitchPowerState(const std::string& ip, const std::string& action);

    /**
     * @brief Get the latest cached power report for all devices.
     *
     * @return JSON snapshot of the most recent power report.
     */
    json getPowerReport();
    /**
     * @brief Get the latest cached memory utilization report.
     */
    json getMemoryUtilization();
    /**
     * @brief Get the latest cached OpenFlow table snapshot as JSON.
     *
     * @return JSON describing OpenFlow tables for switches (schema implementation-defined).
     */
    json getOpenFlowTables();

    /**
     * @brief Get parsed OpenFlow table entries as a structured map.
     *
     * @param dpid If non-zero, return entries only for the specified switch DPID.
     *             If zero, return entries for all switches.
     * @return Map: dpid -> list of (dstIp, outPort, priority, mask) tuples.
     */
    std::unordered_map<uint64_t, std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>>>
    getOpenFlowTable(uint64_t dpid = 0);
    /**
     * @brief Get the latest cached CPU utilization report.
     */
    json getCpuUtilization();
    /**
     * @brief Get the latest cached temperature report.
     */
    json getTemperature();

    /**
     * @brief Get the cached power report for one device/switch.
     *
     * @param deviceIdentifier Switch identifier (IP, name, or DPID depending on implementation).
     * @return JSON report for the specified device, or an error JSON if unknown.
     */
    json getSingleSwitchPowerReport(const std::string& deviceIdentifier);
    /**
     * @brief Get the cached CPU report for one device/switch.
     *
     * @param deviceIdentifier Switch identifier (IP, name, or DPID depending on implementation).
     * @return JSON report for the specified device, or an error JSON if unknown.
     */
    json getSingleSwitchCpuReport(const std::string& deviceIdentifier);

    /**
     * @brief Update cached OpenFlow tables with externally provided JSON.
     *
     * Useful when another subsystem fetches OpenFlow state and pushes it into this manager.
     *
     * @param j OpenFlow table JSON snapshot.
     */
    void updateOpenFlowTables(const json& j);

    /**
     * @brief Start background workers that refresh cached status and OpenFlow tables.
     *
     * Launches m_pingThread, m_statusUpdateThread and m_openflowTablesUpdateThread.
     */
    void start();
    /**
     * @brief Stop all background workers and release resources.
     *
     * Signals shutdown, joins threads, and leaves cached values intact.
     */
    void stop();

  private:
    std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
    utils::DeploymentMode m_mode;
    std::atomic<bool> m_running{false};
    std::thread m_pingThread;

    void fetchSmartPlugInfoFromFile(const std::string& path);
    // Extract "ip" parameter from target; empty if absent
    std::string parseIpParam(const std::string& target) const;

    // Query real hardware via Flask relay for TESTBED mode
    json queryTestbed(const std::string& ipParam) const;

    // Query Mininet topology for switch up/down state
    json queryMininet(const std::string& ipParam) const;
    json parseFlowStatsTextToJson(const std::string& responseText) const;
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> parseFlowStatsTextToVector(
        const std::string& responseText) const;

    bool pingSwitch(const std::string& ip, int timeout_sec);
    void pingWorker(int interval_sec);

    // Helpers for TESTBED mode
    bool setPowerStateTestbed(const SwitchInfo& si, const std::string& action);
    // Helpers for MININET mode
    bool setPowerStateMininet(uint32_t ipUint, const std::string& action);

    /**
     * @brief The main loop for the m_statsUpdateThread.
     *
     * Periodically calls all the "fetch...Internal" functions and
     * updates the cached member variables under a lock.
     */
    void statusUpdateWorker();
    void openflowTablesUpdateWorker();
    // --- The *actual* (slow) data-fetching functions ---
    // These are the original implementations, just renamed.
    json fetchPowerReportInternal();
    json fetchMemoryReportInternal();
    json fetchCpuReportInternal();
    json fetchTemperatureReportInternal();
    json fetchOpenFlowTablesInternal();

    std::vector<SwitchInfo> switchSmartPlugTable;

    std::thread m_statusUpdateThread;
    std::thread m_openflowTablesUpdateThread;
    mutable std::shared_mutex m_statusMutex;
    mutable std::shared_mutex m_openflowTablesMutex;

    json m_cachedPowerReport;
    json m_cachedCpuReport;
    json m_cachedMemoryReport;
    json m_cachedTemperatureReport;
    json m_cachedOpenFlowTables;

    std::string GW_IP;

    std::shared_ptr<ndtClassifier::Classifier> m_classifier;
};