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

#include "contexthub-impl/ContextHub.h"
#include <cstddef>
#include "aidl/android/hardware/contexthub/IContextHubCallback.h"

#ifndef LOG_TAG
#define LOG_TAG "CHRE"
#endif

#include <cutils/ashmem.h>
#include <inttypes.h>
#include <log/log.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <optional>
#include <queue>
#include <thread>

#include "data_flow/host/notification_manager.h"
#include "data_flow/host/region_manager.h"
#include "data_flow/queue.h"

namespace aidl::android::hardware::contexthub {

namespace {

using ::android::contexthub::data_flow::AllocatorRegion;
using ::android::contexthub::data_flow::Consumer;
using ::android::contexthub::data_flow::ConsumerPolicyBuilder;
using ::android::contexthub::data_flow::DataNotifier;
using ::android::contexthub::data_flow::NotificationManager;
using ::android::contexthub::data_flow::Producer;
using ::android::contexthub::data_flow::queueLayout;
using ::android::contexthub::data_flow::RegionManager;
using ::android::contexthub::data_flow::RemoteNotifyArgs;
using ::android::contexthub::data_flow::internal::ProducerBase;
using ::ndk::ScopedAStatus;
using ::ndk::ScopedFileDescriptor;

constexpr uint64_t kMockVendorHubId = 0x1234567812345678;
constexpr uint64_t kMockVendorHub2Id = 0x0EADBEEFDEADBEEF;

// Mock endpoints for the default implementation.
// These endpoints just echo back any messages sent to them.
constexpr size_t kMockEndpointCount = 4;
const EndpointInfo kMockEndpointInfos[kMockEndpointCount] = {
        {
                .id = {.hubId = kMockVendorHubId, .id = UINT64_C(0x1)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 1",
                .version = 1,
        },
        {
                .id = {.hubId = kMockVendorHubId, .id = UINT64_C(0x2)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 2",
                .version = 2,
        },
        {
                .id = {.hubId = kMockVendorHub2Id, .id = UINT64_C(0x1)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 3",
                .version = 1,
        },
        {
                .id = {.hubId = kMockVendorHub2Id, .id = UINT64_C(0x2)},
                .type = EndpointInfo::EndpointType::GENERIC,
                .name = "Mock Endpoint 4",
                .version = 2,
        },
};

//! Mutex used to ensure callbacks are called after the initial function returns.
std::mutex gCallbackMutex;

}  // anonymous namespace

ScopedAStatus ContextHub::getContextHubs(std::vector<ContextHubInfo>* /* out_contextHubInfos */) {
    return ScopedAStatus::ok();
}

// We don't expose any nanoapps for the default impl, therefore all nanoapp-related APIs fail.
ScopedAStatus ContextHub::loadNanoapp(int32_t /* in_contextHubId */,
                                      const NanoappBinary& /* in_appBinary */,
                                      int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::unloadNanoapp(int32_t /* in_contextHubId */, int64_t /* in_appId */,
                                        int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::disableNanoapp(int32_t /* in_contextHubId */, int64_t /* in_appId */,
                                         int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::enableNanoapp(int32_t /* in_contextHubId */, int64_t /* in_appId */,
                                        int32_t /* in_transactionId */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::onSettingChanged(Setting /* in_setting */, bool /*in_enabled */) {
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::queryNanoapps(int32_t in_contextHubId) {
    if (in_contextHubId == kMockHubId && mCallback != nullptr) {
        std::vector<NanoappInfo> nanoapps;
        mCallback->handleNanoappInfo(nanoapps);
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ScopedAStatus ContextHub::getPreloadedNanoappIds(int32_t /* in_contextHubId */,
                                                 std::vector<int64_t>* out_preloadedNanoappIds) {
    if (out_preloadedNanoappIds == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    for (uint64_t i = 0; i < 10; ++i) {
        out_preloadedNanoappIds->push_back(i);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::onNanSessionStateChanged(const NanSessionStateUpdate& /*in_update*/) {
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::registerCallback(int32_t in_contextHubId,
                                           const std::shared_ptr<IContextHubCallback>& in_cb) {
    if (in_contextHubId == kMockHubId) {
        mCallback = in_cb;
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ScopedAStatus ContextHub::sendMessageToHub(int32_t in_contextHubId,
                                           const ContextHubMessage& /* in_message */) {
    if (in_contextHubId == kMockHubId) {
        // Return true here to indicate that the HAL has accepted the message.
        // Successful delivery of the message to a nanoapp should be handled at
        // a higher level protocol.
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
}

ScopedAStatus ContextHub::setTestMode(bool enable) {
    if (enable) {
        std::lock_guard lock(mHostHubsLock);
        // Remove all hubs except the Context Hub Service hub to allow Java endpoints while testing.
        constexpr uint64_t kServiceHubId = 0x416e64726f696400L;
        std::vector<uint64_t> hubsIdsToRemoveList;
        for (auto& [id, hub] : mIdToHostHub) {
            if (id != kServiceHubId) {
                hubsIdsToRemoveList.push_back(id);
                hub->mActive = false;
            }
        }
        for (auto id : hubsIdsToRemoveList) {
            mIdToHostHub.erase(id);
        }
    }
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::onHostEndpointConnected(const HostEndpointInfo& in_info) {
    mConnectedHostEndpoints.insert(in_info.hostEndpointId);

    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::onHostEndpointDisconnected(char16_t in_hostEndpointId) {
    if (mConnectedHostEndpoints.count(in_hostEndpointId) > 0) {
        mConnectedHostEndpoints.erase(in_hostEndpointId);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::sendMessageDeliveryStatusToHub(
        int32_t /* in_contextHubId */,
        const MessageDeliveryStatus& /* in_messageDeliveryStatus */) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ScopedAStatus ContextHub::getHubs(std::vector<HubInfo>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    VendorHubInfo vendorHub = {};
    vendorHub.name = "Mock Vendor Hub";
    vendorHub.version = 42;

    HubInfo hubInfo1 = {};
    hubInfo1.hubId = kMockVendorHubId;
    hubInfo1.hubDetails =
            HubInfo::HubDetails::make<HubInfo::HubDetails::Tag::vendorHubInfo>(vendorHub);

    VendorHubInfo vendorHub2 = {};
    vendorHub2.name = "Mock Vendor Hub 2";
    vendorHub2.version = 24;

    HubInfo hubInfo2 = {};
    hubInfo2.hubId = kMockVendorHub2Id;
    hubInfo2.hubDetails =
            HubInfo::HubDetails::make<HubInfo::HubDetails::Tag::vendorHubInfo>(vendorHub2);

    _aidl_return->push_back(hubInfo1);
    _aidl_return->push_back(hubInfo2);

    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::getEndpoints(std::vector<EndpointInfo>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    Service echoService;
    echoService.format = Service::RpcFormat::CUSTOM;
    echoService.serviceDescriptor = "android.hardware.contexthub.test.EchoService";
    echoService.majorVersion = 1;
    echoService.minorVersion = 0;

    for (const EndpointInfo& endpoint : kMockEndpointInfos) {
        EndpointInfo endpointWithService(endpoint);
        endpointWithService.services.push_back(echoService);
        _aidl_return->push_back(std::move(endpointWithService));
    }

    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::registerEndpointHub(
        const std::shared_ptr<IEndpointCallback>& in_callback, const HubInfo& in_hubInfo,
        std::shared_ptr<IEndpointCommunication>* _aidl_return) {
    std::lock_guard lock(mHostHubsLock);
    if (mIdToHostHub.count(in_hubInfo.hubId)) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    auto hub = ndk::SharedRefBase::make<HubInterface>(*this, in_callback, in_hubInfo);
    mIdToHostHub.insert({in_hubInfo.hubId, hub});
    *_aidl_return = std::move(hub);
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::HubInterface::registerEndpoint(const EndpointInfo& in_endpoint) {
    if (!mActive) {
        ALOGE("registerEndpoint failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (in_endpoint.id.hubId != kInfo.hubId) {
        ALOGE("registerEndpoint failed: hub ID mismatch: %" PRIu64 "vs expected %" PRIu64,
              in_endpoint.id.hubId, kInfo.hubId);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::unique_lock<std::mutex> lock(mEndpointMutex);

    for (const EndpointInfo& endpoint : mEndpoints) {
        if (endpoint.id.id == in_endpoint.id.id || endpoint.name == in_endpoint.name) {
            ALOGE("registerEndpoint failed: endpoint already exists (id= %" PRIu64 " name=%s)",
                  in_endpoint.id.id, in_endpoint.name.c_str());
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }
    mEndpoints.push_back(in_endpoint);
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::HubInterface::unregisterEndpoint(const EndpointInfo& in_endpoint) {
    if (!mActive) {
        ALOGE("unregisterEndpoint failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::unique_lock<std::mutex> lock(mEndpointMutex);

    for (auto it = mEndpoints.begin(); it != mEndpoints.end(); ++it) {
        if (it->id.id == in_endpoint.id.id && it->id.hubId == in_endpoint.id.hubId) {
            mEndpoints.erase(it);
            return ScopedAStatus::ok();
        }
    }
    ALOGE("unregisterEndpoint failed: endpoint not found (id= %" PRIu64 " name=%s)",
          in_endpoint.id.id, in_endpoint.name.c_str());
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
};

ScopedAStatus ContextHub::HubInterface::requestSessionIdRange(
        int32_t in_size, std::array<int32_t, 2>* _aidl_return) {
    if (!mActive) {
        ALOGE("requestSessionIdRange failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    constexpr int32_t kMaxSize = 1024;
    if (in_size > kMaxSize || _aidl_return == nullptr) {
        ALOGE("requestSessionIdRange failed: invalid arguments: size=%" PRId32, in_size);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    uint16_t base = 0;
    {
        std::lock_guard lock(mHal.mHostHubsLock);
        if (static_cast<int32_t>(USHRT_MAX) - mHal.mNextSessionIdBase + 1 < in_size) {
            return ScopedAStatus::fromServiceSpecificError(EX_CONTEXT_HUB_UNSPECIFIED);
        }
        base = mHal.mNextSessionIdBase;
        mHal.mNextSessionIdBase += in_size;
    }

    {
        std::lock_guard<std::mutex> lock(mEndpointMutex);
        (*_aidl_return)[0] = mBaseSessionId = base;
        (*_aidl_return)[1] = mMaxSessionId = base + (in_size - 1);
    }
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::HubInterface::openEndpointSession(
        int32_t in_sessionId, const EndpointId& in_destination, const EndpointId& in_initiator,
        const std::optional<std::string>& in_serviceDescriptor) {
    if (!mActive) {
        ALOGE("openEndpointSession failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    // We are not calling onCloseEndpointSession on failure because the remote endpoints (our
    // mock endpoints) always accept the session.

    std::weak_ptr<IEndpointCallback> callback;
    {
        std::unique_lock<std::mutex> lock(mEndpointMutex);
        if (in_sessionId < mBaseSessionId || in_sessionId > mMaxSessionId) {
            ALOGE("openEndpointSession: session ID %" PRId32 " is invalid", in_sessionId);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        for (const EndpointSession& session : mEndpointSessions) {
            bool sessionAlreadyExists =
                    (session.initiator == in_destination && session.peer == in_initiator) ||
                    (session.peer == in_destination && session.initiator == in_initiator);
            if (sessionAlreadyExists) {
                ALOGD("openEndpointSession: session ID %" PRId32 " already exists", in_sessionId);
                return (session.sessionId == in_sessionId &&
                        session.serviceDescriptor == in_serviceDescriptor)
                               ? ScopedAStatus::ok()
                               : ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
            } else if (session.sessionId == in_sessionId) {
                ALOGE("openEndpointSession: session ID %" PRId32 " is invalid: endpoint mismatch",
                      in_sessionId);
                return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
            }
        }

        // Verify the initiator and destination are valid endpoints
        bool initiatorIsValid = findEndpoint(in_initiator, mEndpoints.begin(), mEndpoints.end());
        if (!initiatorIsValid) {
            ALOGE("openEndpointSession: initiator %" PRIu64 ":%" PRIu64 " is invalid",
                  in_initiator.id, in_initiator.hubId);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        bool destinationIsValid = findEndpoint(in_destination, &kMockEndpointInfos[0],
                                               &kMockEndpointInfos[kMockEndpointCount]);
        if (!destinationIsValid) {
            ALOGE("openEndpointSession: destination %" PRIu64 ":%" PRIu64 " is invalid",
                  in_destination.id, in_destination.hubId);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        mEndpointSessions.push_back({
                .sessionId = in_sessionId,
                .initiator = in_initiator,
                .peer = in_destination,
                .serviceDescriptor = in_serviceDescriptor,
        });

        if (mEndpointCallback == nullptr) {
            return ScopedAStatus::ok();
        }
        callback = mEndpointCallback;
    }

    std::unique_lock<std::mutex> lock(gCallbackMutex);
    std::thread{[callback, in_sessionId]() {
        std::unique_lock<std::mutex> lock(gCallbackMutex);
        if (auto cb = callback.lock(); cb != nullptr) {
            cb->onEndpointSessionOpenComplete(in_sessionId);
        }
    }}.detach();
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::HubInterface::sendMessageToEndpoint(int32_t in_sessionId,
                                                              const Message& in_msg) {
    if (!mActive) {
        ALOGE("sendMessageToEndpoint failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::weak_ptr<IEndpointCallback> callback;
    {
        std::unique_lock<std::mutex> lock(mEndpointMutex);
        bool foundSession = false;
        for (const EndpointSession& session : mEndpointSessions) {
            if (session.sessionId == in_sessionId) {
                foundSession = true;
                break;
            }
        }

        if (!foundSession) {
            ALOGE("sendMessageToEndpoint: session ID %" PRId32 " is invalid", in_sessionId);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        if (mEndpointCallback == nullptr) {
            return ScopedAStatus::ok();
        }
        callback = mEndpointCallback;
    }

    std::unique_lock<std::mutex> lock(gCallbackMutex);
    if ((in_msg.flags & Message::FLAG_REQUIRES_DELIVERY_STATUS) != 0) {
        MessageDeliveryStatus msgStatus = {};
        msgStatus.messageSequenceNumber = in_msg.sequenceNumber;
        msgStatus.errorCode = ErrorCode::OK;

        std::thread{[callback, in_sessionId, msgStatus]() {
            std::unique_lock<std::mutex> lock(gCallbackMutex);
            if (auto cb = callback.lock(); cb != nullptr) {
                cb->onMessageDeliveryStatusReceived(in_sessionId, msgStatus);
            }
        }}.detach();
    }

    // Echo the message back
    std::thread{[callback, in_sessionId, in_msg]() {
        std::unique_lock<std::mutex> lock(gCallbackMutex);
        if (auto cb = callback.lock(); cb != nullptr) {
            cb->onMessageReceived(in_sessionId, in_msg);
        }
    }}.detach();
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::HubInterface::sendMessageDeliveryStatusToEndpoint(
        int32_t /* in_sessionId */, const MessageDeliveryStatus& /* in_msgStatus */) {
    if (!mActive) {
        ALOGE("sendMessageDeliveryStatusToEndpoint failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::HubInterface::closeEndpointSession(int32_t in_sessionId,
                                                             Reason /* in_reason */) {
    if (!mActive) {
        ALOGE("closeEndpointSession failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::unique_lock<std::mutex> lock(mEndpointMutex);

    for (auto it = mEndpointSessions.begin(); it != mEndpointSessions.end(); ++it) {
        if (it->sessionId == in_sessionId) {
            mEndpointSessions.erase(it);
            return ScopedAStatus::ok();
        }
    }
    ALOGE("closeEndpointSession: session ID %" PRId32 " is invalid", in_sessionId);
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
};

ScopedAStatus ContextHub::HubInterface::endpointSessionOpenComplete(int32_t /* in_sessionId */) {
    if (!mActive) {
        ALOGE("endpointSessionOpenComplete failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ScopedAStatus::ok();
};

ScopedAStatus ContextHub::HubInterface::unregister() {
    if (!mActive.exchange(false)) {
        ALOGE("unregister failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::lock_guard lock(mHal.mHostHubsLock);
    mHal.mIdToHostHub.erase(kInfo.hubId);
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::HubInterface::allocateSharedDataRegion(
        const SharedDataRegionRequirements& in_requirements, SharedDataRegion* _aidl_return) {
    if (!mActive) {
        ALOGE("allocateSharedDataRegion failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (in_requirements.sizeBytes <= 0 || !_aidl_return) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    int fd = ashmem_create_region("ContextHubSharedData", in_requirements.sizeBytes);
    if (fd < 0) {
        ALOGE("ashmem_create_region failed, size=%" PRId64, in_requirements.sizeBytes);
        return ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(
                IEndpointCommunication::SharedDataErrors::ERR_INSUFFICIENT_MEMORY));
    }

    int dupFd = ::dup(fd);
    if (dupFd < 0) {
        ALOGE("allocateSharedDataRegion dup FD failed");
        close(fd);
        return ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(
                IEndpointCommunication::SharedDataErrors::ERR_INSUFFICIENT_MEMORY));
    }

    int32_t regionId = mNextRegionId++;

    AllocatedRegion regionToStore;
    regionToStore.region.id = regionId;
    regionToStore.region.sharedMemory.set(fd);
    regionToStore.permissions = std::move(in_requirements.permissions);
    regionToStore.size = in_requirements.sizeBytes;
    regionToStore.targetHubIds = std::move(in_requirements.targetHubIds);

    {
        std::lock_guard lock(mSharedDataMutex);
        mAllocatedRegions[regionId] = std::move(regionToStore);
    }

    _aidl_return->id = regionId;
    _aidl_return->sizeBytes = in_requirements.sizeBytes;
    _aidl_return->sharedMemory.set(dupFd);
    if (!in_requirements.permissions.empty()) {
        _aidl_return->permissions.emplace(in_requirements.permissions.begin(),
                                          in_requirements.permissions.end());
    }

    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::HubInterface::freeSharedDataRegion(int32_t in_id) {
    if (!mActive) {
        ALOGE("freeSharedDataRegion failed: hub is not active");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    std::lock_guard lock(mSharedDataMutex);
    auto it = mAllocatedRegions.find(in_id);

    if (it == mAllocatedRegions.end()) {
        ALOGE("freeSharedDataRegion failed: region %" PRId32 " not found", in_id);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if (!it->second.publishedDataFlowIds.empty()) {
        ALOGE("freeSharedDataRegion failed: region %" PRId32 " is in use", in_id);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    mAllocatedRegions.erase(it);
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::HubInterface::registerDataFlowHostProducer(const EndpointId& in_endpoint,
                                                                     const DataFlowInfo& in_info,
                                                                     int32_t* _aidl_return) {
    if (!mActive) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (_aidl_return == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    std::lock_guard lock(mSharedDataMutex);

    // Verifies if region exists
    auto regionIt = mAllocatedRegions.find(in_info.region.id);
    if (regionIt == mAllocatedRegions.end()) {
        ALOGE("registerDataFlowHostProducer: region %d not found", in_info.region.id);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    int32_t dataFlowId = mNextDataFlowId++;

    // Stores data flow information
    DataFlow flow;
    flow.id = dataFlowId;
    flow.producerId = in_endpoint;
    flow.info.region.id = in_info.region.id;
    flow.info.metadataOffsetBytes = in_info.metadataOffsetBytes;
    if (in_info.notificationFds.waking.get() < 0) {
        ALOGE("registerDataFlowHostProducer: invalid notificationFds.waking");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    } else if (in_info.notificationFds.nonWaking.get() < 0) {
        ALOGE("registerDataFlowHostProducer: invalid notificationFds.nonWaking");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    auto maybeFds = ::android::contexthub::data_flow::dupEventFds(in_info.notificationFds,
                                                                  /*needsHalAck=*/true);
    if (!maybeFds.ok()) {
        ALOGE("registerDataFlowHostProducer: failed to dup notificationFds");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    flow.info.notificationFds = std::move(*maybeFds);

    mDataFlows[dataFlowId] = std::move(flow);

    // Marks region is used
    regionIt->second.publishedDataFlowIds.insert(dataFlowId);

    *_aidl_return = dataFlowId;
    ALOGD("Registered Host Producer DataFlow ID %d in Region %d", dataFlowId, in_info.region.id);
    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::HubInterface::unregisterDataFlowHostProducer(int32_t in_id) {
    if (!mActive) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    std::lock_guard lock(mSharedDataMutex);
    auto flowIt = mDataFlows.find(in_id);
    if (flowIt == mDataFlows.end()) {
        ALOGE("unregisterDataFlowHostProducer: data flow %d not found", in_id);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    // Removes from region data flow usage list
    int32_t regionId = flowIt->second.info.region.id;
    auto regionIt = mAllocatedRegions.find(regionId);
    if (regionIt != mAllocatedRegions.end()) {
        regionIt->second.publishedDataFlowIds.erase(in_id);
    }

    // Removes data flow
    mDataFlows.erase(flowIt);
    ALOGD("Unregistered Host Producer DataFlow ID %d", in_id);

    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::HubInterface::registerDataFlowOffloadConsumer(
        const DataFlowConsumerHandle& in_handle, const EndpointId& in_consumerId,
        const std::shared_ptr<IEndpointCommunication::IRegisterOffloadConsumerCallback>&
                in_callback,
        const std::optional<Message>& /*in_msg*/, int32_t /*in_sessionId*/) {
    if (!mActive) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    // Checks consumer (mock endpoint) validity
    bool consumerIsValid = findEndpoint(in_consumerId, &kMockEndpointInfos[0],
                                        &kMockEndpointInfos[kMockEndpointCount]);
    if (!consumerIsValid) {
        ALOGE("registerDataFlowOffloadConsumer: consumer endpoint invalid");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    EndpointId hostProducerId;
    {
        std::lock_guard lock(mSharedDataMutex);
        // Checks if data flow exists
        auto flowIt = mDataFlows.find(in_handle.id.id);
        if (flowIt == mDataFlows.end()) {
            ALOGE("registerDataFlowOffloadConsumer: data flow %d not found", in_handle.id.id);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        // Checks if consumer Hub is in the region of targetHubIds.
        auto regionIt = mAllocatedRegions.find(flowIt->second.info.region.id);
        if (regionIt == mAllocatedRegions.end()) {
            ALOGE("registerDataFlowOffloadConsumer: region %d not found for data flow %d",
                  flowIt->second.info.region.id, in_handle.id.id);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }

        bool isHubTargeted = false;
        for (int64_t targetHubId : regionIt->second.targetHubIds) {
            if (targetHubId == in_consumerId.hubId) {
                isHubTargeted = true;
                break;
            }
        }

        if (!isHubTargeted) {
            ALOGE("registerDataFlowOffloadConsumer: consumer hub %" PRIu64
                  " is not a target for region %d",
                  in_consumerId.hubId, flowIt->second.info.region.id);
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        flowIt->second.offloadConsumerIds.insert(in_consumerId.id);
        hostProducerId = flowIt->second.producerId;

        // Prepares metadata region if exists
        std::optional<SharedDataRegion> metadataRegion;
        constexpr int32_t kEchoMetadataRegionSize = 16384;
        int outputFd = ashmem_create_region("ContextHubEchoMetadata", kEchoMetadataRegionSize);
        if (outputFd < 0) {
            ALOGE("Echo: ashmem failed");
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
            ;
        }

        int32_t outputSharedRegionId = mNextRegionId++;  // Allocate a new region ID
        metadataRegion.emplace();
        metadataRegion->id = outputSharedRegionId;
        metadataRegion->sizeBytes = kEchoMetadataRegionSize;
        metadataRegion->sharedMemory.set(outputFd);  // Take outputFd's ownership
        metadataRegion->permissions = {};
        int64_t consumerMetadataOffset;
        auto status = in_callback->addConsumerInRegion(metadataRegion, &consumerMetadataOffset);
        if (!status.isOk()) {
            ALOGE("registerDataFlowOffloadConsumer: callback addConsumerInRegion failed");
            return status;
        }

        createEchoDataFlow(in_handle, consumerMetadataOffset, in_consumerId, hostProducerId,
                           regionIt->second, flowIt->second, metadataRegion);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus ContextHub::HubInterface::unregisterDataFlowHostConsumer(
        const EndpointId& in_consumerId, const DataFlowId& in_dataFlowId) {
    if (!mActive) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    std::lock_guard lock(mEchoDataFlowMutex);
    auto it = mEchoDataFlows.find(in_dataFlowId.id);
    if (it == mEchoDataFlows.end()) {
        ALOGE("unregisterDataFlowHostConsumer: data flow %d not found", in_dataFlowId.id);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    // Verifies if Customer ID matches
    if (it->second.hostConsumerId.id != in_consumerId.id) {
        ALOGE("unregisterDataFlowHostConsumer: consumer ID mismatch (expected %" PRIu64
              ", got %" PRIu64 ")",
              it->second.hostConsumerId.id, in_consumerId.id);
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    // Writes stop signal
    uint64_t stopVal = 1;
    write(it->second.stopFd, &stopVal, sizeof(stopVal));

    // Remove echo data flow record.
    mEchoDataFlows.erase(it);
    ALOGD("Unregistered Host Consumer %" PRIu64 " from DataFlow %d", in_consumerId.id,
          in_dataFlowId.id);

    return ScopedAStatus::ok();
}

// EchoEpollWaiter for NotificationManager
class EchoEpollWaiter : public NotificationManager::EpollWaiter {
  public:
    EchoEpollWaiter() { mEpollFd = epoll_create1(EPOLL_CLOEXEC); }
    ~EchoEpollWaiter() {
        if (mEpollFd >= 0) close(mEpollFd);
    }

    void addFd(int fd) override {
        ALOGD("Echo: Adding FD %d to epoll", fd);
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &ev);
    }

    void addStopFd(int fd) {
        ALOGD("Echo: Adding stop FD %d to epoll", fd);
        mStopFd = fd;
        addFd(fd);
    }

    void removeFd(int fd) override { epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr); }

    void waitAndDispatch() {
        epoll_event events[4];
        int nfds = epoll_wait(mEpollFd, events, 4, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == mStopFd) {
                mStopped = true;
                break;
            }
            handleNotification(events[i].data.fd, false);
        }
    }

    bool isStopped() { return mStopped; }

  private:
    int mEpollFd = -1;
    int mStopFd = -1;
    std::atomic<bool> mStopped{false};
};

void ContextHub::HubInterface::createEchoDataFlow(
        const DataFlowConsumerHandle& in_handle, int64_t consumerMetadataOffset,
        const EndpointId& in_consumerId, const EndpointId& hostProducerId,
        const AllocatedRegion& allocatedRegion, const DataFlow& dataFlow,
        const std::optional<SharedDataRegion>& metadataRegion) {
    // ========================================================================
    // Setup notification manager
    // ========================================================================
    auto waiter = std::make_unique<EchoEpollWaiter>();
    auto waiterPtr = waiter.get();
    auto receivedEvents = std::make_unique<std::queue<DataFlowId>>();
    auto notificationManager = NotificationManager::create(
            std::move(waiter), [](DataFlowId /*flowId*/, bool /*waking*/) {});

    // ========================================================================
    // Setup consumer (host -> HAL)
    // ========================================================================
    RegionManager::RegionToMap regionToMap;
    regionToMap.id = dataFlow.info.region.id;
    regionToMap.size = allocatedRegion.size;
    regionToMap.fd = ScopedFileDescriptor(dup(allocatedRegion.region.sharedMemory.get()));

    std::optional<RegionManager::RegionToMap> metadataRegionToMap;
    if (metadataRegion.has_value()) {
        metadataRegionToMap.emplace();
        metadataRegionToMap->id = metadataRegion->id;
        metadataRegionToMap->size = metadataRegion->sizeBytes;
        metadataRegionToMap->fd = ScopedFileDescriptor(dup(metadataRegion->sharedMemory.get()));
    }

    auto mapConsRes = mRegionManager.mapHostConsumerRegions(
            std::move(regionToMap), std::move(metadataRegionToMap), in_handle.id);
    if (!mapConsRes.ok()) {
        ALOGE("Echo: mapHostConsumerRegions failed, status: %s", mapConsRes.status().str());
        return;
    }
    auto [echoRegion, echoMetadataRegion] = std::move(mapConsRes.value());

    // Enables host consumer on the echo data flow.
    auto consumerHandleId = in_handle.id;

    DataFlowNotificationFds notifyHostFds, notifyOffloadFds;
    notifyOffloadFds.waking = ScopedFileDescriptor(dup(dataFlow.info.notificationFds.waking.get()));
    notifyOffloadFds.nonWaking =
            ScopedFileDescriptor(dup(dataFlow.info.notificationFds.nonWaking.get()));
    notifyOffloadFds.halAck = ScopedFileDescriptor(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));

    notifyHostFds.waking = ScopedFileDescriptor(dup(in_handle.notificationFds.waking.get()));
    notifyHostFds.nonWaking = ScopedFileDescriptor(dup(in_handle.notificationFds.nonWaking.get()));
    notifyHostFds.halAck = ScopedFileDescriptor(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));

    auto status = notificationManager->enableHostConsumerFromEventFds(
            consumerHandleId, std::move(notifyHostFds), std::move(notifyOffloadFds));
    if (!status.ok()) {
        ALOGE("Echo: enableHostConsumerFromHandle failed, status: %s", status.str());
        return;
    }

    auto consumerRes = Consumer<uint8_t>::createRemote(
            echoRegion, echoMetadataRegion, dataFlow.info.metadataOffsetBytes,
            consumerMetadataOffset, RemoteNotifyArgs{[](pw::ConstByteSpan) {}});
    if (!consumerRes.ok()) {
        ALOGE("Echo: Consumer<uint8_t>::createRemote failed, status: %s",
              consumerRes.status().str());
        return;
    }
    std::optional<Consumer<uint8_t>> consumerOpt;
    consumerOpt.emplace(std::move(consumerRes.value()));

    // ========================================================================
    // Setup producer (HAL -> Host)
    // ========================================================================

    // Init ashmem and region.
    constexpr int32_t kEchoRegionSize = 16384;
    int outputFd = ashmem_create_region("ContextHubEchoSharedData", kEchoRegionSize);
    if (outputFd < 0) {
        ALOGE("Echo: ashmem failed");
        return;
    }

    // Dup the region FD used to fill in the dfInfo afterwards.
    int dupRegionFd = dup(outputFd);
    if (dupRegionFd < 0) {
        ALOGE("Echo: failed to dup ashmem fd for VTS");
        close(outputFd);
        return;
    }

    int32_t outputSharedRegionId = mNextRegionId++;  // Allocate a new region ID
    SharedDataRegion outputSharedRegion;
    outputSharedRegion.id = outputSharedRegionId;
    outputSharedRegion.sizeBytes = kEchoRegionSize;
    outputSharedRegion.sharedMemory.set(outputFd);  // Take outputFd's ownership
    outputSharedRegion.permissions = {};

    // Map Region
    pw::Result<AllocatorRegion> hostProdRegionRes =
            mRegionManager.mapHostProducerRegion(std::move(outputSharedRegion));
    if (!hostProdRegionRes.ok()) {
        ALOGE("Echo: mapHostProducerRegion failed");
        return;
    }
    AllocatorRegion& hostRegion = hostProdRegionRes.value();

    // Create Producer
    DataNotifier dataNotifier;
    constexpr size_t kQueueBlockCapacity = 1024;
    auto producerRes = Producer<uint8_t>::createRemote(hostRegion, kQueueBlockCapacity,
                                                       16,  // max blocks
                                                       1,   // min blocks
                                                       dataNotifier,
                                                       RemoteNotifyArgs{[](pw::ConstByteSpan) {}});
    if (!producerRes.ok()) {
        ALOGE("Echo: Producer<uint8_t>::createRemote failed");
        return;
    }
    std::optional<Producer<uint8_t>> producerOpt;
    producerOpt.emplace(std::move(producerRes.value()));
    size_t queueOffset = producerOpt->getQueueOffset();
    ALOGD("Echo: Queue allocated at offset: %zu", queueOffset);

    // ========================================================================
    // Setup the consumer and send back to VTS
    // ========================================================================

    auto prepRes = notificationManager->prepareHostProducerDataFlowInfo();
    if (!prepRes.ok()) {
        ALOGE("Echo: prepareHostProducerDataFlowInfo failed");
        return;
    }
    auto [dfInfo, notifyHandle] = std::move(prepRes.value());
    dfInfo.region.id = outputSharedRegionId;
    dfInfo.region.sizeBytes = kEchoRegionSize;
    dfInfo.region.sharedMemory.set(dupRegionFd);
    dfInfo.metadataOffsetBytes = queueOffset;

    // Register Producer
    int32_t flowIdVal = mNextDataFlowId++;
    ALOGD("Echo: Producer Registered (FlowID=%d)", flowIdVal);

    if (!notificationManager->activateHostProducerDataFlow(flowIdVal, notifyHandle).ok()) {
        ALOGE("Echo: activateHostProducerDataFlow failed");
        return;
    }
    if (!mRegionManager.linkHostProducerDataFlowToRegion(outputSharedRegionId, flowIdVal).ok()) {
        ALOGE("Echo: linkHostProducerDataFlowToRegion failed");
        return;
    }
    ALOGD("Echo: Host Producer Activated and Linked");

    // Register Consumer sending back to VTS
    ALOGD("Echo: Attempting to add consumer");
    ConsumerPolicyBuilder policy;
    policy.setStreaming();

    const char* kConsumerName = "VtsEchoConsumer";
    pw::ConstByteSpan nameSpan(reinterpret_cast<const std::byte*>(kConsumerName), 15);
    pw::Result<uint32_t> consDescOffsetRes =
            producerOpt->getConsumerManager().addConsumer(nameSpan, policy, &hostRegion);
    if (!consDescOffsetRes.ok()) {
        ALOGE("Echo: addConsumer failed");
        return;
    }
    ALOGD("Echo: Consumer Descriptor Added at offset: %u", consDescOffsetRes.value());

    pw::Result<DataFlowConsumerHandle> consumerHandleRes =
            notificationManager->addOffloadConsumerAndCreateHandle(flowIdVal, hostProducerId);
    if (!consumerHandleRes.ok()) {
        ALOGE("Echo: addOffloadConsumerAndCreateHandle failed");
        return;
    }
    consumerHandleRes.value().id.hubId = in_consumerId.hubId;
    consumerHandleRes.value().id.id = flowIdVal;
    consumerHandleRes.value().metadataOffsetBytes = consDescOffsetRes.value();
    consumerHandleRes.value().info = std::move(dfInfo);
    consumerHandleRes.value().notificationFds.halAck =
            ScopedFileDescriptor(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));

    // Callback to send the consumer DataFlowConsumerHandle back to VTS.
    if (mEndpointCallback != nullptr) {
        mEndpointCallback->onDataFlowHostConsumerRegistered(
                std::move(consumerHandleRes).value(), in_consumerId, hostProducerId, std::nullopt,
                IEndpointCommunication::SESSION_ID_INVALID);
        ALOGD("Echo: called onDataFlowHostConsumerRegistered callback function");
    }

    // Register flow locally
    int stopFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (stopFd < 0) {
        ALOGE("Echo: Failed to create stopFd");
        return;
    }
    {
        std::lock_guard lock(mEchoDataFlowMutex);
        EchoDataFlow echoFlow;
        echoFlow.id = flowIdVal;
        echoFlow.offloadProducerId = in_consumerId;
        echoFlow.hostConsumerId = hostProducerId;
        echoFlow.stopFd = stopFd;
        mEchoDataFlows[flowIdVal] = echoFlow;
    }

    // ========================================================================
    // Starts thread for echo data flow
    // ========================================================================

    ALOGE("Echo: starting echo thread");
    std::shared_ptr<HubInterface> lifebuoy = ref<HubInterface>();
    std::thread([this, lifebuoy, producerOpt = std::move(producerOpt),
                 consumerOpt = std::move(consumerOpt), notificationManager, waiterPtr,
                 hostProducerId, flowIdVal, consumerHandleId, outputSharedRegionId]() mutable {
        while (!waiterPtr->isStopped()) {
            waiterPtr->waitAndDispatch();

            if (waiterPtr->isStopped()) {
                break;
            }

            while (true) {
                auto popRes = consumerOpt->pop();
                if (!popRes.ok()) {
                    break;
                }
                auto value = popRes.value();
                producerOpt->push(value);
                notificationManager->notifyOffloadConsumer(hostProducerId, true);
            }
        }

        // Clean up
        consumerOpt->disable();
        // Reset the std::optional to explicitly deconstruct consumer and producer.
        // This should happen before the queue deallocation, or else if will have segmentation
        // fault.
        consumerOpt.reset();
        producerOpt.reset();
        notificationManager->disableHostConsumer(consumerHandleId);
        mRegionManager.unlinkHostConsumerDataFlow(consumerHandleId);
        notificationManager->removeHostProducerDataFlow(flowIdVal);
        mRegionManager.unmapHostProducerRegion(outputSharedRegionId);
    }).detach();
}

}  // namespace aidl::android::hardware::contexthub
