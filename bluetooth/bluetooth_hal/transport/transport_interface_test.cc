/*
 * Copyright 2024 The Android Open Source Project
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

#include "bluetooth_hal/transport/transport_interface.h"

#include <memory>
#include <vector>

#include "bluetooth_hal/config/config_constants.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/mock/mock_hal_config_loader.h"
#include "bluetooth_hal/test/mock/mock_subscriber.h"
#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"
#include "bluetooth_hal/transport/transport_factory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::transport {
namespace {

using ::testing::_;
using ::testing::AtMost;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;
using ::testing::Test;

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::config::MockHalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::MockSystemCallWrapper;

namespace cfg_consts = ::bluetooth_hal::config::constants;

TEST(TransportInterfaceTest, GetTransportTypeReturnDefaultType) {
  EXPECT_EQ(TransportFactory::GetTransportType(), TransportType::kUnknown);
}

TEST(TransportInterfaceTest, HalStateNotChangeAndNotNotifySubscriber) {
  MockSubscriber subscriber;
  TransportFactory::Subscribe(subscriber);

  EXPECT_CALL(subscriber, NotifyHalStateChange(_)).Times(0);
  TransportFactory::NotifyHalStateChange(HalState::kInit);
}

TEST(TransportInterfaceTest, HalStateChangeAndNotifySubscriber) {
  MockSubscriber subscriber;
  TransportFactory::Subscribe(subscriber);

  EXPECT_CALL(subscriber, NotifyHalStateChange(HalState::kRunning)).Times(1);
  TransportFactory::NotifyHalStateChange(HalState::kRunning);
}

TEST(TransportInterfaceTest, UnsubscribeAndNoSubscriberToNotify) {
  MockSubscriber subscriber;
  TransportFactory::Subscribe(subscriber);
  TransportFactory::Unsubscribe(subscriber);

  EXPECT_CALL(subscriber, NotifyHalStateChange(_)).Times(0);
  TransportFactory::NotifyHalStateChange(HalState::kFirmwareReady);
}

TEST(TransportInterfaceTest, MultipleSubscribersReceiveNotification) {
  MockSubscriber subscriber1;
  MockSubscriber subscriber2;
  MockSubscriber subscriber3;

  TransportFactory::Subscribe(subscriber1);
  TransportFactory::Subscribe(subscriber2);
  TransportFactory::Subscribe(subscriber3);

  EXPECT_CALL(subscriber1, NotifyHalStateChange(HalState::kRunning)).Times(1);
  EXPECT_CALL(subscriber2, NotifyHalStateChange(HalState::kRunning)).Times(1);
  EXPECT_CALL(subscriber3, NotifyHalStateChange(HalState::kRunning)).Times(1);

  TransportFactory::NotifyHalStateChange(HalState::kRunning);

  Mock::VerifyAndClearExpectations(&subscriber1);
  Mock::VerifyAndClearExpectations(&subscriber2);
  Mock::VerifyAndClearExpectations(&subscriber3);

  TransportFactory::Unsubscribe(subscriber1);
  TransportFactory::Unsubscribe(subscriber2);
  TransportFactory::Unsubscribe(subscriber3);
}

TEST(TransportInterfaceTest, UnsubscribingOneOfMultipleStillNotifiesOthers) {
  MockSubscriber subscriber1;
  MockSubscriber subscriber2;
  MockSubscriber subscriber3;

  TransportFactory::Subscribe(subscriber1);
  TransportFactory::Subscribe(subscriber2);
  TransportFactory::Subscribe(subscriber3);

  TransportFactory::Unsubscribe(subscriber2);

  EXPECT_CALL(subscriber1, NotifyHalStateChange(HalState::kFirmwareReady))
      .Times(1);
  EXPECT_CALL(subscriber2, NotifyHalStateChange(_)).Times(0);
  EXPECT_CALL(subscriber3, NotifyHalStateChange(HalState::kFirmwareReady))
      .Times(1);

  TransportFactory::NotifyHalStateChange(HalState::kFirmwareReady);

  Mock::VerifyAndClearExpectations(&subscriber1);
  Mock::VerifyAndClearExpectations(&subscriber2);
  Mock::VerifyAndClearExpectations(&subscriber3);

  TransportFactory::Unsubscribe(subscriber1);
  TransportFactory::Unsubscribe(subscriber3);
}

TEST(TransportInterfaceTest,
     SubscribingSameSubscriberMultipleTimesNotifiesOnce) {
  MockSubscriber subscriber;

  TransportFactory::Subscribe(subscriber);
  TransportFactory::Subscribe(subscriber);
  TransportFactory::Subscribe(subscriber);

  EXPECT_CALL(subscriber, NotifyHalStateChange(HalState::kBtChipReady))
      .Times(1);

  TransportFactory::NotifyHalStateChange(HalState::kBtChipReady);

  Mock::VerifyAndClearExpectations(&subscriber);

  TransportFactory::Unsubscribe(subscriber);
}

class MockTransportInterfaceCallback : public TransportInterfaceCallback {
 public:
  MOCK_METHOD(void, OnTransportClosed, (), (override));
  MOCK_METHOD(void, OnTransportPacketReady, (const HalPacket&), (override));
};

class MockVendorTransport : public TransportInterface {
 public:
  explicit MockVendorTransport(TransportType type)
      : instance_type_(type), active_(false), initialized_(false) {}

  bool Initialize(TransportInterfaceCallback* cb) override {
    callback_ = cb;
    initialized_ = MockedInitialize(cb);
    active_ = initialized_;
    return initialized_;
  }

  void Cleanup() override {
    active_ = false;
    initialized_ = false;
    MockedCleanup();
  }

  bool IsTransportActive() const override {
    return initialized_ && active_ && MockedIsTransportActive();
  }

  bool Send(const ::bluetooth_hal::hci::HalPacket& packet) override {
    return MockedSend(packet);
  }

  TransportType GetInstanceTransportType() const override {
    return instance_type_;
  }

  MOCK_METHOD(bool, MockedInitialize, (TransportInterfaceCallback*));
  MOCK_METHOD(void, MockedCleanup, ());
  MOCK_METHOD(bool, MockedIsTransportActive, (), (const));
  MOCK_METHOD(bool, MockedSend, (const ::bluetooth_hal::hci::HalPacket&));

 private:
  TransportType instance_type_;
  bool active_;
  bool initialized_;
  TransportInterfaceCallback* callback_ = nullptr;
};

class VendorTransportTest : public Test {
 protected:
  void SetUp() override {
    MockHalConfigLoader::SetMockLoader(&mock_hal_config_loader_);
    MockSystemCallWrapper::SetMockWrapper(&mock_system_call_wrapper_);

    ON_CALL(mock_hal_config_loader_, GetRfkillFolderPrefix())
        .WillByDefault(ReturnRef(rfkill_folder_prefix_str_));
    ON_CALL(mock_system_call_wrapper_, Open(_, _)).WillByDefault(Return(-1));
  }

  void TearDown() override {
    TransportFactory::CleanupTransport();
    // No transport is active.
    EXPECT_EQ(TransportFactory::GetTransportType(), TransportType::kUnknown);
    TransportFactory::VendorFactory::UnregisterAllProviderFactories();
  }

  static constexpr TransportType kVendorType1 = TransportType::kVendorStart;
  static constexpr TransportType kVendorType2 =
      static_cast<TransportType>(static_cast<int>(kVendorType1) + 1);

  MockHalConfigLoader mock_hal_config_loader_;
  MockSystemCallWrapper mock_system_call_wrapper_;
  MockTransportInterfaceCallback mock_callback_;
  std::string rfkill_folder_prefix_str_{cfg_consts::kRfkillFolderPrefix};
  std::string rfkill_type_bluetooth_str_{cfg_consts::kRfkillTypeBluetooth};
};

TEST_F(VendorTransportTest, RegisterNullVendorTransportReturnsFalse) {
  EXPECT_FALSE(
      TransportFactory::RegisterVendorTransport(kVendorType1, nullptr));
}

TEST_F(VendorTransportTest,
       RegisterVendorTransportWithInvalidTypeTooLowReturnsFalse) {
  auto type = static_cast<TransportType>(99);  // Below kVendorStart.
  auto factory = [type]() {
    return std::make_unique<MockVendorTransport>(type);
  };
  EXPECT_FALSE(TransportFactory::RegisterVendorTransport(type, factory));
}

TEST_F(VendorTransportTest,
       RegisterVendorTransportWithInvalidTypeTooHighReturnsFalse) {
  auto type = static_cast<TransportType>(200);  // Above kVendorEnd.
  auto factory = [type]() {
    return std::make_unique<MockVendorTransport>(type);
  };
  EXPECT_FALSE(TransportFactory::RegisterVendorTransport(type, factory));
}

TEST_F(VendorTransportTest, RegisterVendorTransportSuccessfully) {
  auto factory = []() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
    EXPECT_CALL(*transport, MockedCleanup()).Times(1);
    return transport;
  };
  EXPECT_TRUE(TransportFactory::RegisterVendorTransport(kVendorType1, factory));

  std::vector<TransportType> priorities = {kVendorType1,
                                           TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities));
  EXPECT_EQ(TransportFactory::GetTransport().GetInstanceTransportType(),
            kVendorType1);
}

TEST_F(VendorTransportTest,
       RegisterDuplicateVendorTransportTypeOverwritesBeforeInit) {
  auto factory1 = []() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
    EXPECT_CALL(*transport, MockedCleanup()).Times(0);
    return transport;
  };
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType1, factory1));

  auto factory2 = []() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
    EXPECT_CALL(*transport, MockedCleanup()).Times(2);
    return transport;
  };
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType1, factory2));

  std::vector<TransportType> priorities = {kVendorType1,
                                           TransportType::kUartH4};

  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities));

  EXPECT_EQ(TransportFactory::GetTransport().GetInstanceTransportType(),
            kVendorType1);
  TransportFactory::GetTransport().Cleanup();
}

TEST_F(VendorTransportTest,
       RegisterDuplicateVendorTransportTypeCannotOverwritesAfterInit) {
  auto factory1 = [this]() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
    EXPECT_CALL(*transport, MockedInitialize(&mock_callback_))
        .WillOnce(Return(true));
    EXPECT_CALL(*transport, MockedCleanup()).Times(2);
    return transport;
  };
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType1, factory1));

  EXPECT_TRUE(TransportFactory::UpdateTransportType(kVendorType1));
  EXPECT_TRUE(TransportFactory::GetTransport().Initialize(&mock_callback_));

  auto factory2 = []() {
    return std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
  };
  EXPECT_FALSE(
      TransportFactory::RegisterVendorTransport(kVendorType1, factory2));

  TransportFactory::GetTransport().Cleanup();
}

TEST_F(VendorTransportTest, GetTransportSelectsHighestPriorityVendor) {
  auto factory1 = []() {
    return std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
  };
  auto factory2 = []() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType2);
    EXPECT_CALL(*transport, MockedCleanup()).Times(2);
    return transport;
  };

  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType1, factory1));
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType2, factory2));

  std::vector<TransportType> priorities = {kVendorType2, kVendorType1,
                                           TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities));

  TransportInterface& transport = TransportFactory::GetTransport();
  EXPECT_EQ(transport.GetInstanceTransportType(), kVendorType2);
  TransportFactory::GetTransport().Cleanup();
}

TEST_F(VendorTransportTest, UnregisterNonExistentVendorTransportReturnsFalse) {
  EXPECT_FALSE(TransportFactory::UnregisterVendorTransport(
      static_cast<TransportType>(150)));
}

TEST_F(VendorTransportTest, UnregisterInvalidVendorTransportTypeReturnsFalse) {
  EXPECT_FALSE(
      TransportFactory::UnregisterVendorTransport(TransportType::kUartH4));
  EXPECT_FALSE(TransportFactory::UnregisterVendorTransport(
      static_cast<TransportType>(99)));  // Below kVendorStart.
  EXPECT_FALSE(TransportFactory::UnregisterVendorTransport(
      static_cast<TransportType>(200)));  // Above kVendorEnd.
}

TEST_F(VendorTransportTest, UnregisterActiveVendorTransportReturnsFalse) {
  auto factory = [this]() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
    EXPECT_CALL(*transport, MockedInitialize(&mock_callback_))
        .WillOnce(Return(true));
    EXPECT_CALL(*transport, MockedCleanup()).Times(1);
    return transport;
  };
  EXPECT_TRUE(TransportFactory::RegisterVendorTransport(kVendorType1, factory));

  std::vector<TransportType> priorities = {kVendorType1,
                                           TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities));

  // Make it active.
  EXPECT_TRUE(TransportFactory::UpdateTransportType(kVendorType1));
  auto* transport_ptr = &TransportFactory::GetTransport();
  TransportFactory::GetTransport().Initialize(&mock_callback_);

  EXPECT_FALSE(TransportFactory::UnregisterVendorTransport(kVendorType1));
  // Verify it's still active.
  EXPECT_EQ(&TransportFactory::GetTransport(), transport_ptr);

  Mock::VerifyAndClearExpectations(&mock_hal_config_loader_);
}

TEST_F(VendorTransportTest, UnregisterInactiveVendorTransportSuccessfully) {
  auto factory1 = []() {
    return std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
  };
  auto factory2 = [this]() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType2);

    EXPECT_CALL(*transport, MockedInitialize(&mock_callback_))
        .Times(AtMost(1))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*transport, MockedCleanup()).Times(1);
    return transport;
  };
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType1, factory1));
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType2, factory2));

  std::vector<TransportType> priorities_2_then_default = {
      kVendorType2, TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities_2_then_default));
  EXPECT_TRUE(TransportFactory::GetTransport().Initialize(&mock_callback_));

  // Unregister inactive vendor transport 1.
  EXPECT_TRUE(TransportFactory::UnregisterVendorTransport(kVendorType1));

  Mock::VerifyAndClearExpectations(&mock_hal_config_loader_);

  // Cleanup active transport and load transprot again
  TransportFactory::CleanupTransport();

  // Verify vendor transport 1 is gone.
  std::vector<TransportType> priorities_1_then_default = {
      kVendorType1, TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities_1_then_default));
  // Since kVendorStart is unregistered, GetTransport should now return UartH4.
  EXPECT_EQ(TransportFactory::GetTransport().GetInstanceTransportType(),
            TransportType::kUartH4);

  Mock::VerifyAndClearExpectations(&mock_hal_config_loader_);

  // Cleanup the UartH4 transport created above.
  TransportFactory::CleanupTransport();

  // Verify vendor transport 2 is still registered and can be used.
  std::vector<TransportType> priorities_2 = {kVendorType2};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities_2));
  EXPECT_EQ(TransportFactory::GetTransport().GetInstanceTransportType(),
            kVendorType2);

  Mock::VerifyAndClearExpectations(&mock_hal_config_loader_);
}

TEST_F(VendorTransportTest, UnregisterAndThenTryToUseReturnsFallback) {
  auto factory = []() {
    return std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
  };
  EXPECT_TRUE(TransportFactory::RegisterVendorTransport(kVendorType1, factory));

  EXPECT_TRUE(TransportFactory::UnregisterVendorTransport(kVendorType1));

  std::vector<TransportType> priorities = {kVendorType1,
                                           TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities));
  EXPECT_EQ(TransportFactory::GetTransport().GetInstanceTransportType(),
            TransportType::kUartH4);

  Mock::VerifyAndClearExpectations(&mock_hal_config_loader_);
}

TEST_F(VendorTransportTest, SwitchToNonExistentVendorFailsAndPreservesCurrent) {
  // 1. Register and activate kVendorType2
  auto factory2 = []() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType2);
    EXPECT_CALL(*transport, MockedCleanup())
        .Times(1);  // Will be cleaned up at the end
    return transport;
  };
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType2, factory2));
  EXPECT_TRUE(TransportFactory::UpdateTransportType(kVendorType2));
  auto* transport2_ptr = &TransportFactory::GetTransport();
  EXPECT_EQ(TransportFactory::GetTransportType(), kVendorType2);

  // 2. Try to switch to kVendorType1 (not registered)
  EXPECT_FALSE(TransportFactory::UpdateTransportType(kVendorType1));

  // 3. Verify that kVendorType2 is still the current transport
  EXPECT_EQ(TransportFactory::GetTransportType(), kVendorType2);
  EXPECT_EQ(&TransportFactory::GetTransport(), transport2_ptr);
}

TEST_F(VendorTransportTest, RegisterTransportAfterInitSuccessfully) {
  auto factory2 = []() {
    auto transport =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType2);
    EXPECT_CALL(*transport, MockedCleanup()).Times(1);
    return transport;
  };
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType2, factory2));

  std::vector<TransportType> priorities = {kVendorType1, kVendorType2,
                                           TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities));

  auto* transport2_ptr = &TransportFactory::GetTransport();
  EXPECT_EQ(TransportFactory::GetTransportType(), kVendorType2);

  auto factory1 = []() {
    return std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
  };
  EXPECT_TRUE(
      TransportFactory::RegisterVendorTransport(kVendorType1, factory1));

  // Transport is still kVendorType2 as it is active.
  EXPECT_EQ(&TransportFactory::GetTransport(), transport2_ptr);
  EXPECT_EQ(TransportFactory::GetTransportType(), kVendorType2);

  // Cleanup and moved back to the map.
  TransportFactory::CleanupTransport();
  EXPECT_EQ(TransportFactory::GetTransportType(), TransportType::kUnknown);
}

TEST_F(VendorTransportTest, GetVendorTransportReturnSameInstance) {
  std::vector<TransportType> priorities = {kVendorType1,
                                           TransportType::kUartH4};
  EXPECT_CALL(mock_hal_config_loader_, GetTransportTypePriority())
      .WillRepeatedly(ReturnRef(priorities));
  EXPECT_CALL(mock_hal_config_loader_, GetRfkillFolderPrefix())
      .WillRepeatedly(ReturnRef(rfkill_folder_prefix_str_));

  auto factory = []() {
    auto transprot =
        std::make_unique<StrictMock<MockVendorTransport>>(kVendorType1);
    EXPECT_CALL(*transprot, MockedCleanup()).Times(1);
    return transprot;
  };
  EXPECT_TRUE(TransportFactory::RegisterVendorTransport(kVendorType1, factory));

  TransportInterface* transport1 = &TransportFactory::GetTransport();
  EXPECT_EQ(transport1->GetInstanceTransportType(), kVendorType1);
  EXPECT_EQ(TransportFactory::GetTransportType(), kVendorType1);

  TransportInterface* transport2 = &TransportFactory::GetTransport();
  EXPECT_EQ(transport1, transport2);
  EXPECT_EQ(TransportFactory::GetTransportType(), kVendorType1);
}

}  // namespace
}  // namespace bluetooth_hal::transport
