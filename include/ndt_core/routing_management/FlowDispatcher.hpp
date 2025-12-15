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
#include "ndt_core/routing_management/FlowJob.hpp"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

class FlowDispatcher
{
  public:
    using SenderFn = std::function<void(const std::vector<FlowJob>& batch)>;

    explicit FlowDispatcher(SenderFn sender, size_t burstSize = 2000, bool fencePerBurst = false);
    ~FlowDispatcher();

    void start();
    void stop();

    void enqueue(const FlowJob& job);        // single
    void enqueue(std::vector<FlowJob> jobs); // bulk

  private:
    void workerLoop_(uint64_t dpid);

    // One queue per DPID
    std::unordered_map<uint64_t, std::deque<FlowJob>> queues_;
    std::unordered_map<uint64_t, std::thread> workers_;

    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};

    // Sender = your southbound push (e.g., calls m_flowRoutingManager->install/modify/delete)
    SenderFn sender_;
    size_t burstSize_;
    bool fencePerBurst_;
};
