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

#include "bluetooth_hal/chip/chip_provisioner.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "bluetooth_hal/test/mock/mock_firmware_config_loader.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"

namespace bluetooth_hal::chip {
namespace {

using ::bluetooth_hal::config::MockFirmwareConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HalPacketCallback;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;
using ::testing::_;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Test;

class TestChipProvisioner : public ChipProvisioner {
  public:
    using ChipProvisioner::OnCommandCallback;
    using ChipProvisioner::SendCommandAndWait;

    uint16_t GetPendingCommandOpcode() {
        std::lock_guard<std::mutex> lock(command_promise_mutex_);
        return pending_command_opcode_;
    }
};

class ChipProvisionerTest : public Test {
  protected:
    void SetUp() override {
        MockHciRouter::SetMockRouter(&mock_hci_router_);
        MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
        MockFirmwareConfigLoader::SetMockLoader(&mock_firmware_config_loader_);
        chip_provisioner_ = std::make_shared<TestChipProvisioner>();
    }

    void TearDown() override {
        chip_provisioner_.reset();
        MockHciRouter::SetMockRouter(nullptr);
        MockHciRouterClientAgent::SetMockAgent(nullptr);
        MockFirmwareConfigLoader::SetMockLoader(nullptr);
    }

    MockHciRouter mock_hci_router_;
    MockHciRouterClientAgent mock_hci_router_client_agent_;
    MockFirmwareConfigLoader mock_firmware_config_loader_;
    std::shared_ptr<TestChipProvisioner> chip_provisioner_;

    static const HalPacket kCommandPacket;
    static const HalPacket kResponsePacket;
};

const HalPacket ChipProvisionerTest::kCommandPacket({
        static_cast<uint8_t>(HciPacketType::kCommand),
        0x01,  // Opcode
        0xFC,  // Opcode
        0x00,  // Length
});

const HalPacket ChipProvisionerTest::kResponsePacket({
        static_cast<uint8_t>(HciPacketType::kEvent),
        0x0E,  // Command Complete
        0x04,  // Length
        0x01,  // Num HCI Command Packets
        0x01,  // Opcode
        0xFC,  // Opcode
        0x00,  // Status Success
});

TEST_F(ChipProvisionerTest, SendCommandAndWait_Success) {
    HalPacketCallback saved_callback;
    EXPECT_CALL(mock_hci_router_, SendCommand(_, _))
            .WillOnce([&](const HalPacket&, const HalPacketCallback& callback) {
                saved_callback = callback;
                return true;
            });

    std::thread response_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        saved_callback(kResponsePacket);
    });

    EXPECT_TRUE(chip_provisioner_->SendCommandAndWait(kCommandPacket));
    response_thread.join();
}

TEST_F(ChipProvisionerTest, SendCommandAndWait_SendCommandFailure_ResetsState) {
    EXPECT_CALL(mock_hci_router_, SendCommand(_, _)).WillOnce(Return(false));

    EXPECT_FALSE(chip_provisioner_->SendCommandAndWait(kCommandPacket));
    // Verify pending opcode is reset to kEmptyCommandOpcode (0xFFFF)
    EXPECT_EQ(chip_provisioner_->GetPendingCommandOpcode(), 0xFFFF);
}

TEST_F(ChipProvisionerTest, SendCommandAndWait_MultipleResponses) {
    HalPacketCallback saved_callback;
    EXPECT_CALL(mock_hci_router_, SendCommand(_, _))
            .WillOnce([&](const HalPacket&, const HalPacketCallback& callback) {
                saved_callback = callback;
                return true;
            });

    std::thread response_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Send multiple responses
        saved_callback(kResponsePacket);
        saved_callback(kResponsePacket);
        saved_callback(kResponsePacket);
    });

    // Should not crash even with multiple responses
    EXPECT_TRUE(chip_provisioner_->SendCommandAndWait(kCommandPacket));
    response_thread.join();
}

}  // namespace
}  // namespace bluetooth_hal::chip
