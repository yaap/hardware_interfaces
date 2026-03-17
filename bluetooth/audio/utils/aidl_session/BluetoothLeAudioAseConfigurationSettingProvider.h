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

#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioProvider.h>

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "audio_set_configurations_generated.h"
#include "audio_set_scenarios_generated.h"
#include "flatbuffers/idl.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using LeAudioAseConfigurationSetting = IBluetoothAudioProvider::LeAudioAseConfigurationSetting;
using AseDirectionConfiguration =
        IBluetoothAudioProvider::LeAudioAseConfigurationSetting::AseDirectionConfiguration;
using LeAudioAseQosConfiguration = IBluetoothAudioProvider::LeAudioAseQosConfiguration;
using LeAudioDataPathConfiguration = IBluetoothAudioProvider::LeAudioDataPathConfiguration;

enum class CodecLocation {
    HOST,
    ADSP,
    CONTROLLER,
};

struct ConfigurationSetFile {
    const char* schema;
    const char* content;
};

struct AseConfig {
    std::vector<std::optional<AseDirectionConfiguration>> source;
    std::vector<std::optional<AseDirectionConfiguration>> sink;
    ConfigurationFlags flags;
};

class AudioSetConfigurationProviderJson {
  public:
    static std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
    GetLeAudioAseConfigurationSettings();

  private:
    static void LoadAudioSetConfigurationProviderJson();

    static void PopulateAudioChannelAllocation(
            CodecSpecificConfigurationLtv::AudioChannelAllocation& audio_channel_allocation,
            uint32_t audio_location);

    static void PopulateConfigurationData(
            LeAudioAseConfiguration& ase,
            const flatbuffers::Vector<flatbuffers::Offset<le_audio::CodecSpecificConfiguration>>*
                    flat_codec_specific_params);

    static void PopulateAseConfiguration(LeAudioAseConfiguration& ase,
                                         const le_audio::AudioSetSubConfiguration* flat_subconfig,
                                         const le_audio::QosConfiguration* qos_cfg,
                                         ConfigurationFlags& configurationFlags);

    static void PopulateAseQosConfiguration(LeAudioAseQosConfiguration& qos,
                                            const le_audio::QosConfiguration* qos_cfg,
                                            LeAudioAseConfiguration& ase, uint8_t ase_channel_cnt);

    static std::optional<std::vector<uint8_t>> PopulateVendorCodecConfiguration(
            const LeAudioAseConfiguration& ase);

    static AseDirectionConfiguration SetConfigurationFromFlatSubconfig(
            const le_audio::AudioSetSubConfiguration* flat_subconfig,
            const le_audio::QosConfiguration* qos_cfg, CodecLocation location,
            ConfigurationFlags& configurationFlags);

    static void ProcessSubconfig(
            const le_audio::AudioSetSubConfiguration* subconfig,
            const le_audio::QosConfiguration* qos_cfg,
            std::vector<std::optional<AseDirectionConfiguration>>& directionAseConfiguration,
            CodecLocation location, ConfigurationFlags& configurationFlags);

    static void PopulateAseConfigurationFromFlat(
            const le_audio::AudioSetConfiguration* flat_cfg,
            const std::map<std::string_view, const le_audio::CodecConfiguration*>& codec_cfgs,
            const std::map<std::string_view, const le_audio::QosConfiguration*>& qos_cfgs,
            CodecLocation location,
            std::vector<std::optional<AseDirectionConfiguration>>& sourceAseConfiguration,
            std::vector<std::optional<AseDirectionConfiguration>>& sinkAseConfiguration,
            ConfigurationFlags& configurationFlags);

    static LeAudioDataPathConfiguration PopulateDatapath(const CodecLocation& location,
                                                         const LeAudioAseConfiguration& ase);

    static bool LoadConfigurationsFromFiles(const ConfigurationSetFile& files,
                                            CodecLocation location);

    static bool LoadScenariosFromFiles(const ConfigurationSetFile& files);

    static bool LoadConfigurationSetFile(const std::vector<ConfigurationSetFile>& config_files,
                                         const std::vector<ConfigurationSetFile>& scenario_files,
                                         CodecLocation location);

    inline static std::map<std::string, AseConfig, std::less<>> ase_configs_;
    inline static std::vector<std::pair<std::string, LeAudioAseConfigurationSetting>>
            ase_configuration_settings_;
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
