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
#include "common_types/GraphTypes.hpp"
#include "event_system/EventBus.hpp"
#include "ndt_core/application_management/ApplicationManager.hpp"
#include "ndt_core/application_management/SimulationRequestManager.hpp"
#include "ndt_core/collection/FlowLinkUsageCollector.hpp"
#include "ndt_core/collection/TopologyAndFlowMonitor.hpp"
#include "ndt_core/data_management/HistoricalDataManager.hpp"
#include "ndt_core/event_handling/ControllerAndOtherEventHandler.hpp"
#include "ndt_core/intent_translator/IntentTranslator.hpp"
#include "ndt_core/power_management/DeviceConfigurationAndPowerManager.hpp"
#include "ndt_core/routing_management/FlowRoutingManager.hpp"
#include "ndt_core/routing_management/Controller.hpp"
#include "ndt_core/lock_management/LockManager.hpp"
#include "spdlog/spdlog.h"
#include "utils/Logger.hpp"
#include <atomic>
#include <boost/asio/impl/io_context.ipp>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <limits>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include "utils/AppConfig.hpp"

// TODO[OPTIMIZATION]: Third-party api url
std::string thirdPartyUrlForRouting = "http://localhost:5000/fair_share_path";
std::string SIM_SERVER_URL = AppConfig::SIM_SERVER_URL;
std::string GW_IP = AppConfig::GW_IP;

static std::atomic<bool> gShutdownRequested{false};

void
handleSigint(int)
{
    gShutdownRequested.store(true);
}


int
promptDeploymentMode()
{
    int choice = -1;
    std::cout << "Select your deployment environment:\n";
    std::cout << "  [1] Local Mininet (simulated testbed)\n";
    std::cout << "  [2] Remote Testbed (physical or virtual deployment)\n";
    std::cout << "Enter your choice (1-2): ";

    while (true)
    {
        std::cin >> choice;

        if (std::cin.fail() || choice < 1 || choice > 2)
        {
            std::cin.clear();                                                   // clear error flags
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard bad input
            std::cout << "Invalid input. Please enter 1 or 2: ";
        }
        else
        {
            break;
        }
    }

    return choice;
}

std::string
promptOpenAIModel()
{
    std::string model;
    // std::cout << "Enter the OpenAI model you want to use (e.g., gpt-4.1-mini): ";
    // std::cin >> model;

    // if (model.empty())
    // {
    //     std::cerr << "Model name cannot be empty. Using default: o4-mini." << std::endl;
    //     model = "o4-mini";
    // }

    return model;
}

int
main(int argc, char* argv[])
{
    // Temporarily use command line prompt to configure running mode.
    int mode = promptDeploymentMode();
    if (mode == 1)
    {
        std::cout << "Running in Mininet environment.\n";
    }
    else
    {
        std::cout << "Running in Remote Testbed environment.\n";
    }


    auto cfg = Logger::parse_cli_args(argc, argv);
    Logger::init(cfg);
    SPDLOG_LOGGER_INFO(Logger::instance(), "Logger Loads Successfully! level");

    std::signal(SIGINT, handleSigint);

    net::io_context ioc{1};

    auto graph = std::make_shared<Graph>();
    auto graphMutex = std::make_shared<std::shared_mutex>();
    auto eventBus = std::make_shared<EventBus>();
    std::shared_ptr<FlowRoutingManager> flowRoutingManager;
    std::shared_ptr<DeviceConfigurationAndPowerManager> deviceConfigurationAndPowerManager;

    auto topologyAndFlowMonitor =
        std::make_shared<TopologyAndFlowMonitor>(graph, graphMutex, eventBus, mode);

    deviceConfigurationAndPowerManager =
        std::make_shared<DeviceConfigurationAndPowerManager>(topologyAndFlowMonitor, mode, GW_IP);

    auto collector =
        std::make_shared<sflow::FlowLinkUsageCollector>(topologyAndFlowMonitor,
                                                        flowRoutingManager,
                                                        deviceConfigurationAndPowerManager,
                                                        eventBus,
                                                        mode);

    auto dataManager = std::make_unique<HistoricalDataManager>(topologyAndFlowMonitor, mode);

    flowRoutingManager = std::make_shared<FlowRoutingManager>(thirdPartyUrlForRouting,
                                                              topologyAndFlowMonitor,
                                                              collector,
                                                              eventBus);

    std::string openaiModel = promptOpenAIModel();
    auto intentTranslator = std::make_shared<IntentTranslator>(deviceConfigurationAndPowerManager,
                                                               topologyAndFlowMonitor,
                                                               flowRoutingManager,
                                                               collector,
                                                               openaiModel);

    auto appManager = std::make_shared<ApplicationManager>("/srv/nfs/sim", "/mnt");

    auto simManager = std::make_shared<SimulationRequestManager>(appManager, SIM_SERVER_URL);

    auto historicalDataManager =
        std::make_shared<HistoricalDataManager>(topologyAndFlowMonitor, mode);

    auto controller = std::make_shared<Controller>(flowRoutingManager);

    auto lockManager = std::make_shared<LockManager>();

    auto handler =
        std::make_shared<ControllerAndOtherEventHandler>(ioc,
                                                         topologyAndFlowMonitor,
                                                         collector,
                                                         flowRoutingManager,
                                                         deviceConfigurationAndPowerManager,
                                                         eventBus,
                                                         appManager,
                                                         simManager,
                                                         intentTranslator,
                                                         historicalDataManager,
                                                         controller,
                                                         lockManager,
                                                         mode);

    topologyAndFlowMonitor->start();
    collector->start();
    dataManager->start();
    handler->start();
    deviceConfigurationAndPowerManager->start();

    while (!gShutdownRequested.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    SPDLOG_LOGGER_INFO(Logger::instance(), "Shutdown requested. Cleaning up…");

    topologyAndFlowMonitor->stop();
    collector->stop();
    dataManager->stop();
    handler->stop();
    deviceConfigurationAndPowerManager->stop();

    SPDLOG_LOGGER_INFO(Logger::instance(), "All subsystems stopped. Exiting.");

    return 0;
}
