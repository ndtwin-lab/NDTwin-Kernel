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
// ndt_core/data_management/HistoricalDataManager.hpp
#pragma once

#include "utils/Utils.hpp"    // for DeploymentMode
#include <atomic>             // for atomic
#include <chrono>             // for minutes
#include <memory>             // for shared_ptr
#include <thread>             // for thread
class TopologyAndFlowMonitor; // lines 34-34

/**
 * @brief Periodically records historical link-bandwidth usage.
 */
class HistoricalDataManager
{
  public:
    static constexpr std::chrono::minutes DEFAULT_INTERVAL{5};
    /**
     * @param monitor  Shared pointer to the topology & flow monitor
     *                 from which to fetch bandwidth data.
     * @param interval Interval between recordings (default: 5 minutes).
     */
    HistoricalDataManager(std::shared_ptr<TopologyAndFlowMonitor> monitor,
                          int mode,
                          std::chrono::minutes interval = DEFAULT_INTERVAL);

    ~HistoricalDataManager();

    /// Start the recording thread.
    void start();

    /// Request shutdown and join the thread.
    void stop();
    void setLoggingState(bool enable);

  private:
    /// Main loop executed in the background thread.
    void run();

    std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
    utils::DeploymentMode m_mode;
    std::chrono::minutes m_interval;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::atomic<bool> m_loggingEnabled{true}; 
};