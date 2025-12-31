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

#include "common_types/SFlowType.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/range/iterator_range.hpp>
#include <cstdint>
#include <set>
#include <variant>
#include <vector>

#define MININET_INTERFACE_SPEED 1000000000

// TODO[OPTIMIZE] the Graph and corelated function (in Topology Monitor)

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;


enum class VertexType
{
    SWITCH,
    HOST
};

/**
 * @brief Kind of ECMP group member.
 *
 * Currently only physical ports are supported, but the enum is extensible
 * to Lag or NextHop members in the future.
 */
enum class MemberType
{
    Port /*, Lag, NextHop */
};

inline MemberType
memberTypeFromString(const std::string& s)
{
    if (s == "port")
    {
        return MemberType::Port;
    }
    // if(s == "lag") return MemberType::Lag;
    // if(s == "next_hop") return MemberType::NextHop;
    throw std::invalid_argument("Unknown ECMP member type " + s);
}


struct PortMember
{
    int portId = 0;
};


using EcmpMember = std::variant<PortMember /*, LagMember, NextHopMember */>;


struct EcmpGroup
{
    std::vector<EcmpMember> members;
};


inline std::string
to_string(MemberType t)
{
    switch (t)
    {
    case MemberType::Port:
        return "port";
        // case MemberType::Lag: return "lag";
        // case MemberType::NextHop: return "next_hop";
    }
    return "unknown";
}

/**
 * @brief Properties associated with a vertex in the topology graph.
 *
 * Stores addressing, identity and configuration information for either
 * a switch or a host, including ECMP groups where applicable.
 */
struct VertexProperties
{
    VertexType vertexType;
    uint64_t mac = 0;
    std::vector<uint32_t> ip;
    uint64_t dpid;
    bool isUp = true;
    bool isEnabled = true;
    std::string deviceName = "";
    std::string nickName = "";
    std::string bridgeNameForMininet = "";
    std::string brandName = "";
    int deviceLayer = -1;
    std::vector<std::string> bridgeConnectedPortsForMininet;
    std::vector<EcmpGroup> ecmpGroups;
};

inline void
from_json(const nlohmann::json& j, PortMember& m)
{
    m.portId = j.at("port_id").get<int>();
}

inline void
to_json(nlohmann::json& j, const PortMember& m)
{
    j = {{"type", "port"}, {"port_id", m.portId}};
}

inline void
from_json(const nlohmann::json& j, EcmpMember& m)
{
    const auto& t = j.at("type").get_ref<const std::string&>();
    if (t == "port")
    {
        m = j.get<PortMember>();
        return;
    }
    // ...more types
    throw std::invalid_argument("Unsupported ECMP member type: " + t);
}

inline void
to_json(nlohmann::json& j, const EcmpMember& m)
{
    std::visit([&](auto&& x) { j = nlohmann::json(x); }, m);
}

inline void
from_json(const nlohmann::json& j, EcmpGroup& g)
{
    g.members = j.at("members").get<std::vector<EcmpMember>>();
}

inline void
to_json(nlohmann::json& j, const EcmpGroup& g)
{
    j = {{"members", g.members}};
}

inline void
from_json(const json& j, VertexProperties& v)
{
    v.vertexType = static_cast<VertexType>(j.at("vertex_type").get<int>());
    v.mac = j.at("mac").get<uint64_t>();
    v.ip = j.at("ip").get<std::vector<uint32_t>>();
    v.dpid = j.at("dpid").get<uint64_t>();
    v.isUp = j.at("is_up").get<bool>();
    v.isEnabled = j.at("is_enabled").get<bool>();
    v.deviceName = j.at("device_name").get<std::string>();
    v.nickName = j.at("nickname").get<std::string>();
    v.brandName = j.at("brand_name").get<std::string>();
    v.deviceLayer = j.at("device_layer").get<int>();
    v.ecmpGroups = j.at("ecmp_groups").get<std::vector<EcmpGroup>>();
}

inline void
to_json(nlohmann::json& j, const VertexProperties& v)
{
    j = nlohmann::json{{"vertex_type", v.vertexType},
                       {"mac", v.mac},
                       {"ip", v.ip},
                       {"dpid", v.dpid},
                       {"is_up", v.isUp},
                       {"is_enabled", v.isEnabled},
                       {"device_name", v.deviceName},
                       {"nickname", v.nickName},
                       {"brand_name", v.brandName},
                       {"device_layer", v.deviceLayer},
                       {"ecmp_groups", v.ecmpGroups}};
}

/**
 * @brief Properties associated with an edge in the topology graph.
 *
 * Tracks link state, capacity, utilization and the set of flows that
 * currently traverse this edge, as well as addressing on both ends.
 */
struct EdgeProperties
{
    bool isUp = true;
    bool isEnabled = true;
    uint64_t leftBandwidth = 0;
    uint64_t linkBandwidth = MININET_INTERFACE_SPEED;
    uint64_t linkBandwidthUsage = 0;
    double linkBandwidthUtilization = 0;

    uint64_t leftBandwidthFromFlowSample = MININET_INTERFACE_SPEED;

    // string srcIp (host or agent ip), port represents "physical" port (on swithc)
    std::vector<uint32_t> srcIp;
    uint64_t srcDpid;
    uint32_t srcInterface;

    std::vector<uint32_t> dstIp;
    uint64_t dstDpid;
    uint32_t dstInterface;

    // std::set<sflow::FlowKey> flowSet; // For finding max flow count, and link failure detection
    std::unordered_map<sflow::FlowKey, TimePoint, sflow::FlowKeyHash> flowSet;
};

inline void
from_json(const json& j, EdgeProperties& e)
{
    e.isUp = j.at("is_up").get<bool>();
    e.isEnabled = j.at("is_enabled").get<bool>();
    e.leftBandwidth = j.at("left_link_bandwidth_bps").get<uint64_t>();
    e.linkBandwidth = j.at("link_bandwidth_bps").get<uint64_t>();
    e.linkBandwidthUsage = j.at("link_bandwidth_usage_bps").get<uint64_t>();
    e.linkBandwidthUtilization = j.at("link_bandwidth_utilization_percent").get<double>();
    e.srcIp = j.at("src_ip").get<std::vector<uint32_t>>();
    e.srcDpid = j.at("src_dpid").get<uint64_t>();
    e.srcInterface = j.at("src_interface").get<uint32_t>();
    e.dstIp = j.at("dst_ip").get<std::vector<uint32_t>>();
    e.dstDpid = j.at("dst_dpid").get<uint64_t>();
    e.dstInterface = j.at("dst_interface").get<uint32_t>();


    e.flowSet.clear();
    const auto now = Clock::now();

    if (j.contains("flow_set"))
    {
        for (const auto& fk : j.at("flow_set").get<std::vector<sflow::FlowKey>>())
        {
            e.flowSet.emplace(fk, now);
        }
    }
}

/**
 * @brief Directed topology graph with annotated vertices and edges.
 *
 * Uses Boost adjacency_list to represent the network, with VertexProperties
 * on each vertex and EdgeProperties on each edge.
 */
using Graph = boost::
    adjacency_list<boost::setS, boost::vecS, boost::directedS, VertexProperties, EdgeProperties>;


