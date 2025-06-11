/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.tv.mediaquality;
@VintfStability
enum ParameterName {
  BRIGHTNESS,
  CONTRAST,
  SHARPNESS,
  SATURATION,
  HUE,
  COLOR_TUNER_BRIGHTNESS,
  COLOR_TUNER_SATURATION,
  COLOR_TUNER_HUE,
  COLOR_TUNER_RED_OFFSET,
  COLOR_TUNER_GREEN_OFFSET,
  COLOR_TUNER_BLUE_OFFSET,
  COLOR_TUNER_RED_GAIN,
  COLOR_TUNER_GREEN_GAIN,
  COLOR_TUNER_BLUE_GAIN,
  NOISE_REDUCTION,
  MPEG_NOISE_REDUCTION,
  FLASH_TONE,
  DE_CONTOUR,
  DYNAMIC_LUMA_CONTROL,
  FILM_MODE,
  BLACK_STRETCH,
  BLUE_STRETCH,
  COLOR_TUNE,
  COLOR_TEMPERATURE,
  GLOBE_DIMMING,
  AUTO_PICTUREQUALITY_ENABLED,
  AUTO_SUPER_RESOLUTION_ENABLED,
  LEVEL_RANGE,
  GAMUT_MAPPING,
  PC_MODE,
  LOW_LATENCY,
  VRR,
  CVRR,
  HDMI_RGB_RANGE,
  COLOR_SPACE,
  PANEL_INIT_MAX_LUMINCE_VALID,
  GAMMA,
  COLOR_TEMPERATURE_RED_GAIN,
  COLOR_TEMPERATURE_GREEN_GAIN,
  COLOR_TEMPERATURE_BLUE_GAIN,
  COLOR_TEMPERATURE_RED_OFFSET,
  COLOR_TEMPERATURE_GREEN_OFFSET,
  COLOR_TEMPERATURE_BLUE_OFFSET,
  ELEVEN_POINT_RED,
  ELEVEN_POINT_GREEN,
  ELEVEN_POINT_BLUE,
  LOW_BLUE_LIGHT,
  LD_MODE,
  OSD_RED_GAIN,
  OSD_GREEN_GAIN,
  OSD_BLUE_GAIN,
  OSD_RED_OFFSET,
  OSD_GREEN_OFFSET,
  OSD_BLUE_OFFSET,
  OSD_HUE,
  OSD_SATURATION,
  OSD_CONTRAST,
  COLOR_TUNER_SWITCH,
  COLOR_TUNER_HUE_RED,
  COLOR_TUNER_HUE_GREEN,
  COLOR_TUNER_HUE_BLUE,
  COLOR_TUNER_HUE_CYAN,
  COLOR_TUNER_HUE_MAGENTA,
  COLOR_TUNER_HUE_YELLOW,
  COLOR_TUNER_HUE_FLESH,
  COLOR_TUNER_SATURATION_RED,
  COLOR_TUNER_SATURATION_GREEN,
  COLOR_TUNER_SATURATION_BLUE,
  COLOR_TUNER_SATURATION_CYAN,
  COLOR_TUNER_SATURATION_MAGENTA,
  COLOR_TUNER_SATURATION_YELLOW,
  COLOR_TUNER_SATURATION_FLESH,
  COLOR_TUNER_LUMINANCE_RED,
  COLOR_TUNER_LUMINANCE_GREEN,
  COLOR_TUNER_LUMINANCE_BLUE,
  COLOR_TUNER_LUMINANCE_CYAN,
  COLOR_TUNER_LUMINANCE_MAGENTA,
  COLOR_TUNER_LUMINANCE_YELLOW,
  COLOR_TUNER_LUMINANCE_FLESH,
  BALANCE,
  BASS,
  TREBLE,
  SURROUND_SOUND_ENABLED,
  EQUALIZER_DETAIL,
  SPEAKERS_ENABLED,
  SPEAKERS_DELAY_MS,
  ENHANCED_AUDIO_RETURN_CHANNEL_ENABLED,
  AUTO_VOLUME_CONTROL,
  DOWNMIX_MODE,
  DTS_DRC,
  DOLBY_AUDIO_PROCESSING,
  DOLBY_DIALOGUE_ENHANCER,
  DTS_VIRTUAL_X,
  DIGITAL_OUTPUT,
  DIGITAL_OUTPUT_DELAY_MS,
  SOUND_STYLE,
}
