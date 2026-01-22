#pragma once

#include "common_types/AppTypes.hpp"
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;


/**
 * @brief Manages per-application registration and NFS-backed workspace setup.
 *
 * ApplicationManager assigns a unique application ID to each registered client,
 * maintains metadata such as the simulation-completed callback URL, and
 * provisions an isolated per-app directory under the configured NFS export
 * directory (e.g., /srv/nfs/sim/<appId>/).
 *
 * It updates NFS server configuration to export/mount the per-app directory,
 * reloads the NFS server when required, and performs cleanup of application
 * folders and stale NFS entries. All public APIs are thread-safe via an
 * internal mutex.
 *
 * Typical usage:
 *  - registerApplication() to obtain an appId and store the callback URL
 *  - setupNFSForApp(appId) to create/export/mount the app workspace
 *  - getSimulationCompletedUrl(appId) to retrieve the callback URL later
 */
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
