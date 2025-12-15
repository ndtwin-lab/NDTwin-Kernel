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

#include <string>
#include <vector>
#include <variant>
#include <map>
#include "nlohmann/json.hpp"

// A flexible structure for match fields
struct Match {
    std::map<std::string, nlohmann::json> fields;
};

// Represents a DROP action (has no parameters)
struct DropAction {};

// Represents an OUTPUT action
struct OutputAction {
    uint32_t port;
};

// Represents a SET_FIELD action
struct SetFieldAction {
    std::string field;
    nlohmann::json value; // Use nlohmann::json to hold various value types
};

// A variant to hold any possible action type
using Action = std::variant<DropAction, OutputAction, SetFieldAction>;

// A struct to hold a complete, fully parsed flow rule
struct ParsedFlowRule {
    uint64_t dpid;
    int priority;
    Match match;
    std::vector<Action> actions;
};