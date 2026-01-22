#include "ndt_core/application_management/SimulationRequestManager.hpp"
#include "ndt_core/application_management/ApplicationManager.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>

SimulationRequestManager::SimulationRequestManager(std::shared_ptr<ApplicationManager> appManager,
                                                   std::string simServerUrl)
    : m_applicatonManager(std::move(appManager)),
      SIM_SERVER_URL(simServerUrl)
{
}

SimulationRequestManager::~SimulationRequestManager() = default;

std::string
SimulationRequestManager::requestSimulation(const std::string& body)
{
    std::ostringstream cmd;
    cmd << "curl -s -X POST \"" << SIM_SERVER_URL << "\" "
        << "-H \"Content-Type: application/json\" " << "-d '" << body << "'";

    std::string resp = utils::execCommand(cmd.str());
    SPDLOG_LOGGER_INFO(Logger::instance(),
                       "Requested simulation on {} - response: {}",
                       SIM_SERVER_URL,
                       resp);
    return resp;
}

void
SimulationRequestManager::onSimulationResult(int appId,
                                             const std::string& body)
{
    // Forward the result asynchronously
    std::thread([this, appId, body]() {
        auto apiUrlOpt = m_applicatonManager->getSimulationCompletedUrl(appId);
        if (!apiUrlOpt.has_value())
        {
            SPDLOG_LOGGER_WARN(Logger::instance(), "Cannot get Url from appId: {}", appId);
            return;
        }
        const std::string& apiUrl = apiUrlOpt.value();

        std::ostringstream cmd;
        cmd << "curl -s -X POST \"" << apiUrl << "\" " << "-H \"Content-Type: application/json\" "
            << "-d '" << body << "'";

        std::string result = utils::execCommand(cmd.str());
        SPDLOG_LOGGER_INFO(Logger::instance(), "Forwarded simulation result, response: {}", result);
    }).detach();
}