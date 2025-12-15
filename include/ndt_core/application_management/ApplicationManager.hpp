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

#include "common_types/AppTypes.hpp"
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class ApplicationManager
{
  public:
    ApplicationManager(const std::string& nfsExportDir,
                       const std::string& nfsMountPoint); // nfsExportDir -> /srv/nfs/sim
    ~ApplicationManager();

    // Register a new application and get its App ID
    int registerApplication(const std::string& appName, const std::string& simulationCompletedUrl);

    // Set up NFS configuration for the application
    bool setupNFSForApp(int appId);

    std::optional<std::string> getSimulationCompletedUrl(int appId) const;

  private:
    std::mutex m_mutex;
    int m_nextAppId;
    std::unordered_map<int, RegisteredApp> m_registeredApps;

    std::string m_nfsExportDir;
    std::string m_nfsMountPoint;

    std::vector<std::string> m_registeredFolders;

    bool updateNFSConfig(int appId, const std::string& appDir);
    bool reloadNFSServer();
    void cleanupNFS();
    bool chownRecursive(const fs::path& root, const std::string& user, const std::string& group);
    void cleanupAppFolder(const std::string& folderPath);
    void cleanupStaleEntries();
    
};
