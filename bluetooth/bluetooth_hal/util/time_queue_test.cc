/*
 * Copyright 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bluetooth_hal/util/time_queue.h"

#include <chrono>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace bluetooth_hal::util {
namespace {

struct TestNode {
    int foo = 0;
};

TEST(TimeQueueTest, InitialPush) {
    TimeQueue<TestNode> queue(std::chrono::minutes(30), 10);
    EXPECT_TRUE(queue.empty());

    queue.current().foo = 42;
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.back().data.foo, 42);
}

TEST(TimeQueueTest, NoAllocationIfTimeNotPassed) {
    TimeQueue<TestNode> queue(std::chrono::hours(1), 10);
    queue.current().foo = 1;
    EXPECT_EQ(queue.size(), 1);

    queue.current().foo = 2;
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.back().data.foo, 2);
}

TEST(TimeQueueTest, AllocationIfTimePassed) {
    // Use a very small time gap for testing
    TimeQueue<TestNode> queue(std::chrono::milliseconds(1), 10);
    queue.current().foo = 1;
    EXPECT_EQ(queue.size(), 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    queue.current().foo = 2;
    EXPECT_EQ(queue.size(), 2);
    EXPECT_EQ(queue.front().data.foo, 1);
    EXPECT_EQ(queue.back().data.foo, 2);
}

TEST(TimeQueueTest, MaxSizeEnforcement) {
    TimeQueue<TestNode> queue(std::chrono::milliseconds(1), 2);
    queue.current().foo = 1;

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    queue.current().foo = 2;
    EXPECT_EQ(queue.size(), 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    queue.current().foo = 3;
    EXPECT_EQ(queue.size(), 2);
    EXPECT_EQ(queue.front().data.foo, 2);
    EXPECT_EQ(queue.back().data.foo, 3);
}

TEST(TimeQueueTest, LocalTimeFormatAndDynamicUpdate) {
    TimeQueue<TestNode> queue(std::chrono::milliseconds(1), 10);
    queue.current();
    std::string lt1 = queue.back().TimePeriodToString();

    // Newest should end with "to Current"
    EXPECT_NE(lt1.find(" to Current"), std::string::npos);

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    queue.current();

    // Old one should now be full range, new one should be "to Current"
    std::string old_lt = queue.front().TimePeriodToString();
    std::string new_lt = queue.back().TimePeriodToString();

    EXPECT_EQ(old_lt.find(" to Current"), std::string::npos);
    EXPECT_NE(old_lt.find(" to "), std::string::npos);
    EXPECT_NE(new_lt.find(" to Current"), std::string::npos);
}

TEST(TimeQueueTest, Iteration) {
    TimeQueue<TestNode> queue(std::chrono::milliseconds(1), 10);
    queue.current().foo = 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    queue.current().foo = 2;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    queue.current().foo = 3;

    std::vector<int> values;
    for (const auto& slot : queue) {
        values.push_back(slot.data.foo);
    }

    EXPECT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);
}

}  // namespace
}  // namespace bluetooth_hal::util
