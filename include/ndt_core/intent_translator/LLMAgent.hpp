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
#include <nlohmann/json.hpp>
#include <map>
#include <vector>

#include "utils/Logger.hpp"
#include "utils/Utils.hpp"
#include "ndt_core/collection/TopologyAndFlowMonitor.hpp"
#include "ndt_core/power_management/DeviceConfigurationAndPowerManager.hpp"
#include "ndt_core/intent_translator/LLMResponseTypes.hpp"

using json = nlohmann::json;

class LLMAgent
{
    public:
        enum Role : bool {
            USER = true,
            AGENT = false
        };
        LLMAgent(
            std::string systemPromptFilePath,
            std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
            std::shared_ptr<DeviceConfigurationAndPowerManager> deviceConfigurationAndPowerManager,
            std::string model
        );
        std::unique_ptr<llmResponse::LLMResponse> callOpenAIApi(const std::string &inputText, const std::string &sessionId);
        std::vector<std::pair<Role, json>> getSessionMsgs(const std::string &sessionId);
        void addMsgToSession(const std::string &sessionId, Role role, const json &msg);
        void cleanSession(const std::string &sessionId);

    private:
        std::string shellEscapeSingleQuotes(const std::string &str);
        std::string getLastMsgId(const std::string &sessionId) const;
        std::string getCurrentTopology();
        std::string getCurrentFlowEntries();

        std::string m_systemPromptFilePath;
        std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
        std::shared_ptr<DeviceConfigurationAndPowerManager> m_deviceConfigManager;
        std::string m_model;
        std::string m_apiKey;
        std::map<std::string, std::vector<std::pair<Role, json>>> m_sessionIdToMsgMap;
        std::map<std::string, std::pair<int, int>> m_sessionTokensCount; // session id -> {input tokens, output tokens}
        std::vector<int> m_responseTime;
        bool m_rateLimit = false;
};