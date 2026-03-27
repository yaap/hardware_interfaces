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

#define LOG_TAG "bluetooth_hal.extensions.cs.v2"

#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session_v2.h"

#include <sys/stat.h>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/ChannelSoudingRawData.h"
#include "aidl/android/hardware/bluetooth/ranging/ChannelSoundingProcedureData.h"
#include "aidl/android/hardware/bluetooth/ranging/Config.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/ProcedureEnableConfig.h"
#include "aidl/android/hardware/bluetooth/ranging/RangingResult.h"
#include "aidl/android/hardware/bluetooth/ranging/Reason.h"
#include "aidl/android/hardware/bluetooth/ranging/ResultType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "android-base/logging.h"
#include "android-base/properties.h"
#include "android/binder_auto_utils.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_distance_estimator_interface.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_session_interface.h"
#include "bluetooth_hal/extensions/cs/bluetooth_channel_sounding_util.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/android_base_wrapper.h"

namespace bluetooth_hal::extensions::cs {
namespace {

using ::aidl::android::hardware::bluetooth::ranging::BluetoothChannelSoundingParameters;
using ::aidl::android::hardware::bluetooth::ranging::ChannelSoudingRawData;
using ::aidl::android::hardware::bluetooth::ranging::ChannelSoundingProcedureData;
using ::aidl::android::hardware::bluetooth::ranging::Config;
using ::aidl::android::hardware::bluetooth::ranging::IBluetoothChannelSoundingSessionCallback;
using ::aidl::android::hardware::bluetooth::ranging::ModeData;
using ::aidl::android::hardware::bluetooth::ranging::ModeOneData;
using ::aidl::android::hardware::bluetooth::ranging::ModeThreeData;
using ::aidl::android::hardware::bluetooth::ranging::ModeTwoData;
using ::aidl::android::hardware::bluetooth::ranging::ModeType;
using ::aidl::android::hardware::bluetooth::ranging::ModeZeroData;
using ::aidl::android::hardware::bluetooth::ranging::PctIQSample;
using ::aidl::android::hardware::bluetooth::ranging::ProcedureEnableConfig;
using ::aidl::android::hardware::bluetooth::ranging::RangingResult;
using ::aidl::android::hardware::bluetooth::ranging::Reason;
using ::aidl::android::hardware::bluetooth::ranging::ResultType;
using ::aidl::android::hardware::bluetooth::ranging::RttToaTodData;
using ::aidl::android::hardware::bluetooth::ranging::StepData;
using ::aidl::android::hardware::bluetooth::ranging::SubeventResultData;
using ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData;
using ::android::base::GetUintProperty;
using ::bluetooth_hal::Property;
using ::bluetooth_hal::util::AndroidBaseWrapper;
using ::ndk::ScopedAStatus;

// Global state for logging
std::string g_cs_log_filename;
bool g_cs_log_first_entry = true;
std::recursive_mutex g_cs_log_mutex;
constexpr std::string_view kIsHalLogEnabled = "vendor.bluetooth.cs_hal_log_enabled";

// Helper to get current timestamp string for the JSON entry
std::string GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0')
       << std::setw(3) << ms.count();
    return ss.str();
}

// Helper to add indentation to a multiline string
std::string PadString(const std::string& input, int spaces, bool indent_first_line = true) {
    if (input.empty()) {
        return input;
    }

    std::string padding(spaces, ' ');
    // Only indent the first line if requested
    std::string output = indent_first_line ? (padding + input) : input;

    // Start searching for newlines.
    // If first line was indented, skip the initial padding.
    size_t pos = indent_first_line ? spaces : 0;

    // Replace every newline with newline + padding
    while ((pos = output.find('\n', pos)) != std::string::npos) {
        if (pos + 1 >= output.length()) {
            break;
        }
        output.insert(pos + 1, padding);
        pos += (spaces + 1);
    }
    return output;
}

void CsWriteLog(const std::string& type, const std::string& content) {
    std::lock_guard<std::recursive_mutex> lock(g_cs_log_mutex);

    if (!AndroidBaseWrapper::GetWrapper().GetBoolProperty(kIsHalLogEnabled.data(), false)) {
        return;
    }

    if (g_cs_log_filename.empty()) {
        LOG(WARNING) << "CsWriteLog called without active log file.";
        return;
    }

    std::fstream log_file(g_cs_log_filename, std::ios::in | std::ios::out | std::ios::ate);
    if (!log_file.is_open()) {
        LOG(ERROR) << "Failed to open CS log file for appending: " << g_cs_log_filename;
        return;
    }

    // Capture the monotonic time in nanoseconds
    auto now_steady = std::chrono::steady_clock::now();
    auto timestampSinceBootNanos =
            std::chrono::duration_cast<std::chrono::nanoseconds>(now_steady.time_since_epoch())
                    .count();

    // If not the first entry, overwrite the previous closing bracket ']' and add
    // a comma
    if (!g_cs_log_first_entry) {
        // Check ensuring file is not empty not strictly necessary if logic flows
        // correctly from CsCreateNewLog
        log_file.seekp(-1, std::ios::end);
        log_file << ",\n";
    }
    g_cs_log_first_entry = false;

    // Indent the content by 4 spaces to align with the JSON object structure
    std::string padded_content = PadString(content, 4, false);

    log_file << "  {\n";
    log_file << "    \"type\": \"" << type << "\",\n";
    log_file << "    \"timestamp\": \"" << GetTimestamp() << "\",\n";
    log_file << "    \"timestampSinceBootNanos\": " << timestampSinceBootNanos << ",\n";
    log_file << "    \"content\": " << padded_content << "\n";
    log_file << "  }\n";
    log_file << "]";  // Always append the closing bracket
    log_file.close();
}

// --- JSON Serializers ---

std::string ToJson(const Config& c) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"modeType\": " << (int)c.modeType << ",\n";
    ss << "  \"subModeType\": \"" << toString(c.subModeType) << "\",\n";
    ss << "  \"rttType\": \"" << toString(c.rttType) << "\",\n";
    ss << "  \"channelMap\": \"0x" << ToHex(c.channelMap) << "\",\n";
    ss << "  \"minMainModeSteps\": " << c.minMainModeSteps << ",\n";
    ss << "  \"maxMainModeSteps\": " << c.maxMainModeSteps << ",\n";
    ss << "  \"mainModeRepetition\": " << (int)c.mainModeRepetition << ",\n";
    ss << "  \"mode0Steps\": " << (int)c.mode0Steps << ",\n";
    ss << "  \"role\": \"" << toString(c.role) << "\",\n";
    ss << "  \"csSyncPhyType\": \"" << toString(c.csSyncPhyType) << "\",\n";
    ss << "  \"channelSelectionType\": \"" << toString(c.channelSelectionType) << "\",\n";
    ss << "  \"ch3cShapeType\": \"" << toString(c.ch3cShapeType) << "\",\n";
    ss << "  \"ch3cJump\": " << (int)c.ch3cJump << ",\n";
    ss << "  \"channelMapRepetition\": " << c.channelMapRepetition << ",\n";
    ss << "  \"tIp1TimeUs\": " << c.tIp1TimeUs << ",\n";
    ss << "  \"tIp2TimeUs\": " << c.tIp2TimeUs << ",\n";
    ss << "  \"tFcsTimeUs\": " << c.tFcsTimeUs << ",\n";
    ss << "  \"tPmTimeUs\": " << (int)c.tPmTimeUs << ",\n";
    ss << "  \"tSwTimeUsSupportedByLocal\": " << (int)c.tSwTimeUsSupportedByLocal << ",\n";
    ss << "  \"tSwTimeUsSupportedByRemote\": " << (int)c.tSwTimeUsSupportedByRemote << ",\n";
    ss << "  \"bleConnInterval\": " << c.bleConnInterval << "\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const ProcedureEnableConfig& p) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"toneAntennaConfigSelection\": " << (int)p.toneAntennaConfigSelection << ",\n";
    ss << "  \"subeventLenUs\": " << p.subeventLenUs << ",\n";
    ss << "  \"subeventsPerEvent\": " << (int)p.subeventsPerEvent << ",\n";
    ss << "  \"subeventInterval\": " << p.subeventInterval << ",\n";
    ss << "  \"eventInterval\": " << p.eventInterval << ",\n";
    ss << "  \"procedureInterval\": " << p.procedureInterval << ",\n";
    ss << "  \"procedureCount\": " << p.procedureCount << ",\n";
    ss << "  \"maxProcedureLen\": " << p.maxProcedureLen << "\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const BluetoothChannelSoundingParameters& p) {
    std::stringstream ss;
    // Use PadString to nest the inner config object properly
    std::string config_json = PadString(ToJson(p.config), 2, false);

    ss << "{\n";
    ss << "  \"sessionType\": \"" << toString(p.sessionType) << "\",\n";
    ss << "  \"aclHandle\": " << p.aclHandle << ",\n";
    ss << "  \"l2capCid\": " << p.l2capCid << ",\n";
    ss << "  \"realTimeProcedureDataAttHandle\": " << p.realTimeProcedureDataAttHandle << ",\n";
    ss << "  \"role\": \"" << toString(p.role) << "\",\n";
    ss << "  \"localSupportsSoundingPhaseBasedRanging\": "
       << (p.localSupportsSoundingPhaseBasedRanging ? "true" : "false") << ",\n";
    ss << "  \"remoteSupportsSoundingPhaseBaseRanging\": "
       << (p.remoteSupportsSoundingPhaseBaseRanging ? "true" : "false") << ",\n";
    ss << "  \"config\": " << config_json << ",\n";
    ss << "  \"locationType\": \"" << toString(p.locationType) << "\",\n";
    ss << "  \"sightType\": \"" << toString(p.sightType) << "\"\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const PctIQSample& sample) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"iSample\": " << sample.iSample << ",\n";
    ss << "  \"qSample\": " << sample.qSample << "\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const RttToaTodData& data) {
    std::stringstream ss;
    ss << "{\n";
    auto tag = data.getTag();
    if (tag == RttToaTodData::toaTodInitiator) {
        ss << "  \"toaTodInitiator\": " << data.get<RttToaTodData::toaTodInitiator>() << "\n";
    } else if (tag == RttToaTodData::todToaReflector) {
        ss << "  \"todToaReflector\": " << data.get<RttToaTodData::todToaReflector>() << "\n";
    } else {
        ss << "  \"error\": \"unknown_tag\"\n";
    }
    ss << "}";
    return ss.str();
}

std::string ToJson(const ModeZeroData& data) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"packetQuality\": " << (int)data.packetQuality << ",\n";
    ss << "  \"packetRssiDbm\": " << (int)data.packetRssiDbm << ",\n";
    ss << "  \"packetAntenna\": " << (int)data.packetAntenna << ",\n";
    ss << "  \"initiatorMeasuredFreqOffset\": " << data.initiatorMeasuredFreqOffset << "\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const ModeOneData& data) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"packetQuality\": " << (int)data.packetQuality << ",\n";
    ss << "  \"packetNadm\": \"" << toString(data.packetNadm) << "\",\n";
    ss << "  \"packetRssiDbm\": " << (int)data.packetRssiDbm << ",\n";
    ss << "  \"packetAntenna\": " << (int)data.packetAntenna << ",\n";

    ss << "  \"rttToaTodData\": " << PadString(ToJson(data.rttToaTodData), 2, false) << ",\n";

    if (data.packetPct1.has_value()) {
        ss << "  \"packetPct1\": " << PadString(ToJson(data.packetPct1.value()), 2, false) << ",\n";
    } else {
        ss << "  \"packetPct1\": null,\n";
    }

    if (data.packetPct2.has_value()) {
        ss << "  \"packetPct2\": " << PadString(ToJson(data.packetPct2.value()), 2) << "\n";
    } else {
        ss << "  \"packetPct2\": null\n";
    }
    ss << "}";
    return ss.str();
}

std::string ToJson(const ModeTwoData& data) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"antennaPermutationIndex\": " << (int)data.antennaPermutationIndex << ",\n";

    ss << "  \"tonePctIQSamples\": [\n";
    for (size_t i = 0; i < data.tonePctIQSamples.size(); ++i) {
        if (i > 0) ss << ",\n";
        ss << PadString(ToJson(data.tonePctIQSamples[i]), 4);
    }
    ss << "\n  ],\n";

    ss << "  \"toneQualityIndicators\": [";
    for (size_t i = 0; i < data.toneQualityIndicators.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << (int)data.toneQualityIndicators[i];
    }
    ss << "]\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const ModeThreeData& data) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"modeOneData\": " << PadString(ToJson(data.modeOneData), 2, false) << ",\n";
    ss << "  \"modeTwoData\": " << PadString(ToJson(data.modeTwoData), 2, false) << "\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const StepData& s) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"stepChannel\": " << (int)s.stepChannel << ",\n";
    ss << "  \"stepMode\": \"" << toString(s.stepMode) << "\",\n";

    std::string modeDataJson = "{}";
    // Switch on stepMode to decide which union member to access
    switch (s.stepMode) {
        case ModeType::ZERO:
            modeDataJson = ToJson(s.stepModeData.get<ModeData::modeZeroData>());
            break;
        case ModeType::ONE:
            modeDataJson = ToJson(s.stepModeData.get<ModeData::modeOneData>());
            break;
        case ModeType::TWO:
            modeDataJson = ToJson(s.stepModeData.get<ModeData::modeTwoData>());
            break;
        case ModeType::THREE:
            modeDataJson = ToJson(s.stepModeData.get<ModeData::modeThreeData>());
            break;
        default:
            break;
    }

    ss << "  \"stepModeData\": " << PadString(modeDataJson, 2, false) << "\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const SubeventResultData& d) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"startAclConnEventCounter\": " << d.startAclConnEventCounter << ",\n";
    ss << "  \"frequencyCompensation\": " << d.frequencyCompensation << ",\n";
    ss << "  \"referencePowerLevelDbm\": " << (int)d.referencePowerLevelDbm << ",\n";
    ss << "  \"numAntennaPaths\": " << (int)d.numAntennaPaths << ",\n";
    ss << "  \"subeventAbortReason\": \"" << toString(d.subeventAbortReason) << "\",\n";
    ss << "  \"timestampNanos\": " << d.timestampNanos << ",\n";

    ss << "  \"stepData\": [\n";
    for (size_t i = 0; i < d.stepData.size(); ++i) {
        if (i > 0) ss << ",\n";
        ss << PadString(ToJson(d.stepData[i]), 4);
    }
    ss << "\n  ]\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const ChannelSoundingProcedureData& d) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"procedureCounter\": " << d.procedureCounter << ",\n";
    ss << "  \"procedureSequence\": " << d.procedureSequence << ",\n";
    ss << "  \"initiatorSelectedTxPower\": " << (int)d.initiatorSelectedTxPower << ",\n";
    ss << "  \"reflectorSelectedTxPower\": " << (int)d.reflectorSelectedTxPower << ",\n";

    ss << "  \"initiatorSubeventResultData\": [\n";
    for (size_t i = 0; i < d.initiatorSubeventResultData.size(); ++i) {
        if (i > 0) ss << ",\n";
        // Pad the inner object by 4 spaces (2 for array, 2 for object)
        ss << PadString(ToJson(d.initiatorSubeventResultData[i]), 4);
    }
    ss << "\n  ],\n";

    ss << "  \"initiatorProcedureAbortReason\": \"" << toString(d.initiatorProcedureAbortReason)
       << "\",\n";

    ss << "  \"reflectorSubeventResultData\": [\n";
    for (size_t i = 0; i < d.reflectorSubeventResultData.size(); ++i) {
        if (i > 0) ss << ",\n";
        ss << PadString(ToJson(d.reflectorSubeventResultData[i]), 4);
    }
    ss << "\n  ],\n";

    ss << "  \"reflectorProcedureAbortReason\": \"" << toString(d.reflectorProcedureAbortReason)
       << "\"\n";
    ss << "}";
    return ss.str();
}

std::string ToJson(const RangingResult& r) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"resultMeters\": " << r.resultMeters << ",\n";
    ss << "  \"errorMeters\": " << r.errorMeters << ",\n";
    ss << "  \"azimuthDegrees\": " << r.azimuthDegrees << ",\n";
    ss << "  \"errorAzimuthDegrees\": " << r.errorAzimuthDegrees << ",\n";
    ss << "  \"altitudeDegrees\": " << r.altitudeDegrees << ",\n";
    ss << "  \"errorAltitudeDegrees\": " << r.errorAltitudeDegrees << ",\n";
    ss << "  \"delaySpreadMeters\": " << r.delaySpreadMeters << ",\n";
    ss << "  \"confidenceLevel\": " << (int)r.confidenceLevel << ",\n";
    ss << "  \"detectedAttackLevel\": \"" << toString(r.detectedAttackLevel) << "\",\n";
    ss << "  \"velocityMetersPerSecond\": " << r.velocityMetersPerSecond << ",\n";

    if (r.vendorSpecificCsRangingResultsData.has_value()) {
        ss << "  \"vendorSpecificCsRangingResultsData\": \""
           << ToHex(r.vendorSpecificCsRangingResultsData.value()) << "\",\n";
    } else {
        ss << "  \"vendorSpecificCsRangingResultsData\": null,\n";
    }

    ss << "  \"rangingResultStatus\": \"" << toString(r.rangingResultStatus) << "\",\n";
    ss << "  \"timestampNanos\": " << r.timestampNanos << "\n";
    ss << "}";
    return ss.str();
}

}  // namespace

BluetoothChannelSoundingSessionV2::BluetoothChannelSoundingSessionV2(
        std::shared_ptr<IBluetoothChannelSoundingSessionCallback> callback, Reason /* reason */)
    : distance_estimator_(ChannelSoundingDistanceEstimatorInterface::Create()) {
    callback_ = callback;
}

ScopedAStatus BluetoothChannelSoundingSessionV2::getVendorSpecificReplies(
        std::optional<std::vector<std::optional<VendorSpecificData>>>* _aidl_return) {
    LOG(INFO) << __func__;

    if (!uuid_matched_) {
        LOG(INFO) << ": UUID doesn't matched, ignore.";
        return ScopedAStatus::ok();
    }

    *_aidl_return = std::make_optional<std::vector<std::optional<VendorSpecificData>>>();
    VendorSpecificData capability;
    capability.characteristicUuid = kUuidSpecialRangingSettingCapability;
    capability.opaqueValue = {kDataTypeReply, 0x00, 0x00, 0x00, 0x00};
    (*_aidl_return)->push_back(capability);

    uint8_t enable_inline_pct =
            enable_fake_notification_ ? kCommandValueEnable : kCommandValueIgnore;

    // Event mask used by `Set event mask for connection` command. Set all event
    // bits to 0 — responder should ignore this if unsupported or inline PCT is
    // not enabled.
    constexpr uint32_t kEventMask = 0x00000000;

    uint8_t enable_mode_0_channel_map =
            enable_mode_0_channel_map_ ? kCommandValueEnable : kCommandValueIgnore;

    VendorSpecificData command;
    command.characteristicUuid = kUuidSpecialRangingSettingCommand;
    command.opaqueValue = {kDataTypeReply,
                           enable_inline_pct,
                           static_cast<uint8_t>((kEventMask >> 24) & 0xFF),
                           static_cast<uint8_t>((kEventMask >> 16) & 0xFF),
                           static_cast<uint8_t>((kEventMask >> 8) & 0xFF),
                           static_cast<uint8_t>((kEventMask) & 0xFF),
                           enable_mode_0_channel_map};
    (*_aidl_return)->push_back(command);

    for (auto& data : _aidl_return->value()) {
        LOG(INFO) << "uuid:" << ToHex(data->characteristicUuid)
                  << ", data:" << ToHex(data->opaqueValue);
    }

    return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::getSupportedResultTypes(
        std::vector<ResultType>* _aidl_return) {
    std::vector<ResultType> supported_result_types = {ResultType::RESULT_METERS};
    *_aidl_return = supported_result_types;

    return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::isAbortedProcedureRequired(bool* _aidl_return) {
    *_aidl_return = false;

    return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::writeProcedureData(
        const ChannelSoundingProcedureData& in_procedureData) {
    CsWriteLog("ChannelSoundingProcedureData", ToJson(in_procedureData));
    RangingResult ranging_result;
    distance_estimator_->ResetVariables();
    ranging_result.resultMeters = distance_estimator_->EstimateDistance(in_procedureData);
    ranging_result.confidenceLevel = distance_estimator_->GetConfidenceLevel() * 100;
    ranging_result.velocityMetersPerSecond = distance_estimator_->GetVelocity();

    if (!in_procedureData.initiatorSubeventResultData.empty()) {
        ranging_result.timestampNanos =
                in_procedureData.initiatorSubeventResultData[0].timestampNanos;
    }
    callback_->onResult(ranging_result);
    CsWriteLog("RangingResult", ToJson(ranging_result));
    return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::writeRawData(
        const ChannelSoudingRawData& in_rawData) {
    if (in_rawData.stepChannels.empty()) {
        LOG(WARNING) << __func__ << " in_rawData.stepChannels is empty, skip";
        return ScopedAStatus::ok();
    }

    RangingResult ranging_result;
    distance_estimator_->ResetVariables();
    ranging_result.resultMeters = distance_estimator_->EstimateDistance(in_rawData);
    ranging_result.confidenceLevel = distance_estimator_->GetConfidenceLevel() * 100;
    callback_->onResult(ranging_result);
    return ScopedAStatus::ok();
}

ScopedAStatus BluetoothChannelSoundingSessionV2::close(Reason in_reason) {
    callback_->onClose(in_reason);

    return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::updateChannelSoundingConfig(
        const Config& in_config) {
    CsWriteLog("Config", ToJson(in_config));
    distance_estimator_->UpdateChannelSoundingConfig(in_config);
    return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::updateProcedureEnableConfig(
        const ProcedureEnableConfig& in_procedureEnableConfig) {
    CsWriteLog("ProcedureEnableConfig", ToJson(in_procedureEnableConfig));
    distance_estimator_->UpdateProcedureEnableConfig(in_procedureEnableConfig);
    return ScopedAStatus::ok();
};

ScopedAStatus BluetoothChannelSoundingSessionV2::updateBleConnInterval(
        [[maybe_unused]] int in_bleConnInterval) {
    return ScopedAStatus::ok();
};

void BluetoothChannelSoundingSessionV2::HandleVendorSpecificData(
        const std::optional<std::vector<std::optional<VendorSpecificData>>> vendor_specific_data) {
    uuid_matched_ = IsUuidMatched(vendor_specific_data);
    if (!uuid_matched_) {
        return;
    }

    auto uuid0 = vendor_specific_data.value()[0];
    uint8_t vendor_specific_data_byte_1 = GetUintProperty(
            Property::kChannelSoundingVendorSpecificFirstDataByte, uuid0.value().opaqueValue[1]);
    LOG(INFO) << __func__ << ": vendor_specific_data_byte_1: " << vendor_specific_data_byte_1;

    if ((vendor_specific_data_byte_1 & static_cast<uint8_t>(CsFeature::kInlinePct)) != 0) {
        LOG(INFO) << __func__ << ": Support 1-side PCT.";
        enable_fake_notification_ = true;
    } else {
        LOG(INFO) << __func__ << ": Do not support Inline PCT.";
        enable_fake_notification_ = false;
    }

    distance_estimator_->SetInlinePCT(enable_fake_notification_);

    if ((vendor_specific_data_byte_1 & static_cast<uint8_t>(CsFeature::kMode0ChannelMap)) != 0) {
        LOG(INFO) << __func__ << ": Support mode 0 Channel Map.";
        enable_mode_0_channel_map_ = true;
    } else {
        LOG(INFO) << __func__ << ": Do not support mode 0 Channel Map.";
        enable_mode_0_channel_map_ = false;
    }
};

void BluetoothChannelSoundingSessionV2::CsCreateNewLog(
        const BluetoothChannelSoundingParameters& in_params) {
    std::lock_guard<std::recursive_mutex> lock(g_cs_log_mutex);
    if (!AndroidBaseWrapper::GetWrapper().GetBoolProperty(kIsHalLogEnabled.data(), false)) {
        LOG(INFO) << "HAL LOG not enabled";
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");

    g_cs_log_filename = "/data/vendor/bluetooth/cs_log_halv2_" + ss.str() + ".json";
    g_cs_log_first_entry = true;

    // Create file and write the start of the JSON array
    std::ofstream log_file(g_cs_log_filename, std::ios::out | std::ios::trunc);
    if (log_file.is_open()) {
        log_file << "[\n";
        log_file.close();
        LOG(INFO) << "Created new CS log file: " << g_cs_log_filename;
    } else {
        LOG(ERROR) << "Failed to create CS log file: " << g_cs_log_filename;
        // Reset filename so WriteLog doesn't try to write to nowhere
        g_cs_log_filename.clear();
    }

    CsWriteLog("BluetoothChannelSoundingParameters", ToJson(in_params));
}

bool BluetoothChannelSoundingSessionV2::ShouldEnableFakeNotification() {
    return enable_fake_notification_;
};

bool BluetoothChannelSoundingSessionV2::ShouldEnableMode0ChannelMap() {
    return enable_mode_0_channel_map_;
};

}  // namespace bluetooth_hal::extensions::cs
