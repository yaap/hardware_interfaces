/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "VtsHalContexthubUtilsCommon.h"

#include <aidl/android/hardware/contexthub/BnContextHub.h>
#include <aidl/android/hardware/contexthub/BnContextHubCallback.h>
#include <aidl/android/hardware/contexthub/BnEndpointCallback.h>
#include <aidl/android/hardware/contexthub/IContextHub.h>
#include <aidl/android/hardware/contexthub/IContextHubCallback.h>
#include <aidl/android/hardware/contexthub/IEndpointCallback.h>
#include <aidl/android/hardware/contexthub/IEndpointCommunication.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <android-base/unique_fd.h>
#include <fcntl.h>
#include <log/log.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cinttypes>
#include <future>
#include <optional>

#include "data_flow/host/notification_manager.h"
#include "data_flow/host/region_manager.h"
#include "data_flow/queue.h"

using ::aidl::android::hardware::contexthub::AsyncEventType;
using ::aidl::android::hardware::contexthub::BnContextHubCallback;
using ::aidl::android::hardware::contexthub::BnEndpointCallback;
using ::aidl::android::hardware::contexthub::ContextHubInfo;
using ::aidl::android::hardware::contexthub::ContextHubMessage;
using ::aidl::android::hardware::contexthub::DataFlowConsumerHandle;
using ::aidl::android::hardware::contexthub::DataFlowId;
using ::aidl::android::hardware::contexthub::DataFlowInfo;
using ::aidl::android::hardware::contexthub::DataFlowNotificationFds;
using ::aidl::android::hardware::contexthub::EndpointId;
using ::aidl::android::hardware::contexthub::EndpointInfo;
using ::aidl::android::hardware::contexthub::ErrorCode;
using ::aidl::android::hardware::contexthub::HostEndpointInfo;
using ::aidl::android::hardware::contexthub::HubInfo;
using ::aidl::android::hardware::contexthub::IContextHub;
using ::aidl::android::hardware::contexthub::IContextHubCallbackDefault;
using ::aidl::android::hardware::contexthub::IEndpointCommunication;
using ::aidl::android::hardware::contexthub::Message;
using ::aidl::android::hardware::contexthub::MessageDeliveryStatus;
using ::aidl::android::hardware::contexthub::NanoappBinary;
using ::aidl::android::hardware::contexthub::NanoappInfo;
using ::aidl::android::hardware::contexthub::NanoappRpcService;
using ::aidl::android::hardware::contexthub::NanSessionRequest;
using ::aidl::android::hardware::contexthub::NanSessionStateUpdate;
using ::aidl::android::hardware::contexthub::Reason;
using ::aidl::android::hardware::contexthub::Service;
using ::aidl::android::hardware::contexthub::Setting;
using ::aidl::android::hardware::contexthub::SharedDataRegion;
using ::aidl::android::hardware::contexthub::SharedDataRegionRequirements;
using ::android::contexthub::data_flow::AllocatorRegion;
using ::android::contexthub::data_flow::Consumer;
using ::android::contexthub::data_flow::ConsumerManager;
using ::android::contexthub::data_flow::ConsumerPolicyBuilder;
using ::android::contexthub::data_flow::createQueue;
using ::android::contexthub::data_flow::DataNotifier;
using ::android::contexthub::data_flow::NotificationManager;
using ::android::contexthub::data_flow::Producer;
using ::android::contexthub::data_flow::queueLayout;
using ::android::contexthub::data_flow::Region;
using ::android::contexthub::data_flow::RegionManager;
using ::android::contexthub::data_flow::RemoteNotifyArgs;
using ::android::contexthub::data_flow::internal::ProducerBase;
using ::android::hardware::contexthub::vts_utils::kNonExistentAppId;
using ::android::hardware::contexthub::vts_utils::waitForCallback;
using ::ndk::ScopedAStatus;
using ::ndk::ScopedFileDescriptor;
using ::ndk::SharedRefBase;
using ::ndk::SpAIBinder;

// 6612b522-b717-41c8-b48d-c0b1cc64e142
constexpr std::array<uint8_t, 16> kUuid = {0x66, 0x12, 0xb5, 0x22, 0xb7, 0x17, 0x41, 0xc8,
                                           0xb4, 0x8d, 0xc0, 0xb1, 0xcc, 0x64, 0xe1, 0x42};

const std::string kName{"VtsAidlHalContextHubTargetTest"};

const std::string kEchoServiceName{"android.hardware.contexthub.test.EchoService"};

constexpr int64_t kDefaultHubId = 1;

class TestEndpointCallback : public BnEndpointCallback {
  public:
    ScopedAStatus onEndpointStarted(const std::vector<EndpointInfo>& /* endpointInfos */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onEndpointStopped(const std::vector<EndpointId>& /* endpointIds */,
                                    Reason /* reason */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onMessageReceived(int32_t /* sessionId */, const Message& message) override {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mMessages.push_back(message);
        }
        mCondVar.notify_one();
        return ScopedAStatus::ok();
    }

    ScopedAStatus onMessageDeliveryStatusReceived(
            int32_t /* sessionId */, const MessageDeliveryStatus& /* msgStatus */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onEndpointSessionOpenRequest(
            int32_t /* sessionId */, const EndpointId& /* destination */,
            const EndpointId& /* initiator */,
            const std::optional<std::string>& /* serviceDescriptor */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onCloseEndpointSession(int32_t /* sessionId */, Reason /* reason */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onEndpointSessionOpenComplete(int32_t /* sessionId */) override {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mWasOnEndpointSessionOpenCompleteCalled = true;
        }
        mCondVar.notify_one();
        return ScopedAStatus::ok();
    }

    bool wasOnEndpointSessionOpenCompleteCalled() {
        return mWasOnEndpointSessionOpenCompleteCalled;
    }

    void resetWasOnEndpointSessionOpenCompleteCalled() {
        mWasOnEndpointSessionOpenCompleteCalled = false;
    }

    DataFlowConsumerHandle deepCopyDataFlowConsumerHandle(const DataFlowConsumerHandle& src) {
        DataFlowConsumerHandle dst;
        dst.id = src.id;

        if (src.info.has_value()) {
            dst.info.emplace();
            dst.info->region.id = src.info->region.id;
            dst.info->region.sizeBytes = src.info->region.sizeBytes;
            dst.info->debugName = src.info->debugName;

            // Deep copy SharedMemory
            if (src.info->region.sharedMemory.get() >= 0) {
                int dupFd = dup(src.info->region.sharedMemory.get());
                dst.info->region.sharedMemory = ScopedFileDescriptor(dupFd);
            }
            dst.info->region.permissions = src.info->region.permissions;
            dst.info->metadataOffsetBytes = src.info->metadataOffsetBytes;

            // Deep copy EventFDs
            auto maybeFds = ::android::contexthub::data_flow::dupEventFds(src.info->notificationFds,
                                                                          /*needsHalAck=*/false);
            if (maybeFds.ok()) {
                dst.info->notificationFds = std::move(*maybeFds);
            } else {
                ALOGE("onDataFlowHostConsumerRegistered: failed to dup producer notificationFds");
            }
        }

        // Deep copy Consumer Region if exists
        if (src.consumerRegion.has_value()) {
            dst.consumerRegion.emplace();
            dst.consumerRegion->id = src.consumerRegion->id;
            dst.consumerRegion->sizeBytes = src.consumerRegion->sizeBytes;
            if (src.consumerRegion->sharedMemory.get() >= 0) {
                int dupFd = dup(src.consumerRegion->sharedMemory.get());
                dst.consumerRegion->sharedMemory = ScopedFileDescriptor(dupFd);
            }
            dst.consumerRegion->permissions = src.consumerRegion->permissions;
        }

        dst.metadataOffsetBytes = src.metadataOffsetBytes;

        // Deep copy Consumer EventFDs
        auto maybeFds = ::android::contexthub::data_flow::dupEventFds(src.notificationFds,
                                                                      /*needsHalAck=*/true);
        if (maybeFds.ok()) {
            dst.notificationFds = std::move(*maybeFds);
        } else {
            ALOGE("onDataFlowHostConsumerRegistered: failed to dup consumer notificationFds");
        }

        return dst;
    }

    ScopedAStatus onDataFlowHostConsumerRegistered(const DataFlowConsumerHandle& handle,
                                                   const EndpointId& producerId,
                                                   const EndpointId& consumerId,
                                                   const ::std::optional<Message>& /*msg*/,
                                                   int32_t /*sessionId*/) override {
        std::unique_lock<std::mutex> lock(mMutex);
        mDataFlowHandle = deepCopyDataFlowConsumerHandle(handle);
        // Prepares halAckEventFd in consumer side.
        // TODO(b/455420744): The HAL should provide this.
        mDataFlowHandle.notificationFds.halAck =
                ScopedFileDescriptor(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));
        mProducerId = producerId;
        mConsumerId = consumerId;
        mWasOnDataFlowHostConsumerRegisteredCalled = true;
        mCondVar.notify_one();
        return ScopedAStatus::ok();
    }

    ScopedAStatus onDataFlowOffloadEndpointUnregistered(
            const DataFlowId& /*dataFlowId*/, const EndpointId& /*endpointId*/,
            const std::vector<EndpointId>& /*destinationIds*/) override {
        // VTS tests act as the host side and no need to implement this function.
        return ScopedAStatus::ok();
    }

    bool wasOnDataFlowHostConsumerRegisteredCalled() {
        return mWasOnDataFlowHostConsumerRegisteredCalled;
    }
    void resetWasOnDataFlowHostConsumerRegisteredCalled() {
        mWasOnDataFlowHostConsumerRegisteredCalled = false;
    }
    const DataFlowConsumerHandle& getDataFlowHandle() { return mDataFlowHandle; }
    EndpointId getProducerId() { return mProducerId; }
    EndpointId getConsumerId() { return mConsumerId; }

    std::mutex& getMutex() { return mMutex; }
    std::condition_variable& getCondVar() { return mCondVar; }
    std::vector<Message> getMessages() { return mMessages; }

  private:
    std::vector<Message> mMessages;
    std::mutex mMutex;
    std::condition_variable mCondVar;
    bool mWasOnEndpointSessionOpenCompleteCalled = false;
    bool mWasOnDataFlowHostConsumerRegisteredCalled = false;
    DataFlowConsumerHandle mDataFlowHandle;
    EndpointId mProducerId;
    EndpointId mConsumerId;
};

class ContextHubAidl : public testing::TestWithParam<std::tuple<std::string, int32_t>> {
  public:
    void SetUp() override {
        std::string serviceName = std::get<0>(GetParam());
        SpAIBinder binder(AServiceManager_waitForService(serviceName.c_str()));
        mContextHub = IContextHub::fromBinder(binder);
        ASSERT_NE(mContextHub, nullptr);
    }

    uint32_t getHubId() { return std::get<1>(GetParam()); }

    void testSettingChanged(Setting setting);

    std::shared_ptr<IContextHub> mContextHub;
};

class ContextHubEndpointAidl : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        std::string serviceName = GetParam();
        SpAIBinder binder(AServiceManager_waitForService(serviceName.c_str()));
        mContextHub = IContextHub::fromBinder(binder);
        ASSERT_NE(mContextHub, nullptr);
        mEndpointCb = SharedRefBase::make<TestEndpointCallback>();
    }

    void TearDown() override {
        if (mHubInterface) mHubInterface->unregister();
    }

    ScopedAStatus registerHub(int64_t id, std::shared_ptr<IEndpointCommunication>* hubInterface) {
        HubInfo info;
        info.hubId = id;
        return mContextHub->registerEndpointHub(mEndpointCb, info, hubInterface);
    }

    bool registerDefaultHub() {
        ScopedAStatus status = registerHub(kDefaultHubId, &mHubInterface);
        if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
            status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
            return false;
        }
        EXPECT_EQ(status.getExceptionCode(), EX_NONE);
        EXPECT_NE(mHubInterface, nullptr);
        if (!mHubInterface) {
            return false;
        }
        return true;
    }

    std::shared_ptr<IContextHub> mContextHub;
    std::shared_ptr<TestEndpointCallback> mEndpointCb;
    std::shared_ptr<IEndpointCommunication> mHubInterface;
};

class ContextHubEndpointAidlWithTestMode : public ContextHubEndpointAidl {
  public:
    void SetUp() override {
        ContextHubEndpointAidl::SetUp();

        // Best effort enable test mode - this may not be supported on older HALS, so we
        // ignore the return value.
        mContextHub->setTestMode(/* enable= */ true);
    }

    void TearDown() override {
        mContextHub->setTestMode(/* enable= */ false);
        ContextHubEndpointAidl::TearDown();
    }

    bool areDataFlowsSupported() {
        EXPECT_NE(mHubInterface, nullptr)
                << "should call registerDefaultHub() to prepare mHubInterface first";
        if (!mHubInterface) {
            return false;
        }
        return true;
    }
};

TEST_P(ContextHubEndpointAidl, TestGetHubs) {
    std::vector<ContextHubInfo> hubs;
    ASSERT_TRUE(mContextHub->getContextHubs(&hubs).isOk());

    ALOGD("System reports %zu hubs", hubs.size());

    for (const ContextHubInfo& hub : hubs) {
        ALOGD("Checking hub ID %" PRIu32, hub.id);

        EXPECT_GT(hub.name.size(), 0);
        EXPECT_GT(hub.vendor.size(), 0);
        EXPECT_GT(hub.toolchain.size(), 0);
        EXPECT_GT(hub.peakMips, 0);
        EXPECT_GT(hub.chrePlatformId, 0);
        EXPECT_GT(hub.chreApiMajorVersion, 0);
        EXPECT_GE(hub.chreApiMinorVersion, 0);
        EXPECT_GE(hub.chrePatchVersion, 0);

        // Minimum 128 byte MTU as required by CHRE API v1.0
        EXPECT_GE(hub.maxSupportedMessageLengthBytes, UINT32_C(128));
    }
}

TEST_P(ContextHubEndpointAidl, TestEnableTestMode) {
    ScopedAStatus status = mContextHub->setTestMode(true);
    if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
        status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
    }
}

TEST_P(ContextHubEndpointAidl, TestDisableTestMode) {
    ScopedAStatus status = mContextHub->setTestMode(false);
    if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
        status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
    }
}

class EmptyContextHubCallback : public BnContextHubCallback {
  public:
    ScopedAStatus handleNanoappInfo(const std::vector<NanoappInfo>& /* appInfo */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleContextHubMessage(
            const ContextHubMessage& /* msg */,
            const std::vector<std::string>& /* msgContentPerms */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleContextHubAsyncEvent(AsyncEventType /* evt */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleTransactionResult(int32_t /* transactionId */,
                                          bool /* success */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleNanSessionRequest(const NanSessionRequest& /* request */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleMessageDeliveryStatus(
            char16_t /* hostEndPointId */,
            const MessageDeliveryStatus& /* messageDeliveryStatus */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus getUuid(std::array<uint8_t, 16>* out_uuid) override {
        *out_uuid = kUuid;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getName(std::string* out_name) override {
        *out_name = kName;
        return ScopedAStatus::ok();
    }
};

TEST_P(ContextHubAidl, TestRegisterCallback) {
    std::shared_ptr<EmptyContextHubCallback> cb = SharedRefBase::make<EmptyContextHubCallback>();
    ASSERT_TRUE(mContextHub->registerCallback(getHubId(), cb).isOk());
}

// Helper callback that puts the async appInfo callback data into a promise
class QueryAppsCallback : public BnContextHubCallback {
  public:
    ScopedAStatus handleNanoappInfo(const std::vector<NanoappInfo>& appInfo) override {
        ALOGD("Got app info callback with %zu apps", appInfo.size());
        promise.set_value(appInfo);
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleContextHubMessage(
            const ContextHubMessage& /* msg */,
            const std::vector<std::string>& /* msgContentPerms */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleContextHubAsyncEvent(AsyncEventType /* evt */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleTransactionResult(int32_t /* transactionId */,
                                          bool /* success */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleNanSessionRequest(const NanSessionRequest& /* request */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleMessageDeliveryStatus(
            char16_t /* hostEndPointId */,
            const MessageDeliveryStatus& /* messageDeliveryStatus */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus getUuid(std::array<uint8_t, 16>* out_uuid) override {
        *out_uuid = kUuid;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getName(std::string* out_name) override {
        *out_name = kName;
        return ScopedAStatus::ok();
    }

    std::promise<std::vector<NanoappInfo>> promise;
};

// Calls queryApps() and checks the returned metadata
TEST_P(ContextHubAidl, TestQueryApps) {
    std::shared_ptr<QueryAppsCallback> cb = SharedRefBase::make<QueryAppsCallback>();
    ASSERT_TRUE(mContextHub->registerCallback(getHubId(), cb).isOk());
    ASSERT_TRUE(mContextHub->queryNanoapps(getHubId()).isOk());

    std::vector<NanoappInfo> appInfoList;
    ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &appInfoList));
    for (const NanoappInfo& appInfo : appInfoList) {
        EXPECT_NE(appInfo.nanoappId, UINT64_C(0));
        EXPECT_NE(appInfo.nanoappId, kNonExistentAppId);

        // Verify services are unique.
        std::set<uint64_t> existingServiceIds;
        for (const NanoappRpcService& rpcService : appInfo.rpcServices) {
            EXPECT_NE(rpcService.id, UINT64_C(0));
            EXPECT_EQ(existingServiceIds.count(rpcService.id), 0);
            existingServiceIds.insert(rpcService.id);
        }
    }
}

// Calls getPreloadedNanoappsIds() and verifies there are preloaded nanoapps
TEST_P(ContextHubAidl, TestGetPreloadedNanoappIds) {
    std::vector<int64_t> preloadedNanoappIds;
    ScopedAStatus status = mContextHub->getPreloadedNanoappIds(getHubId(), &preloadedNanoappIds);
    if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
        status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
    }
}

// Helper callback that puts the TransactionResult for the expectedTransactionId into a
// promise
class TransactionResultCallback : public BnContextHubCallback {
  public:
    ScopedAStatus handleNanoappInfo(const std::vector<NanoappInfo>& /* appInfo */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleContextHubMessage(
            const ContextHubMessage& /* msg */,
            const std::vector<std::string>& /* msgContentPerms */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleContextHubAsyncEvent(AsyncEventType /* evt */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleTransactionResult(int32_t transactionId, bool success) override {
        ALOGD("Got transaction result callback for transactionId %" PRIu32 " (expecting %" PRIu32
              ") with success %d",
              transactionId, expectedTransactionId, success);
        if (transactionId == expectedTransactionId) {
            promise.set_value(success);
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleNanSessionRequest(const NanSessionRequest& /* request */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus handleMessageDeliveryStatus(
            char16_t /* hostEndPointId */,
            const MessageDeliveryStatus& /* messageDeliveryStatus */) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus getUuid(std::array<uint8_t, 16>* out_uuid) override {
        *out_uuid = kUuid;
        return ScopedAStatus::ok();
    }

    ScopedAStatus getName(std::string* out_name) override {
        *out_name = kName;
        return ScopedAStatus::ok();
    }

    uint32_t expectedTransactionId = 0;
    std::promise<bool> promise;
};

// Parameterized fixture that sets the callback to TransactionResultCallback
class ContextHubTransactionTest : public ContextHubAidl {
  public:
    virtual void SetUp() override {
        ContextHubAidl::SetUp();
        ASSERT_TRUE(mContextHub->registerCallback(getHubId(), cb).isOk());
    }

    std::shared_ptr<TransactionResultCallback> cb =
            SharedRefBase::make<TransactionResultCallback>();
};

TEST_P(ContextHubTransactionTest, TestSendMessageToNonExistentNanoapp) {
    ContextHubMessage message;
    message.nanoappId = kNonExistentAppId;
    message.messageType = 1;
    message.messageBody.resize(4);
    std::fill(message.messageBody.begin(), message.messageBody.end(), 0);

    ALOGD("Sending message to non-existent nanoapp");
    ASSERT_TRUE(mContextHub->sendMessageToHub(getHubId(), message).isOk());
}

TEST_P(ContextHubTransactionTest, TestLoadEmptyNanoapp) {
    cb->expectedTransactionId = 0123;
    NanoappBinary emptyApp;

    emptyApp.nanoappId = kNonExistentAppId;
    emptyApp.nanoappVersion = 1;
    emptyApp.flags = 0;
    emptyApp.targetChreApiMajorVersion = 1;
    emptyApp.targetChreApiMinorVersion = 0;

    ALOGD("Loading empty nanoapp");
    bool success = mContextHub->loadNanoapp(getHubId(), emptyApp, cb->expectedTransactionId).isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

TEST_P(ContextHubTransactionTest, TestUnloadNonexistentNanoapp) {
    cb->expectedTransactionId = 1234;

    ALOGD("Unloading nonexistent nanoapp");
    bool success =
            mContextHub->unloadNanoapp(getHubId(), kNonExistentAppId, cb->expectedTransactionId)
                    .isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

TEST_P(ContextHubTransactionTest, TestEnableNonexistentNanoapp) {
    cb->expectedTransactionId = 2345;

    ALOGD("Enabling nonexistent nanoapp");
    bool success =
            mContextHub->enableNanoapp(getHubId(), kNonExistentAppId, cb->expectedTransactionId)
                    .isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

TEST_P(ContextHubTransactionTest, TestDisableNonexistentNanoapp) {
    cb->expectedTransactionId = 3456;

    ALOGD("Disabling nonexistent nanoapp");
    bool success =
            mContextHub->disableNanoapp(getHubId(), kNonExistentAppId, cb->expectedTransactionId)
                    .isOk();
    if (success) {
        bool transactionSuccess;
        ASSERT_TRUE(waitForCallback(cb->promise.get_future(), &transactionSuccess));
        ASSERT_FALSE(transactionSuccess);
    }
}

void ContextHubAidl::testSettingChanged(Setting setting) {
    // In VTS, we only test that sending the values doesn't cause things to blow up - GTS tests
    // verify the expected E2E behavior in CHRE
    std::shared_ptr<EmptyContextHubCallback> cb = SharedRefBase::make<EmptyContextHubCallback>();
    ASSERT_TRUE(mContextHub->registerCallback(getHubId(), cb).isOk());

    ASSERT_TRUE(mContextHub->onSettingChanged(setting, true /* enabled */).isOk());
    ASSERT_TRUE(mContextHub->onSettingChanged(setting, false /* enabled */).isOk());
}

TEST_P(ContextHubAidl, TestOnLocationSettingChanged) {
    testSettingChanged(Setting::LOCATION);
}

TEST_P(ContextHubAidl, TestOnWifiMainSettingChanged) {
    testSettingChanged(Setting::WIFI_MAIN);
}

TEST_P(ContextHubAidl, TestOnWifiScanningSettingChanged) {
    testSettingChanged(Setting::WIFI_SCANNING);
}

TEST_P(ContextHubAidl, TestOnAirplaneModeSettingChanged) {
    testSettingChanged(Setting::AIRPLANE_MODE);
}

TEST_P(ContextHubAidl, TestOnMicrophoneSettingChanged) {
    testSettingChanged(Setting::MICROPHONE);
}

TEST_P(ContextHubAidl, TestOnBtMainSettingChanged) {
    testSettingChanged(Setting::BT_MAIN);
}

TEST_P(ContextHubAidl, TestOnBtScanningSettingChanged) {
    testSettingChanged(Setting::BT_SCANNING);
}

std::vector<std::tuple<std::string, int32_t>> generateContextHubMapping() {
    std::vector<std::tuple<std::string, int32_t>> tuples;
    auto contextHubAidlNames = android::getAidlHalInstanceNames(IContextHub::descriptor);
    std::vector<ContextHubInfo> contextHubInfos;

    for (int i = 0; i < contextHubAidlNames.size(); i++) {
        auto contextHubName = contextHubAidlNames[i].c_str();
        SpAIBinder binder(AServiceManager_waitForService(contextHubName));
        auto contextHub = IContextHub::fromBinder(binder);
        if (contextHub->getContextHubs(&contextHubInfos).isOk()) {
            for (auto& info : contextHubInfos) {
                tuples.push_back(std::make_tuple(contextHubName, info.id));
            }
        }
    }

    return tuples;
}

TEST_P(ContextHubTransactionTest, TestHostConnection) {
    constexpr char16_t kHostEndpointId = 1;
    HostEndpointInfo hostEndpointInfo;
    hostEndpointInfo.type = HostEndpointInfo::Type::NATIVE;
    hostEndpointInfo.hostEndpointId = kHostEndpointId;

    ScopedAStatus status = mContextHub->onHostEndpointConnected(hostEndpointInfo);
    if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
        status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
        ASSERT_TRUE(mContextHub->onHostEndpointDisconnected(kHostEndpointId).isOk());
    }
}

TEST_P(ContextHubTransactionTest, TestInvalidHostConnection) {
    constexpr char16_t kHostEndpointId = 1;
    ScopedAStatus status = mContextHub->onHostEndpointDisconnected(kHostEndpointId);
    if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
        status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
    }
}

TEST_P(ContextHubTransactionTest, TestNanSessionStateChange) {
    NanSessionStateUpdate update;
    update.state = true;
    ScopedAStatus status = mContextHub->onNanSessionStateChanged(update);
    if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
        status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        ASSERT_TRUE(status.isOk());
        update.state = false;
        ASSERT_TRUE(mContextHub->onNanSessionStateChanged(update).isOk());
    }
}

TEST_P(ContextHubAidl, TestSendMessageDeliveryStatusToHub) {
    MessageDeliveryStatus messageDeliveryStatus;
    messageDeliveryStatus.messageSequenceNumber = 123;
    messageDeliveryStatus.errorCode = ErrorCode::OK;

    ScopedAStatus status =
            mContextHub->sendMessageDeliveryStatusToHub(getHubId(), messageDeliveryStatus);
    if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
        status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    } else {
        EXPECT_TRUE(status.isOk());
    }
}

TEST_P(ContextHubEndpointAidlWithTestMode, RegisterHub) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    std::shared_ptr<IEndpointCommunication> hub2;
    ScopedAStatus status = registerHub(kDefaultHubId + 1, &hub2);
    EXPECT_TRUE(status.isOk());

    std::shared_ptr<IEndpointCommunication> hub3;
    status = registerHub(kDefaultHubId + 1, &hub3);
    EXPECT_EQ(status.getExceptionCode(), EX_ILLEGAL_STATE);

    hub2->unregister();
    status = registerHub(kDefaultHubId + 1, &hub3);
    EXPECT_TRUE(status.isOk());
    hub3->unregister();
}

TEST_P(ContextHubEndpointAidlWithTestMode, RegisterEndpoint) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    EndpointInfo endpointInfo;
    endpointInfo.id.id = 1;
    endpointInfo.id.hubId = kDefaultHubId;
    endpointInfo.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo.name = std::string("Test host endpoint 1");
    endpointInfo.version = 42;

    ScopedAStatus status = mHubInterface->registerEndpoint(endpointInfo);
    EXPECT_EQ(status.getExceptionCode(), EX_NONE);
}

TEST_P(ContextHubEndpointAidlWithTestMode, RegisterEndpointForDifferentHub) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    EndpointInfo endpointInfo;
    endpointInfo.id.id = 1;
    endpointInfo.id.hubId = kDefaultHubId + 1;
    endpointInfo.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo.name = std::string("Test host endpoint 1");
    endpointInfo.version = 42;

    ScopedAStatus status = mHubInterface->registerEndpoint(endpointInfo);
    EXPECT_NE(status.getExceptionCode(), EX_NONE);
}

TEST_P(ContextHubEndpointAidlWithTestMode, RegisterEndpointSameNameFailure) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    EndpointInfo endpointInfo;
    endpointInfo.id.id = 2;
    endpointInfo.id.hubId = kDefaultHubId;
    endpointInfo.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo.name = std::string("Test host endpoint 2");
    endpointInfo.version = 42;

    EndpointInfo endpointInfo2;
    endpointInfo2.id.id = 3;
    endpointInfo2.id.hubId = kDefaultHubId;
    endpointInfo2.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo2.name = std::string("Test host endpoint 2");
    endpointInfo2.version = 42;

    ScopedAStatus status = mHubInterface->registerEndpoint(endpointInfo);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);
    EXPECT_FALSE(mHubInterface->registerEndpoint(endpointInfo2).isOk());
}

TEST_P(ContextHubEndpointAidlWithTestMode, RegisterEndpointSameIdFailure) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    EndpointInfo endpointInfo;
    endpointInfo.id.id = 4;
    endpointInfo.id.hubId = kDefaultHubId;
    endpointInfo.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo.name = std::string("Test host endpoint 4");
    endpointInfo.version = 42;

    EndpointInfo endpointInfo2;
    endpointInfo2.id.id = 4;
    endpointInfo2.id.hubId = kDefaultHubId;
    endpointInfo2.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo2.name = std::string("Test host endpoint - same ID test");
    endpointInfo2.version = 42;

    ScopedAStatus status = mHubInterface->registerEndpoint(endpointInfo);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);
    EXPECT_FALSE(mHubInterface->registerEndpoint(endpointInfo2).isOk());
}

TEST_P(ContextHubEndpointAidlWithTestMode, UnregisterEndpoint) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    EndpointInfo endpointInfo;
    endpointInfo.id.id = 6;
    endpointInfo.id.hubId = kDefaultHubId;
    endpointInfo.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo.name = std::string("Test host endpoint 6");
    endpointInfo.version = 42;

    ScopedAStatus status = mHubInterface->registerEndpoint(endpointInfo);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);
    status = mHubInterface->unregisterEndpoint(endpointInfo);
    EXPECT_EQ(status.getExceptionCode(), EX_NONE);
}

TEST_P(ContextHubEndpointAidlWithTestMode, UnregisterEndpointNonexistent) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    EndpointInfo endpointInfo;
    endpointInfo.id.id = 100;
    endpointInfo.id.hubId = kDefaultHubId;
    endpointInfo.type = EndpointInfo::EndpointType::NATIVE;
    endpointInfo.name = std::string("Test host endpoint 100");
    endpointInfo.version = 42;

    EXPECT_FALSE(mHubInterface->unregisterEndpoint(endpointInfo).isOk());
}

TEST_P(ContextHubEndpointAidlWithTestMode, OpenEndpointSessionInvalidRange) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }

    // Register the endpoint
    EndpointInfo initiatorEndpoint;
    initiatorEndpoint.id.id = 7;
    initiatorEndpoint.id.hubId = kDefaultHubId;
    initiatorEndpoint.type = EndpointInfo::EndpointType::NATIVE;
    initiatorEndpoint.name = std::string("Test host endpoint 7");
    initiatorEndpoint.version = 42;
    ScopedAStatus status = mHubInterface->registerEndpoint(initiatorEndpoint);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);

    // Find the destination, if it exists
    std::vector<EndpointInfo> endpoints;
    EXPECT_TRUE(mContextHub->getEndpoints(&endpoints).isOk());
    const EndpointInfo* destinationEndpoint = nullptr;
    for (const EndpointInfo& endpoint : endpoints) {
        for (const Service& service : endpoint.services) {
            if (service.serviceDescriptor == kEchoServiceName) {
                destinationEndpoint = &endpoint;
                break;
            }
        }
    }
    if (destinationEndpoint == nullptr) {
        return;  // no echo service endpoint -> just return
    }

    // Request the range
    constexpr int32_t requestedRange = 100;
    std::array<int32_t, 2> range;
    status = mHubInterface->requestSessionIdRange(requestedRange, &range);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);
    EXPECT_EQ(range.size(), 2);
    EXPECT_GE(range[1] - range[0] + 1, requestedRange);

    // Open the session
    int32_t sessionId = range[1] + 10;  // invalid
    EXPECT_FALSE(mHubInterface
                         ->openEndpointSession(sessionId, destinationEndpoint->id,
                                               initiatorEndpoint.id,
                                               /* in_serviceDescriptor= */ kEchoServiceName)
                         .isOk());
}

TEST_P(ContextHubEndpointAidlWithTestMode, OpenEndpointSessionAndSendMessageEchoesBack) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }
    std::unique_lock<std::mutex> lock(mEndpointCb->getMutex());

    // Register the endpoint
    EndpointInfo initiatorEndpoint;
    initiatorEndpoint.id.id = 8;
    initiatorEndpoint.id.hubId = kDefaultHubId;
    initiatorEndpoint.type = EndpointInfo::EndpointType::NATIVE;
    initiatorEndpoint.name = std::string("Test host endpoint 7");
    initiatorEndpoint.version = 42;
    ScopedAStatus status = mHubInterface->registerEndpoint(initiatorEndpoint);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);

    // Find the destination, if it exists
    std::vector<EndpointInfo> endpoints;
    EXPECT_TRUE(mContextHub->getEndpoints(&endpoints).isOk());
    const EndpointInfo* destinationEndpoint = nullptr;
    for (const EndpointInfo& endpoint : endpoints) {
        for (const Service& service : endpoint.services) {
            if (service.serviceDescriptor == kEchoServiceName) {
                destinationEndpoint = &endpoint;
                break;
            }
        }
    }
    if (destinationEndpoint == nullptr) {
        return;  // no echo service endpoint -> just return
    }

    // Request the range
    constexpr int32_t requestedRange = 100;
    std::array<int32_t, 2> range;
    status = mHubInterface->requestSessionIdRange(requestedRange, &range);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);
    EXPECT_EQ(range.size(), 2);
    EXPECT_GE(range[1] - range[0] + 1, requestedRange);

    // Open the session
    mEndpointCb->resetWasOnEndpointSessionOpenCompleteCalled();
    int32_t sessionId = range[0];
    status = mHubInterface->openEndpointSession(sessionId, destinationEndpoint->id,
                                                initiatorEndpoint.id,
                                                /* in_serviceDescriptor= */ kEchoServiceName);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);
    mEndpointCb->getCondVar().wait(lock);
    EXPECT_TRUE(mEndpointCb->wasOnEndpointSessionOpenCompleteCalled());

    // Send the message
    Message message;
    message.flags = 0;
    message.sequenceNumber = 0;
    message.content.push_back(42);
    status = mHubInterface->sendMessageToEndpoint(sessionId, message);
    ASSERT_EQ(status.getExceptionCode(), EX_NONE);

    // Check for echo
    mEndpointCb->getCondVar().wait(lock);
    EXPECT_FALSE(mEndpointCb->getMessages().empty());
    EXPECT_EQ(mEndpointCb->getMessages().back().content.back(), 42);
}

TEST_P(ContextHubEndpointAidlWithTestMode, TestAllocateSharedDataRegionInvalidSize) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }
    if (!areDataFlowsSupported()) {
        GTEST_SKIP() << "Not supported data flows -> old API; or not implemented";
    }

    // Test allocateSharedDataRegion with invalid parameter (size=0)
    SharedDataRegionRequirements badReqs;
    badReqs.sizeBytes = 0;  // Invalid size
    SharedDataRegion badRegion;
    ScopedAStatus status = mHubInterface->allocateSharedDataRegion(badReqs, &badRegion);

    // Expect failure with EX_ILLEGAL_ARGUMENT
    EXPECT_FALSE(status.isOk());
    EXPECT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

TEST_P(ContextHubEndpointAidlWithTestMode, TestAllocateAndFreeSharedDataRegionSuccess) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }
    if (!areDataFlowsSupported()) {
        GTEST_SKIP() << "Not supported data flows -> old API; or not implemented";
    }

    // Test allocateSharedDataRegion with valid parameter
    SharedDataRegionRequirements requirements;
    constexpr int32_t kRegionSize = 4096;
    requirements.sizeBytes = kRegionSize;
    requirements.permissions = {std::string("com.android.vts.permission.TEST")};
    requirements.targetHubIds = {kDefaultHubId};

    SharedDataRegion region;
    ScopedAStatus status = mHubInterface->allocateSharedDataRegion(requirements, &region);
    ASSERT_TRUE(status.isOk());

    // Checks region information.
    EXPECT_GT(region.id, 0);
    ASSERT_GE(region.sharedMemory.get(), 0);
    ASSERT_TRUE(region.permissions.has_value());
    ASSERT_EQ(region.permissions->size(), 1);
    EXPECT_EQ(region.permissions->at(0), requirements.permissions[0]);

    int32_t allocatedRegionId = region.id;

    // Validates mmap-ing the returned file descriptor.
    void* mappedAddr = mmap(NULL, requirements.sizeBytes, PROT_READ | PROT_WRITE, MAP_SHARED,
                            region.sharedMemory.get(), 0);
    ASSERT_NE(mappedAddr, MAP_FAILED) << "mmap failed: " << strerror(errno);

    // Tests mmap region read/write.
    volatile uint8_t* testPtr = static_cast<volatile uint8_t*>(mappedAddr);
    testPtr[0] = 0xAB;
    EXPECT_EQ(testPtr[0], 0xAB) << "Read-back failed at start of region";
    testPtr[kRegionSize - 1] = 0xCD;
    EXPECT_EQ(testPtr[kRegionSize - 1], 0xCD) << "Read-back failed at end of region";

    int munmap_status = munmap(mappedAddr, requirements.sizeBytes);
    EXPECT_EQ(munmap_status, 0) << "munmap failed: " << strerror(errno);

    // Test freeSharedDataRegion
    status = mHubInterface->freeSharedDataRegion(allocatedRegionId);
    EXPECT_TRUE(status.isOk());
}

TEST_P(ContextHubEndpointAidlWithTestMode, TestFreeSharedDataRegionNonExistent) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }
    if (!areDataFlowsSupported()) {
        GTEST_SKIP() << "Not supported data flows -> old API; or not implemented";
    }

    // Test free non-existent region.
    ScopedAStatus status = mHubInterface->freeSharedDataRegion(-999);  // non-existent region ID
    EXPECT_FALSE(status.isOk());
    EXPECT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

TEST_P(ContextHubEndpointAidlWithTestMode, TestFreeSharedDataRegionDoubleFree) {
    if (!registerDefaultHub()) {
        GTEST_SKIP() << "Not supported -> old API; or not implemented";
    }
    if (!areDataFlowsSupported()) {
        GTEST_SKIP() << "Not supported data flows -> old API; or not implemented";
    }

    // Allocate a valid region
    SharedDataRegionRequirements requirements;
    requirements.sizeBytes = 1024;
    requirements.targetHubIds = {kDefaultHubId};
    SharedDataRegion region;
    ScopedAStatus status = mHubInterface->allocateSharedDataRegion(requirements, &region);
    ASSERT_TRUE(status.isOk());
    ASSERT_GT(region.id, 0);
    int32_t allocatedRegionId = region.id;

    // Free it the first time (should succeed)
    status = mHubInterface->freeSharedDataRegion(allocatedRegionId);
    EXPECT_TRUE(status.isOk());

    // Free it the second time (should fail)
    status = mHubInterface->freeSharedDataRegion(allocatedRegionId);
    EXPECT_FALSE(status.isOk());
    EXPECT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

TEST_P(ContextHubEndpointAidlWithTestMode, TestRegisterHostProducerDataFlowBasic) {
    if (!registerDefaultHub()) GTEST_SKIP() << "Not implemented";
    if (!areDataFlowsSupported()) GTEST_SKIP() << "Data flows not supported";

    // Allocate Region
    SharedDataRegionRequirements requirements;
    requirements.sizeBytes = 4096;
    requirements.targetHubIds = {kDefaultHubId};
    SharedDataRegion region;
    ASSERT_TRUE(mHubInterface->allocateSharedDataRegion(requirements, &region).isOk());

    // Register Data Flow
    EndpointId hostEndpoint;
    hostEndpoint.hubId = kDefaultHubId;
    hostEndpoint.id = 0xCAFE;  // Arbitrary host ID

    DataFlowInfo info;
    info.region.id = region.id;
    // Mock eventfds
    int efd = eventfd(0, 0);
    info.notificationFds.waking = ScopedFileDescriptor(dup(efd));
    info.notificationFds.nonWaking = ScopedFileDescriptor(dup(efd));
    close(efd);

    int32_t dataFlowId = -1;
    ScopedAStatus status =
            mHubInterface->registerDataFlowHostProducer(hostEndpoint, info, &dataFlowId);
    EXPECT_TRUE(status.isOk());
    EXPECT_GE(dataFlowId, 0);

    // Unregister Data Flow
    EXPECT_TRUE(mHubInterface->unregisterDataFlowHostProducer(dataFlowId).isOk());

    // Free Region
    EXPECT_TRUE(mHubInterface->freeSharedDataRegion(region.id).isOk());
}

TEST_P(ContextHubEndpointAidlWithTestMode, TestHostProducerDataFlowInvalidRegion) {
    if (!registerDefaultHub()) GTEST_SKIP() << "Not implemented";
    if (!areDataFlowsSupported()) GTEST_SKIP() << "Data flows not supported";

    EndpointId hostEndpoint;
    hostEndpoint.hubId = kDefaultHubId;
    hostEndpoint.id = 0xCAFE;

    DataFlowInfo info;
    info.region.id = 99999;  // Invalid Region ID
    int efd = eventfd(0, 0);
    info.notificationFds.waking = ScopedFileDescriptor(dup(efd));
    info.notificationFds.nonWaking = ScopedFileDescriptor(dup(efd));
    close(efd);

    int32_t dataFlowId = -1;
    ScopedAStatus status =
            mHubInterface->registerDataFlowHostProducer(hostEndpoint, info, &dataFlowId);
    EXPECT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
}

// VtsEpollWaiter for NotificationManager
class VtsEpollWaiter : public NotificationManager::EpollWaiter {
  public:
    VtsEpollWaiter() { mEpollFd = epoll_create1(EPOLL_CLOEXEC); }
    ~VtsEpollWaiter() {
        if (mEpollFd >= 0) close(mEpollFd);
    }

    void addFd(int fd) override {
        ALOGD("VTS: Adding FD %d to epoll", fd);
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev);
    }

    void removeFd(int fd) override { epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr); }

    void waitAndDispatch(int timeoutMs) {
        epoll_event events[4];
        int nfds = epoll_wait(mEpollFd, events, 4, timeoutMs);
        for (int i = 0; i < nfds; i++) {
            handleNotification(events[i].data.fd, false);
        }
    }

  private:
    int mEpollFd = -1;
};

class ContextHubDataFlowEchoTest : public ContextHubEndpointAidlWithTestMode {
  public:
    void SetUp() override {
        ContextHubEndpointAidlWithTestMode::SetUp();
        mRegionManager = std::make_unique<RegionManager>();
        auto waiter = std::make_unique<VtsEpollWaiter>();
        mWaiterPtr = waiter.get();

        mNotificationManager = NotificationManager::create(
                std::move(waiter), [this](DataFlowId flowId, bool /*waking*/) {
                    std::lock_guard<std::mutex> lock(mEventMutex);
                    mReceivedEvents.push_back(flowId);
                });
    }

    void TearDown() override { ContextHubEndpointAidlWithTestMode::TearDown(); }

    std::unique_ptr<RegionManager> mRegionManager;
    std::shared_ptr<NotificationManager> mNotificationManager;
    VtsEpollWaiter* mWaiterPtr;

    std::mutex mEventMutex;
    std::vector<DataFlowId> mReceivedEvents;
};

TEST_P(ContextHubDataFlowEchoTest, TestDataFlowEchoVerifyContent) {
    if (!registerDefaultHub()) GTEST_SKIP() << "Not implemented";
    if (!areDataFlowsSupported()) GTEST_SKIP() << "Data flows not supported";

    std::vector<EndpointInfo> endpoints;
    mContextHub->getEndpoints(&endpoints);
    if (endpoints.empty()) {
        FAIL() << "No endpoints returned by HAL";
    }
    EndpointId halEndpointId = endpoints[0].id;

    // 1. Allocate shared data region and act as producer.
    SharedDataRegionRequirements reqs;
    reqs.sizeBytes = 16384;  // 16KB
    reqs.targetHubIds = {kDefaultHubId, halEndpointId.hubId};

    SharedDataRegion regionInfo;
    auto status = mHubInterface->allocateSharedDataRegion(reqs, &regionInfo);
    ASSERT_TRUE(status.isOk()) << "Allocation failed: " << status.getDescription();

    // Save region ID for later usage.
    auto regionId = regionInfo.id;

    // 2. Map Region
    pw::Result<AllocatorRegion> hostProdRegionRes =
            mRegionManager->mapHostProducerRegion(std::move(regionInfo));
    ASSERT_TRUE(hostProdRegionRes.ok())
            << "mapHostProducerRegion failed: " << hostProdRegionRes.status().str();
    AllocatorRegion& hostRegion = hostProdRegionRes.value();

    // 3. Initialize Queue
    DataNotifier dataNotifier;

    constexpr size_t kQueueBlockCapacity = 1024;
    pw::Result<void*> queueRes =
            createQueue<uint8_t, kQueueBlockCapacity>(*hostRegion.allocator, /*local=*/false);

    ASSERT_TRUE(queueRes.ok()) << "Queue creation failed with status: "
                               << static_cast<int>(queueRes.status().code());
    void* queuePtr = queueRes.value();

    // Calculate offset for HAL
    size_t queueOffset = reinterpret_cast<uintptr_t>(queuePtr) - hostRegion.base;
    ALOGD("VTS: Queue allocated at offset: %zu", queueOffset);

    // 4. Create Producer
    auto producerRes = Producer<uint8_t>::createRemote(hostRegion, queuePtr,
                                                       16,  // max blocks
                                                       1,   // min blocks
                                                       dataNotifier,
                                                       RemoteNotifyArgs{[](pw::ConstByteSpan) {}});
    ASSERT_TRUE(producerRes.ok()) << "Producer createRemote failed with status: "
                                  << producerRes.status().str();
    std::optional<Producer<uint8_t>> producerOpt;
    producerOpt.emplace(std::move(producerRes.value()));

    // 5. Setup Notifications
    auto prepRes = mNotificationManager->prepareHostProducerDataFlowInfo();
    ASSERT_TRUE(prepRes.ok());
    auto [dfInfo, notifyHandle] = std::move(prepRes.value());
    // Only set region ID and leave other fields null according to the API description.
    dfInfo.region.id = regionId;
    dfInfo.metadataOffsetBytes = queueOffset;

    // 6. Register Producer
    EndpointId hostEndpoint;
    hostEndpoint.hubId = kDefaultHubId;
    hostEndpoint.id = 0x1234;
    int32_t flowIdVal = -1;
    ASSERT_TRUE(
            mHubInterface->registerDataFlowHostProducer(hostEndpoint, dfInfo, &flowIdVal).isOk());
    ALOGD("VTS: Host Producer Registered (FlowID=%d)", flowIdVal);

    ASSERT_TRUE(mNotificationManager->activateHostProducerDataFlow(flowIdVal, notifyHandle).ok());
    ASSERT_TRUE(mRegionManager->linkHostProducerDataFlowToRegion(regionId, flowIdVal).ok());
    ALOGD("VTS: Host Producer Activated and Linked");

    // 7. Register Consumer (HAL)
    ALOGD("VTS: Attempting to add consumer");
    ConsumerPolicyBuilder policy;
    policy.setStreaming();

    const char* kConsumerName = "HalEchoConsumer";
    pw::ConstByteSpan nameSpan(reinterpret_cast<const std::byte*>(kConsumerName), 15);
    pw::Result<uint32_t> consDescOffsetRes =
            producerOpt->getConsumerManager().addConsumer(nameSpan, policy, &hostRegion);
    ASSERT_TRUE(consDescOffsetRes.ok()) << "addConsumer failed with status: "
                                        << static_cast<int>(consDescOffsetRes.status().code());
    ALOGD("VTS: Consumer Descriptor Added at offset: %u", consDescOffsetRes.value());

    pw::Result<DataFlowConsumerHandle> halConsHandleRes =
            mNotificationManager->addOffloadConsumerAndCreateHandle(flowIdVal, halEndpointId);
    ASSERT_TRUE(halConsHandleRes.ok());
    halConsHandleRes.value().id.hubId = kDefaultHubId;
    halConsHandleRes.value().id.id = flowIdVal;
    halConsHandleRes.value().metadataOffsetBytes = consDescOffsetRes.value();

    mEndpointCb->resetWasOnDataFlowHostConsumerRegisteredCalled();
    ASSERT_TRUE(mHubInterface
                        ->registerDataFlowOffloadConsumer(std::move(halConsHandleRes).value(),
                                                          halEndpointId, nullptr, std::nullopt, -1)
                        .isOk());

    // 8. Wait for Echo Setup
    ALOGD("VTS: Waiting for Echo Callback...");
    {
        std::unique_lock<std::mutex> lock(mEndpointCb->getMutex());
        bool signaled = mEndpointCb->getCondVar().wait_for(lock, std::chrono::seconds(5), [&] {
            return mEndpointCb->wasOnDataFlowHostConsumerRegisteredCalled();
        });
        ASSERT_TRUE(signaled) << "Timeout waiting for HAL to register echo consumer";
    }
    ALOGD("VTS: Received Echo Callback");

    // Setup consumer
    const DataFlowConsumerHandle& echoHandle = mEndpointCb->getDataFlowHandle();
    ASSERT_TRUE(echoHandle.info.has_value());

    ALOGD("VTS: FDs Check - ConsWake: %d, ConsNonWake: %d, Ack: %d, ProdWake: %d, ProdNonWake: %d",
          echoHandle.notificationFds.waking.get(), echoHandle.notificationFds.nonWaking.get(),
          echoHandle.notificationFds.halAck.get(), echoHandle.info->notificationFds.waking.get(),
          echoHandle.info->notificationFds.nonWaking.get());

    RegionManager::RegionToMap regionToMap;
    regionToMap.id = echoHandle.info->region.id;
    regionToMap.size = echoHandle.info->region.sizeBytes;
    regionToMap.fd = ScopedFileDescriptor(dup(echoHandle.info->region.sharedMemory.get()));

    auto mapConsRes = mRegionManager->mapHostConsumerRegions(std::move(regionToMap), std::nullopt,
                                                             echoHandle.id);
    ASSERT_TRUE(mapConsRes.ok());
    auto [echoRegion, _] = std::move(mapConsRes.value());

    // Enables host consumer on the echo data flow.
    ASSERT_TRUE(mNotificationManager->enableHostConsumerFromHandle(echoHandle).ok());

    auto consumerRes = Consumer<uint8_t>::createRemote(
            echoRegion, std::nullopt, echoHandle.info->metadataOffsetBytes,
            echoHandle.metadataOffsetBytes, RemoteNotifyArgs{[](pw::ConstByteSpan) {}});
    ASSERT_TRUE(consumerRes.ok()) << "failed to create remote consumer: "
                                  << consumerRes.status().str();
    std::optional<Consumer<uint8_t>> consumerOpt;
    consumerOpt.emplace(std::move(consumerRes.value()));

    // 9. Verify Echo
    uint8_t testVal = 0x42;
    producerOpt->push(testVal);
    mNotificationManager->notifyOffloadConsumer(halEndpointId, true);

    bool received = false;
    mWaiterPtr->waitAndDispatch(5000);
    std::lock_guard<std::mutex> lock(mEventMutex);
    for (auto& ev : mReceivedEvents) {
        ALOGD("VTS: Received Event for Flow ID: %d", ev.id);
        if (ev.id == echoHandle.id.id) {
            received = true;
            break;
        }
    }
    ASSERT_TRUE(received) << "Did not receive echo notification";

    auto popRes = consumerOpt->pop();
    ASSERT_TRUE(popRes.ok());
    EXPECT_EQ(popRes.value(), testVal);

    // Cleanup
    consumerOpt->disable();
    // Reset the std::optional to explicitly deconstruct consumer and producer.
    // This should happen before the queue deallocation, or else if will have segmentation fault.
    consumerOpt.reset();
    producerOpt.reset();
    if (hostRegion.allocator) {
        hostRegion.allocator->Deallocate(queuePtr, queueLayout());
        ALOGD("VTS: Queue memory deallocated successfully");
    }
    mNotificationManager->disableHostConsumer(echoHandle.id);
    mRegionManager->unlinkHostConsumerDataFlow(echoHandle.id);
    mHubInterface->unregisterDataFlowHostConsumer(hostEndpoint, echoHandle.id);
    mNotificationManager->removeHostProducerDataFlow(flowIdVal);
    mHubInterface->unregisterDataFlowHostProducer(flowIdVal);
    mRegionManager->unmapHostProducerRegion(regionId);
    mHubInterface->freeSharedDataRegion(regionId);
}

std::string PrintGeneratedTest(const testing::TestParamInfo<ContextHubAidl::ParamType>& info) {
    return std::string("CONTEXT_HUB_ID_") + std::to_string(std::get<1>(info.param));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContextHubAidl);
INSTANTIATE_TEST_SUITE_P(ContextHub, ContextHubAidl, testing::ValuesIn(generateContextHubMapping()),
                         PrintGeneratedTest);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContextHubEndpointAidl);
INSTANTIATE_TEST_SUITE_P(
        ContextHub, ContextHubEndpointAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IContextHub::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContextHubEndpointAidlWithTestMode);
INSTANTIATE_TEST_SUITE_P(
        ContextHub, ContextHubEndpointAidlWithTestMode,
        testing::ValuesIn(android::getAidlHalInstanceNames(IContextHub::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContextHubDataFlowEchoTest);
INSTANTIATE_TEST_SUITE_P(
        ContextHub, ContextHubDataFlowEchoTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IContextHub::descriptor)),
        android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ContextHubTransactionTest);
INSTANTIATE_TEST_SUITE_P(ContextHub, ContextHubTransactionTest,
                         testing::ValuesIn(generateContextHubMapping()), PrintGeneratedTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(2);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
