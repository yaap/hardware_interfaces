/*
 * Copyright 2021 The Android Open Source Project
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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.bluetooth.audio;
@VintfStability
interface IBluetoothAudioProvider {
  void endSession();
  android.hardware.common.fmq.MQDescriptor<byte,android.hardware.common.fmq.SynchronizedReadWrite> startSession(in android.hardware.bluetooth.audio.IBluetoothAudioPort hostIf, in android.hardware.bluetooth.audio.AudioConfiguration audioConfig, in android.hardware.bluetooth.audio.LatencyMode[] supportedLatencyModes);
  void streamStarted(in android.hardware.bluetooth.audio.BluetoothAudioStatus status);
  void streamSuspended(in android.hardware.bluetooth.audio.BluetoothAudioStatus status);
  void updateAudioConfiguration(in android.hardware.bluetooth.audio.AudioConfiguration audioConfig);
  void setLowLatencyModeAllowed(in boolean allowed);
  android.hardware.bluetooth.audio.A2dpStatus parseA2dpConfiguration(in android.hardware.bluetooth.audio.CodecId codecId, in byte[] configuration, out android.hardware.bluetooth.audio.CodecParameters codecParameters);
  @nullable android.hardware.bluetooth.audio.A2dpConfiguration getA2dpConfiguration(in android.hardware.bluetooth.audio.A2dpRemoteCapabilities[] remoteA2dpCapabilities, in android.hardware.bluetooth.audio.A2dpConfigurationHint hint);
  void setCodecPriority(in android.hardware.bluetooth.audio.CodecId codecId, int priority);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseConfigurationSetting[] getLeAudioAseConfiguration(in @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDeviceCapabilities[] remoteSinkAudioCapabilities, in @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDeviceCapabilities[] remoteSourceAudioCapabilities, in android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioConfigurationRequirement[] requirements);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationPair getLeAudioAseQosConfiguration(in android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationRequirement qosRequirement);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfigurationPair getLeAudioAseDatapathConfiguration(in @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.StreamConfig sinkConfig, in @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.StreamConfig sourceConfig);
  void onSinkAseMetadataChanged(in android.hardware.bluetooth.audio.IBluetoothAudioProvider.AseState state, int cigId, int cisId, in @nullable android.hardware.bluetooth.audio.MetadataLtv[] metadata);
  void onSourceAseMetadataChanged(in android.hardware.bluetooth.audio.IBluetoothAudioProvider.AseState state, int cigId, int cisId, in @nullable android.hardware.bluetooth.audio.MetadataLtv[] metadata);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastConfigurationSetting getLeAudioBroadcastConfiguration(in @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDeviceCapabilities[] remoteSinkAudioCapabilities, in android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastConfigurationRequirement requirement);
  android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration getLeAudioBroadcastDatapathConfiguration(in android.hardware.bluetooth.audio.AudioContext audioContext, in android.hardware.bluetooth.audio.LeAudioBroadcastConfiguration.BroadcastStreamMap[] streamMap);
  @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseCodecConfiguredResponse getLeAudioAseCodecConfiguredParameters(in @nullable android.hardware.bluetooth.audio.LeAudioAseConfiguration[] sinkAseConfiguration, in @nullable android.hardware.bluetooth.audio.LeAudioAseConfiguration[] sourceAseConfiguration);
  const int CODEC_PRIORITY_DISABLED = (-1) /* -1 */;
  const int CODEC_PRIORITY_NONE = 0;
  @VintfStability
  parcelable LeAudioDeviceCapabilities {
    android.hardware.bluetooth.audio.CodecId codecId;
    android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv[] codecSpecificCapabilities;
    @nullable byte[] vendorCodecSpecificCapabilities;
    @nullable android.hardware.bluetooth.audio.MetadataLtv[] metadata;
  }
  @VintfStability
  parcelable LeAudioDataPathConfiguration {
    int dataPathId;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration.DataPathConfiguration dataPathConfiguration;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration.IsoDataPathConfiguration isoDataPathConfiguration;
    @VintfStability
    parcelable IsoDataPathConfiguration {
      android.hardware.bluetooth.audio.CodecId codecId;
      boolean isTransparent;
      int controllerDelayUs;
      @nullable byte[] configuration;
    }
    @VintfStability
    parcelable DataPathConfiguration {
      @nullable byte[] configuration;
    }
  }
  @VintfStability
  parcelable LeAudioAseQosConfiguration {
    int sduIntervalUs;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Framing framing;
    android.hardware.bluetooth.audio.Phy[] phy;
    int maxTransportLatencyMs;
    int maxSdu;
    int retransmissionNum;
    int codedRates;
    int hdtRates;
    int hdtMicLength;
    int hdtPacketFormat;
    @nullable int[] maxSduForAbrCodec;
  }
  @Backing(type="byte") @VintfStability
  enum Packing {
    SEQUENTIAL = 0x00,
    INTERLEAVED = 0x01,
  }
  @Backing(type="byte") @VintfStability
  enum Framing {
    UNFRAMED = 0x00,
    FRAMED = 0x01,
  }
  @VintfStability
  parcelable LeAudioAseConfigurationSetting {
    android.hardware.bluetooth.audio.AudioContext audioContext;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Packing packing;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseConfigurationSetting.AseDirectionConfiguration[] sinkAseConfiguration;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseConfigurationSetting.AseDirectionConfiguration[] sourceAseConfiguration;
    @nullable android.hardware.bluetooth.audio.ConfigurationFlags flags;
    @nullable android.hardware.bluetooth.audio.LeAudioUpdateLatencySetting latencySetting;
    @VintfStability
    parcelable AseDirectionConfiguration {
      android.hardware.bluetooth.audio.LeAudioAseConfiguration aseConfiguration;
      @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfiguration qosConfiguration;
      @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration dataPathConfiguration;
    }
  }
  @VintfStability
  parcelable LeAudioConfigurationRequirement {
    android.hardware.bluetooth.audio.AudioContext audioContext;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioConfigurationRequirement.AseDirectionRequirement[] sinkAseRequirement;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioConfigurationRequirement.AseDirectionRequirement[] sourceAseRequirement;
    @nullable android.hardware.bluetooth.audio.ConfigurationFlags flags;
    @VintfStability
    parcelable AseDirectionRequirement {
      android.hardware.bluetooth.audio.LeAudioAseConfiguration aseConfiguration;
    }
  }
  @VintfStability
  parcelable LeAudioAseQosConfigurationRequirement {
    android.hardware.bluetooth.audio.AudioContext audioContext;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationRequirement.AseQosDirectionRequirement sinkAseQosRequirement;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfigurationRequirement.AseQosDirectionRequirement sourceAseQosRequirement;
    @nullable android.hardware.bluetooth.audio.ConfigurationFlags flags;
    @VintfStability
    parcelable AseQosDirectionRequirement {
      android.hardware.bluetooth.audio.IBluetoothAudioProvider.Framing framing;
      android.hardware.bluetooth.audio.Phy[] preferredPhy;
      int preferredRetransmissionNum;
      int maxTransportLatencyMs;
      int presentationDelayMinUs;
      int presentationDelayMaxUs;
      int preferredPresentationDelayMinUs;
      int preferredPresentationDelayMaxUs;
      android.hardware.bluetooth.audio.LeAudioAseConfiguration aseConfiguration;
    }
  }
  @VintfStability
  parcelable LeAudioAseQosConfigurationPair {
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfiguration sinkQosConfiguration;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseQosConfiguration sourceQosConfiguration;
  }
  parcelable LeAudioDataPathConfigurationPair {
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration inputConfig;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration outputConfig;
  }
  parcelable StreamConfig {
    android.hardware.bluetooth.audio.AudioContext audioContext;
    android.hardware.bluetooth.audio.LeAudioConfiguration.StreamMap[] streamMap;
  }
  @Backing(type="byte") @VintfStability
  enum AseState {
    ENABLING = 0x00,
    STREAMING = 0x01,
    DISABLING = 0x02,
  }
  @Backing(type="byte") @VintfStability
  enum BroadcastQuality {
    STANDARD,
    HIGH,
  }
  @VintfStability
  parcelable LeAudioBroadcastSubgroupConfigurationRequirement {
    android.hardware.bluetooth.audio.AudioContext audioContext;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.BroadcastQuality quality;
    int bisNumPerSubgroup;
  }
  @VintfStability
  parcelable LeAudioBroadcastConfigurationRequirement {
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastSubgroupConfigurationRequirement[] subgroupConfigurationRequirements;
  }
  @VintfStability
  parcelable LeAudioSubgroupBisConfiguration {
    int numBis;
    android.hardware.bluetooth.audio.LeAudioBisConfiguration bisConfiguration;
  }
  @VintfStability
  parcelable LeAudioBroadcastSubgroupConfiguration {
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioSubgroupBisConfiguration[] bisConfigurations;
    @nullable byte[] vendorCodecConfiguration;
  }
  @VintfStability
  parcelable LeAudioBroadcastConfigurationSetting {
    int sduIntervalUs;
    int numBis;
    int maxSduOctets;
    int maxTransportLatencyMs;
    int retransmitionNum;
    android.hardware.bluetooth.audio.Phy[] phy;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Packing packing;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Framing framing;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration dataPathConfiguration;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioBroadcastSubgroupConfiguration[] subgroupsConfigurations;
  }
  @VintfStability
  parcelable LeAudioAseCodecConfiguredParameters {
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.Framing framing;
    android.hardware.bluetooth.audio.Phy[] preferredPhy;
    int preferredRetransmissionNum;
    int maxTransportLatencyMs;
    int presentationDelayMinUs;
    int presentationDelayMaxUs;
    int preferredPresentationDelayMinUs;
    int preferredPresentationDelayMaxUs;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioDataPathConfiguration dataPathConfiguration;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseCodecConfiguredParameters.ResponseCode responseCode;
    android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseCodecConfiguredParameters.Reason reason;
    @Backing(type="byte") @VintfStability
    enum ResponseCode {
      SUCCESS = 0x00,
      UNSUPPORTED_CAPABILITIES = 0x06,
      UNSUPPORTED_CONFIGURATION_PARAM = 0x07,
      REJECTED_CONFIGURATION_PARAM = 0x08,
      INVALID_CONFIGURATION_PARAM_VALUE = 0x09,
      INSUFFICIENT_RESOURCES = 0x0D,
      UNSPECIFIED_ERROR = 0x0E,
    }
    @Backing(type="byte") @VintfStability
    enum Reason {
      NO_REASON = 0x00,
      CODEC_ID = 0x01,
      CODEC_SPECIFIC_CONFIGURATION = 0x02,
      SDU_INTERVAL = 0x03,
      FRAMING = 0x04,
      PHY = 0x05,
      MAX_SDU_SIZE = 0x06,
      RTN = 0x07,
      MAX_TRANSPORT_LATENCY = 0x08,
      PRESENTATION_DELAY = 0x09,
    }
  }
  @VintfStability
  parcelable LeAudioAseCodecConfiguredResponse {
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseCodecConfiguredParameters[] sinkAseCodecConfiguredParams;
    @nullable android.hardware.bluetooth.audio.IBluetoothAudioProvider.LeAudioAseCodecConfiguredParameters[] sourceAseCodecConfiguredParams;
  }
}
