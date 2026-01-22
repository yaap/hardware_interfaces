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

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/subscriber.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/util/provider_factory.h"

namespace bluetooth_hal::transport {

/**
 * @brief Factory and manager for TransportInterface instances.
 *
 * This class manages the singleton transport instance and handles
 * transport type switching, vendor transport registration, and
 * HAL state notifications.
 */
class TransportFactory {
 public:
  using VendorFactory =
      ::bluetooth_hal::util::MultiKeyProviderFactory<TransportType,
                                                     TransportInterface>;
  using FactoryFn = VendorFactory::FactoryFn;

  /**
   * @brief Retrieves the current transport instance.
   *
   * @return A reference to the active TransportInterface.
   */
  static TransportInterface& GetTransport();

  /**
   * @brief Cleans up the current transport and releases resources.
   */
  static void CleanupTransport();

  /**
   * @brief Updates the current transport type.
   *
   * This method allows switching the transport type. If the provided type
   * differs from the current one, the internal transport will be updated.
   *
   * @param requested_type The new TransportType to set.
   * @return `true` if the transport was successfully updated, `false`
   * otherwise.
   */
  static bool UpdateTransportType(TransportType requested_type);

  /**
   * @brief Retrieves the current transport type.
   *
   * @return The current TransportType.
   */
  static TransportType GetTransportType();

  /**
   * @brief Registers a vendor-specific transport implementation.
   *
   * @param type The TransportType for the vendor transport.
   * @param factory A factory function to create the transport instance.
   * @return `true` if registration succeeds, `false` otherwise.
   */
  static bool RegisterVendorTransport(TransportType type, FactoryFn factory);

  /**
   * @brief Unregisters a vendor-specific transport implementation.
   *
   * @param type The TransportType to unregister.
   * @return `true` if unregistration succeeds, `false` otherwise.
   */
  static bool UnregisterVendorTransport(TransportType type);

  /**
   * @brief Notifies the transport layer of a change in the HAL state.
   *
   * @param hal_state The new state of the HAL.
   */
  static void NotifyHalStateChange(::bluetooth_hal::HalState hal_state);

  /**
   * @brief Subscribes a new subscriber to receive notifications.
   *
   * @param subscriber The subscriber to be added.
   */
  static void Subscribe(Subscriber& subscriber);

  /**
   * @brief Unsubscribes an existing subscriber.
   *
   * @param subscriber The subscriber to be removed.
   */
  static void Unsubscribe(Subscriber& subscriber);

 private:
  static std::pair<std::unique_ptr<TransportInterface>, TransportType>
  CreateOrAcquireTransport(TransportType requested_type);

  static inline TransportType current_transport_type_{TransportType::kUnknown};
  static inline std::recursive_mutex transport_mutex_;
  static std::unique_ptr<TransportInterface> current_transport_;
  static inline std::atomic<::bluetooth_hal::HalState> hal_state_{
      ::bluetooth_hal::HalState::kInit};
  static inline std::vector<std::reference_wrapper<Subscriber>> subscribers_;
};

}  // namespace bluetooth_hal::transport
