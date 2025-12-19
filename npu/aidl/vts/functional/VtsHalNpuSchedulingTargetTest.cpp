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

#define LOG_TAG "npu_aidl_hal_test"

#include <cassert>
#include <chrono>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/npu/BnSchedulingCallback.h>
#include <aidl/android/hardware/npu/IScheduling.h>
#include <aidl/android/hardware/npu/SchedulingConfig.h>
#include <aidl/android/hardware/npu/WorkInfo.h>
#include <android-base/properties.h>
#include <android/binder_auto_utils.h>
#include <android/binder_enums.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/content/pm/IPackageManagerNative.h>
#include <binder/IServiceManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "InferenceRunner.h"

using android::getAidlHalInstanceNames;
using android::PrintInstanceNameToString;
using android::sp;
using ndk::enum_range;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;
using ndk::SpAIBinder;
using std::chrono::steady_clock;
using std::chrono::time_point;
using testing::AllOf;
using testing::AnyOf;
using testing::AnyOfArray;
using testing::AssertionFailure;
using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::Contains;
using testing::Each;
using testing::Eq;
using testing::ExplainMatchResult;
using testing::Ge;
using testing::Gt;
using testing::Le;
using testing::Lt;
using testing::Matcher;
using testing::Not;
using namespace std::string_literals;
using namespace std::chrono_literals;

const std::string kFeatureHardwareNpu = "android.hardware.npu";

namespace aidl::android::hardware::npu {

class NpuSchedulingAidl : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        scheduling = IScheduling::fromBinder(binder);
        ASSERT_NE(scheduling, nullptr);

        runner = new InferenceRunner();

        ASSERT_TRUE(runner->checkToolExists()) << "No inference tool found. Please place this at "
                                                  "/data/local/tmp/run-test-inference or another "
                                                  "well-known location.";
    }

    void TearDown() override {
        // Wait for any debouncing of onWorkEnd() to complete
        std::this_thread::sleep_for(200ms);

        scheduling->setCallback(nullptr);
    }

    std::shared_ptr<IScheduling> scheduling;
    sp<InferenceRunner> runner;
};

enum WorkEventType { kIdle, kRequested, kStarted, kEnded };

class WorkEvent {
  public:
    WorkEvent(WorkInfo info, WorkEventType eventType) : mInfo(info), mEventType(eventType) {}

    WorkInfo info() const { return mInfo; }
    WorkEventType type() const { return mEventType; }

    inline bool operator>(const WorkEvent& rhs) const { return rhs.info().id < info().id; }
    inline bool operator<(const WorkEvent& rhs) const { return rhs > *this; }

    using priority_queue =
            std::priority_queue<WorkEvent, std::vector<WorkEvent>, std::greater<WorkEvent>>;

  private:
    WorkInfo mInfo;
    WorkEventType mEventType;
};

class SchedulingCallback : public BnSchedulingCallback {
  public:
    SchedulingCallback() {
        // Only listen to our own UID by default
        mUid = geteuid();
    }

    void setUidFilter(int uid) { mUid = uid; }

    ::ndk::ScopedAStatus onWorkRequested(const WorkInfo& workInfo) override {
        if (mUid >= 0 && workInfo.uid != mUid) {
            return ndk::ScopedAStatus::ok();
        }
        std::lock_guard guard(mEventsMutex);
        mEvents.push(WorkEvent(workInfo, kRequested));
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onWorkStarted(const WorkInfo& workInfo,
                                       [[maybe_unused]] const StartReason reason) override {
        if (mUid >= 0 && workInfo.uid != mUid) {
            return ndk::ScopedAStatus::ok();
        }
        std::lock_guard guard(mEventsMutex);
        mEvents.push(WorkEvent(workInfo, kStarted));
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onWorkEnded(const WorkInfo& workInfo,
                                     [[maybe_unused]] const EndReason reason) override {
        if (mUid >= 0 && workInfo.uid != mUid) {
            return ndk::ScopedAStatus::ok();
        }
        std::lock_guard guard(mEventsMutex);
        mEvents.push(WorkEvent(workInfo, kEnded));
        return ndk::ScopedAStatus::ok();
    }

    void clearEvents() {
        std::lock_guard guard(mEventsMutex);
        mEvents = std::priority_queue<WorkEvent, std::vector<WorkEvent>, std::greater<WorkEvent>>();
    }

    WorkEvent::priority_queue events() {
        std::lock_guard guard(mEventsMutex);
        return mEvents;
    }

  private:
    int mUid = -1;
    std::mutex mEventsMutex;
    WorkEvent::priority_queue mEvents;
};

TEST(SchedulingCallbackTests, EventQueue) {
    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();

    auto infoRequested = WorkInfo(1, std::nullopt, geteuid(), 0, 0, std::nullopt, 0, 0);
    auto infoStarted = WorkInfo(10, std::nullopt, geteuid(), 0, 0, std::nullopt, 0, 0);
    auto infoEnded = WorkInfo(20, std::nullopt, geteuid(), 0, 0, std::nullopt, 0, 0);

    callback->onWorkStarted(infoStarted, StartReason::INITIAL);
    callback->onWorkEnded(infoEnded, EndReason::COMPLETED);
    callback->onWorkRequested(infoRequested);

    auto events = callback->events();
    ASSERT_THAT(events.size(), Eq(3));
    ASSERT_THAT(events.top().type(), Eq(kRequested));
    ASSERT_THAT(events.top().info(), Eq(infoRequested));
    events.pop();
    ASSERT_THAT(events.top().type(), Eq(kStarted));
    ASSERT_THAT(events.top().info(), Eq(infoStarted));
    events.pop();
    ASSERT_THAT(events.top().type(), Eq(kEnded));
    ASSERT_THAT(events.top().info(), Eq(infoEnded));
    events.pop();
    ASSERT_TRUE(events.empty());
}

TEST(SchedulingCallbackTests, EventQueueNoUidFilter) {
    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();
    callback->setUidFilter(-1);

    auto infoRequestedA = WorkInfo(1, std::nullopt, 100, 0, 0, std::nullopt, 0, 0);
    auto infoRequestedB = WorkInfo(2, std::nullopt, 200, 0, 0, std::nullopt, 0, 0);

    callback->onWorkRequested(infoRequestedA);
    callback->onWorkRequested(infoRequestedB);

    auto events = callback->events();
    ASSERT_THAT(events.size(), Eq(2));
    ASSERT_THAT(events.top().type(), Eq(kRequested));
    ASSERT_THAT(events.top().info(), Eq(infoRequestedA));
    events.pop();
    ASSERT_THAT(events.top().type(), Eq(kRequested));
    ASSERT_THAT(events.top().info(), Eq(infoRequestedB));
}

TEST(SchedulingCallbackTests, EventQueueUidFilter) {
    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();
    callback->setUidFilter(100);

    auto infoRequestedA = WorkInfo(1, std::nullopt, 100, 0, 0, std::nullopt, 0, 0);
    auto infoRequestedB = WorkInfo(2, std::nullopt, 200, 0, 0, std::nullopt, 0, 0);

    callback->onWorkRequested(infoRequestedA);
    callback->onWorkRequested(infoRequestedB);

    auto events = callback->events();
    ASSERT_THAT(events.size(), Eq(1));
    ASSERT_THAT(events.top().type(), Eq(kRequested));
    ASSERT_THAT(events.top().info(), Eq(infoRequestedA));
}

TEST(WorkEventTests, ComparisonOperator) {
    auto a = WorkEvent(WorkInfo(10, std::nullopt, 100, 0, 0, std::nullopt, 0, 0), kRequested);
    auto b = WorkEvent(WorkInfo(11, std::nullopt, 100, 0, 0, std::nullopt, 0, 0), kRequested);
    ASSERT_THAT(a, Lt(b));
    ASSERT_THAT(b, Gt(a));
}

TEST(WorkEventTests, PriorityQueue) {
    auto a = WorkEvent(WorkInfo(10, std::nullopt, 100, 0, 0, std::nullopt, 0, 0), kRequested);
    auto b = WorkEvent(WorkInfo(11, std::nullopt, 100, 0, 0, std::nullopt, 0, 0), kStarted);
    auto c = WorkEvent(WorkInfo(13, std::nullopt, 100, 0, 0, std::nullopt, 0, 0), kEnded);

    auto q = WorkEvent::priority_queue();
    q.push(b);
    q.push(a);
    q.push(c);

    ASSERT_THAT(q.top().info().id, Eq(a.info().id));
    q.pop();
    ASSERT_THAT(q.top().info().id, Eq(b.info().id));
    q.pop();
    ASSERT_THAT(q.top().info().id, Eq(c.info().id));
}

/*
 * Waits on all futures and returns them in the order in which they complete
 */
template <class T>
std::list<std::future<T>*> raceFutures(std::list<std::future<T>*> futures,
                                       std::chrono::milliseconds waitDuration = 5ms) {
    auto completedFutures = std::list<std::future<T>*>();

    while (!futures.empty()) {
        for (auto it = futures.begin(); it != futures.end();) {
            auto f = *it;
            if (f->wait_for(0ms) == std::future_status::ready) {
                completedFutures.push_back(f);
                it = futures.erase(it);
            } else {
                ++it;
            }
        }

        std::this_thread::sleep_for(waitDuration);
    }

    return completedFutures;
}

TEST(RaceFutures, Ordering) {
    auto shorter = std::async(std::launch::async, [] { std::this_thread::sleep_for(10ms); });
    auto longer = std::async(std::launch::async, [] { std::this_thread::sleep_for(50ms); });

    auto completed = raceFutures<void>({&longer, &shorter});
    ASSERT_THAT(completed.front(), Eq(&shorter));
    completed.pop_front();

    ASSERT_THAT(completed.front(), Eq(&longer));
}

/*
 * Tests that setSchedulingConfigs() works with valid input
 */
TEST_P(NpuSchedulingAidl, SetSchedulingConfigs) {
    std::vector<SchedulingConfig> configs = {
            {.uid = 1001, .priority = 500, .hasDirectAccess = true, .canAttributeOtherUid = false}};

    auto status = scheduling->setSchedulingConfigs(configs);
    ASSERT_TRUE(status.isOk()) << "setSchedulingConfigs failed: " << status.getDescription();
}

/*
 * Tests that setSchedulingConfigs() rejects invalid priorities
 */
TEST_P(NpuSchedulingAidl, SetSchedulingConfigsInvalidPriority) {
    std::vector<SchedulingConfig> configs = {
            {.uid = 1001, .priority = -1, .hasDirectAccess = true, .canAttributeOtherUid = false}};

    auto status = scheduling->setSchedulingConfigs(configs);
    ASSERT_FALSE(status.isOk())
            << "setSchedulingConfigs with invalid priority must return EX_ILLEGAL_ARGUMENT";
    ASSERT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

/*
 * Tests that setSchedulingConfigs() rejects multiple configs for the same UID
 */
TEST_P(NpuSchedulingAidl, SetSchedulingConfigsDuplicateUid) {
    std::vector<SchedulingConfig> configs = {
            {.uid = 1001, .priority = 0, .hasDirectAccess = true, .canAttributeOtherUid = false},
            {.uid = 1001,
             .priority = 1000,
             .hasDirectAccess = true,
             .canAttributeOtherUid = false}};

    auto status = scheduling->setSchedulingConfigs(configs);
    ASSERT_FALSE(status.isOk()) << "setSchedulingConfigs with multiple configs for the same UID "
                                   "must return EX_ILLEGAL_ARGUMENT";
    ASSERT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

/*
 * Tests that updateSchedulingConfigs() works with valid input
 */
TEST_P(NpuSchedulingAidl, UpdateSchedulingConfigs) {
    std::vector<SchedulingConfig> configs = {
            {.uid = 1001, .priority = 500, .hasDirectAccess = true, .canAttributeOtherUid = false}};

    auto status = scheduling->updateSchedulingConfigs(configs);
    ASSERT_TRUE(status.isOk()) << "updateSchedulingConfigs failed: " << status.getDescription();
}

/*
 * Tests that setCallback() works with valid instance
 */
TEST_P(NpuSchedulingAidl, SetCallback) {
    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();

    auto status = scheduling->setCallback(callback);
    ASSERT_TRUE(status.isOk()) << "setCallback failed: " << status.getDescription();
}

/*
 * Tests that setCallback() works with a null instance
 */
TEST_P(NpuSchedulingAidl, SetCallbackNull) {
    auto status = scheduling->setCallback(nullptr);
    ASSERT_TRUE(status.isOk()) << "setCallback failed: " << status.getDescription();
}

/*
 * Tests that running an inference generates appropriate status callbacks
 */
TEST_P(NpuSchedulingAidl, RunInferenceStatusCallbacks) {
    const int uidPriority = 250;
    const int jobPriority = 500;

    std::vector<SchedulingConfig> configs = {{.uid = static_cast<int>(geteuid()),
                                              .priority = uidPriority,
                                              .hasDirectAccess = true,
                                              .canAttributeOtherUid = true}};
    ASSERT_TRUE(scheduling->setSchedulingConfigs(configs).isOk());

    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();
    ASSERT_TRUE(scheduling->setCallback(callback).isOk());

    auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now().time_since_epoch())
                             .count();

    ASSERT_TRUE(runner->runInference({.priority = jobPriority}));

    // Ensure debouncing has completed
    std::this_thread::sleep_for(100ms);

    // We expect kRequested, kStarted, kEnded
    auto events = callback->events();
    ASSERT_THAT(events.size(), Eq(3));

    std::queue<WorkEventType> expectedEventTypes{{kRequested, kStarted, kEnded}};
    int lastId = -1;

    while (!events.empty()) {
        auto event = events.top();

        ASSERT_THAT(event.type(), Eq(expectedEventTypes.front()));
        ASSERT_THAT(event.info().jobPriority, Eq(jobPriority));
        ASSERT_THAT(event.info().effectivePriority, Ge(uidPriority));
        auto eventTime = event.info().timestampMs;
        ASSERT_THAT(eventTime, Ge(startTime));
        ASSERT_THAT(event.info().id, Gt(lastId));

        startTime = eventTime;
        lastId = event.info().id;
        events.pop();
        expectedEventTypes.pop();
    }
}

/*
 * Tests that effective priority is computed correctly. This takes into consideration
 * that the exact job priority may not be used as it is converted into a vendor priority.
 * The "high" priority value just needs to be lower than the "low" priority value.
 */
TEST_P(NpuSchedulingAidl, RunInferenceEffectivePriority) {
    const int uidPriority = 0;
    const int lowJobPriority = 1000;
    const int highJobPriority = 0;

    int lowEffectivePriority = 0;
    int highEffectivePriority = 0;

    std::vector<SchedulingConfig> configs = {{.uid = static_cast<int>(geteuid()),
                                              .priority = uidPriority,
                                              .hasDirectAccess = true,
                                              .canAttributeOtherUid = true}};
    ASSERT_TRUE(scheduling->setSchedulingConfigs(configs).isOk());

    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();
    ASSERT_TRUE(scheduling->setCallback(callback).isOk());

    // Run an inference with low job priority
    ASSERT_TRUE(runner->runInference({.priority = lowJobPriority}));

    // Ensure debouncing has completed
    std::this_thread::sleep_for(100ms);

    auto events = callback->events();
    ASSERT_FALSE(events.empty());

    auto event = events.top();
    ASSERT_THAT(event.type(), Eq(kRequested));
    ASSERT_THAT(event.info().jobPriority, Eq(lowJobPriority));
    lowEffectivePriority = event.info().effectivePriority;

    callback->clearEvents();

    // Now run an inference with high job priority
    ASSERT_TRUE(runner->runInference({.priority = highJobPriority}));

    // Ensure debouncing has completed
    std::this_thread::sleep_for(100ms);

    events = callback->events();
    ASSERT_FALSE(events.empty());

    event = events.top();
    ASSERT_THAT(event.type(), Eq(kRequested));
    ASSERT_THAT(event.info().jobPriority, Eq(highJobPriority));
    highEffectivePriority = event.info().effectivePriority;

    ASSERT_THAT(lowEffectivePriority, Gt(highEffectivePriority));
}

/*
 * Tests that the highest priority work finishes first
 */
TEST_P(NpuSchedulingAidl, RunInferencesPrioritized) {
    const int lowUid = 10000;
    const int lowAppPriority = 1000;
    const int lowJobPriority = 1000;

    const int highUid = 10001;
    const int highAppPriority = 0;
    const int highJobPriority = 0;

    int lowEffectivePriority = 0;
    int highEffectivePriority = 0;

    std::vector<SchedulingConfig> configs = {{.uid = lowUid,
                                              .priority = lowAppPriority,
                                              .hasDirectAccess = true,
                                              .canAttributeOtherUid = false},
                                             {.uid = highUid,
                                              .priority = highAppPriority,
                                              .hasDirectAccess = true,
                                              .canAttributeOtherUid = false}};
    ASSERT_TRUE(scheduling->setSchedulingConfigs(configs).isOk());

    auto callback = ndk::SharedRefBase::make<SchedulingCallback>();
    ASSERT_TRUE(scheduling->setCallback(callback).isOk());

    auto first = runner->runInferenceAsync({.priority = lowJobPriority});
    std::this_thread::sleep_for(100ms);
    auto second = runner->runInferenceAsync({.priority = highJobPriority});

    auto completed = raceFutures<bool>({&first, &second});
    std::list<std::future<bool>*> expected = {&second, &first};

    // Ensure all completed successfully
    for (auto f : completed) {
        ASSERT_TRUE(f->get());
    }

    // Ensure they finished in the expected order
    ASSERT_THAT(expected, Eq(completed));
}

/*
 * Tests that inference fails when hasDirectAccess=false
 */
TEST_P(NpuSchedulingAidl, RunInferenceFailsWithoutDirectAccess) {
    std::vector<SchedulingConfig> configs = {{.uid = static_cast<int>(geteuid()),
                                              .priority = 0,
                                              .hasDirectAccess = false,
                                              .canAttributeOtherUid = false}};

    auto status = scheduling->setSchedulingConfigs(configs);
    ASSERT_TRUE(status.isOk()) << "setSchedulingConfigs failed: " << status.getDescription();
    ASSERT_FALSE(runner->runInference());
}

/*
 * Tests that inference fails when canAttributeOtherUid=false and other uid specified
 */
TEST_P(NpuSchedulingAidl, RunInferenceFailsWithoutAttributeOtherUid) {
    std::vector<SchedulingConfig> configs = {{.uid = static_cast<int>(geteuid()),
                                              .priority = 0,
                                              .hasDirectAccess = true,
                                              .canAttributeOtherUid = false}};

    auto status = scheduling->setSchedulingConfigs(configs);
    ASSERT_TRUE(status.isOk()) << "setSchedulingConfigs failed: " << status.getDescription();
    ASSERT_FALSE(runner->runInference({.originalUid = 42}));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NpuSchedulingAidl);
INSTANTIATE_TEST_SUITE_P(NpuScheduling, NpuSchedulingAidl,
                         testing::ValuesIn(getAidlHalInstanceNames(IScheduling::descriptor)),
                         PrintInstanceNameToString);

// Check whether the given named feature is available.
static bool checkFeature(const std::string& name) {
    ::android::sp<::android::IServiceManager> sm(::android::defaultServiceManager());
    ::android::sp<::android::IBinder> binder(
            sm->waitForService(::android::String16("package_native")));
    if (binder == nullptr) {
        GTEST_LOG_(ERROR) << "waitForService package_native failed";
        return false;
    }
    ::android::sp<::android::content::pm::IPackageManagerNative> packageMgr =
            ::android::interface_cast<::android::content::pm::IPackageManagerNative>(binder);
    if (packageMgr == nullptr) {
        GTEST_LOG_(ERROR) << "Cannot find package manager";
        return false;
    }
    bool hasFeature = false;
    auto status = packageMgr->hasSystemFeature(::android::String16(name.c_str()), 0, &hasFeature);
    if (!status.isOk()) {
        GTEST_LOG_(ERROR) << "hasSystemFeature('" << name << "') failed: " << status;
        return false;
    }
    return hasFeature;
}

// [VSR-5.7-001] (if device has an NPU as indicated by kFeatureHardwareNpu) MUST support
// FEATURE_NPU and implement the android.hardware.npu HAL interface
TEST(NpuFeature, ImplementsHal) {
    if (!checkFeature(kFeatureHardwareNpu)) {
        GTEST_SKIP() << "Device does not declare feature " << kFeatureHardwareNpu;
        return;
    }

    ASSERT_FALSE(getAidlHalInstanceNames(IScheduling::descriptor).empty())
            << "No implementation for " << IScheduling::descriptor << " found";
}

}  // namespace aidl::android::hardware::npu

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
