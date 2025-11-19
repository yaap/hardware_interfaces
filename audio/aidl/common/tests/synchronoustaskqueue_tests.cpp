/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <atomic>
#include <chrono>
#include <optional>
#include <thread>
#include <vector>

#include <SynchronousTaskQueue.h>

#include <gtest/gtest.h>
#define LOG_TAG "SynchronousTaskQueue_Test"
#include <log/log.h>

using android::hardware::audio::common::PostponedMethodCall;
using android::hardware::audio::common::PostponedMethodsCaller;
using android::hardware::audio::common::SynchronousTaskQueue;

class SynchronousTaskQueueTest : public ::testing::Test {
  protected:
    SynchronousTaskQueue<int> queue;
};

// Test basic obtain and release
TEST_F(SynchronousTaskQueueTest, ObtainRelease) {
    queue.obtain();
    // A second obtain in the same thread should not block, but the design implies ownership.
    // Let's check release status.
    EXPECT_TRUE(queue.release());
    // Re-obtain to confirm release worked
    queue.obtain();
    EXPECT_TRUE(queue.release());
}

// Test release without obtain
TEST_F(SynchronousTaskQueueTest, ReleaseWithoutObtain) {
    EXPECT_FALSE(queue.release());
}

// Test pop on empty queue
TEST_F(SynchronousTaskQueueTest, PopEmpty) {
    queue.obtain();
    EXPECT_EQ(queue.pop(), std::nullopt);
    EXPECT_TRUE(queue.release());
}

// Test tryObtainOrPush when not obtained
TEST_F(SynchronousTaskQueueTest, TryObtainOrPush_Obtains) {
    EXPECT_TRUE(queue.tryObtainOrPush(1));  // Should obtain
    EXPECT_EQ(queue.pop(), std::nullopt);   // Queue should be empty
    EXPECT_TRUE(queue.release());
}

// Test basic Push and Pop
TEST_F(SynchronousTaskQueueTest, PushPop) {
    queue.obtain();
    // Since this thread owns the queue, tryObtainOrPush will push.
    EXPECT_FALSE(queue.tryObtainOrPush(10));
    EXPECT_FALSE(queue.tryObtainOrPush(20));

    EXPECT_EQ(queue.pop(), 10);
    EXPECT_EQ(queue.pop(), 20);
    EXPECT_EQ(queue.pop(), std::nullopt);
    EXPECT_TRUE(queue.release());
}

// Test tryObtainOrPush from another thread
TEST_F(SynchronousTaskQueueTest, TryObtainOrPush_PushesFromOtherThread) {
    queue.obtain();  // Main thread obtains

    std::thread t1([this]() {
        EXPECT_FALSE(queue.tryObtainOrPush(100));  // Should push
    });
    t1.join();

    EXPECT_EQ(queue.pop(), 100);
    EXPECT_EQ(queue.pop(), std::nullopt);
    EXPECT_TRUE(queue.release());
}

// Test obtain waiting
TEST_F(SynchronousTaskQueueTest, ObtainWaits) {
    queue.obtain();  // Main thread obtains

    std::atomic<bool> obtained = false;
    std::thread t1([this, &obtained]() {
        queue.obtain();
        obtained = true;
        EXPECT_TRUE(queue.release());
    });

    // Give thread t1 some time to potentially (and incorrectly) obtain
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(obtained);

    // Release from main thread
    EXPECT_TRUE(queue.release());

    t1.join();
    EXPECT_TRUE(obtained);
}

// Test multiple threads pushing
TEST_F(SynchronousTaskQueueTest, MultiThreadPush) {
    queue.obtain();

    const int num_threads = 5;
    const int num_pushes = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < num_pushes; ++j) {
                EXPECT_FALSE(queue.tryObtainOrPush(i * num_pushes + j));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    int count = 0;
    while (queue.pop() != std::nullopt) {
        count++;
    }
    EXPECT_EQ(count, num_threads * num_pushes);
    EXPECT_TRUE(queue.release());
}

// Test pop from non-owning thread
TEST_F(SynchronousTaskQueueTest, PopFromNonOwner) {
    queue.obtain();
    EXPECT_FALSE(queue.tryObtainOrPush(1));  // Push an item

    std::thread t1([this]() {
        EXPECT_EQ(queue.pop(), std::nullopt);  // Should fail as t1 doesn't own
    });
    t1.join();

    EXPECT_EQ(queue.pop(), 1);  // Main thread can pop
    EXPECT_TRUE(queue.release());
}

// Test release from non-owning thread
TEST_F(SynchronousTaskQueueTest, ReleaseFromNonOwner) {
    queue.obtain();

    std::thread t1([this]() {
        EXPECT_FALSE(queue.release());  // Should fail
    });
    t1.join();

    EXPECT_TRUE(queue.release());  // Main thread can release
}

class PostponedMethodsCallerTest : public ::testing::Test {
  public:
    using CallTask = PostponedMethodCall<PostponedMethodsCallerTest>;
    using CallTaskQueue = SynchronousTaskQueue<CallTask>;

    void SetUp() override {
        mCallCounter = 0;
        mMethod1Called = 0;
        mMethod2Called = 0;
    }

  protected:
    void Method1() { mMethod1Called = ++mCallCounter; }
    void Method2(int) { mMethod2Called = ++mCallCounter; }
    void EnqueueMethod1() {
        queue.obtain();  // Simulate main thread obtaining the queue
        std::thread t([this]() {
            // This call from a different thread should return false and enqueue the task
            ASSERT_FALSE(queue.tryObtainOrPush(
                    CallTask::create(this, &PostponedMethodsCallerTest::Method1)));
        });
        t.join();
        ASSERT_TRUE(queue.release());  // Simulate main thread releasing the queue
    }
    void EnqueueMethod2() {
        queue.obtain();  // Simulate main thread obtaining the queue
        std::thread t([this]() {
            ASSERT_FALSE(queue.tryObtainOrPush(
                    CallTask::create(this, &PostponedMethodsCallerTest::Method2, 42)));
        });
        t.join();
        ASSERT_TRUE(queue.release());  // Simulate main thread releasing the queue
    }

    CallTaskQueue queue;
    int mCallCounter = 0;
    int mMethod1Called = 0;
    int mMethod2Called = 0;
};

TEST_F(PostponedMethodsCallerTest, NonObtained) {
    // When the queue is not obtained via PostponedMethodsCaller, the tasks in
    // the queue must not be executed. Normally in this case the queue should
    // remain empty because the queue was not obtained thus other clients did not
    // need to queue any tasks.
    {
        PostponedMethodsCaller<PostponedMethodsCallerTest> caller(queue);
    }
    EXPECT_EQ(0, mMethod1Called);
    EXPECT_EQ(0, mMethod2Called);
    EXPECT_FALSE(queue.release());

    ASSERT_NO_FATAL_FAILURE(EnqueueMethod1());
    {
        PostponedMethodsCaller<PostponedMethodsCallerTest> caller(queue);
    }
    EXPECT_EQ(0, mMethod1Called);
    EXPECT_EQ(0, mMethod2Called);
    EXPECT_FALSE(queue.release());

    ASSERT_NO_FATAL_FAILURE(EnqueueMethod2());
    {
        PostponedMethodsCaller<PostponedMethodsCallerTest> caller(queue);
    }
    EXPECT_EQ(0, mMethod1Called);
    EXPECT_EQ(0, mMethod2Called);
    EXPECT_FALSE(queue.release());
}

TEST_F(PostponedMethodsCallerTest, ObtainedEmpty) {
    {
        PostponedMethodsCaller<PostponedMethodsCallerTest> caller(queue);
        caller.obtainQueue();
    }
    EXPECT_EQ(0, mMethod1Called);
    EXPECT_EQ(0, mMethod2Called);
    EXPECT_FALSE(queue.release());
}

TEST_F(PostponedMethodsCallerTest, ObtainedOneMethod) {
    ASSERT_NO_FATAL_FAILURE(EnqueueMethod1());
    {
        PostponedMethodsCaller<PostponedMethodsCallerTest> caller(queue);
        caller.obtainQueue();
    }
    EXPECT_EQ(1, mMethod1Called);
    EXPECT_EQ(0, mMethod2Called);
    EXPECT_EQ(std::nullopt, queue.pop());
    EXPECT_FALSE(queue.release());

    ASSERT_NO_FATAL_FAILURE(EnqueueMethod2());
    {
        PostponedMethodsCaller<PostponedMethodsCallerTest> caller(queue);
        caller.obtainQueue();
    }
    EXPECT_EQ(1, mMethod1Called);
    EXPECT_EQ(2, mMethod2Called);
    EXPECT_EQ(std::nullopt, queue.pop());
    EXPECT_FALSE(queue.release());
}

TEST_F(PostponedMethodsCallerTest, ObtainedTwoMethods) {
    ASSERT_NO_FATAL_FAILURE(EnqueueMethod1());
    ASSERT_NO_FATAL_FAILURE(EnqueueMethod2());
    {
        PostponedMethodsCaller<PostponedMethodsCallerTest> caller(queue);
        caller.obtainQueue();
    }
    EXPECT_EQ(1, mMethod1Called);
    EXPECT_EQ(2, mMethod2Called);
    EXPECT_EQ(std::nullopt, queue.pop());
    EXPECT_FALSE(queue.release());
}
