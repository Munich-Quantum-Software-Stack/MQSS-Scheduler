/*------------------------------------------------------------------------------
Copyright 2024 - 2026 Munich Quantum Software Stack
All rights reserved.

Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://llvm.org/LICENSE.txt

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

// This test binary needs neither QDMI/LLVM nor this repo's own
// submitter/QuantumTask - only the two headers below, both header-only-
// usable. That's the point being demonstrated: Scheduler is meant to be
// pulled into another project (e.g. QRM) with none of this repo's other
// dependencies in tow.

#include <algorithm>
#include <deque>
#include <random>
#include <utility>

#include <gtest/gtest.h>

#include "scheduler/scheduler.hpp"
#include "scheduler/quantum_task.hpp"

using mqss::scheduler::QuantumTask;
using mqss::scheduler::Scheduler;
using mqss::scheduler::SchedulingPolicy;

namespace {

QuantumTask generateTask(int task_id, int priority, int n_qbits = 1) {
    return QuantumTask(task_id, priority, n_qbits);
}

} // namespace

// --------------------------------------------------------
// FirstInFirstOut: dequeue order matches arrival order
// --------------------------------------------------------
TEST(Scheduler, FirstInFirstOut_PreservesArrivalOrder) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::FirstInFirstOut);
    sched.scheduleTask(generateTask(1, 0));
    sched.scheduleTask(generateTask(2, 0));
    sched.scheduleTask(generateTask(3, 0));
    sched.scheduleTask(generateTask(4, 0));
    sched.scheduleTask(generateTask(5, 0));

    ASSERT_EQ(sched.getTaskCount(), 5u);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 1);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 2);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 3);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 4);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 5);
    EXPECT_FALSE(sched.getNextReadyTask().has_value());
}

// --------------------------------------------------------
// PriorityBased: higher priority dequeues first; equal priorities keep
// their relative arrival order.
// --------------------------------------------------------
TEST(Scheduler, PriorityBased_HighestFirst_StableAmongTies) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::PriorityBased);
    sched.scheduleTask(generateTask(1 /*id*/, 3 /*priority*/));
    sched.scheduleTask(generateTask(2, 1));
    sched.scheduleTask(generateTask(3, 4));
    sched.scheduleTask(generateTask(4, 1)); // second priority-1 job
    sched.scheduleTask(generateTask(5, 5));

    std::vector<int> order;
    while (auto job = sched.getNextReadyTask()) {
        order.push_back(job->task_id());
    }
    EXPECT_EQ(order, (std::vector<int>{5, 3, 1, 2, 4}));
}

// --------------------------------------------------------
// scheduleTasks: scheduling a batch of tasks but not sure this scenario
// is common in practice. Maybe a batch of tasks from a specific user
// application submitted at once, but the scheduler is still handling
// one at a time.
// --------------------------------------------------------
TEST(Scheduler, ScheduleJobs_BatchDoesNotDeadlockAndEnqueuesAll) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::FirstInFirstOut);
    std::vector<QuantumTask> batch = {generateTask(1, 0), generateTask(2, 0), generateTask(3, 0)};
    sched.scheduleTasks(batch);
    EXPECT_EQ(sched.getTaskCount(), 3u);
}

// --------------------------------------------------------
// clearTasks / getSchedulingPolicy / empty-queue behavior
// --------------------------------------------------------
TEST(Scheduler, ClearJobsAndPolicyAccessors) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::PriorityBased);
    EXPECT_EQ(sched.getSchedulingPolicy(), SchedulingPolicy::PriorityBased);

    sched.scheduleTask(generateTask(1, 0));
    sched.scheduleTask(generateTask(2, 0));
    ASSERT_EQ(sched.getTaskCount(), 2u);

    sched.clearTasks();
    EXPECT_EQ(sched.getTaskCount(), 0u);
    EXPECT_FALSE(sched.getNextReadyTask().has_value());
}

// --------------------------------------------------------
// RoundRobin: jobs are dispatched lane-by-lane (lane = task_id % lanes),
// cycling through lanes so no single lane's backlog can starve the others.
// --------------------------------------------------------
TEST(Scheduler, RoundRobin_CyclesThroughLanes) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::RoundRobin);
    sched.setNumLanes(2);
    for (int id = 1; id <= 6; ++id) {
        sched.scheduleTask(generateTask(id, 0));
    }
    // lane(id) = id % 2. Cursor starts at lane 0.
    // Worked out by hand from the algorithm (see scheduler.tpp's
    // nextRoundRobinLocked): lane0 has {2,4,6}, lane1 has {1,3,5}, and the
    // dispatch alternates lane0/lane1 starting at lane0, FIFO within a
    // lane.
    std::vector<int> order;
    while (auto job = sched.getNextReadyTask()) {
        order.push_back(job->task_id());
    }
    EXPECT_EQ(order, (std::vector<int>{2, 1, 4, 3, 6, 5}));
}

// --------------------------------------------------------
// Backfilling with no capacity configured behaves like FirstInFirstOut.
// --------------------------------------------------------
TEST(Scheduler, Backfilling_NoCapacitySet_BehavesLikeFIFO) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::Backfilling);
    sched.scheduleTask(generateTask(1, 0, /*n_qbits=*/20));
    sched.scheduleTask(generateTask(2, 0, /*n_qbits=*/1));

    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 1);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 2);
}

// --------------------------------------------------------
// Backfilling: a big job stuck at the head of the queue (too large for
// currently available capacity) is skipped in favor of a smaller job
// further back that does fit - without losing the big job, which is
// dispatched once capacity grows enough for it.
// --------------------------------------------------------
TEST(Scheduler, Backfilling_SmallerLaterJobJumpsBlockedHead) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::Backfilling);
    sched.setAvailableQubits(5);
    sched.scheduleTask(generateTask(1, 0, /*n_qbits=*/10)); // too big to run yet
    sched.scheduleTask(generateTask(2, 0, /*n_qbits=*/2));  // fits now

    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 2);
    // Job 1 still doesn't fit in 5 qubits of capacity - nothing is ready.
    EXPECT_FALSE(sched.getNextReadyTask().has_value());
    ASSERT_EQ(sched.getTaskCount(), 1u);

    // Capacity grows enough for the previously-blocked job.
    sched.setAvailableQubits(10);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 1);
}

// --------------------------------------------------------
// MixNMulti: a low-priority job that has waited long enough outranks a
// freshly-arrived higher-priority one, preventing starvation.
// --------------------------------------------------------
TEST(Scheduler, MixNMulti_AgingPreventsStarvation) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::MixNMulti);
    sched.setAgingWeight(20.0);

    sched.scheduleTask(generateTask(1 /*id*/, 1 /*priority*/)); // low priority, arrives first
    sched.scheduleTask(generateTask(2, 10));
    sched.scheduleTask(generateTask(3, 10));
    sched.scheduleTask(generateTask(4, 10));

    // Score(job) = priority + agingWeight * (arrivals since it was queued).
    // Job 1: 1 + 20*4 = 81. Job 2: 10 + 20*3 = 70. Job 3: 10 + 20*2 = 50.
    // Job 4: 10 + 20*1 = 30. Highest score wins each pick.
    std::vector<int> order;
    while (auto job = sched.getNextReadyTask()) {
        order.push_back(job->task_id());
    }
    EXPECT_EQ(order, (std::vector<int>{1, 2, 3, 4}));
}

// --------------------------------------------------------
// MixNMulti with aging weight 0 degrades to pure priority ordering.
// --------------------------------------------------------
TEST(Scheduler, MixNMulti_ZeroAgingWeightIsPurePriority) {
    Scheduler<QuantumTask> sched(SchedulingPolicy::MixNMulti);
    sched.setAgingWeight(0.0);

    sched.scheduleTask(generateTask(1, 1));
    sched.scheduleTask(generateTask(2, 10));
    sched.scheduleTask(generateTask(3, 5));

    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 2);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 3);
    EXPECT_EQ(sched.getNextReadyTask()->task_id(), 1);
}

// ==========================================================
// Property-based correctness checks.
// ==========================================================

// --------------------------------------------------------
// FirstInFirstOut: for any sequence of arrivals, dequeue order is exactly
// arrival order - regardless of priority/n_qbits, which FIFO ignores.
// --------------------------------------------------------
TEST(Scheduler, FirstInFirstOut_Property_MatchesArrivalOrder) {
    std::mt19937 rng(1);
    std::uniform_int_distribution<int> priorityDist(-100, 100);
    std::uniform_int_distribution<int> qbitsDist(1, 64);

    Scheduler<QuantumTask> sched(SchedulingPolicy::FirstInFirstOut);
    std::vector<int> expectedOrder;
    for (int i = 0; i < 200; ++i) {
        sched.scheduleTask(generateTask(i, priorityDist(rng), qbitsDist(rng)));
        expectedOrder.push_back(i);
    }
    for (int expected : expectedOrder) {
        auto job = sched.getNextReadyTask();
        ASSERT_TRUE(job.has_value());
        EXPECT_EQ(job->task_id(), expected);
    }
    EXPECT_FALSE(sched.getNextReadyTask().has_value());
}

// --------------------------------------------------------
// PriorityBased: draining the queue must equal a stable sort of all
// scheduled jobs by descending priority (i.e., higher priority first, 
// ties broken by arrival order).
// Priorities are drawn from a small range to force frequent ties, since
// that's where a non-stable implementation would show up.
// --------------------------------------------------------
TEST(Scheduler, PriorityBased_Property_MatchesStableSortByPriorityDescending) {
    std::mt19937 rng(2);
    std::uniform_int_distribution<int> priorityDist(0, 5);

    for (int trial = 0; trial < 50; ++trial) {
        Scheduler<QuantumTask> sched(SchedulingPolicy::PriorityBased);
        struct Rec {
            int task_id;
            int priority;
        };
        std::vector<Rec> recs;
        constexpr int n = 30;
        for (int i = 0; i < n; ++i) {
            int p = priorityDist(rng);
            sched.scheduleTask(generateTask(i, p));
            recs.push_back({i, p});
        }
        std::stable_sort(recs.begin(), recs.end(),
                          [](const Rec &a, const Rec &b) { return a.priority > b.priority; });

        for (const auto &rec : recs) {
            auto job = sched.getNextReadyTask();
            ASSERT_TRUE(job.has_value());
            EXPECT_EQ(job->task_id(), rec.task_id);
        }
        EXPECT_FALSE(sched.getNextReadyTask().has_value());
    }
}

// --------------------------------------------------------
// RoundRobin: reference model keeps one FIFO deque per lane and applies the
// rule directly ("starting at the cursor, dispatch the first
// lane that has anything waiting; advance the cursor past whichever lane
// was served").
//
// task_ids are constructed as `lane + k * numLanes` so lane membership
// (task_id % numLanes) is exact by construction; arrival order is then
// shuffled so lane membership and arrival order are decoupled, the way
// they'd be in practice.
// --------------------------------------------------------
TEST(Scheduler, RoundRobin_Property_MatchesPerLaneFifoReferenceModel) {
    std::mt19937 rng(3);
    std::uniform_int_distribution<int> laneCountDist(2, 5);
    std::uniform_int_distribution<int> countDist(0, 8);

    for (int trial = 0; trial < 50; ++trial) {
        const size_t numLanes = static_cast<size_t>(laneCountDist(rng));
        Scheduler<QuantumTask> sched(SchedulingPolicy::RoundRobin);
        sched.setNumLanes(numLanes);

        std::vector<int> lanesToSchedule;
        for (size_t lane = 0; lane < numLanes; ++lane) {
            const int count = countDist(rng);
            for (int k = 0; k < count; ++k) {
                lanesToSchedule.push_back(static_cast<int>(lane));
            }
        }
        std::shuffle(lanesToSchedule.begin(), lanesToSchedule.end(), rng);

        std::vector<std::deque<int>> laneQueues(numLanes); // reference model
        std::vector<int> perLaneCounter(numLanes, 0);
        size_t total = 0;
        for (int lane : lanesToSchedule) {
            const int id = lane + static_cast<int>(numLanes) * perLaneCounter[static_cast<size_t>(lane)]++;
            sched.scheduleTask(generateTask(id, 0));
            laneQueues[static_cast<size_t>(lane)].push_back(id);
            ++total;
        }

        size_t cursor = 0;
        for (size_t dispatched = 0; dispatched < total; ++dispatched) {
            size_t found = numLanes;
            for (size_t attempt = 0; attempt < numLanes; ++attempt) {
                const size_t lane = (cursor + attempt) % numLanes;
                if (!laneQueues[lane].empty()) {
                    found = lane;
                    break;
                }
            }
            ASSERT_NE(found, numLanes) << "reference model ran out of jobs early - test bug, not scheduler bug";
            const int expectedId = laneQueues[found].front();
            laneQueues[found].pop_front();
            cursor = (found + 1) % numLanes;

            auto job = sched.getNextReadyTask();
            ASSERT_TRUE(job.has_value());
            EXPECT_EQ(job->task_id(), expectedId);
        }
        EXPECT_FALSE(sched.getNextReadyTask().has_value());
    }
}

// --------------------------------------------------------
// Backfilling: reference model is a plain "first job in arrival order whose
// n_qbits fits the current capacity" scan. Capacity is ramped up in random
// steps so every job eventually gets dispatched, checking at
// every step that.
// --------------------------------------------------------
TEST(Scheduler, Backfilling_Property_NeverExceedsCapacityAndPicksEarliestFit) {
    std::mt19937 rng(4);
    std::uniform_int_distribution<int> qbitsDist(1, 10);
    std::uniform_int_distribution<int> capacityStepDist(1, 4);

    for (int trial = 0; trial < 50; ++trial) {
        Scheduler<QuantumTask> sched(SchedulingPolicy::Backfilling);
        constexpr int n = 20;
        std::deque<std::pair<int, int>> pending; // (task_id, n_qbits), reference model
        for (int i = 0; i < n; ++i) {
            const int q = qbitsDist(rng);
            sched.scheduleTask(generateTask(i, 0, q));
            pending.push_back({i, q});
        }

        int capacity = 0;
        sched.setAvailableQubits(capacity);
        int rounds = 0;
        while (!pending.empty() && rounds < 100) {
            ++rounds;
            while (true) {
                auto it = std::find_if(pending.begin(), pending.end(),
                                        [&](const auto &p) { return p.second <= capacity; });
                auto actual = sched.getNextReadyTask();
                if (it == pending.end()) {
                    EXPECT_FALSE(actual.has_value());
                    break;
                }
                ASSERT_TRUE(actual.has_value());
                EXPECT_EQ(actual->task_id(), it->first);
                EXPECT_LE(actual->n_qbits(), capacity);
                pending.erase(it);
            }
            capacity += capacityStepDist(rng); // strictly grows -> guarantees termination
            sched.setAvailableQubits(capacity);
        }
        EXPECT_TRUE(pending.empty()) << "capacity never grew enough to admit every job - test design issue";
        EXPECT_FALSE(sched.getNextReadyTask().has_value());
    }
}

// --------------------------------------------------------
// MixNMulti: a reference model computes each job's score as
// score = priority + agingWeight * (arrivals since this job 
// was queued), highest score dispatched first, ties broken by 
// earliest arrival - applied consistently for many random 
// (priority, agingWeight) combinations.
// --------------------------------------------------------
TEST(Scheduler, MixNMulti_Property_AlwaysPicksMaxScoreByItsOwnFormula) {
    std::mt19937 rng(5);
    std::uniform_int_distribution<int> priorityDist(0, 20);
    std::uniform_real_distribution<double> weightDist(0.0, 5.0);

    for (int trial = 0; trial < 50; ++trial) {
        const double weight = weightDist(rng);
        Scheduler<QuantumTask> sched(SchedulingPolicy::MixNMulti);
        sched.setAgingWeight(weight);

        struct Rec {
            int task_id;
            int priority;
            uint64_t sequence;
        };
        constexpr int n = 25;
        std::vector<Rec> pending;
        for (int i = 0; i < n; ++i) {
            const int p = priorityDist(rng);
            sched.scheduleTask(generateTask(i, p));
            pending.push_back({i, p, static_cast<uint64_t>(i)});
        }
        const auto nextSequence = static_cast<uint64_t>(n);

        for (int step = 0; step < n; ++step) {
            auto best = std::max_element(pending.begin(), pending.end(), [&](const Rec &a, const Rec &b) {
                const double scoreA = a.priority + weight * static_cast<double>(nextSequence - a.sequence);
                const double scoreB = b.priority + weight * static_cast<double>(nextSequence - b.sequence);
                if (scoreA != scoreB) {
                    return scoreA < scoreB;
                }
                return a.sequence > b.sequence; // earlier arrival wins ties
            });
            const int expectedId = best->task_id;
            pending.erase(best);

            auto job = sched.getNextReadyTask();
            ASSERT_TRUE(job.has_value());
            EXPECT_EQ(job->task_id(), expectedId);
        }
        EXPECT_FALSE(sched.getNextReadyTask().has_value());
    }
}
