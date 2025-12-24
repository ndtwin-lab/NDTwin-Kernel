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
 * @brief A unit of work representing one flow rule operation on a specific switch (DPID).
 *
 * FlowJob is the message passed from higher-level handlers (API/events/routing logic)
 * to the FlowDispatcher / FlowRoutingManager layer. Each job specifies:
 *  - which switch to program (dpid),
 *  - which operation to perform (install/modify/delete),
 *  - match criteria (JSON),
 *  - actions for install/modify (JSON array),
 *  - optional metadata to speed up downstream processing and improve observability.
 *
 * JSON expectations (by convention in this codebase):
 *  - match must include "ipv4_dst" (string) for install/modify and often for delete.
 *  - actions is used for Install/Modify; may be empty for Delete.
 */
struct FlowJob
{
    uint64_t dpid;
    FlowOp op;

    // Core fields your handlers already use
    int priority = 0;                                 // not used for Delete, but harmless
    nlohmann::json match;                             // must include "ipv4_dst" (string)
    nlohmann::json actions = nlohmann::json::array(); // for Install/Modify

    // Optional metadata
    uint32_t dstIpU32 = 0; // parsed once for fast routing/cache
    int idleTimeout = 0;
    std::string corrId; // for tracing (optional)
};
