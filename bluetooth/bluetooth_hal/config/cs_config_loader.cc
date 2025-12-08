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

#define LOG_TAG "bluetooth_hal.cs_config"

#include "bluetooth_hal/config/cs_config_loader.h"

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/android_base_wrapper.h"
#include "cs_config.pb.h"
#include "google/protobuf/util/json_util.h"

namespace bluetooth_hal {
namespace config {

using ::bluetooth_hal::Property;
using ::bluetooth_hal::config::proto::CalibrationCommands;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::util::AndroidBaseWrapper;

using ::google::protobuf::util::JsonParseOptions;
using ::google::protobuf::util::JsonStringToMessage;

constexpr std::string_view kDefaultCsConfigFile =
    "/vendor/etc/bluetooth/bluetooth_channel_sounding_calibration.json";

class CsConfigLoaderImpl : public CsConfigLoader {
 public:
  CsConfigLoaderImpl();

  bool LoadConfig() override;
  bool LoadConfigFromFile(std::string_view path) override;
  bool LoadConfigFromString(std::string_view content) override;

  const std::vector<HalPacket>& GetCsCalibrationCommands() const override;

  std::string DumpConfigToString() const override;

 private:
  bool ParseCommandsIntoVector(
      const CalibrationCommands& calibration_commands_proto,
      std::vector<HalPacket>* out_commands);

  void MaybeReloadConfig();

  std::vector<HalPacket> cs_calibration_commands_;
};

CsConfigLoaderImpl::CsConfigLoaderImpl() {
#ifndef UNIT_TEST
  LoadConfig();
#endif
}

bool CsConfigLoaderImpl::LoadConfig() {
  // 1. Try to load from kDefaultCsConfigFile.
  if (LoadConfigFromFile(kDefaultCsConfigFile)) {
    LOG(INFO) << __func__
              << ": Successfully loaded calibration config from default file '"
              << kDefaultCsConfigFile << "'.";
    return true;
  }

  // 2. Try to load from product-specific path.
  std::string product_name =
      AndroidBaseWrapper::GetWrapper().GetProperty(Property::kProductName, "");
  if (!product_name.empty()) {
    size_t pos = kDefaultCsConfigFile.rfind(".json");
    std::string product_specific_path =
        std::string(kDefaultCsConfigFile).substr(0, pos) + "_" + product_name +
        ".json";

    if (LoadConfigFromFile(product_specific_path)) {
      LOG(INFO) << __func__
                << ": Successfully loaded product-specific calibration config "
                   "from '"
                << product_specific_path << "'.";
      return true;
    }
  }

  LOG(WARNING) << __func__
               << ": Failed to load calibration config from any source.";

  return false;
}

bool CsConfigLoaderImpl::LoadConfigFromFile(std::string_view path) {
  std::ifstream json_file(path.data());
  if (!json_file.is_open()) {
    LOG(ERROR) << __func__ << ": Failed to open json file " << path.data();
    return false;
  }

  std::string json_str((std::istreambuf_iterator<char>(json_file)),
                       std::istreambuf_iterator<char>());

  return LoadConfigFromString(json_str);
}

bool CsConfigLoaderImpl::LoadConfigFromString(std::string_view content) {
  CalibrationCommands calibration_commands;
  JsonParseOptions options;
  options.ignore_unknown_fields = true;

  auto status = JsonStringToMessage(content, &calibration_commands, options);
  if (!status.ok()) {
    LOG(ERROR) << __func__
               << ": Failed to parse json content, error: " << status.message();
    return false;
  }

  std::vector<HalPacket> new_commands;
  if (!ParseCommandsIntoVector(calibration_commands, &new_commands) ||
      new_commands.empty()) {
    return false;
  }

  cs_calibration_commands_ = std::move(new_commands);

  LOG(INFO) << DumpConfigToString();

  return true;
}

bool CsConfigLoaderImpl::ParseCommandsIntoVector(
    const CalibrationCommands& calibration_commands_proto,
    std::vector<HalPacket>* out_commands) {
  out_commands->clear();
  for (const auto& command : calibration_commands_proto.commands()) {
    HalPacket packet;

    packet.push_back(command.packet_type());
    packet.push_back(command.opcode() & 0xff);
    packet.push_back((command.opcode() >> 8u) & 0xff);
    packet.push_back(command.payload_length());

    packet.insert(packet.end(), command.sub_opcode().begin(),
                  command.sub_opcode().end());
    packet.insert(packet.end(), command.data().begin(), command.data().end());

    out_commands->emplace_back(packet);
  }
  return true;
}

const std::vector<HalPacket>& CsConfigLoaderImpl::GetCsCalibrationCommands()
    const {
  // Attempt to reload config. This is a const_cast workaround as the design
  // requires updating internal state from a const getter.
  const_cast<CsConfigLoaderImpl*>(this)->MaybeReloadConfig();
  return cs_calibration_commands_;
}

std::string CsConfigLoaderImpl::DumpConfigToString() const {
  std::stringstream ss;
  ss << "--- CsConfigLoaderImpl State ---\n";
  ss << "CS Calibration Commands Loaded: " << cs_calibration_commands_.size()
     << " command(s)\n";
  ss << "-------------------------------\n";
  return ss.str();
}

void CsConfigLoaderImpl::MaybeReloadConfig() {
  std::string config_file_from_property =
      AndroidBaseWrapper::GetWrapper().GetProperty(Property::kCsConfigFile, "");

  // If property is not set, do not attempt reload and do not warn.
  if (config_file_from_property.empty()) {
    return;
  }

  // Temporarily store original commands to revert if loading fails.
  auto original_commands = cs_calibration_commands_;

  if (LoadConfigFromFile(config_file_from_property)) {
    LOG(INFO) << __func__
              << ": Successfully reloaded calibration config from property '"
              << config_file_from_property << "'.";
    return;
  }

  // If loading from kCsConfigFile property failed, revert to original.
  cs_calibration_commands_ = std::move(original_commands);
}

CsConfigLoader& CsConfigLoader::GetLoader() {
  static CsConfigLoaderImpl loader;
  return loader;
}

}  // namespace config
}  // namespace bluetooth_hal
