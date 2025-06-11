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
union PictureParameter {
  float brightness;
  int contrast;
  int sharpness;
  int saturation;
  int hue;
  int colorTunerBrightness;
  int colorTunerSaturation;
  int colorTunerHue;
  int colorTunerRedOffset;
  int colorTunerGreenOffset;
  int colorTunerBlueOffset;
  int colorTunerRedGain;
  int colorTunerGreenGain;
  int colorTunerBlueGain;
  android.hardware.tv.mediaquality.QualityLevel noiseReduction;
  android.hardware.tv.mediaquality.QualityLevel mpegNoiseReduction;
  android.hardware.tv.mediaquality.QualityLevel fleshTone;
  android.hardware.tv.mediaquality.QualityLevel deContour;
  android.hardware.tv.mediaquality.QualityLevel dynamicLumaControl;
  boolean filmMode;
  boolean blueStretch;
  boolean colorTune;
  android.hardware.tv.mediaquality.ColorTemperature colorTemperature;
  boolean globeDimming;
  boolean autoPictureQualityEnabled;
  boolean autoSuperResolutionEnabled;
  android.hardware.tv.mediaquality.ColorRange levelRange;
  boolean gamutMapping;
  boolean pcMode;
  boolean lowLatency;
  boolean vrr;
  boolean cvrr;
  android.hardware.tv.mediaquality.ColorRange hdmiRgbRange;
  android.hardware.tv.mediaquality.ColorSpace colorSpace;
  int panelInitMaxLuminceNits;
  boolean panelInitMaxLuminceValid;
  android.hardware.tv.mediaquality.Gamma gamma;
  int colorTemperatureRedGain;
  int colorTemperatureGreenGain;
  int colorTemperatureBlueGain;
  int colorTemperatureRedOffset;
  int colorTemperatureGreenOffset;
  int colorTemperatureBlueOffset;
  int[11] elevenPointRed;
  int[11] elevenPointGreen;
  int[11] elevenPointBlue;
  android.hardware.tv.mediaquality.QualityLevel lowBlueLight;
  android.hardware.tv.mediaquality.QualityLevel LdMode;
  int osdRedGain;
  int osdGreenGain;
  int osdBlueGain;
  int osdRedOffset;
  int osdGreenOffset;
  int osdBlueOffset;
  int osdHue;
  int osdSaturation;
  int osdContrast;
  boolean colorTunerSwitch;
  int colorTunerHueRed;
  int colorTunerHueGreen;
  int colorTunerHueBlue;
  int colorTunerHueCyan;
  int colorTunerHueMagenta;
  int colorTunerHueYellow;
  int colorTunerHueFlesh;
  int colorTunerSaturationRed;
  int colorTunerSaturationGreen;
  int colorTunerSaturationBlue;
  int colorTunerSaturationCyan;
  int colorTunerSaturationMagenta;
  int colorTunerSaturationYellow;
  int colorTunerSaturationFlesh;
  int colorTunerLuminanceRed;
  int colorTunerLuminanceGreen;
  int colorTunerLuminanceBlue;
  int colorTunerLuminanceCyan;
  int colorTunerLuminanceMagenta;
  int colorTunerLuminanceYellow;
  int colorTunerLuminanceFlesh;
  boolean activeProfile;
  android.hardware.tv.mediaquality.PictureQualityEventType pictureQualityEventType;
}
