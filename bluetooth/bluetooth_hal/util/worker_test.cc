/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/util/worker.h"

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include "android-base/logging.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace util {
namespace {

using ::testing::Test;

constexpr std::chrono::milliseconds kFutureWaitTimeout{100};
constexpr std::chrono::milliseconds kSleepForTask{20};
constexpr std::chrono::milliseconds kSleepForQueuedTask{120};
constexpr std::chrono::milliseconds kSleepForDiscardedTask{200};
constexpr size_t kMaxQueueSizeTwo{2};
constexpr size_t kMaxQueueSizeFive{5};
constexpr size_t kMaxQueueSizeOne{1};
constexpr int kMessageOne{1};
constexpr int kMessageTwo{2};
constexpr int kMessageThree{3};

class WorkerTest : public Test {
 protected:
  /**
   * @brief Returns a pair of promise and future. This is a helper for testing
   * if a scheduled task on a worker is executed.
   *
   * Example:
   *  auto [promise, future] = GetPromiseFuturePair<void>();
   *  worker.Post([&promise]() { promise->set_value(); });
   *  ASSERT_EQ(std::future_status::ready, future.wait_for(milliseconds(100)));
   *
   * @return A pair of promise and future.
   */
  template <typename T>
  std::pair<std::unique_ptr<std::promise<T>>, std::future<T>>
  GetPromiseFuturePair() {
    auto promise = std::make_unique<std::promise<T>>();
    auto future = promise->get_future();
    return std::make_pair(std::move(promise), std::move(future));
  }
};

TEST_F(WorkerTest, PostTask) {
  auto [promise, future] = GetPromiseFuturePair<void>();
  Worker<std::function<void()>> worker([](auto task) { task(); });
  worker.Post([&promise]() { promise->set_value(); });
  ASSERT_EQ(std::future_status::ready, future.wait_for(kFutureWaitTimeout));
}

TEST_F(WorkerTest, PostMultipleTasks) {
  std::vector<int> results;
  std::mutex mutex;
  auto [promise, future] = GetPromiseFuturePair<void>();

  Worker<int> worker([&](int task) {
    std::lock_guard<std::mutex> lock(mutex);
    results.push_back(task);
    if (results.size() == 3) {
      promise->set_value();
    }
  });

  worker.Post(kMessageOne);
  worker.Post(kMessageTwo);
  worker.Post(kMessageThree);

  ASSERT_EQ(std::future_status::ready, future.wait_for(kFutureWaitTimeout));

  std::lock_guard<std::mutex> lock(mutex);
  ASSERT_EQ(results.size(), 3);
  EXPECT_EQ(results[0], kMessageOne);
  EXPECT_EQ(results[1], kMessageTwo);
  EXPECT_EQ(results[2], kMessageThree);
}

TEST_F(WorkerTest, QueueFull) {
  auto [promise, future] = GetPromiseFuturePair<void>();
  std::atomic<int> processed_count = 0;

  Worker<std::function<void()>> worker(
      [&](auto task) {
        task();
        processed_count++;
        // Slow down processing to fill the queue
        std::this_thread::sleep_for(kSleepForTask);
      },
      kMaxQueueSizeTwo /* max_queue_size */);

  // Fill the queue
  worker.Post([]() {});
  worker.Post([]() {});

  // This post should block
  auto post_thread = std::thread(
      [&]() { worker.Post([&promise]() { promise->set_value(); }); });

  // The task should not be executed yet as the queue is full and post is
  // blocked.
  ASSERT_NE(std::future_status::ready, future.wait_for(kSleepForTask / 2));

  // Wait for the task to be processed
  ASSERT_EQ(std::future_status::ready, future.wait_for(kFutureWaitTimeout));

  post_thread.join();
}

TEST_F(WorkerTest, StopWorker) {
  Worker<std::function<void()>> worker([](auto task) {
    task();
    FAIL();  // Should not be called
  });

  worker.Post([]() {});
  worker.Post([]() {});

  worker.Stop();

  // Post after stop should fail
  ASSERT_FALSE(worker.Post([]() {}));

  // Give some time to see if FAIL() is called.
  std::this_thread::sleep_for(kSleepForQueuedTask);
}

TEST_F(WorkerTest, PostAfterStop) {
  Worker<std::function<void()>> worker([](auto task) { task(); });
  worker.Stop();
  ASSERT_FALSE(worker.Post([]() { FAIL(); }));
}

TEST_F(WorkerTest, GetQueuedMessageSize) {
  Worker<int> worker(
      [](int /*task*/) { std::this_thread::sleep_for(kSleepForTask); },
      kMaxQueueSizeFive);

  EXPECT_EQ(worker.GetQueuedMessageSize(), 0);
  worker.Post(kMessageOne);
  worker.Post(kMessageTwo);
  EXPECT_EQ(worker.GetQueuedMessageSize(), 2);
  std::this_thread::sleep_for(kSleepForQueuedTask);
  EXPECT_EQ(worker.GetQueuedMessageSize(), 0);
}

TEST_F(WorkerTest, DestructorDiscardsPendingTasks) {
  auto [promise, future] = GetPromiseFuturePair<void>();
  {
    Worker<std::function<void()>> worker(
        [](auto task) {
          // Block the worker to ensure the second task is queued
          std::this_thread::sleep_for(kSleepForQueuedTask);
          task();
        },
        kMaxQueueSizeTwo);
    worker.Post([]() {});  // This will be running
    worker.Post(
        [&promise]() { promise->set_value(); });  // This will be in queue
  }  // Destructor called here
  // The second task should be discarded.
  ASSERT_NE(std::future_status::ready, future.wait_for(kSleepForDiscardedTask));
}

TEST_F(WorkerTest, DestructorWaitsForRunningTask) {
  auto [promise, future] = GetPromiseFuturePair<void>();
  {
    Worker<std::function<void()>> worker([&](auto task) {
      std::this_thread::sleep_for(kSleepForTask);
      task();
    });
    worker.Post([&promise]() { promise->set_value(); });
    // Give time for the task to be picked up by the worker thread
    std::this_thread::sleep_for(kSleepForTask);
  }  // Destructor called here, should wait for the running task.
  ASSERT_EQ(std::future_status::ready, future.wait_for(kFutureWaitTimeout));
}

TEST_F(WorkerTest, StopUnblocksPost) {
  std::promise<void> handler_started_promise;
  auto handler_started_future = handler_started_promise.get_future();
  std::promise<void> unblock_handler_promise;
  auto unblock_handler_future = unblock_handler_promise.get_future();

  Worker<int> worker(
      [&](int /*task*/) {
        handler_started_promise.set_value();
        unblock_handler_future.wait();
      },
      kMaxQueueSizeOne /* max_queue_size */);

  // Block the handler
  worker.Post(kMessageOne);
  handler_started_future.wait();

  // Fill the queue
  worker.Post(kMessageTwo);

  auto [post_result_promise, post_result_future] = GetPromiseFuturePair<bool>();

  // This post should block
  auto post_thread = std::thread([&]() {
    bool result = worker.Post(kMessageThree);
    post_result_promise->set_value(result);
  });

  // Give the thread some time to block on Post()
  std::this_thread::sleep_for(kSleepForQueuedTask);

  worker.Stop();

  // The blocked Post should now return false.
  ASSERT_EQ(std::future_status::ready,
            post_result_future.wait_for(kFutureWaitTimeout));
  EXPECT_FALSE(post_result_future.get());

  post_thread.join();

  // Unblock the handler to allow the worker to shut down gracefully.
  unblock_handler_promise.set_value();
}

}  // namespace
}  // namespace util
}  // namespace bluetooth_hal
