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

// ndt_core/power_management/DeviceConfigurationAndPowerManager.hpp
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

/**
 * SwitchInfo describes mapping between switch IP and smart plug details.
 */
struct SwitchInfo
{
    std::string switch_ip;
    std::string plug_ip;
    int plug_idx;
};

// This should align with real-world smart plug configuration

/**
 * DeviceConfigurationAndPowerManager centralizes querying switch power state
 * either via TESTBED (real hardware) or MININET (simulator).
 */
class DeviceConfigurationAndPowerManager
{
  public:
    DeviceConfigurationAndPowerManager(std::shared_ptr<TopologyAndFlowMonitor> topoMonitor,
                                       int mode);

    /**
     * Handle a get_switches_power_state request.
     * @param target full request target, e.g. "/ndt/get_switches_power_state?ip=..."
     * @return JSON mapping switch IP → "ON"/"OFF"/"error"
     * @throws runtime_error if ip is unknown
     */
    json getSwitchesPowerState(const std::string& target);
    bool setSwitchPowerState(std::string ip, std::string action, SwitchInfo si);

    json getPowerReport();
    json getMemoryUtilization();
    json getOpenFlowTables();
    // first element in tuple -> dst ip, second -> output port, third -> priority

    std::unordered_map<uint64_t, std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>>
    getOpenFlowTable(uint64_t dpid = 0);
    json getCpuUtilization();
    json getTemperature();

    /// Toggle power for the given switch IP ("on" or "off").
    /// Returns true on success, false if IP not found or the operation failed.
    bool setSwitchPowerState(const std::string& ip, const std::string& action);

    // For llm
    json getSingleSwitchPowerReport(const std::string& deviceIdentifier);
    json getSingleSwitchCpuReport(const std::string& deviceIdentifier);
    // For llm

    void updateOpenFlowTables(const json& j);

    void start();
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
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> parseFlowStatsTextToVector(
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
};