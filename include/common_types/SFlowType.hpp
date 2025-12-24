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

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>
#include <vector>

using json = nlohmann::json;

constexpr int64_t TIME_UNIT_INTERVAL = 1000; // e.g. 1000 ms = 1 second

namespace sflow
{

/**
 * @brief Key that uniquely identifies a network flow.
 *
 * Describes a flow by its 5-tuple (src/dst IP and ports, protocol) plus
 * optional ICMP type/code for finer classification.
 */
struct FlowKey
{
    uint32_t srcIP;
    uint32_t dstIP;
    uint16_t srcPort;
    uint16_t dstPort;
    uint8_t protocol = 0;
    uint16_t icmpType = 0;
    uint16_t icmpCode = 0;

    bool operator==(const FlowKey& o) const = default;

    bool operator<(const FlowKey& o) const
    {
        return std::tie(srcIP, dstIP, srcPort, dstPort, protocol) <
               std::tie(o.srcIP, o.dstIP, o.srcPort, o.dstPort, o.protocol);
    }
};

/**
 * @brief Key for identifying an sFlow agent and interface.
 *
 * Combines the agent's IP address and a specific interface port index.
 */
struct AgentKey
{
    uint32_t agentIP;
    uint32_t interfacePort;

    bool operator==(const AgentKey& o) const = default;

    bool operator<(const AgentKey& o) const
    {
        return std::tie(agentIP, interfacePort) < std::tie(o.agentIP, o.interfacePort);
    }
};

/**
 * @brief End-to-end path represented as (node, interface) hops.
 *
 * Each element stores a datapath or host identifier together with the
 * outgoing interface used at that hop.
 */
typedef std::vector<std::pair<uint64_t, uint32_t>> Path;

/**
 * @brief Minimal sFlow sample data used for rate calculations.
 *
 * Stores the observed packet length and the sampling timestamp in milliseconds.
 */
struct ExtractedSFlowData
{
    uint32_t packetFrameLengthInByte;
    int64_t timestampInMilliseconds = 0;
};

/**
 * @brief Time-based sliding window over packet samples.
 *
 * Maintains a deque of ExtractedSFlowData entries and keeps only those
 * within the most recent configured interval, allowing fast access to
 * the total byte count in that window.
 */
class AutoRefreshQueue
{
  public:
    explicit AutoRefreshQueue(int64_t interval = TIME_UNIT_INTERVAL)
        : m_interval(interval),
          m_sum(0)
    {
    }

    /**
     * @brief Adds a new sample and prunes stale entries.
     *
     * The sample is appended to the queue, its size is added to the sum,
     * and any entries older than the interval are removed.
     */
    void push(const ExtractedSFlowData& sample)
    {
        m_queue.push_back(sample);
        m_sum += sample.packetFrameLengthInByte;
        refresh();
    }

    /**
     * @brief Returns the sum of packet lengths in the current window.
     *
     * Before returning, the queue is refreshed so that only samples from
     * the last interval are counted.
     */
    uint64_t getSum()
    {
        refresh();
        return m_sum;
    }

    /**
     * @brief Clears all samples and resets the accumulated sum.
     */
    void clear()
    {
        m_queue.clear();
        m_sum = 0;
    }

    /**
     * @brief Returns how many samples are currently in the window.
     */
    size_t size() const
    {
        return m_queue.size();
    }

  private:
    /**
     * @brief Removes samples older than the configured interval.
     *
     * Compares each sample timestamp against the current time and drops
     * those that fall outside the time window, updating the running sum.
     */
    void refresh()
    {
        int64_t now = duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
        while (!m_queue.empty() && now - m_queue.front().timestampInMilliseconds > m_interval)
        {
            m_sum -= m_queue.front().packetFrameLengthInByte;
            m_queue.pop_front();
        }
    }

    std::deque<ExtractedSFlowData> m_queue;
    const int64_t m_interval;
    uint64_t m_sum;
};

/**
 * @brief Per-flow traffic counters and derived rates.
 *
 * Tracks ingress/egress byte and packet counters over time, along with
 * computed average rates and a sliding window of recent samples.
 */
struct FlowStats
{
    uint64_t ingressByteCountCurrent = 0;
    uint64_t egressByteCountCurrent = 0;
    uint64_t ingressByteCountPrevious = 0;
    uint64_t egressByteCountPrevious = 0;
    uint64_t ingresspacketCountCurrent = 0;
    uint64_t egresspacketCountCurrent = 0;
    uint64_t ingresspacketCountPrevious = 0;
    uint64_t egresspacketCountPrevious = 0;

    uint64_t avgByteRateInBps = 0;
    uint64_t avgPacketRate = 0;
    uint32_t samplingRate = 1;
    AutoRefreshQueue packetQueue;
};

/**
 * @brief Detailed view of a single flow across the network.
 *
 * Aggregates statistics from all observing agents, estimated sending
 * rates, lifetime timestamps and elephant-flow classification flags.
 */
struct FlowInfo
{
    /**
     * @brief Flow statistics grouped by observing agent.
     *
     * The key identifies the sFlow agent and interface; the value
     * describes counters and computed rates for that agent.
     */
    std::map<AgentKey, FlowStats> agentFlowStats;
    uint64_t estimatedFlowSendingRatePeriodically = 0;
    uint64_t estimatedFlowSendingRateImmediately = 0;
    uint64_t estimatedPacketSendingRatePeriodically = 0;
    uint64_t estimatedPacketSendingRateImmediately = 0;
    int64_t startTime = 0;
    int64_t endTime = 0;
    bool isElephantFlowPeriodically = false;
    bool isElephantFlowImmediately = false;
    bool isAck = false;
    bool isPureAck = false;
};

template <typename T>
inline void
hashCombine(std::size_t& seed, const T& val)
{
    seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct FlowKeyHash
{
    std::size_t operator()(const FlowKey& key) const
    {
        std::size_t seed = 0;
        hashCombine(seed, key.srcIP);
        hashCombine(seed, key.dstIP);
        hashCombine(seed, key.srcPort);
        hashCombine(seed, key.dstPort);
        hashCombine(seed, key.protocol);
        return seed;
    }
};

/**
 * @brief Cached counter state for a single link.
 *
 * Stores the last reported octet values and computed byte counts for
 * input and output directions on a link.
 */
struct CounterInfo
{
    int64_t lastReportTimestampInMilliseconds = 0;
    uint64_t lastReceivedInputOctets;
    uint64_t lastReceivedOutputOctets;
    uint64_t inputByteCountOnALinkMultiplySampingRate = 0;
    uint64_t outputByteCountOnALink = 0;
};

struct FlowChange
{
    uint32_t dstIp;
    uint32_t oldOutInterface; // 0 if added
    uint32_t newOutInterface; // 0 if removed
};

struct FlowDiff
{
    uint64_t dpid;
    std::vector<FlowChange> added;
    std::vector<FlowChange> removed;
    std::vector<FlowChange> modified;
};

inline void
to_json(nlohmann::json& j, const FlowKey& fk)
{
    j = nlohmann::json{{"src_ip", fk.srcIP},
                       {"dst_ip", fk.dstIP},
                       {"src_port", fk.srcPort},
                       {"dst_port", fk.dstPort},
                       {"protocol_number", fk.protocol}};
}

inline void
from_json(const nlohmann::json& j, FlowKey& fk)
{
    fk.srcIP = j.at("src_ip").get<uint32_t>();
    fk.dstIP = j.at("dst_ip").get<uint32_t>();
    fk.srcPort = j.at("src_port").get<uint16_t>();
    fk.dstPort = j.at("dst_port").get<uint16_t>();
    fk.protocol = j.at("protocol_number").get<uint8_t>();
}

inline std::vector<FlowDiff>
getFlowTableDiff(
    const std::unordered_map<uint64_t, std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>>&
        oldTable,
    const std::unordered_map<uint64_t, std::vector<std::pair<uint32_t, uint32_t>>>& newTable)

{
    std::vector<FlowDiff> diffs;

    for (const auto& [dpid, newFlows] : newTable)
    {
        const auto& oldFlowsIter = oldTable.find(dpid);
        std::unordered_map<uint32_t, uint32_t> oldMap;
        if (oldFlowsIter != oldTable.end())
        {
            for (const auto& [dstIp, outPort, priority] : oldFlowsIter->second)
            {
                oldMap[dstIp] = outPort;
            }
        }

        std::unordered_map<uint32_t, uint32_t> newMap;
        for (const auto& [dstIp, outPort] : newFlows)
        {
            newMap[dstIp] = outPort;
        }

        FlowDiff diff;
        diff.dpid = dpid;

        // Detect added and modified
        for (const auto& [dstIp, newOutPort] : newMap)
        {
            auto oldIt = oldMap.find(dstIp);
            if (oldIt == oldMap.end())
            {
                // Added
                diff.added.push_back({dstIp, 0, newOutPort});
            }
            else if (oldIt->second != newOutPort)
            {
                // Modified
                diff.modified.push_back({dstIp, oldIt->second, newOutPort});
            }
        }

        // Detect removed
        for (const auto& [dstIp, oldOutPort] : oldMap)
        {
            if (newMap.find(dstIp) == newMap.end())
            {
                diff.removed.push_back({dstIp, oldOutPort, 0});
            }
        }

        if (!diff.added.empty() || !diff.removed.empty() || !diff.modified.empty())
        {
            diffs.push_back(std::move(diff));
        }
    }

    // Check switches in oldTable but not in newTable
    for (const auto& [dpid, oldFlows] : oldTable)
    {
        if (newTable.find(dpid) != newTable.end())
        {
            continue;
        }

        FlowDiff diff;
        diff.dpid = dpid;

        for (const auto& [dstIp, oldOutPort, priority] : oldFlows)
        {
            diff.removed.push_back({dstIp, oldOutPort, 0});
        }

        if (!diff.removed.empty())
        {
            diffs.push_back(std::move(diff));
        }
    }

    return diffs;
}

} // namespace sflow
