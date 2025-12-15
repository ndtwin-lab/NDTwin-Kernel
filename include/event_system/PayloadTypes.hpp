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

#include "common_types/SFlowType.hpp"
#include <vector>

// Payload structure for FlowAdded event
struct FlowAddedEventPayload
{
    sflow::FlowKey flowKey;                     // Source and destination IP
    std::vector<sflow::Path> allAvailablePaths; // Candidate paths for the flow
};

// Payload structure for LinkFailureDetected event
struct LinkFailedEventPayload
{
    uint64_t srcDpid;
    uint32_t srcInterface;
    uint64_t dstDpid;
    uint32_t dstInterface;
};
