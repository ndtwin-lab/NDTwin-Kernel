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

// utils/SSHHelper.hpp
#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// This version simulates a user typing and waiting between commands.
// It uses standard shell tools (echo, sleep, |) and requires no new installations.
inline std::string
getPowerReportViaSsh(const std::string& ip, const std::string& username)
{
    // We build a single shell command that does the following:
    // 1. ( ... ) creates a subshell to group our commands.
    // 2. 'echo' sends a command, and then we 'sleep 1' to wait for the switch.
    // 3. The '|' (pipe) sends the entire sequence to the ssh client's input.

    std::stringstream command_stream;

    command_stream
        << "(echo 'terminal length 0'; sleep 1; echo 'show power'; sleep 1; echo 'exit') | "
        << "ssh -T " << "-oKexAlgorithms=+diffie-hellman-group1-sha1 " << "-oCiphers=+aes128-cbc "
        << "-oHostKeyAlgorithms=+ssh-rsa " << "-oStrictHostKeyChecking=no "
        << "-oUserKnownHostsFile=/dev/null " << "-oBatchMode=yes " << "-oConnectTimeout=10 "
        << username << "@" << ip;
    
    std::string command = command_stream.str();


    // This part is correct and remains the same.
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }

    std::array<char, 256> buffer;
    std::stringstream result_stream;

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result_stream << buffer.data();
    }

    pclose(pipe);
    return result_stream.str();
}

// The parser function is also correct and remains the same.
inline uint64_t
parsePowerOutput(const std::string& output)
{
    std::istringstream stream(output);
    std::string line;
    const std::string targetPrefix = "1    ICX7250-24 24-p";

    while (std::getline(stream, line))
    {
        size_t pos = line.find(targetPrefix);
        if (pos != std::string::npos)
        {
            std::string sub = line.substr(pos + targetPrefix.length());
            std::istringstream lineStream(sub);
            uint64_t power;
            if (lineStream >> power)
            {
                return power/10000000;
            }
        }
    }
    return 0;
}