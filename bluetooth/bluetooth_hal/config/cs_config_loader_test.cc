/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/config/cs_config_loader.h"

#include <string_view>
#include <vector>

#include "android-base/file.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/test/mock/mock_android_base_wrapper.h"
#include "gtest/gtest.h"

namespace bluetooth_hal::config {
namespace {

using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::MockAndroidBaseWrapper;

using ::testing::Return;
using ::testing::Test;

constexpr std::string_view kValidContent = R"({
  "commands": [
      {
          "packet_type": 1,
          "opcode": 4660,
          "payload_length": 8,
          "sub_opcode": [1, 2, 3],
          "data": [4, 5, 6, 7, 8]
      },
      {
          "packet_type": 1,
          "opcode": 4660,
          "payload_length": 5,
          "sub_opcode": [10, 11],
          "data": [12, 13, 14]
      }
  ]
})";

// Another valid JSON content for testing reload logic.
constexpr std::string_view kAnotherValidContent = R"({
  "commands": [
      {
          "packet_type": 2,
          "opcode": 1234,
          "payload_length": 2,
          "sub_opcode": [],
          "data": [99, 100]
      }
  ]
})";

// Content with an empty commands array.
constexpr std::string_view kEmptyContent = R"({
  "commands": [

  ]
})";

class CsConfigLoaderTest : public Test {
 protected:
  void SetUp() override {
    MockAndroidBaseWrapper::SetMockWrapper(&mock_android_base_wrapper_);
  }

  MockAndroidBaseWrapper mock_android_base_wrapper_;
};

TEST_F(CsConfigLoaderTest, ParseValidContentAndGetCsCalibrationCommands) {
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromString(kValidContent));

  const std::vector<HalPacket>& commands =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();

  EXPECT_EQ(commands.size(), 2);

  EXPECT_EQ(commands[0],
            std::vector<uint8_t>({0x01, 0x34, 0x12, 0x08, 0x01, 0x02, 0x03,
                                  0x04, 0x05, 0x06, 0x07, 0x08}));

  EXPECT_EQ(commands[1], std::vector<uint8_t>({0x01, 0x34, 0x12, 0x05, 0x0A,
                                               0x0B, 0x0C, 0x0D, 0x0E}));
}

TEST_F(CsConfigLoaderTest, ParseEmptyContent) {
  EXPECT_FALSE(CsConfigLoader::GetLoader().LoadConfigFromString(kEmptyContent));
}

TEST_F(CsConfigLoaderTest, LoadConfigFromFile_ValidFile) {
  // Create a temporary file with valid JSON content.
  TemporaryFile temp_file;
  ASSERT_TRUE(temp_file.fd != -1);
  ASSERT_TRUE(
      android::base::WriteStringToFd(std::string(kValidContent), temp_file.fd));

  // Attempt to load the configuration from the created file.
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromFile(temp_file.path));

  // Verify that the commands from the file were loaded correctly.
  const std::vector<HalPacket>& commands =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();
  EXPECT_EQ(commands.size(), 2);
  EXPECT_EQ(commands[0],
            std::vector<uint8_t>({0x01, 0x34, 0x12, 0x08, 0x01, 0x02, 0x03,
                                  0x04, 0x05, 0x06, 0x07, 0x08}));
}

TEST_F(CsConfigLoaderTest, LoadConfigFromFile_NonExistentFile) {
  // Pre-load a valid configuration to ensure the state is not cleared on
  // failure.
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromString(kValidContent));
  EXPECT_FALSE(CsConfigLoader::GetLoader().GetCsCalibrationCommands().empty());

  // Attempt to load from a file that does not exist.
  EXPECT_FALSE(CsConfigLoader::GetLoader().LoadConfigFromFile(
      "/a/b/c/d/non_existent_file.json"));

  // Verify that the previously loaded commands are preserved.
  EXPECT_FALSE(CsConfigLoader::GetLoader().GetCsCalibrationCommands().empty());
}

TEST_F(CsConfigLoaderTest, LoadConfigFromFile_InvalidContent_KeepsOldConfig) {
  // 1. Pre-load a valid configuration.
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromString(kValidContent));
  const auto& original_commands =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();
  EXPECT_FALSE(original_commands.empty());

  // 2. Create a temporary file with invalid JSON content.
  TemporaryFile temp_file;
  ASSERT_TRUE(temp_file.fd != -1);
  ASSERT_TRUE(android::base::WriteStringToFd("invalid json", temp_file.fd));

  // 3. Attempt to load from the file with invalid content.
  EXPECT_FALSE(CsConfigLoader::GetLoader().LoadConfigFromFile(temp_file.path));

  // 4. Verify that the original commands are preserved after the failed load.
  const auto& commands_after_failed_load =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();
  EXPECT_EQ(original_commands, commands_after_failed_load);
}

TEST_F(CsConfigLoaderTest, GetCsCalibrationCommands_ReloadsFromProperty) {
  // 1. Load an initial configuration.
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromString(kValidContent));
  EXPECT_EQ(CsConfigLoader::GetLoader().GetCsCalibrationCommands().size(), 2);

  // 2. Create a new config file and set the system property to its path.
  TemporaryFile temp_file;
  ASSERT_TRUE(temp_file.fd != -1);
  ASSERT_TRUE(android::base::WriteStringToFd(std::string(kAnotherValidContent),
                                             temp_file.fd));
  EXPECT_CALL(mock_android_base_wrapper_,
              GetProperty(Property::kCsConfigFile, ""))
      .WillOnce(Return(temp_file.path));

  // 3. Call GetCsCalibrationCommands, which should trigger a reload.
  const std::vector<HalPacket>& reloaded_commands =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();

  // 4. Verify that the new configuration has been loaded.
  EXPECT_EQ(reloaded_commands.size(), 1);
  // opcode 1234 is 0x04D2. data 99, 100 is 0x63, 0x64
  EXPECT_EQ(reloaded_commands[0],
            std::vector<uint8_t>({0x02, 0xD2, 0x04, 0x02, 0x63, 0x64}));
}

TEST_F(CsConfigLoaderTest,
       GetCsCalibrationCommands_ReloadFails_KeepsOriginalConfig) {
  // 1. Load an initial configuration.
  EXPECT_TRUE(CsConfigLoader::GetLoader().LoadConfigFromString(kValidContent));
  const auto& original_commands =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();
  EXPECT_EQ(original_commands.size(), 2);

  // 2. Set the property to a non-existent file to cause a reload failure.
  EXPECT_CALL(mock_android_base_wrapper_,
              GetProperty(Property::kCsConfigFile, ""))
      .WillOnce(Return("/a/b/c/d/non_existent_file.json"));

  // 3. Call GetCsCalibrationCommands to trigger the failed reload attempt.
  const std::vector<HalPacket>& commands_after_failed_reload =
      CsConfigLoader::GetLoader().GetCsCalibrationCommands();

  // 4. Verify that the original configuration is preserved.
  EXPECT_EQ(commands_after_failed_reload.size(), 2);
  EXPECT_EQ(original_commands, commands_after_failed_reload);
}

}  // namespace
}  // namespace bluetooth_hal::config
