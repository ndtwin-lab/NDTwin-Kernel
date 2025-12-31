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
#include <string>
#include <map>
#include "ndt_core/intent_translator/LLMAgent.hpp"
#include "ndt_core/intent_translator/LLMResponseTypes.hpp"
#include "ndt_core/power_management/DeviceConfigurationAndPowerManager.hpp"
#include "ndt_core/collection/TopologyAndFlowMonitor.hpp"
#include "ndt_core/collection/FlowLinkUsageCollector.hpp"
#include "ndt_core/routing_management/FlowRoutingManager.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"

using json = nlohmann::json;

class IntentTranslator
{
    public:
        IntentTranslator(
            std::shared_ptr<DeviceConfigurationAndPowerManager> deviceConfigManager,
            std::shared_ptr<TopologyAndFlowMonitor> topologyAndFlowMonitor,
            std::shared_ptr<FlowRoutingManager> flowRoutingManager,
            std::shared_ptr<sflow::FlowLinkUsageCollector> flowUsageCollector,
            std::string openaiModel
        );
        std::unique_ptr<llmResponse::LLMResponse> inputTextIntent(std::string inputText, const std::string &sessionId);
        void cleanSession(const std::string &sessionId);

    private:
        json performAgentsNegotiation(const std::string &sessionId);
        std::string performTask(llmResponse::Task* task);
        optional<std::string> getSwitchIpByName(const std::string &switchName);

        std::shared_ptr<DeviceConfigurationAndPowerManager> m_deviceConfigManager;
        std::shared_ptr<TopologyAndFlowMonitor> m_topologyAndFlowMonitor;
        std::shared_ptr<FlowRoutingManager> m_flowRoutingManager;
        std::shared_ptr<sflow::FlowLinkUsageCollector> m_flowLinkUsageCollector;
        std::shared_ptr<LLMAgent> m_answerAgent;
        std::string m_answerAgentPromptFilePath = "../src/ndt_core/intent_translator/answer_agent_prompt.txt";
        std::shared_ptr<LLMAgent> m_validationAgent;
        std::string m_validationAgentPromptFilePath = "../src/ndt_core/intent_translator/validation_agent_prompt.txt";
        
        std::optional<nlohmann::json> getFlowEntriesForSwitch(const std::string& deviceName);
};