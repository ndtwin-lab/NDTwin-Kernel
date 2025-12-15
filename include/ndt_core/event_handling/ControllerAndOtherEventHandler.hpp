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

// ndt_core/event_handling/ControllerAndOtherEvenHandler.hpp
#pragma once

#include "utils/Utils.hpp"                // for DeploymentMode
#include <atomic>                         // for atomic
#include <boost/asio/ip/tcp.hpp>          // for tcp
#include <boost/beast/http/error.hpp>     // for http
#include <memory>                         // for shared_ptr, allocator, enable_...
#include <mutex>                          // for mutex
#include <nlohmann/json.hpp>              // for json
#include <string>                         // for string
#include <thread>                         // for thread
#include <unordered_set>                  // for unordered_set
class DeviceConfigurationAndPowerManager; // lines 61-61
class EventBus;                           // lines 59-59
class FlowRoutingManager;                 // lines 58-58
class TopologyAndFlowMonitor;             // lines 51-51
class ApplicationManager;
class SimulationRequestManager;
class IntentTranslator;
class HistoricalDataManager;
class Controller;
class LockManager;

namespace boost
{
namespace asio
{
class io_context;
}
} // namespace boost

namespace sflow
{
class FlowLinkUsageCollector;
} // namespace sflow

#define NDT_PORT 8000

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

class ControllerAndOtherEventHandler
    : public std::enable_shared_from_this<ControllerAndOtherEventHandler>
{
  public:
    explicit ControllerAndOtherEventHandler(
        boost::asio::io_context& ioc,
        std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
        std::shared_ptr<sflow::FlowLinkUsageCollector> collector,
        std::shared_ptr<FlowRoutingManager> flowRoutingManager,
        std::shared_ptr<DeviceConfigurationAndPowerManager> deviceConfigurationAndPowermanager,
        std::shared_ptr<EventBus> eventBus,
        std::shared_ptr<ApplicationManager> applicationManager,
        std::shared_ptr<SimulationRequestManager> simManager,
        std::shared_ptr<IntentTranslator> intentTranslator,
        std::shared_ptr<HistoricalDataManager> historicalDataManager, 
        std::shared_ptr<Controller> ctrl,
        std::shared_ptr<LockManager> lockManager,
        int mode,
        std::string api_url = "http://localhost:8000/ndt");
    ~ControllerAndOtherEventHandler();

    void runServer();

    void start();
    void stop();

    json parseFlowStatsText(const std::string& responseText);

  private:
    void doAccept();

    std::atomic<bool> m_serverRunning{false};
    net::io_context& m_ioContext;

    std::unique_ptr<tcp::acceptor> m_serverAcceptor;
    std::thread m_serverThread;

    std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
    std::shared_ptr<sflow::FlowLinkUsageCollector> m_flowLinkUsageCollector;
    std::shared_ptr<FlowRoutingManager> m_flowRoutingManager;
    std::shared_ptr<DeviceConfigurationAndPowerManager> m_deviceConfigurationAndPowerManager;
    std::shared_ptr<EventBus> m_eventBus;
    std::shared_ptr<ApplicationManager> m_applicationManager;
    std::shared_ptr<SimulationRequestManager> m_simulationRequestManager;
    std::shared_ptr<IntentTranslator> m_intentTranslator;
    std::shared_ptr<HistoricalDataManager> m_historicalDataManager; 
    std::shared_ptr<Controller> m_controller; 
    
    utils::DeploymentMode m_mode;
    std::string m_apiUrl;
   

    std::mutex m_socketsMutex;
    std::unordered_set<std::shared_ptr<tcp::socket>> m_activeSockets;

    std::shared_ptr<LockManager> m_lockManager;
};