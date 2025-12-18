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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <utils/SystemClock.h>

#include <aidl/android/hardware/motioncontext/BnMotionContextCallback.h>
#include <aidl/android/hardware/motioncontext/EventDeliveryReason.h>
#include <aidl/android/hardware/motioncontext/IMotionContext.h>
#include <aidl/android/hardware/motioncontext/IMotionContextCallback.h>
#include <aidl/android/hardware/motioncontext/IMotionContextClient.h>
#include <aidl/android/hardware/motioncontext/MotionEvent.h>
#include <aidl/android/hardware/motioncontext/MotionState.h>
#include <aidl/android/hardware/motioncontext/MotionSubscription.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

using aidl::android::hardware::motioncontext::EventDeliveryReason;
using aidl::android::hardware::motioncontext::IMotionContext;
using aidl::android::hardware::motioncontext::IMotionContextCallback;
using aidl::android::hardware::motioncontext::IMotionContextClient;
using aidl::android::hardware::motioncontext::MotionEvent;
using aidl::android::hardware::motioncontext::MotionState;
using aidl::android::hardware::motioncontext::MotionSubscription;
using namespace std::chrono_literals;

class MotionContextCallback
    : public aidl::android::hardware::motioncontext::BnMotionContextCallback {
  public:
    ::ndk::ScopedAStatus onMotionEvent(const MotionEvent& in_motionEvent) override {
        std::lock_guard<std::mutex> lock(mMutex);
        mLastEvent = in_motionEvent;
        mEventReceivedCount++;
        mCv.notify_one();
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onMotionContextError(aidl::android::hardware::motioncontext::ErrorCode,
                                              int) override {
        return ::ndk::ScopedAStatus::ok();
    }

    bool waitForMotionEvent(const std::chrono::milliseconds& timeout, MotionEvent* event) {
        std::unique_lock<std::mutex> lock(mMutex);
        const int currentCount = mEventReceivedCount;
        if (mCv.wait_for(lock, timeout, [&] { return mEventReceivedCount > currentCount; })) {
            if (event != nullptr) {
                *event = mLastEvent;
            }
            return true;
        }
        return false;
    }

    int getEventReceivedCount() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mEventReceivedCount;
    }

    MotionEvent getLastEvent() {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLastEvent;
    }

  private:
    std::mutex mMutex;
    std::condition_variable mCv;
    int mEventReceivedCount = 0;
    MotionEvent mLastEvent;
};

class VtsHalMotionContextTargetTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        ABinderProcess_startThreadPool();
        mService = IMotionContext::fromBinder(
                ::ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(mService, nullptr);
    }

    std::shared_ptr<IMotionContext> mService;
};

TEST_P(VtsHalMotionContextTargetTest, RegisterClient) {
    std::shared_ptr<MotionContextCallback> callback =
            ndk::SharedRefBase::make<MotionContextCallback>();
    std::shared_ptr<IMotionContextClient> client;
    auto status = mService->registerClient(callback, &client);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(client, nullptr);
}

TEST_P(VtsHalMotionContextTargetTest, ConfigureSubscriptionAndReceiveInitialEvent) {
    std::shared_ptr<MotionContextCallback> callback =
            ndk::SharedRefBase::make<MotionContextCallback>();
    std::shared_ptr<IMotionContextClient> client;
    auto status = mService->registerClient(callback, &client);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(client, nullptr);

    MotionSubscription subscription = {
            .targetState = MotionState::STILL,
            .dwellTimeMs = 1000,
    };
    std::vector<MotionSubscription> subscriptions = {subscription};

    const int cookie = 42;
    client->configureMotionSubscription(subscriptions, cookie);

    MotionEvent event;
    ASSERT_TRUE(callback->waitForMotionEvent(5s, &event));

    EXPECT_EQ(event.deliveryReason, EventDeliveryReason::MOTION_SUBSCRIPTION_CONFIGURED);
    EXPECT_GE(event.currentState, MotionState::UNKNOWN);
    EXPECT_LE(event.currentState, MotionState::LOCAL_MOTION);
}

TEST_P(VtsHalMotionContextTargetTest, AckEvent) {
    std::shared_ptr<MotionContextCallback> callback =
            ndk::SharedRefBase::make<MotionContextCallback>();
    std::shared_ptr<IMotionContextClient> client;
    auto status = mService->registerClient(callback, &client);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(client, nullptr);

    MotionSubscription subscription = {
            .targetState = MotionState::STILL,
            .dwellTimeMs = 1000,
    };
    std::vector<MotionSubscription> subscriptions = {subscription};
    const int cookie = 42;
    client->configureMotionSubscription(subscriptions, cookie);
    MotionEvent event;
    ASSERT_TRUE(callback->waitForMotionEvent(5s, &event));
    const int eventCount = callback->getEventReceivedCount();
    client->ackEvent(event.sequenceNumber);
    // No event should be sent in response to an ack
    ASSERT_FALSE(callback->waitForMotionEvent(1s, nullptr));
    ASSERT_EQ(eventCount, callback->getEventReceivedCount());
}

TEST_P(VtsHalMotionContextTargetTest, ClearSubscriptions) {
    std::shared_ptr<MotionContextCallback> callback =
            ndk::SharedRefBase::make<MotionContextCallback>();
    std::shared_ptr<IMotionContextClient> client;
    auto status = mService->registerClient(callback, &client);
    ASSERT_TRUE(status.isOk());
    ASSERT_NE(client, nullptr);

    MotionSubscription subscription = {
            .targetState = MotionState::STILL,
            .dwellTimeMs = 1000,
    };
    std::vector<MotionSubscription> subscriptions = {subscription};
    const int cookie = 42;
    client->configureMotionSubscription(subscriptions, cookie);
    ASSERT_TRUE(callback->waitForMotionEvent(5s, nullptr));

    std::vector<MotionSubscription> empty_subscriptions;
    const int cookie2 = 43;
    client->configureMotionSubscription(empty_subscriptions, cookie2);
    ASSERT_TRUE(callback->waitForMotionEvent(5s, nullptr));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VtsHalMotionContextTargetTest);
INSTANTIATE_TEST_SUITE_P(
        MotionContext, VtsHalMotionContextTargetTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IMotionContext::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
