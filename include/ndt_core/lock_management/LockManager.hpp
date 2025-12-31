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
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include "utils/Logger.hpp"

// Define available lock types using an Enum Class for type safety
enum class LockType {
    Routing,
    Graph,
    Power,
    Unknown // Used to indicate an invalid string input
};

// Structure to hold the state of a single lock
struct LockState {
    bool isLocked = false;
    std::chrono::steady_clock::time_point expiryTime;
};

class LockManager
{
  public:
    // --- Constants for Default Values (Single Source of Truth) ---
    static constexpr int DEFAULT_TTL_SECONDS = 5;
    static constexpr const char* DEFAULT_LOCK_TYPE_STR = "routing_lock";

  private:
    std::mutex m_mutex; 
    // Using Enum as the key for the map is more efficient than using strings
    std::unordered_map<LockType, LockState> m_locks;

    /**
     * @brief Helper function to convert string input to LockType enum.
     * This enforces the naming convention (routing_lock, graph_lock, power_lock).
     */
    LockType stringToLockType(const std::string& str) {
        if (str == "routing_lock") return LockType::Routing;
        if (str == "graph_lock")   return LockType::Graph;
        if (str == "power_lock")   return LockType::Power;
        return LockType::Unknown;
    }

  public:
    /**
     * @brief Check if the provided lock type string is valid.
     */
    bool isValidType(const std::string& typeStr) {
        return stringToLockType(typeStr) != LockType::Unknown;
    }

    /**
     * @brief Attempt to acquire a lock.
     * @param lockNameStr The string name of the lock (e.g., "routing_lock").
     * @param ttlSeconds Time-To-Live in seconds.
     * @return true if acquired successfully, false if busy or invalid name.
     */
    bool acquireLock(const std::string& lockNameStr, int ttlSeconds)
    {
        // 1. Convert string to Enum
        LockType type = stringToLockType(lockNameStr);

        // 2. Validation: Reject unknown lock types
        if (type == LockType::Unknown) {
            SPDLOG_LOGGER_WARN(Logger::instance(), "Invalid lock type requested: {}", lockNameStr);
            return false; 
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        
        // 3. Access lock state using the Enum key
        LockState& state = m_locks[type];
        auto now = std::chrono::steady_clock::now();

        // 4. Check if it is currently locked and has not expired
        if (state.isLocked && now < state.expiryTime)
        {
            return false; // Lock is held by someone else
        }

        // 5. Acquire the lock
        state.isLocked = true;
        state.expiryTime = now + std::chrono::seconds(ttlSeconds);
        return true;
    }

    /**
     * @brief Release a lock.
     */
    void unlock(const std::string& lockNameStr)
    {
        LockType type = stringToLockType(lockNameStr);
        if (type == LockType::Unknown) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_locks.find(type) != m_locks.end()) {
            m_locks[type].isLocked = false;
        }
    }

    /**
     * @brief Renew the TTL of a lock.
     */
    bool renew(const std::string& lockNameStr, int ttlSeconds)
    {
        LockType type = stringToLockType(lockNameStr);
        if (type == LockType::Unknown) return false;

        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Cannot renew if the lock entry doesn't exist or is not currently locked
        if (m_locks.find(type) == m_locks.end() || !m_locks[type].isLocked) {
            return false;
        }

        // Extend the expiry time
        m_locks[type].expiryTime = std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds);
        return true;
    }
};