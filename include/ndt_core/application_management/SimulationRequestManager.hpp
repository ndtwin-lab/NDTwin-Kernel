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
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class ApplicationManager;

class SimulationRequestManager
{
  public:
    SimulationRequestManager(std::shared_ptr<ApplicationManager> appManager,
                             std::string simServerUrl);
    ~SimulationRequestManager();

    /**
     * @brief Asynchronously request the simulator server to run a case
     * @param simulatorName  Name of the simulator
     * @param version        Version of the simulator
     * @param appId          Application ID
     * @param caseId         Case ID
     * @param inputFilePath  Path to the input file for the simulation
     */
    std::string requestSimulation(const std::string& body);

    /**
     * @brief This method should be called by the network layer when the simulator server
     *        notifies that the simulation has finished. It will forward the result
     *        back to the application via the registered callback.
     * @param appId            Application ID
     * @param caseId           Case ID
     * @param outputFilePath   Path to the output file produced by the simulation
     */
    void onSimulationResult(int appId, const std::string& body);

  private:
    std::shared_ptr<ApplicationManager> m_applicatonManager;
    std::string SIM_SERVER_URL;
};