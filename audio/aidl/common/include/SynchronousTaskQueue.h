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

#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>  // For std::is_copy_constructible
#include <utility>      // For std::move, std::forward

#include <android-base/thread_annotations.h>

namespace android::hardware::audio::common {

// PostponedMethodCall for a specific class type T.
// Can store a call to any void-returning member function of T.
template <typename T>
class PostponedMethodCall {
  public:
    // Default constructor: represents no call.
    PostponedMethodCall() = default;

    // Static factory function to create and configure a PostponedMethodCall.
    // The method and argument types are deduced by the compiler.
    // Returns an instance of PostponedMethodCall<T>.
    template <typename... Args>
    static PostponedMethodCall<T> create(T* instance, void (T::*method)(Args...), Args&&... args) {
        PostponedMethodCall<T> call;
        if (!instance || !method) {
            // Silently return an empty call if instance or method is null.
            // Consider throwing an exception for stricter error handling.
            return call;
        }

        // std::bind_front creates a callable object that binds the method,
        // the instance pointer, and the provided arguments.
        // This callable is then stored in std::function<void()>.
        //
        // IMPORTANT LIMITATION: Because std::function requires the stored
        // callable to be CopyConstructible, all arguments captured by
        // std::bind_front must also be CopyConstructible. Move-only types
        // like std::unique_ptr cannot be used as arguments with this design
        // in C++20 without more complex, non-STL type erasure mechanisms.
        call.mMethod.emplace(std::bind_front(method, instance, std::forward<Args>(args)...));
        return call;
    }

    // Move-only: Ensures single ownership of the pending execution.
    PostponedMethodCall(PostponedMethodCall&& other) noexcept = default;
    PostponedMethodCall& operator=(PostponedMethodCall&& other) noexcept = default;
    PostponedMethodCall(const PostponedMethodCall&) = delete;
    PostponedMethodCall& operator=(const PostponedMethodCall&) = delete;

    // Executes the postponed call, at most once.
    void execute() {
        if (mMethod.has_value()) {
            (*mMethod)();     // Invoke the stored callable.
            mMethod.reset();  // Clear the optional to prevent re-execution.
        }
    }

    // Checks if the call is still pending execution.
    bool isPending() const { return mMethod.has_value(); }

  private:
    // std::optional is used to make the action consumable (execute only once).
    // std::function<void()> type-erases the specific method and arguments
    // into a callable object that takes no arguments and returns void.
    std::optional<std::function<void()>> mMethod;
};

template <typename T>
class SynchronousTaskQueue {
  public:
    SynchronousTaskQueue() = default;

    // Not copyable or movable
    SynchronousTaskQueue(const SynchronousTaskQueue&) = delete;
    SynchronousTaskQueue& operator=(const SynchronousTaskQueue&) = delete;

    // Atomically obtains the queue, waiting if necessary until the queue is released.
    // This function is intended for use by the "main" thread.
    void obtain() {
        std::unique_lock lock(mLock);
        ::android::base::ScopedLockAssertion lock_assertion(mLock);
        mCv.wait(lock, [this] {
            ::android::base::ScopedLockAssertion lock_assertion(mLock);
            return !mIsObtained;
        });
        mIsObtained = true;
        mOwnerThreadId = std::this_thread::get_id();
    }

    // Atomically attempts to obtain the queue. If the queue is already obtained
    // by another thread, it pushes the task into the queue instead.
    // Returns true if the queue was successfully obtained by the calling thread.
    // Returns false if the queue was already obtained, in which case the task is enqueued.
    // This function is intended for use by "non-main" threads.
    bool tryObtainOrPush(T task) {
        std::lock_guard lock(mLock);
        if (!mIsObtained) {
            mIsObtained = true;
            mOwnerThreadId = std::this_thread::get_id();
            return true;  // Obtained
        } else {
            mQueue.push(std::move(task));
            return false;  // Pushed
        }
    }

    // Pops a task from the FIFO queue.
    // This operation requires the calling thread to have previously obtained the queue.
    // Returns the task, or std::nullopt if the queue is empty or if the
    // calling thread does not currently own the queue.
    std::optional<T> pop() {
        std::lock_guard lock(mLock);
        if (!mIsObtained || std::this_thread::get_id() != mOwnerThreadId) {
            // Error: Calling thread does not own the queue.
            // Consider logging this event in a real application.
            return std::nullopt;
        }
        if (mQueue.empty()) {
            return std::nullopt;
        }
        T val = std::move(mQueue.front());
        mQueue.pop();
        return val;
    }

    // Releases the queue, allowing another thread to obtain it.
    // This operation requires the calling thread to have previously obtained the queue.
    // Returns true if the release was successful.
    // Returns false if the calling thread did not own the queue.
    bool release() {
        std::thread::id current_thread_id = std::this_thread::get_id();
        std::unique_lock lock(mLock);
        ::android::base::ScopedLockAssertion lock_assertion(mLock);

        if (!mIsObtained || mOwnerThreadId != current_thread_id) {
            // Error: Calling thread does not own the queue.
            return false;
        }
        mIsObtained = false;
        mOwnerThreadId = std::thread::id();  // Reset owner
        // Unlock before notifying. This allows the thread woken by notify_one()
        // to acquire the lock without immediately contending with this thread.
        lock.unlock();
        mCv.notify_one();
        return true;
    }

  private:
    std::mutex mLock;
    std::condition_variable GUARDED_BY(mLock) mCv;
    std::queue<T> mQueue GUARDED_BY(mLock);
    bool mIsObtained GUARDED_BY(mLock) = false;
    std::thread::id GUARDED_BY(mLock) mOwnerThreadId;
};

// RAII wrapper around 'SynchronousTaskQueue<PostponedMethodCall<T>>' which
// automatically executes all tasks from on destruction if the queue has been
// obtained via it.
template <typename T>
class PostponedMethodsCaller {
  public:
    explicit PostponedMethodsCaller(SynchronousTaskQueue<PostponedMethodCall<T>>& queue)
        : mQueue(queue) {}
    ~PostponedMethodsCaller() {
        if (!mIsObtained) return;
        while (true) {
            if (auto call = mQueue.pop(); call) {
                call->execute();
            } else {
                break;
            }
        }
        mQueue.release();
    }
    void obtainQueue() {
        mQueue.obtain();
        mIsObtained = true;
    }

  private:
    SynchronousTaskQueue<PostponedMethodCall<T>>& mQueue;
    bool mIsObtained = false;
};

}  // namespace android::hardware::audio::common
