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

/*
 * spdlog Log Levels:
 *   trace     - Very detailed logs, typically only of interest when diagnosing problems.
 *   debug     - Debugging information, helpful during development.
 *   info      - Informational messages that highlight the progress of the application.
 *   warn      - Potentially harmful situations which still allow the application to continue
 * running. err       - Error events that might still allow the application to continue running.
 *   critical  - Serious errors that lead the application to abort.
 *   off       - Disables logging.
 */

#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

/**
 * @brief Runtime logging configuration options for the global logger.
 *
 * enableFile controls whether logs are also written to a file sink.
 * level selects the minimum log severity that will be emitted.
 */
struct LogConfig
{
    bool enableFile = false;
    spdlog::level::level_enum level = spdlog::level::info;
};

/**
 * @brief Centralized spdlog wrapper providing a process-wide logger instance.
 *
 * Logger encapsulates initialization and access to a single shared spdlog logger
 * used across the codebase (via Logger::instance()).
 *
 * Responsibilities:
 *  - Parse log level names (e.g., "info", "debug") into spdlog enums.
 *  - Parse CLI arguments into LogConfig (if your binary supports it).
 *  - Initialize spdlog sinks/formatters and set the global log level.
 *  - Provide access to the initialized logger.
 *
 * Usage:
 *  - Call Logger::init(cfg) once at program startup.
 *  - Use Logger::instance() anywhere to log via SPDLOG_LOGGER_* macros.
 *
 * Threading:
 *  - spdlog is thread-safe; Logger::instance() returns a shared logger.
 *  - init() should be called once during startup before concurrent logging.
 */
class Logger
{
  public:
    /**
     * @brief Convert a textual log level into a spdlog level enum.
     *
     * @param name Log level name (e.g., "trace", "debug", "info", "warn", "err", "critical",
     * "off").
     * @return Corresponding spdlog level. Unknown values typically default to info.
     */
    static spdlog::level::level_enum parse_level(const std::string& name);
    /**
     * @brief Parse command-line arguments into LogConfig.
     *
     * Intended for configuring log sinks/levels from CLI flags.
     *
     * @param argc Argument count.
     * @param argv Argument vector.
     * @return Parsed LogConfig.
     */
    static LogConfig parse_cli_args(int argc, char* argv[]);
    /**
     * @brief Initialize the global logger instance.
     *
     * Creates spdlog sinks (console and optional file), sets formatting and log level,
     * and stores the logger for later retrieval via instance().
     *
     * @param cfg Logging configuration.
     */
    static void init(const LogConfig& cfg);
    /**
     * @brief Access the global logger instance.
     *
     * @return Shared pointer to the initialized spdlog logger.
     * @note Logger::init() should be called before first use.
     */
    static std::shared_ptr<spdlog::logger> instance();

  private:
    static std::shared_ptr<spdlog::logger> m_logger;
};