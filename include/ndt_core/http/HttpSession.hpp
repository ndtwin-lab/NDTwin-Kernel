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

#include "utils/Utils.hpp" // For utils::DeploymentMode
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Forward declarations to reduce header dependencies
class TopologyAndFlowMonitor;
class EventBus;
class FlowRoutingManager;
class DeviceConfigurationAndPowerManager;
class ApplicationManager;
class SimulationRequestManager;
class IntentTranslator;
class HistoricalDataManager;
class Controller;
class LockManager;

namespace sflow
{
class FlowLinkUsageCollector;
}

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

/**
 * @class HttpSession
 * @brief Manages an asynchronous HTTP server session using Boost.Beast.
 *
 * This class handles reading an HTTP request from a connected socket,
 * processing it, and sending back an appropriate HTTP response. It is
 * designed to be managed by a std::shared_ptr and keeps itself alive
 * during asynchronous operations.
 */
class HttpSession : public std::enable_shared_from_this<HttpSession>
{
  public:
    /**
     * @brief Construct a new Http Session object.
     * @param socket The connected TCP socket to handle.
     * @param ...deps Various shared pointers to core application components.
     */
    HttpSession(
        tcp::socket socket,
        std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
        std::shared_ptr<EventBus> eventBus,
        int mode,
        std::shared_ptr<sflow::FlowLinkUsageCollector> collector,
        std::shared_ptr<FlowRoutingManager> flowRoutingManager,
        std::shared_ptr<DeviceConfigurationAndPowerManager> deviceConfigurationAndPowermanager,
        std::shared_ptr<ApplicationManager> appManager,
        std::shared_ptr<SimulationRequestManager> simManager,
        std::shared_ptr<IntentTranslator> intentTranslator,
        std::shared_ptr<HistoricalDataManager> historicalDataManager,
        std::shared_ptr<Controller> ctrl,
        std::shared_ptr<LockManager> lockManager);

    /**
     * @brief Starts the asynchronous operation for the session.
     */
    void start();

  private:
    // --- Asynchronous Operation Handlers ---
    void readRequest();
    void onRead(beast::error_code ec, std::size_t bytesTransferred);
    void writeResponse();
    void onWrite(beast::error_code ec, std::size_t bytesTransferred);
    void closeSocket();

    // --- Request Routing and Handling ---
    void handleRequest();

    // Each API endpoint gets its own handler function for clarity.
    void handleFlowAdded(http::response<http::string_body>& res);
    void handleLinkFailure(http::response<http::string_body>& res);
    void handleLinkRecovery(http::response<http::string_body>& res);
    void handleGetGraphData(http::response<http::string_body>& res);
    void handleGetDetectedFlowData(http::response<http::string_body>& res);
    void handleGetSwitchOpenflowEntries(http::response<http::string_body>& res);
    void handleGetPowerReport(http::response<http::string_body>& res);
    void handleDisableSwitch(http::response<http::string_body>& res);
    void handleEnableSwitch(http::response<http::string_body>& res);
    void handleGetSwitchesPowerState(http::response<http::string_body>& res);
    void handleSetSwitchesPowerState(http::response<http::string_body>& res);
    void handleInstallFlowEntry(http::response<http::string_body>& res);
    void handleDeleteFlowEntry(http::response<http::string_body>& res);
    void handleModifyFlowEntry(http::response<http::string_body>& res);
    void handleInstallGroupEntry(http::response<http::string_body>& res);
    void handleDeleteGroupEntry(http::response<http::string_body>& res);
    void handleModifyGroupEntry(http::response<http::string_body>& res);
    void handleInstallMeterEntry(http::response<http::string_body>& res);
    void handleDeleteMeterEntry(http::response<http::string_body>& res);
    void handleModifyMeterEntry(http::response<http::string_body>& res);
    void handleInstallModifyDeleteFlowEntries(http::response<http::string_body>& res);
    void handleGetCpuUtilization(http::response<http::string_body>& res);
    void handleGetMemoryUtilization(http::response<http::string_body>& res);
    void handleInformSwitchEntered(http::response<http::string_body>& res);
    void handleModifyDeviceName(http::response<http::string_body>& res);
    void handleReceivedSimulationCase(http::response<http::string_body>& res);
    void handleSimulationCompleted(http::response<http::string_body>& res);
    void handleGetStaticTopology(http::response<http::string_body>& res);
    void handleInformAllDestinationPaths(http::response<http::string_body>& res);
    void handleAppRegister(http::response<http::string_body>& res);
    void handleNotFound(http::response<http::string_body>& res);
    void handleInputTextIntent(http::response<http::string_body>& res);
    void handleGetNickname(http::response<http::string_body>& res);
    void handleModifyNickname(http::response<http::string_body>& res);
    void handleGetTemperature(http::response<http::string_body>& res);
    void handleGetPathSwitchCount(http::response<http::string_body>& res);
    void handleGetOpenflowCapacity(http::response<http::string_body>& res);
    void handleSetHistoricalLoggingState(http::response<http::string_body>& res);
    void handleGetAvgLinkUsage(http::response<http::string_body>& res);
    void handleGetTotalInputTrafficLoadPassingASwitch(http::response<http::string_body>& res);
    void handleGetNumOfFlowsPassingASwitch(http::response<http::string_body>& res);
    void handleAcquireLock(http::response<http::string_body>& res);
    void handleRenewLock(http::response<http::string_body>& res);
    void handleReleaseLock(http::response<http::string_body>& res);

    void processFlowBatch(const json& j, http::response<http::string_body>& res);

    std::function<void()> after_write_;

    void doClose()
    {
        beast::error_code ec;
        m_socket.shutdown(tcp::socket::shutdown_send, ec);
    }

    // --- Member Variables ---
    tcp::socket m_socket;
    beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;

    // The response must be stored in a shared_ptr to keep it alive during async write
    std::shared_ptr<http::response<http::string_body>> m_res;

    // Core application components (dependencies)
    std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
    std::shared_ptr<EventBus> m_eventBus;
    utils::DeploymentMode m_mode;
    std::shared_ptr<sflow::FlowLinkUsageCollector> m_flowLinkUsageCollector;
    std::shared_ptr<FlowRoutingManager> m_flowRoutingManager;
    std::shared_ptr<DeviceConfigurationAndPowerManager> m_deviceConfigurationAndPowerManager;
    std::shared_ptr<ApplicationManager> m_applicationManager;
    std::shared_ptr<SimulationRequestManager> m_simulationRequestManager;
    std::shared_ptr<IntentTranslator> m_intentTranslator;
    std::shared_ptr<HistoricalDataManager> m_historicalDataManager;
    std::shared_ptr<Controller> m_controller;
    std::shared_ptr<LockManager> m_lockManager;
};