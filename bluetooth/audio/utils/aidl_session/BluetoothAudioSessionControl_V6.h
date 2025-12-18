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

#pragma once

#include "BluetoothAudioSessionControl.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

/***
 * The BluetoothAudioSessionControl_V6 class is to ensure the backware
 * compatibility for the V6 version of Bluetooth Audio HAL where new session
 * types for broadcast sink and peripheral sink are added.
 *
 * All methods except the ones that utilize the new session types should call
 * the corresponding method in BluetoothAudioSessionControl directly.
 */
class BluetoothAudioSessionControl_V6 : public BluetoothAudioSessionControl {
 public:
  /***
   * The control API for the bluetooth_audio module to get current
   * AudioConfiguration
   ***/
  static const AudioConfiguration GetAudioConfig(
      const SessionType& session_type) {
    std::shared_ptr<BluetoothAudioSession> session_ptr =
        BluetoothAudioSessionInstance::GetSessionInstance(session_type);
    if (session_ptr != nullptr) {
      return session_ptr->GetAudioConfig();
    }
    switch (session_type) {
      case SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH:
        return AudioConfiguration(CodecConfiguration{});
      case SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH:
        return AudioConfiguration(HfpConfiguration{});
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH:
      case SessionType::LE_AUDIO_PERIPHERAL_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_PERIPHERAL_OFFLOAD_DECODING_DATAPATH:
        return AudioConfiguration(LeAudioConfiguration{});
      case SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
      case SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_DECODING_DATAPATH:
        return AudioConfiguration(LeAudioBroadcastConfiguration{});
      default:
        return AudioConfiguration(PcmConfiguration{});
    }
  }
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
