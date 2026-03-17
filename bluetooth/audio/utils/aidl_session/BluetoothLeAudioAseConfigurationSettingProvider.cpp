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
constexpr std::string_view kDefaultQos = "QoS_Config_Balanced_Reliability";

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

const le_audio::CodecConfiguration* GetCodecConfig(
        const le_audio::AudioSetConfiguration* flat_cfg,
        const std::map<std::string_view, const le_audio::CodecConfiguration*>& codec_cfgs) {
    if (flat_cfg->codec_config_name() == nullptr) {
        LOG(ERROR) << "codec_config_name cannot be null";
        return nullptr;
    }

    auto it = codec_cfgs.find(flat_cfg->codec_config_name()->string_view());
    if (it == codec_cfgs.end() || it->second->subconfigurations() == nullptr) {
        LOG(ERROR) << "No codec config matching key "
                   << flat_cfg->codec_config_name()->string_view() << " found";
        return nullptr;
    }
    return it->second;
}

std::optional<std::pair<const le_audio::QosConfiguration*, const le_audio::QosConfiguration*>>
GetQosConfig(const le_audio::AudioSetConfiguration* flat_cfg,
             const std::map<std::string_view, const le_audio::QosConfiguration*>& qos_cfgs) {
    const auto* qos_names = flat_cfg->qos_config_name();
    const std::string_view qos_sink_key = (qos_names != nullptr && qos_names->size() > 0)
                                                  ? qos_names->Get(0)->string_view()
                                                  : kDefaultQos;
    const std::string_view qos_source_key = (qos_names != nullptr && qos_names->size() > 1)
                                                    ? qos_names->Get(1)->string_view()
                                                    : qos_sink_key;

    auto get_qos_cfg = [&](std::string_view key,
                           const char* type) -> const le_audio::QosConfiguration* {
        auto it = qos_cfgs.find(key);
        if (it == qos_cfgs.end()) {
            LOG(ERROR) << "No valid " << type << " qos config matching key " << std::string(key)
                       << " found";
            return nullptr;
        }
        return it->second;
    };

    const auto* qos_sink_cfg = get_qos_cfg(qos_sink_key, "sink");
    if (qos_sink_cfg == nullptr) return std::nullopt;

    const auto* qos_source_cfg =
            (qos_source_key == qos_sink_key) ? qos_sink_cfg : get_qos_cfg(qos_source_key, "source");
    if (qos_source_cfg == nullptr) return std::nullopt;

    return std::make_pair(qos_sink_cfg, qos_source_cfg);
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

std::optional<AseConfig> AudioSetConfigurationProviderJson::PopulateAseConfigsFromFlat(
        const le_audio::AudioSetConfiguration* flat_cfg,
        const std::map<std::string_view, const le_audio::CodecConfiguration*>& codec_cfgs,
        const std::map<std::string_view, const le_audio::QosConfiguration*>& qos_cfgs,
        CodecLocation location) {
    if (flat_cfg == nullptr) {
        LOG(ERROR) << "flat_cfg cannot be null";
        return std::nullopt;
    }

    const auto* codec_cfg = GetCodecConfig(flat_cfg, codec_cfgs);
    if (codec_cfg == nullptr) return std::nullopt;

    auto qos_cfg_pair = GetQosConfig(flat_cfg, qos_cfgs);
    if (!qos_cfg_pair.has_value()) return std::nullopt;

    const auto* qos_sink_cfg = qos_cfg_pair->first;
    const auto* qos_source_cfg = qos_cfg_pair->second;

    AseConfig result;
    /* Load subconfigurations */
    for (auto subconfig : *codec_cfg->subconfigurations()) {
        const auto* qos_cfg =
                (subconfig->direction() == kLeAudioDirectionSink) ? qos_sink_cfg : qos_source_cfg;

        AseDirectionConfiguration config;

        // Translate into LeAudioAseConfiguration directly into config member
        auto ase_cfg = PopulateAseConfiguration(subconfig, qos_cfg);
        if (!ase_cfg.has_value()) continue;
        config.aseConfiguration = std::move(ase_cfg.value());

        // Translate into LeAudioAseQosConfiguration directly into config member
        auto qos_cfg_aidl = PopulateAseQosConfiguration(qos_cfg, config.aseConfiguration,
                                                        subconfig->ase_channel_cnt());
        if (!qos_cfg_aidl.has_value()) continue;
        config.qosConfiguration = std::move(qos_cfg_aidl.value());

        // Populate the correct datapath.
        config.dataPathConfiguration = PopulateDatapath(location, config.aseConfiguration);

        // Populate vendorCodecConfiguration using the correct LTV
        config.aseConfiguration.vendorCodecConfiguration =
                PopulateVendorCodecConfiguration(config.aseConfiguration);

        auto& directionAseConfiguration =
                (subconfig->direction() == kLeAudioDirectionSink) ? result.sink : result.source;

        // Put the same setting again.
        auto ase_cnt = subconfig->ase_cnt();
        if (ase_cnt == 2) {
            directionAseConfiguration.push_back(config);
        }
        directionAseConfiguration.emplace_back(std::move(config));
    }

    UpdateConfigurationFlags(result);

    if (result.source.empty() && result.sink.empty()) {
        return std::nullopt;
    }

    return result;
}

void AudioSetConfigurationProviderJson::UpdateConfigurationFlags(AseConfig& result) {
    auto any_match = [](const std::vector<std::optional<AseDirectionConfiguration>>& configs,
                        auto predicate) {
        return std::any_of(configs.begin(), configs.end(),
                           [&](const auto& cfg) { return cfg.has_value() && predicate(*cfg); });
    };

    if (any_match(result.sink, IsLowLatencyConfiguration) ||
        any_match(result.source, IsLowLatencyConfiguration)) {
        result.flags.bitmask |= ConfigurationFlags::LOW_LATENCY;
    }

    // Check if it's an asymmetric configuration
    const size_t check_count = std::min(result.sink.size(), result.source.size());
    for (size_t i = 0; i < check_count; ++i) {
        const auto& sink_cfg = result.sink[i];
        const auto& src_cfg = result.source[i];

        if (sink_cfg.has_value() && src_cfg.has_value()) {
            if (IsAseConfigurationAsymmetrical(*sink_cfg, *src_cfg)) {
                result.flags.bitmask |= ConfigurationFlags::ALLOW_ASYMMETRIC_CONFIGURATIONS;
                break;
            }
        }
    }

    // Check all the source configuration for DSA 2.0 headtracking codec
    if (any_match(result.source,
                  [](const auto& cfg) { return IsDsaHeadTrackingCodec(cfg.aseConfiguration); })) {
        result.flags.bitmask |= ConfigurationFlags::SPATIAL_AUDIO;
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
    for (const auto& flat_codec_cfg : *flat_codec_configs) {
        codec_cfgs[flat_codec_cfg->name()->string_view()] = flat_codec_cfg;
    }

    auto flat_qos_configs = configurations_root->qos_configurations();
    if (!flat_qos_configs || flat_qos_configs->size() == 0) return false;

    std::map<std::string_view, const le_audio::QosConfiguration*> qos_cfgs;
    for (const auto& flat_qos_cfg : *flat_qos_configs) {
        qos_cfgs[flat_qos_cfg->name()->string_view()] = flat_qos_cfg;
    }

    auto flat_configs = configurations_root->configurations();
    if (!flat_configs || flat_configs->size() == 0) return false;

    for (const auto& flat_cfg : *flat_configs) {
        if (flat_cfg->name() == nullptr) continue;
        auto config_data = PopulateAseConfigsFromFlat(flat_cfg, codec_cfgs, qos_cfgs, location);
        if (config_data.has_value()) {
            ase_configs_.insert_or_assign(flat_cfg->name()->str(), std::move(config_data.value()));
        }
    }

    return true;
}

bool AudioSetConfigurationProviderJson::LoadScenariosFromFiles(const ConfigurationSetFile& files) {
    flatbuffers::Parser parser;
    if (!LoadFileAndParse(parser, files)) return false;

    auto scenarios_root = le_audio::GetAudioSetScenarios(parser.builder_.GetBufferPointer());
    if (!scenarios_root) return false;

    auto flat_scenarios = scenarios_root->scenarios();
    if (!flat_scenarios || flat_scenarios->size() == 0) return false;

    // Define contexts
    static const AudioContext media_context = {
            .bitmask = (AudioContext::ALERTS | AudioContext::INSTRUCTIONAL |
                        AudioContext::NOTIFICATIONS | AudioContext::EMERGENCY_ALARM |
                        AudioContext::UNSPECIFIED | AudioContext::MEDIA |
                        AudioContext::SOUND_EFFECTS)};
    static const AudioContext conversational_context = {
            .bitmask = (AudioContext::RINGTONE_ALERTS | AudioContext::CONVERSATIONAL)};
    static const AudioContext live_context = {.bitmask = AudioContext::LIVE_AUDIO};
    static const AudioContext game_context = {.bitmask = AudioContext::GAME};
    static const AudioContext voice_assistants_context = {.bitmask =
                                                                  AudioContext::VOICE_ASSISTANTS};

    static const std::map<std::string_view, AudioContext> scenario_to_context = {
            {"Media", media_context},
            {"Conversational", conversational_context},
            {"Live", live_context},
            {"Game", game_context},
            {"VoiceAssistants", voice_assistants_context},
    };

    for (const auto& scenario : *flat_scenarios) {
        if (!scenario->configurations() || !scenario->name()) continue;

        std::string_view scenario_name = scenario->name()->string_view();
        auto context_it = scenario_to_context.find(scenario_name);
        AudioContext context =
                (context_it != scenario_to_context.end()) ? context_it->second : AudioContext{};

        for (const auto& config_name_fb : *scenario->configurations()) {
            std::string_view config_name = config_name_fb->string_view();
            auto configuration_it = ase_configs_.find(config_name);
            if (configuration_it == ase_configs_.end()) continue;

            const auto& configuration = configuration_it->second;
            LeAudioAseConfigurationSetting setting = {
                    .audioContext = context,
                    .sinkAseConfiguration = configuration.sink,
                    .sourceAseConfiguration = configuration.source,
                    .flags = configuration.flags,
            };
            ase_configuration_settings_.emplace_back(std::string(config_name), std::move(setting));
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
