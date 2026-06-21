/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <optional>
#include <set>

#include "aidl/android/hardware/bluetooth/audio/ChannelMode.h"
#include "aidl/android/hardware/bluetooth/audio/CodecId.h"
#include "aidl/android/hardware/bluetooth/audio/CodecInfo.h"
#include "aidl/android/hardware/bluetooth/audio/ConfigurationFlags.h"
#include "aidl_android_hardware_bluetooth_audio_setting_enums.h"
#define LOG_TAG "BTAudioCodecsProviderAidl"

#include "BluetoothLeAudioCodecsProvider.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

void BluetoothLeAudioCodecsProvider::SetLeAudioOffloadSettingForTesting(
        std::optional<setting::LeAudioOffloadSetting> setting) {
    if (setting) {
        le_audio_offload_setting_.emplace(std::move(*setting));
    } else {
        le_audio_offload_setting_.reset();
    }
}

static const char* kLeAudioCodecCapabilitiesFile = "/vendor/etc/le_audio_codec_capabilities.xml";

static const AudioLocation kStereoAudio =
        static_cast<AudioLocation>(static_cast<uint8_t>(AudioLocation::FRONT_LEFT) |
                                   static_cast<uint8_t>(AudioLocation::FRONT_RIGHT));
static const AudioLocation kMonoAudio = AudioLocation::UNKNOWN;

// TODO: reuse from utils/aidl_session/BluetoothAudioType.h
/* Vendor codec ID */
constexpr uint16_t kLeAudioVendorCompanyIdGoogle = 0x00E0;
constexpr uint16_t kLeAudioVendorCodecIdOpus = 0x0001;

const CodecId::Vendor opus_codec{
        .codecId = kLeAudioVendorCodecIdOpus,
        .id = kLeAudioVendorCompanyIdGoogle,
};

bool BluetoothLeAudioCodecsProvider::LoadCapabilitiesFile() {
    if (le_audio_offload_setting_.has_value()) return true;

    if (auto result = setting::readLeAudioOffloadSetting(kLeAudioCodecCapabilitiesFile)) {
        le_audio_offload_setting_.emplace(std::move(*result));
        return true;
    }

    LOG(ERROR) << __func__ << ": Failed to read " << kLeAudioCodecCapabilitiesFile;
    return false;
}

void add_flag(CodecInfo& codec_info, int32_t bitmask) {
    auto& transport = codec_info.transport.get<CodecInfo::Transport::Tag::leAudio>();
    if (!transport.flags.has_value()) transport.flags = ConfigurationFlags();
    transport.flags->bitmask |= bitmask;
}

// Compare 2 codec info to see if they are equal.
// Currently only compare bitdepth, frameDurationUs and samplingFrequencyHz
bool is_equal(CodecInfo& codec_info_a, CodecInfo& codec_info_b) {
    auto& transport_a = codec_info_a.transport.get<CodecInfo::Transport::Tag::leAudio>();
    auto& transport_b = codec_info_b.transport.get<CodecInfo::Transport::Tag::leAudio>();
    return codec_info_a.name == codec_info_b.name && transport_a.bitdepth == transport_b.bitdepth &&
           transport_a.frameDurationUs == transport_b.frameDurationUs &&
           transport_a.samplingFrequencyHz == transport_b.samplingFrequencyHz;
}

std::unordered_map<SessionType, std::vector<CodecInfo>>
BluetoothLeAudioCodecsProvider::GetLeAudioCodecInfo() {
    // Load from previous storage if present
    if (!session_codecs_info_.empty()) return session_codecs_info_;

    if (!LoadCapabilitiesFile() || !ParseCapabilitiesToCache()) return {};

    // Map each configuration into a CodecInfo
    std::unordered_map<std::string, CodecInfo> codecs_info;

    for (auto& [config_name, configuration] : supported_configuration_) {
        // Getting informations from codecConfig and strategyConfig
        const auto codec_config_name = configuration.getCodecConfiguration();
        const auto codec_config_iter = supported_codec_configuration_.find(codec_config_name);
        if (codec_config_iter == supported_codec_configuration_.end()) continue;

        const auto strategy_config_name = configuration.getStrategyConfiguration();
        const auto strategy_config_iter =
                supported_strategy_configuration_.find(strategy_config_name);
        if (strategy_config_iter == supported_strategy_configuration_.end()) continue;

        codecs_info.try_emplace(config_name);

        const auto& codec_config = codec_config_iter->second;
        // Initiate information
        auto& codec_info = codecs_info[config_name];
        switch (codec_config.getCodec()) {
            case setting::CodecType::LC3:
                codec_info.name = "LC3";
                codec_info.id = CodecId::Core::LC3;
                break;
            case setting::CodecType::OPUS:
                codec_info.name = "OPUS";
                codec_info.id = opus_codec;
                break;
            default:
                codec_info.name = "UNDEFINE";
                codec_info.id = CodecId::Vendor();
                break;
        }

        codec_info.transport = CodecInfo::Transport::make<CodecInfo::Transport::Tag::leAudio>();
        // Mapping codec configuration information
        auto& transport = codec_info.transport.get<CodecInfo::Transport::Tag::leAudio>();
        transport.samplingFrequencyHz.push_back(codec_config.getSamplingFrequency());
        transport.frameDurationUs.push_back(codec_config.getFrameDurationUs());
        // Mapping octetsPerCodecFrame to bitdepth for easier comparison.
        transport.bitdepth.push_back(codec_config.getOctetsPerCodecFrame());

        const auto& strategy_config = strategy_config_iter->second;
        const auto channel_count = strategy_config.getChannelCount();
        if (strategy_config.hasAudioLocation()) {
            switch (strategy_config.getAudioLocation()) {
                case setting::AudioLocation::MONO:
                    if (channel_count == 1)
                        transport.channelMode.push_back(ChannelMode::MONO);
                    else
                        transport.channelMode.push_back(ChannelMode::DUALMONO);
                    break;
                case setting::AudioLocation::STEREO:
                    transport.channelMode.push_back(ChannelMode::STEREO);
                    break;
                default:
                    transport.channelMode.push_back(ChannelMode::UNKNOWN);
                    break;
            }
        } else if (strategy_config.hasAudioChannelAllocation()) {
            auto count = std::bitset<32>(strategy_config.getAudioChannelAllocation()).count();
            if (count <= 1) {
                if (channel_count == 1)
                    transport.channelMode.push_back(ChannelMode::MONO);
                else
                    transport.channelMode.push_back(ChannelMode::DUALMONO);
            } else if (count == 2) {
                transport.channelMode.push_back(ChannelMode::STEREO);
            } else {
                transport.channelMode.push_back(ChannelMode::UNKNOWN);
            }
        } else {
            transport.channelMode.push_back(ChannelMode::UNKNOWN);
        }

        // Add low latency support by default
        add_flag(codec_info, ConfigurationFlags::LOW_LATENCY);
    }

    std::set<std::string> encoding_config, decoding_config, broadcast_config;
    for (auto& scenario : supported_scenarios_) {
        CodecInfo* encode_info_ptr = nullptr;
        CodecInfo* decode_info_ptr = nullptr;

        // Goes through every scenario, deduplicate configuration, skip the invalid
        // config references (e.g. the "invalid" entries in the xml file).
        if (scenario.hasEncode()) {
            const auto& name = scenario.getEncode();
            auto it = codecs_info.find(name);
            if (it != codecs_info.end()) {
                encoding_config.insert(name);
                encode_info_ptr = &it->second;
            }
        }

        if (scenario.hasDecode()) {
            const auto& name = scenario.getDecode();
            auto it = codecs_info.find(name);
            if (it != codecs_info.end()) {
                decoding_config.insert(name);
                decode_info_ptr = &it->second;
            }
        }

        if (scenario.hasBroadcast()) {
            const auto& name = scenario.getBroadcast();
            auto it = codecs_info.find(name);
            if (it != codecs_info.end()) {
                broadcast_config.insert(name);
            }
        }

        if (encode_info_ptr && decode_info_ptr) {
            if (!is_equal(*encode_info_ptr, *decode_info_ptr)) {
                add_flag(*encode_info_ptr, ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS);
                add_flag(*decode_info_ptr, ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS);
            }
        }
    }

    session_codecs_info_.clear();

    auto fill_session = [&](SessionType type, const std::set<std::string>& configs) {
        if (configs.empty()) return;
        auto& target_vec = session_codecs_info_[type];
        target_vec.reserve(configs.size());
        for (const auto& name : configs) {
            target_vec.push_back(codecs_info.at(name));
        }
    };

    fill_session(SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH, encoding_config);
    fill_session(SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH, decoding_config);
    fill_session(SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH,
                 broadcast_config);

    return session_codecs_info_;
}

std::vector<LeAudioCodecCapabilitiesSetting>
BluetoothLeAudioCodecsProvider::GetLeAudioCodecCapabilities() {
    if (!codec_capabilities_setting_.empty()) {
        return codec_capabilities_setting_;
    }

    if (!LoadCapabilitiesFile() || !ParseCapabilitiesToCache()) {
        LOG(ERROR) << __func__ << ": Failed to parse LE audio offload settings file.";
        return {};
    }

    codec_capabilities_setting_ = ComposeLeAudioCodecCapabilities();

    return codec_capabilities_setting_;
}

void BluetoothLeAudioCodecsProvider::ClearCache() {
    codec_capabilities_setting_.clear();
    supported_configuration_.clear();
    supported_codec_configuration_.clear();
    supported_strategy_configuration_.clear();
    session_codecs_info_.clear();
    supported_scenarios_.clear();
}

template <typename ListGetter, typename ItemGetter, typename Validator, typename Inserter>
void ForEachValidItem(ListGetter list_getter, ItemGetter item_getter, Validator validator,
                      Inserter inserter) {
    for (const auto& list_wrapper : list_getter()) {
        for (const auto& item : item_getter(list_wrapper)) {
            if (validator(item)) {
                inserter(item);
            }
        }
    }
}

bool BluetoothLeAudioCodecsProvider::UpdateScenariosToCache() {
    if (le_audio_offload_setting_->hasScenarioList()) {
        ForEachValidItem([&]() { return le_audio_offload_setting_->getScenarioList(); },
                         [](const auto& list) { return list.getScenario(); }, IsValidScenario,
                         [&](const auto& item) { supported_scenarios_.push_back(item); });
    }
    if (supported_scenarios_.empty()) {
        LOG(ERROR) << __func__ << ": No scenarios in " << kLeAudioCodecCapabilitiesFile;
        return false;
    }
    return true;
}

bool BluetoothLeAudioCodecsProvider::UpdateConfigurationsToCache() {
    if (le_audio_offload_setting_->hasConfigurationList()) {
        ForEachValidItem(
                [&]() { return le_audio_offload_setting_->getConfigurationList(); },
                [](const auto& list) { return list.getConfiguration(); }, IsValidConfiguration,
                [&](const auto& item) { supported_configuration_.emplace(item.getName(), item); });
    }
    if (supported_configuration_.empty()) {
        LOG(ERROR) << __func__ << ": No configurations in " << kLeAudioCodecCapabilitiesFile;
        return false;
    }
    return true;
}

bool BluetoothLeAudioCodecsProvider::UpdateCodecConfigurationsToCache() {
    if (le_audio_offload_setting_->hasCodecConfigurationList()) {
        ForEachValidItem([&]() { return le_audio_offload_setting_->getCodecConfigurationList(); },
                         [](const auto& list) { return list.getCodecConfiguration(); },
                         IsValidCodecConfiguration,
                         [&](const auto& item) {
                             supported_codec_configuration_.emplace(item.getName(), item);
                         });
    }
    if (supported_codec_configuration_.empty()) {
        LOG(ERROR) << __func__ << ": No codec configurations in " << kLeAudioCodecCapabilitiesFile;
        return false;
    }
    return true;
}

bool BluetoothLeAudioCodecsProvider::UpdateStrategyConfigurationsToCache() {
    if (le_audio_offload_setting_->hasStrategyConfigurationList()) {
        ForEachValidItem(
                [&]() { return le_audio_offload_setting_->getStrategyConfigurationList(); },
                [](const auto& list) { return list.getStrategyConfiguration(); },
                IsValidStrategyConfiguration,
                [&](const auto& item) {
                    supported_strategy_configuration_.emplace(item.getName(), item);
                });
    }
    if (supported_strategy_configuration_.empty()) {
        LOG(ERROR) << __func__ << ": No strategy configurations in "
                   << kLeAudioCodecCapabilitiesFile;
        return false;
    }
    return true;
}

bool BluetoothLeAudioCodecsProvider::ParseCapabilitiesToCache() {
    ClearCache();

    if (!UpdateScenariosToCache() || !UpdateConfigurationsToCache() ||
        !UpdateCodecConfigurationsToCache() || !UpdateStrategyConfigurationsToCache()) {
        return false;
    }

    return true;
}

std::vector<LeAudioCodecCapabilitiesSetting>
BluetoothLeAudioCodecsProvider::ComposeLeAudioCodecCapabilities() {
    std::vector<LeAudioCodecCapabilitiesSetting> le_audio_codec_capabilities;
    for (const auto& scenario : supported_scenarios_) {
        UnicastCapability unicast_encode_capability = {.codecType = CodecType::UNKNOWN};
        if (scenario.hasEncode()) {
            unicast_encode_capability = GetUnicastCapability(scenario.getEncode());
            LOG(INFO) << __func__
                      << ": Unicast capability encode = " << unicast_encode_capability.toString();
        }

        UnicastCapability unicast_decode_capability = {.codecType = CodecType::UNKNOWN};
        if (scenario.hasDecode()) {
            unicast_decode_capability = GetUnicastCapability(scenario.getDecode());
            LOG(INFO) << __func__
                      << ": Unicast capability decode = " << unicast_decode_capability.toString();
        }

        BroadcastCapability broadcast_capability = {.codecType = CodecType::UNKNOWN};
        if (scenario.hasBroadcast()) {
            broadcast_capability = GetBroadcastCapability(scenario.getBroadcast());
            LOG(INFO) << __func__ << ": Broadcast capability = " << broadcast_capability.toString();
        }

        // At least one capability should be valid
        if (unicast_encode_capability.codecType == CodecType::UNKNOWN &&
            unicast_decode_capability.codecType == CodecType::UNKNOWN &&
            broadcast_capability.codecType == CodecType::UNKNOWN) {
            LOG(ERROR) << __func__ << ": None of the capability is valid.";
            continue;
        }

        le_audio_codec_capabilities.push_back({.unicastEncodeCapability = unicast_encode_capability,
                                               .unicastDecodeCapability = unicast_decode_capability,
                                               .broadcastCapability = broadcast_capability});
    }
    return le_audio_codec_capabilities;
}

UnicastCapability BluetoothLeAudioCodecsProvider::GetUnicastCapability(
        const std::string& coding_direction) {
    if (coding_direction == "invalid") {
        return {.codecType = CodecType::UNKNOWN};
    }

    auto configuration_iter = supported_configuration_.find(coding_direction);
    if (configuration_iter == supported_configuration_.end()) {
        return {.codecType = CodecType::UNKNOWN};
    }

    auto codec_configuration_iter =
            supported_codec_configuration_.find(configuration_iter->second.getCodecConfiguration());
    if (codec_configuration_iter == supported_codec_configuration_.end()) {
        return {.codecType = CodecType::UNKNOWN};
    }

    auto strategy_configuration_iter = supported_strategy_configuration_.find(
            configuration_iter->second.getStrategyConfiguration());
    if (strategy_configuration_iter == supported_strategy_configuration_.end()) {
        return {.codecType = CodecType::UNKNOWN};
    }

    // Populate audio location
    AudioLocation audio_location = AudioLocation::UNKNOWN;
    if (strategy_configuration_iter->second.hasAudioLocation()) {
        audio_location = GetAudioLocation(strategy_configuration_iter->second.getAudioLocation());
    }

    // Populate audio channel allocation
    std::optional<CodecSpecificConfigurationLtv::AudioChannelAllocation> audio_channel_allocation =
            std::nullopt;
    if (strategy_configuration_iter->second.hasAudioChannelAllocation()) {
        LOG(INFO) << __func__ << ": has allocation";
        CodecSpecificConfigurationLtv::AudioChannelAllocation tmp;
        tmp.bitmask = strategy_configuration_iter->second.getAudioChannelAllocation();
        audio_channel_allocation = tmp;
    }

    CodecType codec_type = GetCodecType(codec_configuration_iter->second.getCodec());
    if (codec_type == CodecType::LC3) {
        return ComposeUnicastCapability(codec_type, audio_location, audio_channel_allocation,
                                        strategy_configuration_iter->second.getConnectedDevice(),
                                        strategy_configuration_iter->second.getChannelCount(),
                                        ComposeLc3Capability(codec_configuration_iter->second));
    } else if (codec_type == CodecType::APTX_ADAPTIVE_LE ||
               codec_type == CodecType::APTX_ADAPTIVE_LEX) {
        return ComposeUnicastCapability(
                codec_type, audio_location, audio_channel_allocation,
                strategy_configuration_iter->second.getConnectedDevice(),
                strategy_configuration_iter->second.getChannelCount(),
                ComposeAptxAdaptiveLeCapability(codec_configuration_iter->second));
    } else if (codec_type == CodecType::OPUS) {
        return ComposeUnicastCapability(codec_type, audio_location, audio_channel_allocation,
                                        strategy_configuration_iter->second.getConnectedDevice(),
                                        strategy_configuration_iter->second.getChannelCount(),
                                        ComposeOpusCapability(codec_configuration_iter->second));
    }
    return {.codecType = CodecType::UNKNOWN};
}

BroadcastCapability BluetoothLeAudioCodecsProvider::GetBroadcastCapability(
        const std::string& coding_direction) {
    if (coding_direction == "invalid") {
        return {.codecType = CodecType::UNKNOWN};
    }

    auto configuration_iter = supported_configuration_.find(coding_direction);
    if (configuration_iter == supported_configuration_.end()) {
        return {.codecType = CodecType::UNKNOWN};
    }

    auto codec_configuration_iter =
            supported_codec_configuration_.find(configuration_iter->second.getCodecConfiguration());
    if (codec_configuration_iter == supported_codec_configuration_.end()) {
        return {.codecType = CodecType::UNKNOWN};
    }

    auto strategy_configuration_iter = supported_strategy_configuration_.find(
            configuration_iter->second.getStrategyConfiguration());
    if (strategy_configuration_iter == supported_strategy_configuration_.end()) {
        return {.codecType = CodecType::UNKNOWN};
    }

    CodecType codec_type = GetCodecType(codec_configuration_iter->second.getCodec());
    std::vector<std::optional<Lc3Capabilities>> bcastLc3Cap(
            1, std::optional(ComposeLc3Capability(codec_configuration_iter->second)));

    // Populate audio location
    AudioLocation audio_location = AudioLocation::UNKNOWN;
    if (strategy_configuration_iter->second.hasAudioLocation()) {
        audio_location = GetAudioLocation(strategy_configuration_iter->second.getAudioLocation());
    }

    // Populate audio channel allocation
    std::optional<CodecSpecificConfigurationLtv::AudioChannelAllocation> audio_channel_allocation =
            std::nullopt;
    if (strategy_configuration_iter->second.hasAudioChannelAllocation()) {
        LOG(INFO) << __func__ << ": has allocation";
        CodecSpecificConfigurationLtv::AudioChannelAllocation tmp;
        tmp.bitmask = strategy_configuration_iter->second.getAudioChannelAllocation();
        audio_channel_allocation = tmp;
    }

    if (codec_type == CodecType::LC3) {
        return ComposeBroadcastCapability(codec_type, audio_location, audio_channel_allocation,
                                          strategy_configuration_iter->second.getChannelCount(),
                                          bcastLc3Cap);
    }
    return {.codecType = CodecType::UNKNOWN};
}

template <class T>
BroadcastCapability BluetoothLeAudioCodecsProvider::ComposeBroadcastCapability(
        const CodecType& codec_type, const AudioLocation& audio_location,
        const std::optional<CodecSpecificConfigurationLtv::AudioChannelAllocation>&
                audio_channel_allocation,
        const uint8_t& channel_count, const std::vector<T>& capability) {
    return {.codecType = codec_type,
            .supportedChannel = audio_location,
            .channelCountPerStream = channel_count,
            .leAudioCodecCapabilities = std::optional(capability),
            .audioLocation = audio_channel_allocation};
}

template <class T>
UnicastCapability BluetoothLeAudioCodecsProvider::ComposeUnicastCapability(
        const CodecType& codec_type, const AudioLocation& audio_location,
        const std::optional<CodecSpecificConfigurationLtv::AudioChannelAllocation>&
                audio_channel_allocation,
        const uint8_t& device_cnt, const uint8_t& channel_count, const T& capability) {
    return {
            .codecType = codec_type,
            .supportedChannel = audio_location,
            .deviceCount = device_cnt,
            .channelCountPerDevice = channel_count,
            .leAudioCodecCapabilities = UnicastCapability::LeAudioCodecCapabilities(capability),
            .audioLocation = audio_channel_allocation,
    };
}

Lc3Capabilities BluetoothLeAudioCodecsProvider::ComposeLc3Capability(
        const setting::CodecConfiguration& codec_configuration) {
    return {.samplingFrequencyHz = {codec_configuration.getSamplingFrequency()},
            .frameDurationUs = {codec_configuration.getFrameDurationUs()},
            .octetsPerFrame = {codec_configuration.getOctetsPerCodecFrame()}};
}

AptxAdaptiveLeCapabilities BluetoothLeAudioCodecsProvider::ComposeAptxAdaptiveLeCapability(
        const setting::CodecConfiguration& codec_configuration) {
    return {.samplingFrequencyHz = {codec_configuration.getSamplingFrequency()},
            .frameDurationUs = {codec_configuration.getFrameDurationUs()},
            .octetsPerFrame = {codec_configuration.getOctetsPerCodecFrame()}};
}

OpusCapabilities BluetoothLeAudioCodecsProvider::ComposeOpusCapability(
        const setting::CodecConfiguration& codec_configuration) {
    return {.samplingFrequencyHz = {codec_configuration.getSamplingFrequency()},
            .frameDurationUs = {codec_configuration.getFrameDurationUs()},
            .octetsPerFrame = {codec_configuration.getOctetsPerCodecFrame()}};
}

AudioLocation BluetoothLeAudioCodecsProvider::GetAudioLocation(
        const setting::AudioLocation& audio_location) {
    switch (audio_location) {
        case setting::AudioLocation::MONO:
            return kMonoAudio;
        case setting::AudioLocation::STEREO:
            return kStereoAudio;
        default:
            return AudioLocation::UNKNOWN;
    }
}

CodecType BluetoothLeAudioCodecsProvider::GetCodecType(const setting::CodecType& codec_type) {
    switch (codec_type) {
        case setting::CodecType::LC3:
            return CodecType::LC3;
        case setting::CodecType::APTX_ADAPTIVE_LE:
            return CodecType::APTX_ADAPTIVE_LE;
        case setting::CodecType::APTX_ADAPTIVE_LEX:
            return CodecType::APTX_ADAPTIVE_LEX;
        case setting::CodecType::OPUS:
            return CodecType::OPUS;
        default:
            return CodecType::UNKNOWN;
    }
}

bool BluetoothLeAudioCodecsProvider::IsValidScenario(const setting::Scenario& scenario) {
    return scenario.hasEncode() && scenario.hasDecode();
}

bool BluetoothLeAudioCodecsProvider::IsValidConfiguration(
        const setting::Configuration& configuration) {
    return configuration.hasName() && configuration.hasCodecConfiguration() &&
           configuration.hasStrategyConfiguration();
}

bool BluetoothLeAudioCodecsProvider::IsValidCodecConfiguration(
        const setting::CodecConfiguration& codec_configuration) {
    return codec_configuration.hasName() && codec_configuration.hasCodec() &&
           codec_configuration.hasSamplingFrequency() && codec_configuration.hasFrameDurationUs() &&
           codec_configuration.hasOctetsPerCodecFrame();
}

bool IsValidStereoAudioLocation(const setting::StrategyConfiguration& strategy_configuration) {
    if ((strategy_configuration.getConnectedDevice() == 2 &&
         strategy_configuration.getChannelCount() == 1) ||
        (strategy_configuration.getConnectedDevice() == 1 &&
         strategy_configuration.getChannelCount() == 2)) {
        // Stereo
        // 1. two connected device, one for L one for R
        // 2. one connected device for both L and R
        return true;
    } else if (strategy_configuration.getConnectedDevice() == 0 &&
               strategy_configuration.getChannelCount() == 2) {
        // Broadcast
        return true;
    }
    return false;
}

bool IsValidMonoAudioLocation(const setting::StrategyConfiguration& strategy_configuration) {
    if (strategy_configuration.getConnectedDevice() == 1 &&
        strategy_configuration.getChannelCount() == 1) {
        return true;
    }
    return false;
}

bool IsValidAudioLocation(const setting::StrategyConfiguration& strategy_configuration) {
    if (strategy_configuration.getAudioLocation() == setting::AudioLocation::STEREO)
        return IsValidStereoAudioLocation(strategy_configuration);
    else if (strategy_configuration.getAudioLocation() == setting::AudioLocation::MONO)
        return IsValidMonoAudioLocation(strategy_configuration);
    return false;
}

bool IsValidAudioChannelAllocation(const setting::StrategyConfiguration& strategy_configuration) {
    // First, ensure that there's only 2 bitmask enabled
    int audio_channel_allocation = strategy_configuration.getAudioChannelAllocation();
    int count = 0;
    for (int bit = 0; bit < 32; ++bit)
        if (audio_channel_allocation & (1 << bit)) ++count;
    if (count > 2) {
        LOG(WARNING) << "Cannot parse more than 2 audio location, input is "
                     << audio_channel_allocation;
        return false;
    }

    if (count == 2)
        return IsValidStereoAudioLocation(strategy_configuration);
    else
        return IsValidMonoAudioLocation(strategy_configuration);
}

bool BluetoothLeAudioCodecsProvider::IsValidStrategyConfiguration(
        const setting::StrategyConfiguration& strategy_configuration) {
    if (!strategy_configuration.hasName() || !strategy_configuration.hasConnectedDevice() ||
        !strategy_configuration.hasChannelCount()) {
        return false;
    }

    // Both audio location field cannot be empty
    if (!strategy_configuration.hasAudioLocation() &&
        !strategy_configuration.hasAudioChannelAllocation())
        return false;

    // Any audio location field that presents must be valid
    if (strategy_configuration.hasAudioLocation() && !IsValidAudioLocation(strategy_configuration))
        return false;

    if (strategy_configuration.hasAudioChannelAllocation() &&
        !IsValidAudioChannelAllocation(strategy_configuration))
        return false;

    return true;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
