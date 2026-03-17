/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <algorithm>
#include <cstdint>
#include <vector>
#define STREAM_TO_UINT8(u8, p)  \
    {                           \
        (u8) = (uint8_t)(*(p)); \
        (p) += 1;               \
    }
#define STREAM_TO_UINT16(u16, p)                                      \
    {                                                                 \
        (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
        (p) += 2;                                                     \
    }
#define STREAM_TO_UINT32(u32, p)                                                           \
    {                                                                                      \
        (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) +                  \
                 ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); \
        (p) += 4;                                                                          \
    }

#define LOG_TAG "BTAudioAseConfigAidl"

#include <aidl/android/hardware/bluetooth/audio/AudioConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/AudioContext.h>
#include <aidl/android/hardware/bluetooth/audio/BluetoothAudioStatus.h>
#include <aidl/android/hardware/bluetooth/audio/CodecId.h>
#include <aidl/android/hardware/bluetooth/audio/CodecSpecificCapabilitiesLtv.h>
#include <aidl/android/hardware/bluetooth/audio/CodecSpecificConfigurationLtv.h>
#include <aidl/android/hardware/bluetooth/audio/ConfigurationFlags.h>
#include <aidl/android/hardware/bluetooth/audio/LeAudioAseConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/Phy.h>
#include <android-base/logging.h>

#include <optional>

#include "BluetoothAudioType.h"
#include "BluetoothLeAudioAseConfigurationSettingProvider.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

namespace {

const std::vector<ConfigurationSetFile> kLeAudioSetConfigs = {
        {.schema = "/vendor/etc/aidl/le_audio/aidl_audio_set_configurations.bfbs",
         .content = "/vendor/etc/aidl/le_audio/aidl_audio_set_configurations.json"},
};
const std::vector<ConfigurationSetFile> kLeAudioSetScenarios = {
        {.schema = "/vendor/etc/aidl/le_audio/aidl_audio_set_scenarios.bfbs",
         .content = "/vendor/etc/aidl/le_audio/aidl_audio_set_scenarios.json"},
};

constexpr uint8_t kVendorCodecConfigReservation = 32;

const le_audio::CodecSpecificConfiguration* LookupCodecSpecificParam(
        const flatbuffers::Vector<flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
                flat_codec_specific_params,
        le_audio::CodecSpecificLtvGenericTypes type) {
    if (flat_codec_specific_params == nullptr) return nullptr;
    auto it = std::find_if(flat_codec_specific_params->cbegin(), flat_codec_specific_params->cend(),
                           [&type](const auto& csc) { return (csc->type() == type); });
    return (it != flat_codec_specific_params->cend()) ? *it : nullptr;
}

LeAudioAseConfiguration::TargetLatency ToAidlTargetLatency(
        le_audio::AudioSetConfigurationTargetLatency latency) {
    switch (latency) {
        case le_audio::AudioSetConfigurationTargetLatency::
                AudioSetConfigurationTargetLatency_BALANCED_RELIABILITY:
            return LeAudioAseConfiguration::TargetLatency::BALANCED_LATENCY_RELIABILITY;
        case le_audio::AudioSetConfigurationTargetLatency::
                AudioSetConfigurationTargetLatency_HIGH_RELIABILITY:
            return LeAudioAseConfiguration::TargetLatency::HIGHER_RELIABILITY;
        case le_audio::AudioSetConfigurationTargetLatency::AudioSetConfigurationTargetLatency_LOW:
            return LeAudioAseConfiguration::TargetLatency::LOWER;
        default:
            return LeAudioAseConfiguration::TargetLatency::UNDEFINED;
    }
}

bool IsOpusHiResCodec(const LeAudioAseConfiguration& ase) {
    if (!ase.codecId.has_value() || ase.codecId->getTag() != CodecId::vendor ||
        ase.codecId->get<CodecId::vendor>() != opus_codec) {
        return false;
    }

    return std::any_of(
            ase.codecConfiguration.begin(), ase.codecConfiguration.end(), [](const auto& ltv) {
                return ltv.getTag() == CodecSpecificConfigurationLtv::samplingFrequency &&
                       ltv.template get<CodecSpecificConfigurationLtv::samplingFrequency>() ==
                               CodecSpecificConfigurationLtv::SamplingFrequency::HZ96000;
            });
}

bool IsDsaHeadTrackingCodec(const LeAudioAseConfiguration& ase) {
    return ase.codecId.has_value() && ase.codecId->getTag() == CodecId::vendor &&
           ase.codecId->get<CodecId::vendor>() == dsa_headtracker_codec;
}

bool IsLowLatencyConfiguration(const AseDirectionConfiguration& cfg) {
    return cfg.aseConfiguration.targetLatency == LeAudioAseConfiguration::TargetLatency::LOWER;
}

// Comparing if 2 AseDirectionConfiguration is asymmetrical.
bool IsAseConfigurationAsymmetrical(const AseDirectionConfiguration& cfg_a,
                                    const AseDirectionConfiguration& cfg_b) {
    auto get_sampling_frequency = [](const AseDirectionConfiguration& cfg) {
        for (const auto& ltv : cfg.aseConfiguration.codecConfiguration) {
            if (ltv.getTag() == CodecSpecificConfigurationLtv::samplingFrequency) {
                return ltv.get<CodecSpecificConfigurationLtv::samplingFrequency>();
            }
        }
        return CodecSpecificConfigurationLtv::SamplingFrequency::HZ8000;  // Default
    };

    return get_sampling_frequency(cfg_a) != get_sampling_frequency(cfg_b);
}

bool LoadFileAndParse(flatbuffers::Parser& parser, const ConfigurationSetFile& files) {
    std::string schema_binary, content_binary;
    if (!flatbuffers::LoadFile(files.schema, true, &schema_binary)) {
        LOG(ERROR) << __func__ << ": Failed to load schema: " << files.schema;
        return false;
    }
    if (!parser.Deserialize(reinterpret_cast<const uint8_t*>(schema_binary.c_str()),
                            schema_binary.length())) {
        LOG(ERROR) << __func__ << ": Failed to deserialize schema: " << files.schema;
        return false;
    }
    if (!flatbuffers::LoadFile(files.content, false, &content_binary)) {
        LOG(ERROR) << __func__ << ": Failed to load json file: " << files.content;
        return false;
    }
    return parser.Parse(content_binary.c_str());
}

}  // namespace

/* Implementation */

std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
AudioSetConfigurationProviderJson::GetLeAudioAseConfigurationSettings() {
    AudioSetConfigurationProviderJson::LoadAudioSetConfigurationProviderJson();
    return ase_configuration_settings_;
}

void AudioSetConfigurationProviderJson::LoadAudioSetConfigurationProviderJson() {
    if (ase_configs_.empty() || ase_configuration_settings_.empty()) {
        ase_configuration_settings_.clear();
        ase_configs_.clear();
        auto loaded = LoadConfigurationSetFile(kLeAudioSetConfigs, kLeAudioSetScenarios,
                                               CodecLocation::ADSP);
        if (!loaded) LOG(ERROR) << ": Unable to load le audio set configuration files.";
    } else
        LOG(INFO) << ": Reusing loaded le audio set configuration";
}

void AudioSetConfigurationProviderJson::PopulateAudioChannelAllocation(
        CodecSpecificConfigurationLtv::AudioChannelAllocation& audio_channel_allocation,
        uint32_t audio_location) {
    audio_channel_allocation.bitmask = 0;
    for (auto [allocation, bitmask] : audio_channel_allocation_map) {
        if (audio_location & allocation) audio_channel_allocation.bitmask |= bitmask;
    }
}

void AudioSetConfigurationProviderJson::PopulateConfigurationData(
        LeAudioAseConfiguration& ase,
        const flatbuffers::Vector<flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
                flat_codec_specific_params) {
    uint8_t sampling_frequency = 0;
    uint8_t frame_duration = 0;
    uint32_t audio_channel_allocation = 0;
    uint16_t octets_per_codec_frame = 0;
    uint8_t codec_frames_blocks_per_sdu = 0;

    auto param = LookupCodecSpecificParam(
            flat_codec_specific_params,
            le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_SAMPLING_FREQUENCY);
    if (param) {
        auto ptr = param->compound_value()->value()->data();
        STREAM_TO_UINT8(sampling_frequency, ptr);
    }

    param = LookupCodecSpecificParam(
            flat_codec_specific_params,
            le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_FRAME_DURATION);
    if (param) {
        auto ptr = param->compound_value()->value()->data();
        STREAM_TO_UINT8(frame_duration, ptr);
    }

    param = LookupCodecSpecificParam(
            flat_codec_specific_params,
            le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_AUDIO_CHANNEL_ALLOCATION);
    if (param) {
        auto ptr = param->compound_value()->value()->data();
        STREAM_TO_UINT32(audio_channel_allocation, ptr);
    }

    param = LookupCodecSpecificParam(
            flat_codec_specific_params,
            le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_OCTETS_PER_CODEC_FRAME);
    if (param) {
        auto ptr = param->compound_value()->value()->data();
        STREAM_TO_UINT16(octets_per_codec_frame, ptr);
    }

    param = LookupCodecSpecificParam(
            flat_codec_specific_params,
            le_audio::CodecSpecificLtvGenericTypes_SUPPORTED_CODEC_FRAME_BLOCKS_PER_SDU);
    if (param) {
        auto ptr = param->compound_value()->value()->data();
        STREAM_TO_UINT8(codec_frames_blocks_per_sdu, ptr);
    }

    // Make the correct value
    ase.codecConfiguration = std::vector<CodecSpecificConfigurationLtv>();

    auto sampling_freq_it = sampling_freq_map.find(sampling_frequency);
    if (sampling_freq_it != sampling_freq_map.end())
        ase.codecConfiguration.push_back(sampling_freq_it->second);
    auto frame_duration_it = frame_duration_map.find(frame_duration);
    if (frame_duration_it != frame_duration_map.end())
        ase.codecConfiguration.push_back(frame_duration_it->second);

    CodecSpecificConfigurationLtv::AudioChannelAllocation channel_allocation;
    PopulateAudioChannelAllocation(channel_allocation, audio_channel_allocation);
    ase.codecConfiguration.push_back(channel_allocation);

    auto octet_structure = CodecSpecificConfigurationLtv::OctetsPerCodecFrame();
    octet_structure.value = octets_per_codec_frame;
    ase.codecConfiguration.push_back(octet_structure);

    auto frame_sdu_structure = CodecSpecificConfigurationLtv::CodecFrameBlocksPerSDU();
    frame_sdu_structure.value = codec_frames_blocks_per_sdu;
    ase.codecConfiguration.push_back(frame_sdu_structure);
}

std::optional<LeAudioAseConfiguration> AudioSetConfigurationProviderJson::PopulateAseConfiguration(
        const le_audio::AudioSetSubConfiguration* flat_subconfig,
        const le_audio::QosConfiguration* qos_cfg) {
    if (flat_subconfig == nullptr || qos_cfg == nullptr) return std::nullopt;

    LeAudioAseConfiguration ase;
    ase.targetLatency = ToAidlTargetLatency(qos_cfg->target_latency());
    ase.targetPhy = Phy::TWO_M;

    // Making CodecId
    const auto* codec_id = flat_subconfig->codec_id();
    if (codec_id != nullptr) {
        if (codec_id->coding_format() == static_cast<uint8_t>(CodecId::Core::LC3)) {
            ase.codecId = CodecId::Core::LC3;
        } else {
            CodecId::Vendor vendor_codec;
            vendor_codec.codecId = codec_id->vendor_codec_id();
            vendor_codec.id = codec_id->vendor_company_id();
            ase.codecId = vendor_codec;
        }
    }

    // Codec configuration data
    PopulateConfigurationData(ase, flat_subconfig->codec_configuration());

    return ase;
}

std::optional<LeAudioAseQosConfiguration>
AudioSetConfigurationProviderJson::PopulateAseQosConfiguration(
        const le_audio::QosConfiguration* qos_cfg, const LeAudioAseConfiguration& ase,
        uint8_t ase_channel_cnt) {
    if (qos_cfg == nullptr) {
        LOG(ERROR) << __func__ << ": qos_cfg is null";
        return std::nullopt;
    }

    LeAudioAseQosConfiguration qos;
    using Ltv = CodecSpecificConfigurationLtv;

    int frame_blocks = 1;
    std::optional<int> octets = std::nullopt;
    std::optional<int> duration_us = std::nullopt;

    for (const auto& cfg_ltv : ase.codecConfiguration) {
        switch (cfg_ltv.getTag()) {
            case Ltv::codecFrameBlocksPerSDU:
                frame_blocks = cfg_ltv.get<Ltv::codecFrameBlocksPerSDU>().value;
                break;
            case Ltv::frameDuration:
                switch (cfg_ltv.get<Ltv::frameDuration>()) {
                    case Ltv::FrameDuration::US7500:
                        duration_us = 7500;
                        break;
                    case Ltv::FrameDuration::US10000:
                        duration_us = 10000;
                        break;
                    case Ltv::FrameDuration::US20000:
                        duration_us = 20000;
                        break;
                    default:
                        break;
                }
                break;
            case Ltv::octetsPerCodecFrame:
                octets = cfg_ltv.get<Ltv::octetsPerCodecFrame>().value;
                break;
            default:
                break;
        }
    }

    // Populate maxSdu
    if (octets.has_value()) {
        bool is_vendor = (ase.codecId.has_value() && ase.codecId->getTag() == CodecId::vendor);
        int multiplier = is_vendor ? 1 : ase_channel_cnt;
        qos.maxSdu = multiplier * octets.value() * frame_blocks;
    }

    // Populate sduIntervalUs
    if (duration_us.has_value()) {
        qos.sduIntervalUs = duration_us.value() * frame_blocks;
    }

    qos.maxTransportLatencyMs = qos_cfg->max_transport_latency();
    qos.retransmissionNum = qos_cfg->retransmission_number();

    return qos;
}

std::optional<std::vector<uint8_t>>
AudioSetConfigurationProviderJson::PopulateVendorCodecConfiguration(
        const LeAudioAseConfiguration& ase) {
    if (!ase.codecId.has_value() || ase.codecId->getTag() != CodecId::vendor) {
        return std::nullopt;
    }

    std::vector<uint8_t> codec_config;
    codec_config.reserve(kVendorCodecConfigReservation);

    // Helper lambda to handle mapped types
    auto push_mapped = [&](uint8_t sub_opcode, const auto& map, const auto& key) {
        if (auto it = map.find(key); it != map.end()) {
            codec_config.push_back(kCodecConfigOpcode);
            codec_config.push_back(sub_opcode);
            codec_config.push_back(it->second);
        }
    };

    // Helper lambda to handle multi-byte values
    auto push_bytes = [&](uint8_t opcode, uint8_t sub_opcode, uint32_t val, int len) {
        codec_config.push_back(opcode);
        codec_config.push_back(sub_opcode);
        for (int i = 0; i < len; ++i) {
            codec_config.push_back((val >> (i * 8)) & 0xff);
        }
    };

    using Ltv = CodecSpecificConfigurationLtv;
    for (const auto& ltv : ase.codecConfiguration) {
        switch (ltv.getTag()) {
            case Ltv::samplingFrequency:
                push_mapped(kSamplingFrequencySubOpcode, sampling_rate_ltv_to_codec_cfg_map,
                            ltv.get<Ltv::samplingFrequency>());
                break;
            case Ltv::frameDuration:
                push_mapped(kFrameDurationSubOpcode, frame_duration_ltv_to_codec_cfg_map,
                            ltv.get<Ltv::frameDuration>());
                break;
            case Ltv::audioChannelAllocation:
                push_bytes(kAudioChannelAllocationOpcode, kAudioChannelAllocationSubOpcode,
                           ltv.get<Ltv::audioChannelAllocation>().bitmask, 4);
                break;
            case Ltv::octetsPerCodecFrame:
                push_bytes(kOctetsPerCodecFrameOpcode, kOctetsPerCodecFrameSubOpcode,
                           ltv.get<Ltv::octetsPerCodecFrame>().value, 2);
                break;
            case Ltv::codecFrameBlocksPerSDU:
                codec_config.push_back(kCodecConfigOpcode);
                codec_config.push_back(kFrameBlocksPerSDUSubOpcode);
                codec_config.push_back(ltv.get<Ltv::codecFrameBlocksPerSDU>().value);
                break;
            default:
                break;
        }
    }
    return codec_config;
}

LeAudioDataPathConfiguration AudioSetConfigurationProviderJson::PopulateDatapath(
        const CodecLocation& location, const LeAudioAseConfiguration& ase) {
    LeAudioDataPathConfiguration path;
    // Move codecId to iso data path
    path.isoDataPathConfiguration.codecId = ase.codecId.value();
    // Specific vendor datapath logic
    if (IsOpusHiResCodec(ase)) {
        path.isoDataPathConfiguration.isTransparent = true;
        path.dataPathId = kIsoDataPathHciLinkFeedback;
        return path;
    }

    // DSA 2.0 DSA_SW data path logic
    if (IsDsaHeadTrackingCodec(ase)) {
        path.dataPathId = kIsoDataPathHci;
        return path;
    }

    // Translate location to data path id
    switch (location) {
        case CodecLocation::ADSP:
            path.isoDataPathConfiguration.isTransparent = true;
            path.dataPathId = kIsoDataPathPlatformDefault;
            break;
        case CodecLocation::HOST:
            path.isoDataPathConfiguration.isTransparent = true;
            path.dataPathId = kIsoDataPathHci;
            break;
        case CodecLocation::CONTROLLER:
            path.isoDataPathConfiguration.isTransparent = false;
            path.dataPathId = kIsoDataPathPlatformDefault;
            break;
    }
    return path;
}

// Parse into AseDirectionConfiguration
AseDirectionConfiguration AudioSetConfigurationProviderJson::SetConfigurationFromFlatSubconfig(
        const le_audio::AudioSetSubConfiguration* flat_subconfig,
        const le_audio::QosConfiguration* qos_cfg, CodecLocation location,
        ConfigurationFlags& configurationFlags) {
    AseDirectionConfiguration direction_conf;

    // Translate into LeAudioAseConfiguration
    auto ase_opt = PopulateAseConfiguration(flat_subconfig, qos_cfg);
    if (!ase_opt.has_value()) return direction_conf;
    auto ase = std::move(ase_opt.value());

    if (ase.targetLatency == LeAudioAseConfiguration::TargetLatency::LOWER) {
        configurationFlags.bitmask |= ConfigurationFlags::LOW_LATENCY;
    }

    // Translate into LeAudioAseQosConfiguration
    auto qos_opt = PopulateAseQosConfiguration(qos_cfg, ase, flat_subconfig->ase_channel_cnt());
    if (!qos_opt.has_value()) return direction_conf;
    auto qos = std::move(qos_opt.value());

    // Populate vendorCodecConfiguration using the correct LTV
    ase.vendorCodecConfiguration = PopulateVendorCodecConfiguration(ase);

    direction_conf.aseConfiguration = ase;
    direction_conf.qosConfiguration = qos;
    // Populate the correct datapath.
    direction_conf.dataPathConfiguration = PopulateDatapath(location, ase);

    return direction_conf;
}

// Parse into AseDirectionConfiguration and the ConfigurationFlags
// and put them in the given list.
void AudioSetConfigurationProviderJson::ProcessSubconfig(
        const le_audio::AudioSetSubConfiguration* subconfig,
        const le_audio::QosConfiguration* qos_cfg,
        std::vector<std::optional<AseDirectionConfiguration>>& directionAseConfiguration,
        CodecLocation location, ConfigurationFlags& configurationFlags) {
    auto ase_cnt = subconfig->ase_cnt();
    auto config =
            SetConfigurationFromFlatSubconfig(subconfig, qos_cfg, location, configurationFlags);
    directionAseConfiguration.push_back(config);
    // Put the same setting again.
    if (ase_cnt == 2) directionAseConfiguration.push_back(config);
}

void AudioSetConfigurationProviderJson::PopulateAseConfigurationFromFlat(
        const le_audio::AudioSetConfiguration* flat_cfg,
        const std::map<std::string_view, const le_audio::CodecConfiguration*>& codec_cfgs,
        const std::map<std::string_view, const le_audio::QosConfiguration*>& qos_cfgs,
        CodecLocation location,
        std::vector<std::optional<AseDirectionConfiguration>>& sourceAseConfiguration,
        std::vector<std::optional<AseDirectionConfiguration>>& sinkAseConfiguration,
        ConfigurationFlags& configurationFlags) {
    if (flat_cfg == nullptr) {
        LOG(ERROR) << "flat_cfg cannot be null";
        return;
    }
    std::string_view codec_config_key = flat_cfg->codec_config_name()->string_view();
    auto* qos_config_key_array = flat_cfg->qos_config_name();

    constexpr std::string_view default_qos = "QoS_Config_Balanced_Reliability";

    std::string_view qos_sink_key(default_qos);
    std::string_view qos_source_key(default_qos);

    /* We expect maximum two QoS settings. First for Sink and second for Source
     */
    if (qos_config_key_array->size() > 0) {
        qos_sink_key = qos_config_key_array->Get(0)->string_view();
        if (qos_config_key_array->size() > 1) {
            qos_source_key = qos_config_key_array->Get(1)->string_view();
        } else {
            qos_source_key = qos_sink_key;
        }
    }

    LOG(INFO) << "Audio set config " << flat_cfg->name()->c_str() << ": codec config "
              << codec_config_key << ", qos_sink " << qos_sink_key << ", qos_source "
              << qos_source_key;

    // Find the first qos config that match the name
    const le_audio::QosConfiguration* qos_sink_cfg = nullptr;
    if (auto it = qos_cfgs.find(qos_sink_key); it != qos_cfgs.end()) {
        qos_sink_cfg = it->second;
    }

    const le_audio::QosConfiguration* qos_source_cfg = nullptr;
    if (auto it = qos_cfgs.find(qos_source_key); it != qos_cfgs.end()) {
        qos_source_cfg = it->second;
    }

    // First codec_cfg with the same name
    const le_audio::CodecConfiguration* codec_cfg = nullptr;
    if (auto it = codec_cfgs.find(codec_config_key); it != codec_cfgs.end()) {
        codec_cfg = it->second;
    }

    // Process each subconfig and put it into the correct list
    if (codec_cfg != nullptr && codec_cfg->subconfigurations()) {
        /* Load subconfigurations */
        for (auto subconfig : *codec_cfg->subconfigurations()) {
            if (subconfig->direction() == kLeAudioDirectionSink) {
                ProcessSubconfig(subconfig, qos_sink_cfg, sinkAseConfiguration, location,
                                 configurationFlags);
            } else {
                ProcessSubconfig(subconfig, qos_source_cfg, sourceAseConfiguration, location,
                                 configurationFlags);
            }
        }

        // After putting all subconfig, check if it's an asymmetric configuration
        // and populate information for ConfigurationFlags
        if (!sinkAseConfiguration.empty() && !sourceAseConfiguration.empty()) {
            for (int i = 0; i < sinkAseConfiguration.size(); ++i) {
                // Only check for comparable source and sink configuration.
                if (sourceAseConfiguration.size() <= i) break;
                if (sinkAseConfiguration[i].has_value() && sourceAseConfiguration[i].has_value()) {
                    // Has both direction, comparing inner fields:
                    if (IsAseConfigurationAsymmetrical(sinkAseConfiguration[i].value(),
                                                       sourceAseConfiguration[i].value())) {
                        configurationFlags.bitmask |=
                                ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS;
                        // Already detect asymmetrical config.
                        break;
                    }
                }
            }
        }

        // Check all the source configuration for DSA 2.0 headtracking codec
        // and set the SPATIAL_AUDIO flag
        for (auto& aseDirectionConfiguration : sourceAseConfiguration) {
            if (aseDirectionConfiguration.has_value()) {
                if (IsDsaHeadTrackingCodec(aseDirectionConfiguration.value().aseConfiguration)) {
                    LOG(INFO) << "Found DSA 2.0 config " << flat_cfg->name()->c_str();
                    configurationFlags.bitmask |= ConfigurationFlags::SPATIAL_AUDIO;
                    break;
                }
            }
        }
    } else {
        if (codec_cfg == nullptr) {
            LOG(ERROR) << "No codec config matching key " << codec_config_key << " found";
        } else {
            LOG(ERROR) << "Configuration '" << flat_cfg->name()->c_str()
                       << "' has no valid subconfigurations.";
        }
    }
}

bool AudioSetConfigurationProviderJson::LoadConfigurationsFromFiles(
        const ConfigurationSetFile& files, CodecLocation location) {
    flatbuffers::Parser parser;
    if (!LoadFileAndParse(parser, files)) return false;

    auto configurations_root =
            le_audio::GetAudioSetConfigurations(parser.builder_.GetBufferPointer());
    if (!configurations_root) return false;

    auto flat_codec_configs = configurations_root->codec_configurations();
    if (!flat_codec_configs || flat_codec_configs->size() == 0) return false;

    std::map<std::string_view, const le_audio::CodecConfiguration*> codec_cfgs;
    for (auto const& flat_codec_cfg : *flat_codec_configs) {
        codec_cfgs[flat_codec_cfg->name()->string_view()] = flat_codec_cfg;
    }

    auto flat_qos_configs = configurations_root->qos_configurations();
    if (!flat_qos_configs || flat_qos_configs->size() == 0) return false;

    std::map<std::string_view, const le_audio::QosConfiguration*> qos_cfgs;
    for (auto const& flat_qos_cfg : *flat_qos_configs) {
        qos_cfgs[flat_qos_cfg->name()->string_view()] = flat_qos_cfg;
    }

    auto flat_configs = configurations_root->configurations();
    if (!flat_configs || flat_configs->size() == 0) return false;

    for (auto const& flat_cfg : *flat_configs) {
        // Create 3 vector to use
        std::vector<std::optional<AseDirectionConfiguration>> sourceAseConfiguration;
        std::vector<std::optional<AseDirectionConfiguration>> sinkAseConfiguration;
        ConfigurationFlags configurationFlags;
        PopulateAseConfigurationFromFlat(flat_cfg, codec_cfgs, qos_cfgs, location,
                                         sourceAseConfiguration, sinkAseConfiguration,
                                         configurationFlags);
        if (sourceAseConfiguration.empty() && sinkAseConfiguration.empty()) continue;
        ase_configs_[flat_cfg->name()->str()] = {sourceAseConfiguration, sinkAseConfiguration,
                                                 configurationFlags};
    }

    return true;
}

bool AudioSetConfigurationProviderJson::LoadScenariosFromFiles(const ConfigurationSetFile& files) {
    flatbuffers::Parser parser;
    if (!LoadFileAndParse(parser, files)) return false;

    auto scenarios_root = le_audio::GetAudioSetScenarios(parser.builder_.GetBufferPointer());
    if (!scenarios_root) return false;

    auto flat_scenarios = scenarios_root->scenarios();
    if ((flat_scenarios == nullptr) || (flat_scenarios->size() == 0)) return false;

    LOG(INFO) << __func__ << ": Turn flat buffer into structure";
    AudioContext media_context = AudioContext();
    media_context.bitmask =
            (AudioContext::ALERTS | AudioContext::INSTRUCTIONAL | AudioContext::NOTIFICATIONS |
             AudioContext::EMERGENCY_ALARM | AudioContext::UNSPECIFIED | AudioContext::MEDIA |
             AudioContext::SOUND_EFFECTS);

    AudioContext conversational_context = AudioContext();
    conversational_context.bitmask = (AudioContext::RINGTONE_ALERTS | AudioContext::CONVERSATIONAL);

    AudioContext live_context = AudioContext();
    live_context.bitmask = AudioContext::LIVE_AUDIO;

    AudioContext game_context = AudioContext();
    game_context.bitmask = AudioContext::GAME;

    AudioContext voice_assistants_context = AudioContext();
    voice_assistants_context.bitmask = AudioContext::VOICE_ASSISTANTS;

    LOG(DEBUG) << "Updating " << flat_scenarios->size() << " scenarios.";
    for (auto const& scenario : *flat_scenarios) {
        if (!scenario->configurations()) continue;
        std::string scenario_name = scenario->name()->c_str();
        AudioContext context;
        if (scenario_name == "Media")
            context = AudioContext(media_context);
        else if (scenario_name == "Conversational")
            context = AudioContext(conversational_context);
        else if (scenario_name == "Live")
            context = AudioContext(live_context);
        else if (scenario_name == "Game")
            context = AudioContext(game_context);
        else if (scenario_name == "VoiceAssistants")
            context = AudioContext(voice_assistants_context);
        LOG(DEBUG) << "Scenario " << scenario->name()->c_str()
                   << " configs: " << scenario->configurations()->size()
                   << " context: " << context.toString();

        for (auto it = scenario->configurations()->begin(); it != scenario->configurations()->end();
             ++it) {
            auto config_name = it->str();
            auto configuration = ase_configs_.find(config_name);
            if (configuration == ase_configs_.end()) continue;
            LOG(DEBUG) << "Getting configuration with name: " << config_name;
            auto [source, sink, flags] = configuration->second;
            // Each configuration will create a LeAudioAseConfigurationSetting
            // with the same {context, packing}
            // and different data
            LeAudioAseConfigurationSetting setting;
            setting.audioContext = context;
            // TODO: Packing
            setting.sourceAseConfiguration = source;
            setting.sinkAseConfiguration = sink;
            setting.flags = flags;
            // Add to list of setting
            LOG(DEBUG) << "Pushing configuration to list: " << config_name;
            ase_configuration_settings_.push_back({config_name, setting});
        }
    }

    return true;
}

bool AudioSetConfigurationProviderJson::LoadConfigurationSetFile(
        const std::vector<ConfigurationSetFile>& config_files,
        const std::vector<ConfigurationSetFile>& scenario_files, CodecLocation location) {
    bool is_success = false;
    for (const auto& file : config_files) {
        if ((is_success = LoadConfigurationsFromFiles(file, location))) {
            break;
        }
    }

    if (!is_success) return false;

    for (const auto& file : scenario_files) {
        if (LoadScenariosFromFiles(file)) {
            return true;
        }
    }
    return false;
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
