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
#include "ndt_core/collection/FlowLinkUsageCollector.hpp"
#include "common_types/GraphTypes.hpp"
#include "ndt_core/collection/Classifier.hpp"
#include "ndt_core/collection/TopologyAndFlowMonitor.hpp"
#include "ndt_core/power_management/DeviceConfigurationAndPowerManager.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/detail/edge.hpp>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <poll.h>
#include <random>
#include <set>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/uio.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <vector>
struct iovec;

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

namespace sflow
{

FlowLinkUsageCollector::FlowLinkUsageCollector(
    std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
    std::shared_ptr<FlowRoutingManager> flowRoutingManager,
    std::shared_ptr<DeviceConfigurationAndPowerManager> deviceManager,
    std::shared_ptr<EventBus> eventBus,
    int mode,
    std::shared_ptr<ndtClassifier::Classifier> classifier)
    : m_sockfd(-1),
      m_topologyAndFlowMonitor(std::move(topologyAndFlowMonitor)),
      m_deviceConfigurationAndPowerManager(std::move(deviceManager)),
      m_eventBus(std::move(eventBus)),
      m_mode(static_cast<utils::DeploymentMode>(mode)),
      m_classifier(classifier)
{
}

FlowLinkUsageCollector::~FlowLinkUsageCollector()
{
    stop();
}

std::string_view
trim(std::string_view s)
{
    s.remove_prefix(std::min(s.find_first_not_of(" \t\n\r\f\v"), s.size()));
    s.remove_suffix(std::min(s.size() - s.find_last_not_of(" \t\n\r\f\v") - 1, s.size()));
    return s;
}

void
FlowLinkUsageCollector::populateIfIndexToOfportMap()
{
    std::unique_lock lock(m_ifIndexMapMutex); // Lock for writing
    m_ifIndexToOfportMap.clear();
    SPDLOG_LOGGER_INFO(Logger::instance(), "Populating ifIndex to OFPort map...");

    FILE* pipe = popen("sudo ovs-vsctl list interface", "r");
    if (!pipe)
    {
        SPDLOG_LOGGER_ERROR(Logger::instance(),
                            "popen() for ovs-vsctl failed: {}",
                            strerror(errno));
        return;
    }

    char buffer[256];
    std::string current_block_name_str;
    // uint32_t current_ifindex = 0;
    // uint32_t current_ofport = 0;
    bool in_block = false;

    // Temporary variables for parsing current interface block
    std::string temp_name_str;
    uint32_t temp_ifindex = 0;
    uint32_t temp_ofport = 0;
    std::string temp_type_str;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        std::string_view line(buffer);

        if (line.find("_uuid") != std::string_view::npos)
        {
            // Start of a new block, process the previous one if valid
            if (in_block && !temp_name_str.empty() && temp_ifindex > 0 && temp_ofport > 0 &&
                temp_ofport != 65534)
            {
                // We only care about ports that are not the "local" port (OFPP_LOCAL = 65534)
                // And typically sX-ethY pattern
                if (temp_name_str.rfind("s", 0) == 0 &&
                    temp_name_str.find("-eth") != std::string::npos)
                {
                    m_ifIndexToOfportMap[temp_ifindex] = temp_ofport;
                    SPDLOG_LOGGER_DEBUG(Logger::instance(),
                                        "Mapped ifIndex: {} to OFPort: {} for Name: {}",
                                        temp_ifindex,
                                        temp_ofport,
                                        temp_name_str);
                }
                else
                {
                    SPDLOG_LOGGER_TRACE(Logger::instance(),
                                        "Skipping interface (not sX-ethY or local): Name: {}, "
                                        "ifIndex: {}, OFPort: {}",
                                        temp_name_str,
                                        temp_ifindex,
                                        temp_ofport);
                }
            }
            // Reset for new block
            temp_name_str.clear();
            temp_ifindex = 0;
            temp_ofport = 0;
            temp_type_str.clear();
            in_block = true;
            continue;
        }

        if (!in_block)
        {
            continue;
        }

        size_t colon_pos = line.find(':');
        if (colon_pos == std::string_view::npos)
        {
            continue;
        }

        std::string_view key = trim(line.substr(0, colon_pos));
        std::string_view value_sv = trim(line.substr(colon_pos + 1));

        // Remove quotes from value if present
        if (!value_sv.empty() && value_sv.front() == '"' && value_sv.back() == '"')
        {
            value_sv.remove_prefix(1);
            value_sv.remove_suffix(1);
        }
        std::string value(value_sv);

        if (key == "name")
        {
            temp_name_str = value;
        }
        else if (key == "ifindex")
        {
            try
            {
                temp_ifindex = std::stoul(value);
            }
            catch (const std::exception& e)
            {
                SPDLOG_LOGGER_WARN(Logger::instance(),
                                   "Failed to parse ifindex value '{}': {}",
                                   value,
                                   e.what());
            }
        }
        else if (key == "ofport")
        {
            try
            {
                temp_ofport = std::stoul(value);
            }
            catch (const std::exception& e)
            {
                SPDLOG_LOGGER_WARN(Logger::instance(),
                                   "Failed to parse ofport value '{}': {}",
                                   value,
                                   e.what());
            }
        }
        else if (key == "type")
        {
            temp_type_str = value;
        }
    }

    // Process the last block after EOF
    if (in_block && !temp_name_str.empty() && temp_ifindex > 0 && temp_ofport > 0 &&
        temp_ofport != 65534)
    {
        if (temp_name_str.rfind("s", 0) == 0 && temp_name_str.find("-eth") != std::string::npos)
        {
            m_ifIndexToOfportMap[temp_ifindex] = temp_ofport;
            SPDLOG_LOGGER_DEBUG(Logger::instance(),
                                "Mapped ifIndex: {} to OFPort: {} for Name: {}",
                                temp_ifindex,
                                temp_ofport,
                                temp_name_str);
        }
        else
        {
            SPDLOG_LOGGER_TRACE(
                Logger::instance(),
                "Skipping interface (not sX-ethY or local): Name: {}, ifIndex: {}, OFPort: {}",
                temp_name_str,
                temp_ifindex,
                temp_ofport);
        }
    }

    int status = pclose(pipe);
    if (status == -1)
    {
        SPDLOG_LOGGER_ERROR(Logger::instance(), "pclose() failed: {}", strerror(errno));
    }
    else
    {
        if (WIFEXITED(status))
        {
            SPDLOG_LOGGER_DEBUG(Logger::instance(),
                                "ovs-vsctl command exited with status {}",
                                WEXITSTATUS(status));
        }
        else
        {
            SPDLOG_LOGGER_WARN(Logger::instance(), "ovs-vsctl command exited abnormally");
        }
    }
    SPDLOG_LOGGER_INFO(Logger::instance(),
                       "Finished populating ifIndex to OFPort map. Size: {}",
                       m_ifIndexToOfportMap.size());
    for (const auto& pair : m_ifIndexToOfportMap)
    {
        SPDLOG_LOGGER_DEBUG(Logger::instance(),
                            "Final Map Entry: ifIndex {} -> OFPort {}",
                            pair.first,
                            pair.second);
    }
}

void
FlowLinkUsageCollector::start()
{
    SPDLOG_LOGGER_INFO(Logger::instance(), "Collector Starts Up");

    if (m_mode == utils::MININET)
    {
        populateIfIndexToOfportMap();
    }

    // Call All Destination When Initialize
    fetchAllDestinationPaths();

    this->m_running.store(true);
    m_pktRcvThread = thread(&FlowLinkUsageCollector::run, this);
    m_calAvgFlowSendingRateThreadPeriodically =
        thread(&FlowLinkUsageCollector::calAvgFlowSendingRatesPeriodically, this);
    m_testCalAvgFlowSendingRatesRandomly =
        thread(&FlowLinkUsageCollector::testCalAvgFlowSendingRatesRandomly, this);
    m_purgeThread = thread(&FlowLinkUsageCollector::purgeIdleFlows, this);
    // TODO
    m_calFlowPathByQueried = thread(&FlowLinkUsageCollector::calFlowPathByQueried, this);
}

void
FlowLinkUsageCollector::stop()
{
    this->m_running.store(false);

    SPDLOG_LOGGER_INFO(Logger::instance(), "Collector Stops");

    if (m_sockfd != -1)
    {
        ::close(m_sockfd);
        m_sockfd = -1;
    }
    if (m_pktRcvThread.joinable())
    {
        m_pktRcvThread.join();
    }
    if (m_calAvgFlowSendingRateThreadPeriodically.joinable())
    {
        m_calAvgFlowSendingRateThreadPeriodically.join();
    }
    if (m_testCalAvgFlowSendingRatesRandomly.joinable())
    {
        m_testCalAvgFlowSendingRatesRandomly.join();
    }
    if (m_purgeThread.joinable())
    {
        m_purgeThread.join();
    }
    if (m_calFlowPathByQueried.joinable())
    {
        m_calFlowPathByQueried.join();
    }
}

void
FlowLinkUsageCollector::run()
{
    SPDLOG_LOGGER_INFO(Logger::instance(), "Run");

    // 1. Create UDP socket
    m_sockfd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sockfd < 0)
    {
        SPDLOG_LOGGER_ERROR(Logger::instance(), "socket() failed: {}", strerror(errno));
        throw std::runtime_error("Failed to create UDP socket");
    }

    // 2. Increase receive buffer
    int rcvbuf = 4 * 1024 * 1024;
    setsockopt(m_sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    // 3. Allow address reuse
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 4. Non-blocking mode
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    fcntl(m_sockfd, F_SETFL, flags | O_NONBLOCK);

    // 5. Bind
    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(SFLOW_PORT);
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(m_sockfd, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) < 0)
    {
        SPDLOG_LOGGER_ERROR(Logger::instance(), "bind() failed: {}", strerror(errno));
        ::close(m_sockfd);
        throw std::runtime_error("Failed to bind UDP socket");
    }

    SPDLOG_LOGGER_INFO(Logger::instance(), "Listening for sFlow on UDP port {}", SFLOW_PORT);

    // 6. Prepare recvmmsg structures
    constexpr int BATCH_SIZE = 32;
    std::vector<std::array<char, BUFFER_SIZE>> buffers(BATCH_SIZE);
    std::vector<iovec> iov(BATCH_SIZE);
    std::vector<mmsghdr> msgs(BATCH_SIZE);
    std::vector<sockaddr_in> srcAddrs(BATCH_SIZE);
    std::vector<socklen_t> addrLens(BATCH_SIZE, sizeof(sockaddr_in));

    for (int i = 0; i < BATCH_SIZE; ++i)
    {
        iov[i].iov_base = buffers[i].data();
        iov[i].iov_len = BUFFER_SIZE;
        msgs[i].msg_hdr.msg_iov = &iov[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_name = &srcAddrs[i];
        msgs[i].msg_hdr.msg_namelen = addrLens[i];
        msgs[i].msg_hdr.msg_control = nullptr;
        msgs[i].msg_hdr.msg_controllen = 0;
        msgs[i].msg_hdr.msg_flags = 0;
        msgs[i].msg_len = 0;
    }

    // 7. Main loop: poll with timeout, then recvmmsg
    struct pollfd pfd
    {
        m_sockfd, POLLIN, 0
    };

    const int POLL_TIMEOUT_MS = 1000;
    while (m_running.load())
    {
        int ret = poll(&pfd, 1, POLL_TIMEOUT_MS);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            SPDLOG_LOGGER_ERROR(Logger::instance(), "poll() failed: {}", strerror(errno));
            break;
        }
        if (ret == 0)
        {
            continue; // timeout, recheck m_running
        }

        int received = recvmmsg(m_sockfd, msgs.data(), BATCH_SIZE, 0, nullptr);
        if (received < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            if (errno == EBADF)
            {
                break; // socket closed
            }
            SPDLOG_LOGGER_ERROR(Logger::instance(), "recvmmsg() failed: {}", strerror(errno));
            break;
        }

        for (int i = 0; i < received; ++i)
        {
            if (msgs[i].msg_len > 0)
            {
                handlePacket(buffers[i].data());
            }
            msgs[i].msg_len = 0;
            msgs[i].msg_hdr.msg_namelen = sizeof(sockaddr_in);
        }
    }

    SPDLOG_LOGGER_INFO(Logger::instance(), "Run loop exiting");
}

// Destructor and stop() unchanged

void
FlowLinkUsageCollector::handlePacket(char* buffer)
{
    uint32_t* data = (uint32_t*)buffer;

    uint32_t version = ntohl(data[0]);

    if (version != 5)
    {
        SPDLOG_LOGGER_WARN(Logger::instance(), "Unsupported SFlow Version {}", version);
        return;
    }

    uint32_t agentIp = data[2];
    uint32_t sampleCount = ntohl(data[6]);
    string agentIpStr = utils::ipToString(agentIp);

    SPDLOG_LOGGER_TRACE(Logger::instance(), "Version: {}", version);
    SPDLOG_LOGGER_TRACE(Logger::instance(), "Agent Address: {}", agentIpStr);
    SPDLOG_LOGGER_TRACE(Logger::instance(), "Sample Count: {}", sampleCount);

    uint32_t index = 7;
    for (uint32_t i = 0; i < sampleCount; i++)
    {
        uint32_t sampleType = ntohl(data[index]);
        //================================================================
        // Handle Counter Samples (Brocade Type 2 and HPE Type 4)
        //================================================================
        if (sampleType == 2 || sampleType == 4)
        {
            // Brocade (2) uses a base offset of 4.
            // HPE (4) uses a base offset of 5.
            uint32_t baseOffset = (sampleType == 2) ? 4 : 5;
            const char* vendor = (sampleType == 2) ? "Brocade" : "HPE";

            SPDLOG_LOGGER_INFO(Logger::instance(),
                                "============{} Counter Sample ==============",
                                vendor);

            uint32_t sampleLen = ntohl(data[index + 1]);

            uint64_t interfaceIndex, interfaceSpeed, inputOctets, outputOctets;

            if (sampleType == 2)
            {
                interfaceIndex = ntohl(data[index + baseOffset + 15 + 3]);

                // Combine high and low 32-bit words to form 64-bit values
                interfaceSpeed =
                    (static_cast<uint64_t>(ntohl(data[index + baseOffset + 15 + 5])) << 32) |
                    ntohl(data[index + baseOffset + 15 + 6]);
                inputOctets =
                    (static_cast<uint64_t>(ntohl(data[index + baseOffset + 15 + 9])) << 32) |
                    ntohl(data[index + baseOffset + 15 + 10]);
                outputOctets =
                    (static_cast<uint64_t>(ntohl(data[index + baseOffset + 15 + 17])) << 32) |
                    ntohl(data[index + baseOffset + 15 + 18]);
            }
            else
            {
                interfaceIndex = ntohl(data[index + baseOffset + 3]);

                // Combine high and low 32-bit words to form 64-bit values
                interfaceSpeed =
                    (static_cast<uint64_t>(ntohl(data[index + baseOffset + 5])) << 32) |
                    ntohl(data[index + baseOffset + 6]);
                inputOctets = (static_cast<uint64_t>(ntohl(data[index + baseOffset + 9])) << 32) |
                              ntohl(data[index + baseOffset + 10]);
                outputOctets = (static_cast<uint64_t>(ntohl(data[index + baseOffset + 17])) << 32) |
                               ntohl(data[index + baseOffset + 18]);
            }

            SPDLOG_LOGGER_INFO(Logger::instance(),
                                "COUNTER SAMPLE {} from Agent {}: ifIndex={}, ifSpeed={}, "
                                "ifInOctets={}, ifOutOctets={}",
                                sampleType,
                                agentIpStr,
                                interfaceIndex,
                                interfaceSpeed,
                                inputOctets,
                                outputOctets);

            // Advance index past the current sample
            index += (sampleLen / 4 + 2);

            if (m_mode == utils::MININET)
            {
                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "==========================================\n");
                continue;
            }

            int64_t now = utils::getCurrentTimeMillisSteadyClock();
            pair<uint32_t, uint32_t> agentIpAndPort(agentIp, interfaceIndex);

            // log time
            // utils::logCurrentTimeSystemClock();

            int64_t interval =
                (now - m_counterReports[agentIpAndPort].lastReportTimestampInMilliseconds) / 1000;
            if (interval == 0)
            {
                continue;
            }

            // Check if this is not the first report
            if (m_counterReports[agentIpAndPort].lastReportTimestampInMilliseconds != 0)
            {
                SPDLOG_LOGGER_TRACE(
                    Logger::instance(),
                    "Agent Address: {}, Sample Len: {}, Iface Index: {}, Iface Speed: {}",
                    agentIpStr,
                    sampleLen,
                    interfaceIndex,
                    interfaceSpeed);

                uint64_t avgIn = 0, avgOut = 0;
                bool inNoOverflow = false, outNoOverflow = false;

                if (inputOctets >= m_counterReports[agentIpAndPort].lastReceivedInputOctets)
                {
                    uint64_t inputOctetsDiff =
                        inputOctets - m_counterReports[agentIpAndPort].lastReceivedInputOctets;
                    avgIn = inputOctetsDiff * 8 / interval; // Calculate average bits per second
                    inNoOverflow = true;
                    SPDLOG_LOGGER_TRACE(Logger::instance(), "Average Link Usage (In): {}", avgIn);
                }
                if (outputOctets >= m_counterReports[agentIpAndPort].lastReceivedOutputOctets)
                {
                    uint64_t outputOctetsDiff =
                        outputOctets - m_counterReports[agentIpAndPort].lastReceivedOutputOctets;
                    avgOut = outputOctetsDiff * 8 / interval; // Calculate average bits per second
                    outNoOverflow = true;
                    SPDLOG_LOGGER_TRACE(Logger::instance(), "Average Link Usage (Out): {}", avgOut);
                }

                uint64_t leftIn = (avgIn > interfaceSpeed) ? 0 : (interfaceSpeed - avgIn);
                uint64_t leftOut = (avgOut > interfaceSpeed) ? 0 : (interfaceSpeed - avgOut);

                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "left_in in SFlow Collector: {} (bps)",
                                    leftIn);
                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "left_out in SFlow Collector: {} (bps)",
                                    leftOut);

                if (inNoOverflow && outNoOverflow)
                {
                    m_topologyAndFlowMonitor->updateLinkInfo(agentIpAndPort,
                                                             leftIn,
                                                             leftOut,
                                                             interfaceSpeed);
                }
            }

            // Update state for the next calculation
            m_counterReports[agentIpAndPort].lastReportTimestampInMilliseconds = now;
            m_counterReports[agentIpAndPort].lastReceivedInputOctets = inputOctets;
            m_counterReports[agentIpAndPort].lastReceivedOutputOctets = outputOctets;

            SPDLOG_LOGGER_TRACE(Logger::instance(), "==========================================\n");
        }
        //================================================================
        // Handle Flow Samples (Brocade Type 1 and HPE Type 3)
        //================================================================
        else if (sampleType == 1 || sampleType == 3)
        {
            unique_lock lock(m_flowInfoTableMutex);
            uint32_t sampleLen = ntohl(data[index + 1]);

            // 1. Extract flow data. Offsets differ by vendor.
            uint32_t inputPort, outputPort, frameLength;
            uint8_t protocol;
            uint32_t srcIp, dstIp;
            uint16_t srcPort, dstPort, icmpType, icmpCode;
            uint32_t flowDataLength = 0;
            uint32_t samplingRate = ntohl(data[index + 4]);

            bool isAckPacket = false;
            const uint8_t TCP_ACK_FLAG = 0x10;

            if (sampleType == 1)
            { // Brocade
                samplingRate = ntohl(data[index + 4]);
                inputPort = ntohl(data[index + 7]);
                outputPort = 0;
                if (m_mode == utils::MININET)
                {
                    flowDataLength = ntohl(data[index + 11]);
                    SPDLOG_LOGGER_TRACE(Logger::instance(), "flowDataLength: {}", flowDataLength);
                    index += flowDataLength / 4 + 2;
                }
                frameLength = ntohl(data[index + 13]);
                protocol = ntohl(data[index + 21]) & 0xFF;
                srcIp = ipFromFrontBack(ntohl(data[index + 22]), ntohl(data[index + 23]));
                dstIp = ipFromFrontBack(ntohl(data[index + 23]), ntohl(data[index + 24]));
                if (protocol != 1)
                {
                    srcPort = ntohl(data[index + 24]) & 0xFFFF;
                    dstPort = (ntohl(data[index + 25]) >> 16) & 0xFFFF;
                    if (protocol == 6)
                    {
                        uint8_t tcpFlags = (ntohl(data[index + 28]) >> 8) & 0xFF;

                        if (tcpFlags & TCP_ACK_FLAG)
                        {
                            isAckPacket = true;
                        }
                    }
                }
                else
                {
                    icmpType = (ntohl(data[index + 24]) >> 8) & 0xFF;
                    icmpCode = ntohl(data[index + 24]) & 0xF;
                }
            }
            else
            { // HPE (sampleType == 3)
                samplingRate = ntohl(data[index + 5]);
                inputPort = ntohl(data[index + 9]);
                outputPort = ntohl(data[index + 11]);
                frameLength = ntohl(data[index + 12 + 4]);
                protocol = ntohl(data[index + 12 + 6 + 7]) & 0xFF;
                srcIp = ipFromFrontBack(ntohl(data[index + 12 + 6 + 7 + 1]),
                                        ntohl(data[index + 12 + 6 + 7 + 2]));
                dstIp = ipFromFrontBack(ntohl(data[index + 12 + 6 + 7 + 2]),
                                        ntohl(data[index + 12 + 6 + 7 + 3]));
                if (protocol != 1)
                {
                    srcPort = ntohl(data[index + 12 + 6 + 7 + 3]) & 0xFFFF;
                    dstPort = (ntohl(data[index + 12 + 6 + 7 + 4]) >> 16) & 0xFFFF;
                    if (protocol == 6) // It's a TCP packet
                    {
                        uint8_t tcpFlags = (ntohl(data[index + 32]) >> 8) & 0xFF;

                        if (tcpFlags & TCP_ACK_FLAG)
                        {
                            isAckPacket = true;
                        }
                    }
                }
                else
                {
                    icmpType = ntohl(data[index + 28] >> 8) & 0xFF;
                    icmpCode = ntohl(data[index + 28]) & 0xF;
                }
            }

            SPDLOG_LOGGER_TRACE(
                Logger::instance(),
                "FLOW SAMPLE from Agent {}: {} -> {} (Proto: {}, Len: {}, Input "
                "port: {}, Ouput port: {} ICMP type {} ICMP code {}, Sampling rate {})",
                agentIpStr,
                utils::ipToString(srcIp),
                utils::ipToString(dstIp),
                protocol,
                frameLength,
                inputPort,
                outputPort,
                icmpType,
                icmpCode,
                samplingRate);

            // check whether it is pure ack
            bool isPureAck = false;
            if (protocol == 6) // Check if it's a TCP packet first
            {
                const uint32_t PURE_ACK_SIZE_THRESHOLD = 80; // Your proposed threshold

                // isAckPacket should be true if the ACK flag is set
                if (isAckPacket && frameLength < PURE_ACK_SIZE_THRESHOLD)
                {
                    SPDLOG_LOGGER_TRACE(Logger::instance(),
                                        "Pure ACK packet (size: {} bytes)",
                                        frameLength);
                    isPureAck = true;
                }
            }

            // 2. Process the extracted data using common logic.
            if (protocol == 6 || protocol == 17 || protocol == 1) // TCP, UDP, or ICMP
            {
                if (m_mode == utils::MININET)
                {
                    inputPort = m_ifIndexToOfportMap[inputPort];
                    outputPort = m_ifIndexToOfportMap[outputPort];
                    SPDLOG_LOGGER_TRACE(
                        Logger::instance(),
                        "FLOW SAMPLE in Mininet from Agent {}: {} -> {} (Proto: {}, Len: {}, Input "
                        "port: {}, Ouput port: {})",
                        agentIpStr,
                        utils::ipToString(srcIp),
                        utils::ipToString(dstIp),
                        protocol,
                        frameLength,
                        inputPort,
                        outputPort);
                }

                bool isIngress = (inputPort != 0); // Simple direction check
                uint32_t relevantPort = isIngress ? inputPort : outputPort;

                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "Flow Sample Recieve Src Ip {}, Dst Ip {}",
                                    utils::ipToString(srcIp),
                                    utils::ipToString(dstIp));

                FlowKey key = {};
                if (protocol != 1)
                {
                    key = {srcIp, dstIp, srcPort, dstPort, protocol};
                }
                else
                {
                    key = {srcIp, dstIp, icmpType, icmpCode, protocol};
                }

                AgentKey agentKey = {agentIp, relevantPort};

                if (m_mode == utils::MININET)
                {
                    m_counterReports[make_pair(agentIp, relevantPort)]
                        .inputByteCountOnALinkMultiplySampingRate +=
                        uint64_t(frameLength) * samplingRate;
                }

                auto it = m_flowInfoTable.find(key);
                if (it != m_flowInfoTable.end()) // Existing flow
                {
                    // Find flow stasts on an agent
                    m_flowInfoTable[key].isPureAck = isPureAck;
                    m_flowInfoTable[key].isAck = isAckPacket;

                    SPDLOG_LOGGER_TRACE(Logger::instance(),
                                        "Ack?{} PureAck?{} ",
                                        m_flowInfoTable[key].isAck,
                                        m_flowInfoTable[key].isPureAck);

                    auto& stats = m_flowInfoTable[key].agentFlowStats[agentKey];
                    stats.samplingRate = samplingRate;

                    if (isIngress)
                    {
                        stats.ingressByteCountCurrent += uint64_t(frameLength);
                        stats.ingresspacketCountCurrent += 1;
                    }
                    else
                    {
                        stats.egressByteCountCurrent += uint64_t(frameLength);
                        stats.egresspacketCountCurrent += 1;
                    }

                    // log time
                    // utils::logCurrentTimeSystemClock();

                    stats.packetQueue.push({frameLength, utils::getCurrentTimeMillisSteadyClock()});
                    m_flowInfoTable[key].endTime = utils::getCurrentTimeMillisSystemClock();
                }
                else // New flow
                {
                    m_flowInfoTable[key].startTime = utils::getCurrentTimeMillisSystemClock();
                    m_flowInfoTable[key].endTime = utils::getCurrentTimeMillisSystemClock();

                    // Initialize stats for the new flow
                    auto& stats = m_flowInfoTable[key].agentFlowStats[agentKey];
                    stats.samplingRate = samplingRate;
                    if (isIngress)
                    {
                        stats.ingressByteCountCurrent = uint64_t(frameLength);
                        stats.egressByteCountCurrent = 0;
                        stats.ingresspacketCountCurrent = 1;
                        stats.egresspacketCountCurrent = 0;
                    }
                    else
                    {
                        stats.egressByteCountCurrent = uint64_t(frameLength);
                        stats.ingressByteCountCurrent = 0;
                        stats.egresspacketCountCurrent = 1;
                        stats.ingresspacketCountCurrent = 0;
                    }
                    stats.packetQueue.push({frameLength, utils::getCurrentTimeMillisSteadyClock()});
                }

                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "Flow Table Entry Updated for {} -> {}. End Time: {}",
                                    utils::ipToString(key.srcIP),
                                    utils::ipToString(key.dstIP),
                                    m_flowInfoTable[key].endTime);

                // 2. Update the network map using the CORRECT direction
                if (m_allPathMap.count({key.srcIP, key.dstIP}))
                {
                    if (isIngress)
                    {
                        // Use existing logic for ingress flows
                        if (auto edgeOpt =
                                m_topologyAndFlowMonitor->findReverseEdgeByAgentIpAndPort(
                                    {agentIp, relevantPort}))
                        {
                            m_topologyAndFlowMonitor->touchEdgeFlow(edgeOpt.value(), key);
                        }
                    }
                    else
                    { // Egress flow
                        // Use a NEW function for egress flows that finds the link connected to the
                        // output port
                        if (auto edgeOpt = m_topologyAndFlowMonitor->findEdgeByAgentIpAndPort(
                                {agentIp, relevantPort}))
                        {
                            m_topologyAndFlowMonitor->touchEdgeFlow(edgeOpt.value(), key);
                        }
                    }
                }
            }
            // Adjust offset index for MININET
            if (m_mode == utils::MININET)
            {
                index += (sampleLen / 4 + 2 - (flowDataLength / 4 + 2));
            }
            else
            {
                index += (sampleLen / 4 + 2);
            }
        }
        //================================================================
        // Handle Unknown Sample Types
        //================================================================
        else
        {
            SPDLOG_LOGGER_ERROR(Logger::instance(), "Unknown sampleType {}", sampleType);
            // Safely advance index to avoid an infinite loop if sampleLen is available
            uint32_t sampleLen = ntohl(data[index + 1]);
            if (sampleLen > 0)
            {
                index += (sampleLen / 4 + 2);
            }
            else
            {
                // Can't determine length, break to avoid getting stuck
                break;
            }
        }
    }
}

void
FlowLinkUsageCollector::calAvgFlowSendingRatesPeriodically()
{
    while (m_running.load())
    {
        this_thread::sleep_for(chrono::seconds(1));

        // Estimate average flow sending rate
        {
            unique_lock lock(m_flowInfoTableMutex);
            for (auto& [flowKey, info] : m_flowInfoTable)
            {
                uint64_t avgFlowSendingRateTemp = 0;
                uint64_t avgPacketSendingRateTemp = 0;
                int hopsCounter = 0;
                for (auto& [agentKey, stats] : info.agentFlowStats)
                {
                    // --- 1. CALCULATE ALL RATES FOR THE CURRENT INTERVAL ---

                    uint32_t currentSamplingRate =
                        (stats.samplingRate > 0) ? stats.samplingRate : 1;

                    // Calculate byte rate
                    uint64_t byte_count_current =
                        stats.ingressByteCountCurrent + stats.egressByteCountCurrent;
                    uint64_t byte_count_previous =
                        stats.ingressByteCountPrevious + stats.egressByteCountPrevious;
                    stats.avgByteRateInBps =
                        (byte_count_current - byte_count_previous) * 8 * currentSamplingRate;

                    SPDLOG_LOGGER_TRACE(Logger::instance(),
                                        "Agent {}:{} Current ingress byte counter: {},Current "
                                        "egress byte counter: {} stats.avgByteRateInBps {}",
                                        utils::ipToString(agentKey.agentIP),
                                        agentKey.interfacePort,
                                        stats.ingressByteCountCurrent,
                                        stats.egressByteCountCurrent,
                                        stats.avgByteRateInBps);

                    // Calculate packet rate
                    uint64_t packetCountCurrent =
                        stats.ingresspacketCountCurrent + stats.egresspacketCountCurrent;
                    uint64_t packetCountPrevious =
                        stats.ingresspacketCountPrevious + stats.egresspacketCountPrevious;
                    stats.avgPacketRate =
                        (packetCountCurrent - packetCountPrevious) * currentSamplingRate;

                    // --- 2. AGGREGATE THE RESULTS  ---

                    avgFlowSendingRateTemp += stats.avgByteRateInBps;
                    avgPacketSendingRateTemp += stats.avgPacketRate;

                    if (stats.avgByteRateInBps != 0)
                    {
                        hopsCounter++;
                    }

                    // --- 3. UPDATE STATE FOR THE *NEXT* INTERVAL ---
                    // All state updates are done together at the end.

                    stats.ingressByteCountPrevious = stats.ingressByteCountCurrent;
                    stats.egressByteCountPrevious = stats.egressByteCountCurrent;
                    stats.ingresspacketCountPrevious = stats.ingresspacketCountCurrent;
                    stats.egresspacketCountPrevious = stats.egresspacketCountCurrent;
                }

                if (hopsCounter == 0)
                {
                    continue;
                }

                SPDLOG_LOGGER_TRACE(Logger::instance(), "Hops counter: {}", hopsCounter);

                uint64_t estimatedFlowSendingRatePeriodically =
                    avgFlowSendingRateTemp / hopsCounter;
                info.estimatedFlowSendingRatePeriodically = estimatedFlowSendingRatePeriodically;

                if (estimatedFlowSendingRatePeriodically >= MICE_FLOW_UNDER_THRESHOLD)
                {
                    info.isElephantFlowPeriodically = true;
                }
                // else
                // {
                //     info.isElephantFlowPeriodically = false;
                // }

                uint64_t estimatedPacketSendingRatePeriodically =
                    avgPacketSendingRateTemp / hopsCounter;
                info.estimatedPacketSendingRatePeriodically =
                    estimatedPacketSendingRatePeriodically;

                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "FlowKey: {} -> {}",
                                    utils::ipToString(flowKey.srcIP),
                                    utils::ipToString(flowKey.dstIP));
                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "Estimated flow sending rate (Periodically): {}",
                                    estimatedFlowSendingRatePeriodically);
            }
        }

        // Estimate left link bandwidth using flow sample
        if (m_mode == utils::MININET)
        {
            for (auto& [key, value] : m_counterReports)
            {
                uint32_t agentIp = key.first;
                uint32_t inputPort = key.second;
                const CounterInfo& counter = value;

                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "Agent IP: {}, Input Port: {}, Bytes: {}",
                                    utils::ipToString(agentIp),
                                    inputPort,
                                    counter.inputByteCountOnALinkMultiplySampingRate);

                // TODO[IMPLEMENT]: Gain sampling rate from flow sample
                // Store to graph
                auto agentKeyOtherSideOpt =
                    m_topologyAndFlowMonitor->getAgentKeyFromTheOtherSide(key);
                if (!agentKeyOtherSideOpt.has_value())
                {
                    SPDLOG_LOGGER_WARN(Logger::instance(), "Other Side Agent Miss");
                    continue;
                }
                m_topologyAndFlowMonitor->updateLinkInfoLeftLinkBandwidth(
                    agentKeyOtherSideOpt.value(),
                    counter.inputByteCountOnALinkMultiplySampingRate * 8);
                value.inputByteCountOnALinkMultiplySampingRate = 0;
            }
        }
    }
    SPDLOG_LOGGER_INFO(Logger::instance(), "Exiting Loop of calAvgFlowSendingRatesPeriodically");
}

void
FlowLinkUsageCollector::calAvgFlowSendingRatesImmediately()
{
    unique_lock lock(m_flowInfoTableMutex);
    for (auto& [flowKey, info] : m_flowInfoTable)
    {
        uint64_t accumulatedEstimatedBytes = 0;
        uint64_t accumulatedEstimatedPackets = 0;

        int hopsCounter = 0;

        for (auto& [link_key, stats] : info.agentFlowStats)
        {
            AutoRefreshQueue& packetQueueTemp = stats.packetQueue;
            uint32_t currentSamplingRate = (stats.samplingRate > 0) ? stats.samplingRate : 1;
            if (packetQueueTemp.size())
            {
                hopsCounter++;

                uint64_t estimatedBytes =
                    static_cast<uint64_t>(packetQueueTemp.getSum()) * currentSamplingRate;
                uint64_t estimatedPackets =
                    static_cast<uint64_t>(packetQueueTemp.size()) * currentSamplingRate;

                accumulatedEstimatedBytes += estimatedBytes;
                accumulatedEstimatedPackets += estimatedPackets;
                SPDLOG_LOGGER_TRACE(Logger::instance(),
                                    "accumulatedEstimatedBytes {}, accumulatedEstimatedPackets {}",
                                    accumulatedEstimatedBytes,
                                    accumulatedEstimatedPackets);
            }
        }

        SPDLOG_LOGGER_TRACE(Logger::instance(), "Hops Counter: {}", hopsCounter);

        if (hopsCounter == 0)
        {
            // No activity, so clear the rates and continue
            info.estimatedFlowSendingRateImmediately = 0;
            info.estimatedPacketSendingRateImmediately = 0;
            info.isElephantFlowImmediately = false;
            continue;
        }

        // TODO[IMPLEMENT]: Gain sampling rate from flow sample
        info.estimatedFlowSendingRateImmediately = accumulatedEstimatedBytes * 8 / hopsCounter;

        if (info.estimatedFlowSendingRateImmediately >= MICE_FLOW_UNDER_THRESHOLD)
        {
            info.isElephantFlowImmediately = true;
        }
        else
        {
            info.isElephantFlowImmediately = false;
        }
        // TODO[IMPLEMENT]: Gain sampling rate from flow sample
        info.estimatedPacketSendingRateImmediately = accumulatedEstimatedBytes / hopsCounter;

        SPDLOG_LOGGER_DEBUG(Logger::instance(),
                            "FlowKey: {} -> {}",
                            utils::ipToString(flowKey.srcIP),
                            utils::ipToString(flowKey.dstIP));
        SPDLOG_LOGGER_DEBUG(Logger::instance(),
                            "Estimated packet sending rate (Immediately): {}",
                            info.estimatedFlowSendingRateImmediately);
    }
}

void
FlowLinkUsageCollector::testCalAvgFlowSendingRatesRandomly()
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(500, 2000); // 500ms-2000ms between calls

    while (m_running.load())
    {
        calAvgFlowSendingRatesImmediately();

        int waitTime = dist(gen);

        SPDLOG_LOGGER_TRACE(Logger::instance(),
                            "FlowLinkUsageCollector::testCalAvgFlowSendingRatesRandomly() "
                            "Waiting for {} ms before next call...",
                            waitTime);
        this_thread::sleep_for(chrono::milliseconds(waitTime));
    }

    SPDLOG_LOGGER_INFO(Logger::instance(), "Exiting Loop of testCalAvgFlowSendingRatesRandomly");
}

inline string
FlowLinkUsageCollector::ourIpToString(uint32_t ipFront, uint32_t ipBack)
{
    string res;
    res = to_string((ipFront & 65535) >> 8) + "." + to_string(ipFront & 255) + "." +
          to_string(ipBack >> 24) + "." + to_string((ipBack >> 16) & 255);
    return res;
}

inline uint32_t
FlowLinkUsageCollector::ipFromFrontBack(uint32_t ipFront, uint32_t ipBack)
{
    // extract octets in network‐order
    uint8_t o1 = (ipFront >> 8) & 0xFF;
    uint8_t o2 = ipFront & 0xFF;
    uint8_t o3 = (ipBack >> 24) & 0xFF;
    uint8_t o4 = (ipBack >> 16) & 0xFF;

    // pack into a network‐order 32‑bit IP
    uint32_t netOrder =
        (uint32_t(o1) << 24) | (uint32_t(o2) << 16) | (uint32_t(o3) << 8) | (uint32_t(o4) << 0);

    return ntohl(netOrder);
}

void
FlowLinkUsageCollector::purgeIdleFlows()
{
    while (m_running.load())
    {
        vector<FlowKey> toRemove;
        {
            shared_lock lock(m_flowInfoTableMutex);
            int64_t now = utils::getCurrentTimeMillisSystemClock();

            for (auto& [flowKey, info] : m_flowInfoTable)
            {
                if (now <= info.endTime)
                {
                    continue;
                }
                int64_t idle_time = now - info.endTime;
                if (idle_time >= FLOW_IDLE_TIMEOUT)
                {
                    toRemove.push_back(flowKey);

                    SPDLOG_LOGGER_DEBUG(Logger::instance(),
                                        "Now: {} End Time: {}",
                                        now,
                                        info.endTime);
                    SPDLOG_LOGGER_INFO(Logger::instance(),
                                       "Flow Key: {} -> {} idles",
                                       utils::ipToString(flowKey.srcIP),
                                       utils::ipToString(flowKey.dstIP));

                    SPDLOG_LOGGER_DEBUG(
                        Logger::instance(),
                        "m_flowInfoTable[flowKey].estimatedFlowSendingRatePeriodically: "
                        "{}",
                        m_flowInfoTable[flowKey].estimatedFlowSendingRatePeriodically);
                }
            }
        }

        for (const auto& key : toRemove)
        {
            {
                unique_lock lock(m_flowInfoTableMutex);
                m_flowInfoTable.erase(key);
            }
        }

        this_thread::sleep_for(chrono::milliseconds(1000));
    }

    SPDLOG_LOGGER_INFO(Logger::instance(), "Exiting Loop of purgeIdleFlows");
}

unordered_map<FlowKey, FlowInfo, FlowKeyHash>
FlowLinkUsageCollector::getFlowInfoTable()
{
    shared_lock lock(m_flowInfoTableMutex);
    return m_flowInfoTable;
}

nlohmann::json
FlowLinkUsageCollector::getFlowInfoJson()
{
    shared_lock lock(m_flowInfoTableMutex);
    nlohmann::json result = nlohmann::json::array();

    for (const auto& [flowKey, flowInfo] : m_flowInfoTable)
    {
        nlohmann::json j;

        j["src_ip"] = flowKey.srcIP;
        j["dst_ip"] = flowKey.dstIP;
        j["src_port"] = flowKey.srcPort;
        j["dst_port"] = flowKey.dstPort;
        j["protocol_id"] = flowKey.protocol;

        j["estimated_flow_sending_rate_bps_in_the_proceeding_1sec_timeslot"] =
            flowInfo.estimatedFlowSendingRatePeriodically;
        j["estimated_flow_sending_rate_bps_in_the_last_sec"] =
            flowInfo.estimatedFlowSendingRateImmediately;
        j["estimated_packet_rate_in_the_proceeding_1sec_timeslot"] =
            flowInfo.estimatedPacketSendingRatePeriodically;
        j["estimated_packet_rate_in_the_last_sec"] = flowInfo.estimatedPacketSendingRateImmediately;
        j["first_sampled_time"] = utils::formatTime(flowInfo.startTime);
        j["latest_sampled_time"] = utils::formatTime(flowInfo.endTime);
        j["path"] = nlohmann::json::array();
        // for (const auto& [node, interface] : m_allPathMap[{flowKey.srcIP, flowKey.dstIP}])
        // {
        //     j["path"].push_back({{"node", node}, {"interface", interface}});
        // }
        // TODO: Test Classifier
        for (const auto& [node, interface] : flowInfo.flowPath)
        {
            j["path"].push_back({{"node", node}, {"interface", interface}});
        }

        result.push_back(j);
    }

    return result;
}

nlohmann::json
FlowLinkUsageCollector::getTopKFlowInfoJson(int k)
{
    SPDLOG_LOGGER_DEBUG(Logger::instance(), "getTopKFlowInfoJson k={}", k);
    shared_lock lock(m_flowInfoTableMutex);
    nlohmann::json flowInfo = getFlowInfoJson();
    SPDLOG_LOGGER_DEBUG(Logger::instance(), "Total flows: {}", flowInfo.size());

    std::sort(flowInfo.begin(),
              flowInfo.end(),
              [](const nlohmann::json& a, const nlohmann::json& b) {
                  return a["estimated_flow_sending_rate_bps_in_the_last_sec"].get<uint64_t>() >
                         b["estimated_flow_sending_rate_bps_in_the_last_sec"].get<uint64_t>();
              });

    nlohmann::json topKFlows = nlohmann::json::array();
    for (int i = 0; i < std::min(k, static_cast<int>(flowInfo.size())); ++i)
    {
        topKFlows.push_back(flowInfo[i]);
    }

    return topKFlows;
}

void
FlowLinkUsageCollector::setAllPaths(std::vector<sflow::Path> allPathsVector)
{
    for (const auto& path : allPathsVector)
    {
        uint32_t srcIp = path.front().first;
        uint32_t dstIp = path.back().first;

        // Number of switches = total nodes - 2 (source and destination)
        size_t switchCount = path.size() > 1 ? path.size() - 2 : 0;
        SPDLOG_LOGGER_TRACE(Logger::instance(),
                            "Path from {} -> {} passes through {} switches.",
                            srcIp,
                            dstIp,
                            switchCount);
        m_switchCountMap[{srcIp, dstIp}] = switchCount;

        m_allPathMap[{srcIp, dstIp}] = path;
    }

    // Print out the map
    // for (const auto& [key, value] : m_allPathMap)
    // {
    //     const auto& [srcIp, dstIp] = key;
    //     std::ostringstream oss;

    //     oss << "Path: ";
    //     for (const auto& [nodeId, port] : value)
    //     {
    //         oss << "(" << nodeId << ", " << port << ") ";
    //     }

    //     SPDLOG_LOGGER_DEBUG(Logger::instance(), "Flow from {} -> {}: {}", srcIp, dstIp,
    //     oss.str());
    // }

    SPDLOG_LOGGER_DEBUG(Logger::instance(), "m_allPathMap size {}", m_allPathMap.size());

    return;
}

std::map<std::pair<uint32_t, uint32_t>, Path>
FlowLinkUsageCollector::getAllPaths()
{
    return m_allPathMap;
}

void
FlowLinkUsageCollector::fetchAllDestinationPaths()
{
    try
    {
        // 1. Build and run the curl command
        //    -s: silent mode
        //    -H: set header
        const std::string cmd = "curl -s "
                                "-H \"User-Agent: NDT-client/1.1\" "
                                "\"http://" +
                                AppConfig::RYU_IP_AND_PORT + "/ryu_server/all_destination_paths\"";
        const std::string output = utils::execCommand(cmd);

        // 2. Parse JSON
        auto body = json::parse(output);

        // 3. Check status field
        if (!body.contains("status") || body["status"] != "success")
        {
            SPDLOG_LOGGER_WARN(Logger::instance(),
                               "Controller returned error or missing status: {}",
                               body.dump());
            return;
        }

        // 4. Extract paths array
        const auto& allPathsJson = body.at("all_destination_paths");
        std::vector<sflow::Path> paths;
        for (const auto& pathJson : allPathsJson)
        {
            sflow::Path p;
            for (const auto& nodeJson : pathJson)
            {
                uint64_t nodeId;
                if (nodeJson[0].is_string())
                {
                    nodeId = utils::ipStringToUint32(nodeJson[0].get<std::string>());
                }
                else
                {
                    nodeId = nodeJson[0].get<uint64_t>();
                }

                uint32_t port;
                if (nodeJson[1].is_string())
                {
                    port = static_cast<uint32_t>(std::stoi(nodeJson[1].get<std::string>()));
                }
                else
                {
                    port = nodeJson[1].get<uint32_t>();
                }

                p.emplace_back(nodeId, port);
            }
            if (!p.empty())
            {
                paths.push_back(std::move(p));
            }
        }

        // 5. Update and log
        setAllPaths(paths);
        SPDLOG_LOGGER_INFO(Logger::instance(), "Pulled {} paths from controller", paths.size());
    }
    catch (const std::exception& e)
    {
        SPDLOG_LOGGER_ERROR(Logger::instance(),
                            "Exception in pull_all_destination_paths (curl): {}",
                            e.what());
    }
}

void
FlowLinkUsageCollector::setAllPath(std::pair<uint32_t, uint32_t> ipPair, Path path)
{
    m_allPathMap[ipPair] = path;
}

std::vector<uint32_t>
FlowLinkUsageCollector::getAllHostIps()
{
    std::set<uint32_t> allHostIps;
    std::map<std::pair<uint32_t, uint32_t>, sflow::Path> allPaths = getAllPaths();

    for (const auto& [flowPair, path] : allPaths)
    {
        allHostIps.insert(flowPair.first);  // srcIp
        allHostIps.insert(flowPair.second); // dstIp
    }

    std::vector<uint32_t> hostIpList(allHostIps.begin(), allHostIps.end());

    return hostIpList;
}

void
FlowLinkUsageCollector::printAllPathMap()
{
    for (const auto& [key, value] : m_allPathMap)
    {
        const auto& [srcIp, dstIp] = key;
        std::ostringstream oss;

        oss << "Path: ";
        for (const auto& [nodeId, port] : value)
        {
            oss << "(" << nodeId << ", " << port << ") ";
        }

        SPDLOG_LOGGER_DEBUG(Logger::instance(),
                            "Flow from {} -> {}: {}",
                            utils::ipToString(srcIp),
                            utils::ipToString(dstIp),
                            oss.str());
    }
}

using Rule = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>;

// (net, mask, outPort, priority)

static inline uint32_t
popcount32(uint32_t x)
{
#if defined(__GNUG__) || defined(__clang__)
    return static_cast<uint32_t>(__builtin_popcount(x));
#else
    // portable fallback
    uint32_t c = 0;
    while (x)
    {
        x &= (x - 1);
        ++c;
    }
    return c;
#endif
}

std::optional<size_t>
FlowLinkUsageCollector::getSwitchCount(std::pair<uint32_t, uint32_t> ipPair)
{
    // 1. Lock the mutex for thread-safe reading.
    // A shared_lock allows multiple readers at the same time.
    std::shared_lock<std::shared_mutex> lock(m_switchCountMapMutex);

    // 2. Use the .find() method to look for the key.
    // This is safer than operator[] because it doesn't insert a new element if the key isn't found.
    auto it = m_switchCountMap.find(ipPair);

    // 3. Check if the iterator is valid (i.e., the key was found).
    if (it != m_switchCountMap.end())
    {
        // The key exists, return the associated value (the switch count).
        return it->second;
    }

    // 4. The key was not found, return an empty optional to indicate failure.
    return std::nullopt;
}

std::map<std::pair<uint32_t, uint32_t>, size_t>
FlowLinkUsageCollector::getAllSwitchCounts()
{
    // 1. Acquire a shared lock for thread-safe reading of the map.
    std::shared_lock<std::shared_mutex> lock(m_switchCountMapMutex);

    // 2. Return a copy of the entire map. This is thread-safe because
    //    the caller gets a snapshot of the data and doesn't hold a lock.
    return m_switchCountMap;
}

json
FlowLinkUsageCollector::getPathBetweenHostsJson(const std::string& srcHostName,
                                                const std::string& dstHostName)
{
    // 1. Find hosts using the member variable m_topologyAndFlowMonitor
    auto srcHostOpt = m_topologyAndFlowMonitor->findVertexByDeviceName(srcHostName);
    auto dstHostOpt = m_topologyAndFlowMonitor->findVertexByDeviceName(dstHostName);

    // 2. Handle cases where one or both hosts are not found
    if (!srcHostOpt.has_value() || !dstHostOpt.has_value())
    {
        json errorJson;
        errorJson["error"] = "One or both hosts could not be found in the topology.";
        if (!srcHostOpt.has_value())
        {
            errorJson["missing_hosts"].push_back(srcHostName);
        }
        if (!dstHostOpt.has_value())
        {
            errorJson["missing_hosts"].push_back(dstHostName);
        }
        return errorJson.dump();
    }

    // 3. Get the IP addresses
    auto graph = m_topologyAndFlowMonitor->getGraph();
    uint32_t srcIp = graph[*srcHostOpt].ip[0];
    uint32_t dstIp = graph[*dstHostOpt].ip[0];

    // 4. Retrieve the path map from this collector
    auto allPaths = this->getAllPaths();
    auto it = allPaths.find({srcIp, dstIp});

    if (it == allPaths.end())
    {
        return "{\"error\":\"No active or known path found between the specified hosts.\"}";
    }

    const auto& path = it->second;

    // 5. Format the result
    json result;
    json pathJson = json::array();

    if (path.size() > 2)
    {
        for (size_t i = 1; i < path.size() - 1; ++i)
        {
            uint64_t dpid = path[i].first;
            auto switchVertexOpt = m_topologyAndFlowMonitor->findSwitchByDpid(dpid);
            if (switchVertexOpt.has_value())
            {
                pathJson.push_back(graph[*switchVertexOpt].deviceName);
            }
            else
            {
                pathJson.push_back("unknown_switch_dpid_" + std::to_string(dpid));
            }
        }
    }

    result["source_host"] = srcHostName;
    result["destination_host"] = dstHostName;
    result["switch_path"] = pathJson;

    return result;
}

void
FlowLinkUsageCollector::calFlowPathByQueried()
{
    while (m_running.load())
    {
        for (const auto& [flowKey, flowInfo] : m_flowInfoTable)
        {
            ndtClassifier::FlowKey fk{};
            fk.ipProto = flowKey.protocol;
            fk.ipv4Dst = ntohl(flowKey.dstIP);
            fk.ipv4Src = ntohl(flowKey.srcIP);
            fk.tpDst = flowKey.dstPort;
            fk.tpSrc = flowKey.srcPort;
            fk.ethType = 0x0800;

            auto edgeOpt = m_topologyAndFlowMonitor->findEdgeByHostIp(flowKey.srcIP);
            if (!edgeOpt.has_value())
            {
                SPDLOG_LOGGER_WARN(Logger::instance(), "edge not found");
                continue;
            }

            auto edge = *edgeOpt;

            sflow::Path p;
            auto graph = m_topologyAndFlowMonitor->getGraph();
            p.push_back(make_pair(flowKey.srcIP, graph[edge].dstInterface));

            for (int hop = 0; hop < 100; ++hop)
            {
                auto srcSw = boost::target(edge, graph);
                if (graph[srcSw].dpid == 0)
                {
                    auto it = find(graph[srcSw].ip.begin(), graph[srcSw].ip.end(), flowKey.dstIP);
                    if (it != graph[srcSw].ip.end())
                    {
                        p.push_back(make_pair(flowKey.dstIP, 0));
                    }
                    break;
                }

                auto effect = m_classifier->lookup(graph[srcSw].dpid, fk);
                if (!effect || effect->outputPorts.empty())
                {
                    break;
                }

                uint32_t outPort = effect->outputPorts.front();
                p.push_back(make_pair(graph[srcSw].dpid, outPort));

                edgeOpt = m_topologyAndFlowMonitor->findEdgeByDpidAndPort(
                    make_pair(graph[srcSw].dpid, outPort));
                edge = *edgeOpt;
            }

            m_flowInfoTable[flowKey].flowPath = p;
        }
        std::this_thread::sleep_for(chrono::microseconds(1000));
    }
}

} // namespace sflow
