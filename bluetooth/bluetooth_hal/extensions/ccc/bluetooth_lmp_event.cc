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

#define LOG_TAG "bluetooth_hal.extensions.ccc"

#include "bluetooth_hal/extensions/ccc/bluetooth_lmp_event.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "aidl/android/hardware/bluetooth/lmp_event/IBluetoothLmpEventCallback.h"
#include "android-base/logging.h"
#include "android/binder_auto_utils.h"
#include "android/binder_status.h"
#include "bluetooth_hal/bluetooth_address.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_handler_callback.h"
#include "bluetooth_hal/extensions/ccc/bluetooth_ccc_util.h"

/*
 * This is a fork of bluetooth_ccc to support the AOSP implementation of timesync.
 */

namespace bluetooth_hal::extensions::ccc {
namespace {

using ::aidl::android::hardware::bluetooth::lmp_event::Direction;
using ::aidl::android::hardware::bluetooth::lmp_event::IBluetoothLmpEventCallback;
using ::aidl::android::hardware::bluetooth::lmp_event::LmpEventId;
using ::aidl::android::hardware::bluetooth::lmp_event::Timestamp;

using ::bluetooth_hal::hci::BluetoothAddress;
using ::ndk::ScopedAStatus;

using ScopedDeathRecipient =
        std::unique_ptr<AIBinder_DeathRecipient, void (*)(AIBinder_DeathRecipient*)>;

ScopedDeathRecipient MakeScopedDeathRecipient(AIBinder_DeathRecipient* death_recipient) {
    return ScopedDeathRecipient(death_recipient, &AIBinder_DeathRecipient_delete);
}

class BluetoothCccDeathRecipient {
  public:
    BluetoothCccDeathRecipient(const BluetoothAddress& address, const AddressType address_type)
        : is_dead_(false),
          ccc_callback_(nullptr),
          client_death_recipient_(MakeScopedDeathRecipient(nullptr)),
          address_(address),
          address_type_(address_type) {}

    void LinkToDeath(const std::shared_ptr<IBluetoothLmpEventCallback>& cb) {
        ccc_callback_ = cb;

        auto on_link_died = [](void* cookie) {
            auto* death_recipient = static_cast<BluetoothCccDeathRecipient*>(cookie);
            death_recipient->ServiceDied();
        };
        client_death_recipient_ =
                MakeScopedDeathRecipient(AIBinder_DeathRecipient_new(on_link_died));

        binder_status_t link_to_death_return_status = AIBinder_linkToDeath(
                ccc_callback_->asBinder().get(), client_death_recipient_.get(), this /* cookie */);
        if (link_to_death_return_status != STATUS_OK) {
            LOG(FATAL) << "Unable to link to death recipient";
        }
    }

    void UnlinkToDeath() {
        if (!is_dead_) {
            binder_status_t unlink_to_death_return_status = AIBinder_unlinkToDeath(
                    ccc_callback_->asBinder().get(), client_death_recipient_.get(), this);
            if (unlink_to_death_return_status != STATUS_OK) {
                LOG(FATAL) << "Unable to unlink to death recipient";
            }
        }
        client_death_recipient_.reset();
    }

    void ServiceDied() {
        LOG(WARNING) << __func__ << ": BluetoothCccDeathRecipient::serviceDied";
        is_dead_ = true;
        BluetoothCccHandler::GetHandler().UnregisterLmpEventsWithType(BluetoothAddress(address_),
                                                                      address_type_);
    }

  private:
    bool is_dead_;
    std::shared_ptr<IBluetoothLmpEventCallback> ccc_callback_;
    ScopedDeathRecipient client_death_recipient_;
    BluetoothAddress address_;
    AddressType address_type_;
};

class BluetoothCccHandlerCallbackImpl : public BluetoothCccHandlerCallback {
  public:
    explicit BluetoothCccHandlerCallbackImpl(
            const std::shared_ptr<IBluetoothLmpEventCallback> bluetooth_lmp_event_callback,
            const BluetoothAddress& address, const AddressType address_type,
            const std::vector<CccLmpEventId>& lmp_event_ids,
            const std::shared_ptr<BluetoothCccDeathRecipient> death_recipient)
        : BluetoothCccHandlerCallback(address, static_cast<AddressType>(address_type),
                                      lmp_event_ids),
          bluetooth_lmp_event_callback_(bluetooth_lmp_event_callback),
          death_recipient_(death_recipient),
          address_type_(address_type) {
        death_recipient_->LinkToDeath(bluetooth_lmp_event_callback_);
    }

    ~BluetoothCccHandlerCallbackImpl() { death_recipient_->UnlinkToDeath(); }

    void OnEventGenerated(const CccTimestamp& timestamp, const BluetoothAddress& address,
                          CccDirection direction, CccLmpEventId lmp_event_id,
                          uint16_t event_counter) override {
        LOG(INFO) << "OnEventGenerated (without AddressType) called for address "
                  << address.ToString() << ".";

        // TODO find correct address_type, currently set to public
        bluetooth_lmp_event_callback_->onEventGenerated(
                Timestamp(timestamp.system_time, timestamp.bluetooth_time),
                static_cast<::aidl::android::hardware::bluetooth::lmp_event::AddressType>(
                        address_type_),
                address, static_cast<Direction>(direction), static_cast<LmpEventId>(lmp_event_id),
                event_counter);
    }

    void OnRegistered(bool status) override {
        if (bluetooth_lmp_event_callback_ == nullptr) {
            return;
        }
        bluetooth_lmp_event_callback_->onRegistered(status);
    }

  private:
    const std::shared_ptr<IBluetoothLmpEventCallback> bluetooth_lmp_event_callback_;
    const std::shared_ptr<BluetoothCccDeathRecipient> death_recipient_;
    AddressType address_type_;
};

std::vector<CccLmpEventId> LmpEventCast(const std::vector<LmpEventId>& event_ids) {
    std::vector<CccLmpEventId> ccc_event_ids;
    for (const auto& event_id : event_ids) {
        ccc_event_ids.push_back(static_cast<CccLmpEventId>(event_id));
    }
    return ccc_event_ids;
}

}  // namespace

BluetoothLmpEvent::BluetoothLmpEvent() : handler_(BluetoothCccHandler::GetHandler()) {};

ScopedAStatus BluetoothLmpEvent::registerForLmpEvents(
        const std::shared_ptr<IBluetoothLmpEventCallback>& callback,
        const ::aidl::android::hardware::bluetooth::lmp_event::AddressType address_type,
        const std::array<uint8_t, 6>& address, const std::vector<LmpEventId>& lmpEventIds) {
    const auto lmp_event_ids = LmpEventCast(lmpEventIds);
    const auto bluetooth_address = BluetoothAddress(address);
    // TODO check if address type cast works on the aidl address type
    const auto address_type_ = static_cast<AddressType>(address_type);
    const auto death_recipient =
            std::make_shared<BluetoothCccDeathRecipient>(bluetooth_address, address_type_);
    bool status = handler_.RegisterForLmpEvents(std::make_shared<BluetoothCccHandlerCallbackImpl>(
            callback, bluetooth_address, address_type_, lmp_event_ids, death_recipient));
    return status ? ScopedAStatus::ok() : ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
}

ScopedAStatus BluetoothLmpEvent::unregisterLmpEvents(
        const ::aidl::android::hardware::bluetooth::lmp_event::AddressType addressType,
        const std::array<uint8_t, 6>& address) {
    bool status = handler_.UnregisterLmpEventsWithType(BluetoothAddress(address),
                                                       static_cast<AddressType>(addressType));
    return status ? ScopedAStatus::ok() : ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
}
}  // namespace bluetooth_hal::extensions::ccc