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

#include "bluetooth_hal/test/mock/mock_transport_factory.h"

#include "android-base/logging.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/transport_factory.h"
#include "bluetooth_hal/transport/transport_instance.h"

namespace bluetooth_hal::transport {

namespace {
using ::bluetooth_hal::HalState;

void CheckMockFactoryOrFatal() {
    if (!MockTransportFactory::GetFactory()) {
        LOG(FATAL) << "MockTransportFactory instance is nullptr. Did you forget to "
                      "call SetMockFactory in your test SetUp?";
    }
}
}  // namespace

TransportInstance& TransportFactory::GetTransport() {
    CheckMockFactoryOrFatal();
    return MockTransportFactory::GetFactory()->GetTransport();
}

bool TransportFactory::UpdateTransportType(TransportType requested_type) {
    CheckMockFactoryOrFatal();
    return MockTransportFactory::GetFactory()->UpdateTransportType(requested_type);
}

TransportType TransportFactory::GetTransportType() {
    CheckMockFactoryOrFatal();
    return MockTransportFactory::GetFactory()->GetTransportType();
}

void TransportFactory::CleanupTransport() {
    CheckMockFactoryOrFatal();
    MockTransportFactory::GetFactory()->CleanupTransport();
}

bool TransportFactory::RegisterVendorTransport(TransportType type, FactoryFn factory) {
    CheckMockFactoryOrFatal();
    return MockTransportFactory::GetFactory()->RegisterVendorTransport(type, std::move(factory));
}

bool TransportFactory::UnregisterVendorTransport(TransportType type) {
    CheckMockFactoryOrFatal();
    return MockTransportFactory::GetFactory()->UnregisterVendorTransport(type);
}

void TransportFactory::NotifyHalStateChange(HalState hal_state) {
    CheckMockFactoryOrFatal();
    MockTransportFactory::GetFactory()->NotifyHalStateChange(hal_state);
}

void TransportFactory::Subscribe(Subscriber& subscriber) {
    CheckMockFactoryOrFatal();
    MockTransportFactory::GetFactory()->Subscribe(subscriber);
}

void TransportFactory::Unsubscribe(Subscriber& subscriber) {
    CheckMockFactoryOrFatal();
    MockTransportFactory::GetFactory()->Unsubscribe(subscriber);
}

}  // namespace bluetooth_hal::transport
