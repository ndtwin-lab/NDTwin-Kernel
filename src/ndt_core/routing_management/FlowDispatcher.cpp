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
#include "ndt_core/routing_management/FlowDispatcher.hpp"

FlowDispatcher::FlowDispatcher(SenderFn sender, size_t burstSize, bool fencePerBurst)
: sender_(std::move(sender)), burstSize_(burstSize), fencePerBurst_(fencePerBurst) {}

FlowDispatcher::~FlowDispatcher() { stop(); }

void FlowDispatcher::start() { running_ = true; }

void FlowDispatcher::stop() {
    running_ = false;
    cv_.notify_all();
    for (auto& [dpid, th] : workers_) {
        if (th.joinable()) th.join();
    }
    workers_.clear();
}

void FlowDispatcher::enqueue(const FlowJob& job) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto& q = queues_[job.dpid];
        q.push_back(job);
        if (!workers_.count(job.dpid)) {
            // spawn worker lazily per DPID
            workers_[job.dpid] = std::thread(&FlowDispatcher::workerLoop_, this, job.dpid);
        }
    }
    cv_.notify_all();
}

void FlowDispatcher::enqueue(std::vector<FlowJob> jobs) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& job : jobs) {
            queues_[job.dpid].push_back(std::move(job));
            if (!workers_.count(job.dpid)) {
                workers_[job.dpid] = std::thread(&FlowDispatcher::workerLoop_, this, job.dpid);
            }
        }
    }
    cv_.notify_all();
}

void FlowDispatcher::workerLoop_(uint64_t dpid) {
    std::vector<FlowJob> burst;
    burst.reserve(burstSize_);

    while (true) {
        {
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait(lk, [&]{
                return !running_ || !queues_[dpid].empty();
            });
            if (!running_ && queues_[dpid].empty()) break;

            burst.clear();
            auto& q = queues_[dpid];

            // Optional: coalesce pending ops by (match key) here if desired.

            while (!q.empty() && burst.size() < burstSize_) {
                burst.push_back(std::move(q.front()));
                q.pop_front();
            }
        }
        if (!burst.empty()) {
            sender_(burst);           // send to southbound in one big push
            // if (fencePerBurst_) ... issue a barrier/commit here inside sender_
        }
    }
}
