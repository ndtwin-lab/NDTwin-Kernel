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

#include "event_system/PayloadTypes.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <nlohmann/json.hpp>
#include <optional>

inline std::optional<FlowAddedEventPayload>
parseFlowAddedEventPayload(const std::string& jsonStr)
{
    // TODO[IMPLEMENT] New Flow Added Event
    // using json = nlohmann::json;
    // using sflow::FlowKey;
    // using sflow::Path;

    // try
    // {
    //     json data = json::parse(jsonStr);

    //     if (!data.contains("all_paths") || !data["all_paths"].is_array())
    //     {
    //         SPDLOG_LOGGER_ERROR(Logger::instance(), "Missing or invalid all_paths array");
    //         return std::nullopt;
    //     }

    //     const auto& allPathsJson = data["all_paths"];
    //     std::vector<Path> allPathsVector;

    //     uint32_t srcIpUint = 0;
    //     uint32_t dstIpUint = 0;

    //     if (allPathsJson.empty() || !allPathsJson[0].is_array())
    //     {
    //         SPDLOG_LOGGER_ERROR(Logger::instance(), "First path is missing or invalid");
    //         return std::nullopt;
    //     }

    //     const auto& srcNode = data["ip_src"];
    //     const auto& dstNode = data["ip_dst"];

    //     if (srcNode.is_string())
    //     {
    //         SPDLOG_LOGGER_DEBUG(Logger::instance(), "ip_src: {}", srcNode.get<std::string>());
    //         srcIpUint = utils::ipStringToUint32(srcNode.get<std::string>());
    //     }
    //     if (dstNode.is_string())
    //     {
    //         SPDLOG_LOGGER_DEBUG(Logger::instance(), "ip_dst: {}", dstNode.get<std::string>());
    //         dstIpUint = utils::ipStringToUint32(dstNode.get<std::string>());
    //     }

    //     FlowKey flowKey = {srcIpUint, dstIpUint};

    //     // Convert all_paths_json to vector<Path>
    //     for (const auto& pathJson : allPathsJson)
    //     {
    //         if (!pathJson.is_array())
    //         {
    //             continue;
    //         }
    //         Path path;
    //         for (const auto& nodeJson : pathJson)
    //         {
    //             if (!nodeJson.is_array() || nodeJson.size() != 2)
    //             {
    //                 continue;
    //             }

    //             uint64_t nodeId = 0;
    //             uint32_t port = 0;

    //             if (nodeJson[0].is_string())
    //             {
    //                 nodeId = utils::ipStringToUint32(nodeJson[0].get<std::string>());
    //             }
    //             else if (nodeJson[0].is_number())
    //             {
    //                 nodeId = nodeJson[0].get<uint64_t>();
    //             }

    //             if (nodeJson[1].is_number())
    //             {
    //                 port = nodeJson[1].get<int>();
    //             }
    //             else if (nodeJson[1].is_string())
    //             {
    //                 port = std::stoi(nodeJson[1].get<std::string>());
    //             }

    //             path.emplace_back(nodeId, port);
    //         }
    //         if (!path.empty())
    //         {
    //             allPathsVector.push_back(std::move(path));
    //         }
    //     }

    //     if (allPathsVector.empty())
    //     {
    //         SPDLOG_LOGGER_ERROR(Logger::instance(), "No valid paths parsed");
    //         return std::nullopt;
    //     }

    //     return FlowAddedEventPayload{flowKey, std::move(allPathsVector)};
    // }
    // catch (const std::exception& e)
    // {
    //     SPDLOG_LOGGER_ERROR(Logger::instance(),
    //                         "Exception while parsing PacketInPayload: {}",
    //                         e.what());
    //     return std::nullopt;
    // }

    return std::nullopt;
}

inline std::optional<LinkFailedEventPayload>
parseLinkFailedEventPayload(const std::string& jsonStr)
{
    using json = nlohmann::json;
    using sflow::FlowKey;
    using sflow::Path;

    try
    {
        json data = json::parse(jsonStr);

        // Validate presence
        static const std::array<const char*, 4> keys = {"src_dpid",
                                                        "src_interface",
                                                        "dst_dpid",
                                                        "dst_interface"};
        for (auto k : keys)
        {
            if (!data.contains(k))
            {
                SPDLOG_LOGGER_ERROR(Logger::instance(), "Link-failure payload missing key: {}", k);
                return std::nullopt;
            }
        }

        // Extract each field (string or number)
        auto getUint32 = [&](const char* k) -> uint32_t {
            if (data[k].is_number_unsigned())
            {
                return data[k].get<uint32_t>();
            }
            else if (data[k].is_string())
            {
                return static_cast<uint32_t>(std::stoul(data[k].get<std::string>()));
            }
            else
            {
                SPDLOG_LOGGER_ERROR(Logger::instance(),
                                    "Invalid type for '{}', expected number or string",
                                    k);
                throw std::runtime_error("bad link-failure payload");
            }
        };

        auto getUint64 = [&](const char* k) -> uint64_t {
            if (data[k].is_number_unsigned())
            {
                return data[k].get<uint64_t>();
            }
            else if (data[k].is_string())
            {
                return static_cast<uint64_t>(std::stoull(data[k].get<std::string>()));
            }
            else
            {
                SPDLOG_LOGGER_ERROR(Logger::instance(),
                                    "Invalid type for '{}', expected number or string",
                                    k);
                throw std::runtime_error("bad link-failure payload");
            }
        };

        uint64_t srcDpid = getUint64("src_dpid");
        uint32_t srcInterface = getUint32("src_interface");
        uint64_t dstDpid = getUint64("dst_dpid");
        uint32_t dstInterface = getUint32("dst_interface");

        return LinkFailedEventPayload{srcDpid, srcInterface, dstDpid, dstInterface};
    }
    catch (const std::exception& e)
    {
        SPDLOG_LOGGER_ERROR(Logger::instance(),
                            "Exception while parsing LinkFailedEventPayload: {}",
                            e.what());
        return std::nullopt;
    }
}
