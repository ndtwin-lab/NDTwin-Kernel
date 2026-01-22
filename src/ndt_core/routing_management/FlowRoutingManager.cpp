#include "ndt_core/routing_management/FlowRoutingManager.hpp"
#include "event_system/EventBus.hpp"                      // for Event
#include "event_system/EventPayloads.hpp"                 // for FlowAd...
#include "event_system/PayloadTypes.hpp"                  // for FlowAd...
#include "ndt_core/collection/FlowLinkUsageCollector.hpp" // for FlowLi...
#include "ndt_core/collection/TopologyAndFlowMonitor.hpp" // for Topolo...
#include "nlohmann/json.hpp"                              // for basic_...
#include "spdlog/spdlog.h"                                // for SPDLOG...
#include "utils/Logger.hpp"                               // for Logger
#include "utils/Utils.hpp"                                // for ipToSt...
#include <any>                                            // for any_cast
#include <functional>                                     // for function
#include <sstream>                                        // for basic_...
#include <stddef.h>                                       // for size_t
#include <unordered_map>                                  // for unorde...
#include <utility>                                        // for pair
#include <vector>                                         // for vector

using json = nlohmann::json;

FlowRoutingManager::FlowRoutingManager(
    std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
    std::shared_ptr<sflow::FlowLinkUsageCollector> collector,
    std::shared_ptr<EventBus> eventBus)
{
    m_topologyAndFlowMonitor = std::move(topologyAndFlowMonitor);
    m_flowLinkUsageCollector = std::move(collector);
    m_eventBus = std::move(eventBus);
}

FlowRoutingManager::~FlowRoutingManager()
{
}

void
FlowRoutingManager::deleteAnEntry(uint64_t dpid, json match, int priority)
{
    json jsonData;
    jsonData["dpid"] = dpid;
    jsonData["match"] = match;

    std::ostringstream cmd;

    if (priority == -1)
    {
        cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/flowentry/delete "
            << "-H \"Content-Type: application/json\" " << "-d '" << jsonData.dump() << "'";
    }
    else
    {
        jsonData["priority"] = priority;
        cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT
            << "/stats/flowentry/delete_strict " << "-H \"Content-Type: application/json\" "
            << "-d '" << jsonData.dump() << "'";
    }

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::installAnEntry(uint64_t dpid,
                                   int priority,
                                   json match,
                                   json action,
                                   int idleTimeout)
{
    json jsonData;
    jsonData["dpid"] = dpid;
    jsonData["priority"] = priority;
    jsonData["match"] = match;
    jsonData["actions"] = action;
    if (idleTimeout != -1)
    {
        jsonData["idle_timeout"] = idleTimeout;
    }

    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/flowentry/add "
        << "-H \"Content-Type: application/json\" " << "-d '" << jsonData.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::modifyAnEntry(uint64_t dpid, int priority, json match, json action)
{
    json jsonData;
    jsonData["dpid"] = dpid;
    jsonData["priority"] = priority;
    jsonData["match"] = match;
    jsonData["actions"] = action;

    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/flowentry/modify "
        << "-H \"Content-Type: application/json\" " << "-d '" << jsonData.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::installAGroupEntry(json j)
{
    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/groupentry/add "
        << "-H \"Content-Type: application/json\" " << "-d '" << j.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::deleteAGroupEntry(json j)
{
    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/groupentry/delete "
        << "-H \"Content-Type: application/json\" " << "-d '" << j.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::modifyAGroupEntry(json j)
{
    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/groupentry/modify "
        << "-H \"Content-Type: application/json\" " << "-d '" << j.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::installAMeterEntry(json j)
{
    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/meterentry/add "
        << "-H \"Content-Type: application/json\" " << "-d '" << j.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::deleteAMeterEntry(json j)
{
    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/meterentry/delete "
        << "-H \"Content-Type: application/json\" " << "-d '" << j.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}

void
FlowRoutingManager::modifyAMeterEntry(json j)
{
    std::ostringstream cmd;
    cmd << "curl -s -X POST http://" << AppConfig::RYU_IP_AND_PORT << "/stats/meterentry/modify "
        << "-H \"Content-Type: application/json\" " << "-d '" << j.dump() << "'";

    SPDLOG_LOGGER_INFO(Logger::instance(), "execCommand: {}", cmd.str());
    utils::execCommand(cmd.str());
}