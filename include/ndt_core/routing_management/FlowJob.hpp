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
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

/**
 * @brief Operation type for a flow rule update.
 */
enum class FlowOp : uint8_t
{
    Install,
    Modify,
    Delete
};

/**
 * @brief A unit of work for OpenFlow rule updates.
 *
 * FlowJob represents one requested change to a switch flow table (install/modify/delete).
 * It carries the original JSON match/actions payload to send southbound, and also caches
 * parsed IPv4 destination information for fast lookups and “affected-flow” recomputation.
 *
 * Fields:
 *  - dpid: Target switch datapath ID.
 *  - op: Operation type (Install / Modify / Delete).
 *  - priority: Flow priority (OpenFlow rule priority).
 *  - match: Match fields in JSON form (e.g., eth_type, ipv4_dst).
 *  - actions: Actions in JSON form (e.g., OUTPUT port).
 *
 * Cached destination IPv4 (host byte order):
 *  - dstIpU32: Destination IPv4 address (the IP part).
 *  - dstMaskU32: Destination mask derived from prefix length (e.g., /24 -> 255.255.255.0).
 *  - dstPrefixLen: CIDR prefix length (0..32). Defaults to 32 (/32 host route).
 *
 *  - idleTimeout: Optional idle timeout in seconds (0 means no idle timeout unless your controller
 *    interprets it differently).
 */

struct FlowJob {
    uint64_t dpid;
    FlowOp op;
    int priority;
    nlohmann::json match;
    nlohmann::json actions;

    uint32_t dstIpU32 = 0;        // host-order IP
    uint32_t dstMaskU32 = 0xFFFFFFFFu;
    uint8_t  dstPrefixLen = 32;

    int idleTimeout = 0;
};

