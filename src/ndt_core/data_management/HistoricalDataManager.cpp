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

#include "ndt_core/data_management/HistoricalDataManager.hpp"
#include "common_types/GraphTypes.hpp"                    // for VertexProp...
#include "ndt_core/collection/TopologyAndFlowMonitor.hpp" // for TopologyAn...
#include "spdlog/spdlog.h"                                // for SPDLOG_LOG...
#include "utils/Logger.hpp"                               // for Logger
#include "utils/Utils.hpp"                                // for macToString
#include <boost/graph/adjacency_list.hpp>                 // for source
#include <boost/graph/detail/adj_list_edge_iterator.hpp>  // for adj_list_e...
#include <boost/graph/detail/adjacency_list.hpp>          // for edges
#include <boost/graph/detail/edge.hpp>                    // for edge_desc_...
#include <boost/iterator/iterator_facade.hpp>             // for operator!=
#include <boost/move/utility_core.hpp>                    // for move
#include <ctime>                                          // for strftime
#include <filesystem>                                     // for create_dir...
#include <fstream>                                        // for char_traits
#include <string>                                         // for operator+
#include <utility>                                        // for move

    HistoricalDataManager::HistoricalDataManager(std::shared_ptr<TopologyAndFlowMonitor> monitor,
                                             int mode,
                                             std::chrono::minutes interval)
    : m_topologyAndFlowMonitor(std::move(monitor)),
      m_mode(static_cast<utils::DeploymentMode>(mode)),
      m_interval(interval)
{
    // ensure our output directory exists
    std::filesystem::create_directories("/home/of-controller-sflow-collector/LinkData");
}

HistoricalDataManager::~HistoricalDataManager()
{
    stop();
}

void
HistoricalDataManager::start()
{
    if (m_running.exchange(true) or m_mode == utils::DeploymentMode::MININET)
    {
        // Already running
        return;
    }
    m_thread = std::thread(&HistoricalDataManager::run, this);
    SPDLOG_LOGGER_INFO(Logger::instance(), "HistoricalDataManager started.");
}

void
HistoricalDataManager::stop()
{
    m_running.store(false);
    if (m_thread.joinable())
    {
        m_thread.join();
        SPDLOG_LOGGER_INFO(Logger::instance(), "HistoricalDataManager stopped.");
    }
}

void
HistoricalDataManager::run()
{
    const std::string outDir = "/home/of-controller-sflow-collector/LinkData/";

    while (m_running.load())
    {
        // 1) Fetch a snapshot clone of the graph
        Graph graph = m_topologyAndFlowMonitor->getGraph();

        // 2) Build timestamp strings
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&t);

        char dateBuf[9]; // YYYYMMDD
        std::strftime(dateBuf, sizeof(dateBuf), "%Y%m%d", &local_tm);

        char dateTimeBuf[20]; // YYYY-MM-DD HH:MM:SS
        std::strftime(dateTimeBuf, sizeof(dateTimeBuf), "%Y-%m-%d %H:%M:%S", &local_tm);

        // 3) Iterate each edge and append to per-link CSV under outDir
        auto [ei, ei_end] = boost::edges(graph);
        int edgeCnt = 0;
        for (; ei != ei_end; ++ei)
        {
            edgeCnt++;
            SPDLOG_LOGGER_DEBUG(Logger::instance(), "edge cnt {}", edgeCnt);
            auto u = boost::source(*ei, graph);
            auto v = boost::target(*ei, graph);
            auto& eprop = graph[*ei];
            auto& up = graph[u];
            auto& vp = graph[v];

            SPDLOG_LOGGER_DEBUG(Logger::instance(),
                                "{} {} {} {} {} {} {} {}",
                                up.deviceName,
                                vp.deviceName,
                                up.dpid,
                                vp.dpid,
                                up.mac,
                                vp.mac,
                                up.vertexType == VertexType::SWITCH,
                                vp.vertexType == VertexType::SWITCH);
            const char* srcType = (up.vertexType == VertexType::SWITCH ? "switch" : "host");
            const char* dstType = (vp.vertexType == VertexType::SWITCH ? "switch" : "host");

            auto srcId = (up.vertexType == VertexType::SWITCH ? std::to_string(up.dpid)
                                                              : utils::macToString(up.mac));
            auto dstId = (vp.vertexType == VertexType::SWITCH ? std::to_string(vp.dpid)
                                                              : utils::macToString(vp.mac));

            // base filename: YYYYMMDD_srcDpid_dstDpid.csv
            std::string baseName = std::string(dateBuf) + "_" + srcId + "_" + dstId + ".csv";

            // full path under LinkData
            std::string fullPath = outDir + baseName;

            bool exists = std::filesystem::exists(fullPath);
            std::ofstream ofs(fullPath, std::ios::app);
            if (!exists)
            {
                ofs << "date-time,srcType,srcId,dstType,dstId,link_bw,link_bw_usage\n";
            }

            ofs << dateTimeBuf << "," << srcType << "," << srcId << "," << dstType << "," << dstId
                << "," << eprop.linkBandwidth << "," << eprop.linkBandwidthUsage << "\n";
        }

        // TODO[OPTIMIZW]: Change below methods to condition_variable (more efficient)
        // 4) Sleep until next interval (or until stop() is called)
        for (int i = 0; i < m_interval.count() * 60 && m_running.load(); ++i)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

}

void 
HistoricalDataManager::setLoggingState(bool enable)
{
    m_loggingEnabled.store(enable);
    SPDLOG_LOGGER_INFO(Logger::instance(), "Historical data logging has been {}.", (enable ? "ENABLED" : "DISABLED"));
}
