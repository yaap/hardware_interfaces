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
//! This module implements the IMediaQuality AIDL interface.

use android_hardware_tv_mediaquality::aidl::android::hardware::tv::mediaquality::{
    IMediaQuality::IMediaQuality,
    IMediaQualityCallback::IMediaQualityCallback,
    AmbientBacklightEvent::AmbientBacklightEvent,
    AmbientBacklightSettings::AmbientBacklightSettings,
    CommonParamCapability::CommonParamCapability,
    IPictureProfileAdjustmentListener::IPictureProfileAdjustmentListener,
    IPictureProfileChangedListener::IPictureProfileChangedListener,
    ParamCapability::ParamCapability,
    ParameterName::ParameterName,
    PictureParameters::PictureParameters,
    PanelTechnologyType::PanelTechnologyType,
    PictureProfile::PictureProfile,
    ISoundProfileAdjustmentListener::ISoundProfileAdjustmentListener,
    ISoundProfileChangedListener::ISoundProfileChangedListener,
    SoundParameters::SoundParameters,
    SoundProfile::SoundProfile,
    VendorParamCapability::VendorParamCapability,
    VendorParameterIdentifier::VendorParameterIdentifier,
    EqualizerCapabilities::EqualizerCapabilities,
    EqualizerDetail::EqualizerDetail,
};
use binder::{Interface, Strong};
use binder::ExceptionCode;
use std::sync::{Arc, Mutex};
use std::thread;

/// Defined so we can implement the IMediaQuality AIDL interface.
pub struct MediaQualityService {
    callback: Arc<Mutex<Option<Strong<dyn IMediaQualityCallback>>>>,
    ambient_backlight_supported: Arc<Mutex<bool>>,
    ambient_backlight_enabled: Arc<Mutex<bool>>,
    ambient_backlight_detector_settings: Arc<Mutex<AmbientBacklightSettings>>,
    auto_pq_supported: Arc<Mutex<bool>>,
    auto_pq_enabled: Arc<Mutex<bool>>,
    auto_sr_supported: Arc<Mutex<bool>>,
    auto_sr_enabled: Arc<Mutex<bool>>,
    auto_aq_supported: Arc<Mutex<bool>>,
    auto_aq_enabled: Arc<Mutex<bool>>,
    picture_profile_adjustment_listener:
            Arc<Mutex<Option<Strong<dyn IPictureProfileAdjustmentListener>>>>,
    sound_profile_adjustment_listener:
            Arc<Mutex<Option<Strong<dyn ISoundProfileAdjustmentListener>>>>,
    picture_profile_changed_listener: Arc<Mutex<Option<Strong<dyn IPictureProfileChangedListener>>>>,
    sound_profile_changed_listener: Arc<Mutex<Option<Strong<dyn ISoundProfileChangedListener>>>>,
    equalizer_capabilities: Arc<Mutex<EqualizerCapabilities>>,
    equalizer_settings: Arc<Mutex<EqualizerDetail>>,
    oled_panel_supported: Arc<Mutex<bool>>,
}

impl MediaQualityService {

    /// Create a new instance of the MediaQualityService.
    pub fn new() -> Self {
        Self {
            callback: Arc::new(Mutex::new(None)),
            ambient_backlight_supported: Arc::new(Mutex::new(true)),
            ambient_backlight_enabled: Arc::new(Mutex::new(false)),
            ambient_backlight_detector_settings:
                    Arc::new(Mutex::new(AmbientBacklightSettings::default())),
            auto_pq_supported: Arc::new(Mutex::new(true)),
            auto_pq_enabled: Arc::new(Mutex::new(false)),
            auto_sr_supported: Arc::new(Mutex::new(true)),
            auto_sr_enabled: Arc::new(Mutex::new(false)),
            auto_aq_supported: Arc::new(Mutex::new(true)),
            auto_aq_enabled: Arc::new(Mutex::new(false)),
            picture_profile_adjustment_listener: Arc::new(Mutex::new(None)),
            sound_profile_adjustment_listener: Arc::new(Mutex::new(None)),
            picture_profile_changed_listener: Arc::new(Mutex::new(None)),
            sound_profile_changed_listener: Arc::new(Mutex::new(None)),
            equalizer_capabilities: Arc::new(Mutex::new(EqualizerCapabilities {
                minLevelDb: -1200,
                maxLevelDb: 1200,
                supportedFrequenciesHz: vec![],
                hasAdjustableQ: false,
            })),
            equalizer_settings: Arc::new(Mutex::new(EqualizerDetail {
                band120Hz: 0,
                band500Hz: 0,
                band1_5kHz: 0,
                band5kHz: 0,
                band10kHz: 0,
                bands: vec![],
            })),
            oled_panel_supported: Arc::new(Mutex::new(true)),
        }
    }
}

impl Interface for MediaQualityService {}

impl IMediaQuality for MediaQualityService {

    fn setAmbientBacklightCallback(
        &self,
        callback: &Strong<dyn IMediaQualityCallback>
    ) -> binder::Result<()> {
        println!("Received callback: {:?}", callback);
        let mut cb = self.callback.lock().unwrap();
        *cb = Some(callback.clone());
        Ok(())
    }

    fn setAmbientBacklightDetector(
        &self,
        settings: &AmbientBacklightSettings
    ) -> binder::Result<()> {
        println!("Received settings: {:?}", settings);
        let mut ambient_backlight_detector_settings = self.ambient_backlight_detector_settings.lock().unwrap();
        ambient_backlight_detector_settings.uid = settings.uid.clone();
        ambient_backlight_detector_settings.source = settings.source;
        ambient_backlight_detector_settings.maxFramerate = settings.maxFramerate;
        ambient_backlight_detector_settings.colorFormat = settings.colorFormat;
        ambient_backlight_detector_settings.hZonesNumber = settings.hZonesNumber;
        ambient_backlight_detector_settings.vZonesNumber = settings.vZonesNumber;
        ambient_backlight_detector_settings.hasLetterbox = settings.hasLetterbox;
        ambient_backlight_detector_settings.colorThreshold = settings.colorThreshold;
        Ok(())
    }

    fn setAmbientBacklightDetectionEnabled(&self, enabled: bool) -> binder::Result<()> {
        println!("Received enabled: {}", enabled);
        let mut ambient_backlight_enabled = self.ambient_backlight_enabled.lock().unwrap();
        let ambient_backlight_supported = self.ambient_backlight_supported.lock().unwrap();
        *ambient_backlight_enabled = enabled;

        if *ambient_backlight_supported {
            if enabled {
                println!("Enable Ambient Backlight detection");
                thread::scope(|s| {
                    s.spawn(|| {
                        let cb = self.callback.lock().unwrap();
                        if let Some(cb) = &*cb {
                            let enabled_event = AmbientBacklightEvent::Enabled(true);
                            cb.notifyAmbientBacklightEvent(&enabled_event).unwrap();
                        }
                    });
                });
            } else {
                println!("Disable Ambient Backlight detection");
                thread::scope(|s| {
                    s.spawn(|| {
                        let cb = self.callback.lock().unwrap();
                        if let Some(cb) = &*cb {
                            let disabled_event = AmbientBacklightEvent::Enabled(false);
                            cb.notifyAmbientBacklightEvent(&disabled_event).unwrap();
                        }
                    });
                });
            }
            return Ok(());
        } else {
            return Err(ExceptionCode::UNSUPPORTED_OPERATION.into());
        }
    }

    fn getAmbientBacklightDetectionEnabled(&self) -> binder::Result<bool> {
        let ambient_backlight_enabled = self.ambient_backlight_enabled.lock().unwrap();
        Ok(*ambient_backlight_enabled)
    }

    fn isAutoPqSupported(&self) -> binder::Result<bool> {
        let auto_pq_supported = self.auto_pq_supported.lock().unwrap();
        Ok(*auto_pq_supported)
    }

    fn getAutoPqEnabled(&self) -> binder::Result<bool> {
        let auto_pq_enabled = self.auto_pq_enabled.lock().unwrap();
        Ok(*auto_pq_enabled)
    }

    fn setAutoPqEnabled(&self, enabled: bool) -> binder::Result<()> {
        let mut auto_pq_enabled = self.auto_pq_enabled.lock().unwrap();
        let auto_pq_supported = self.auto_pq_supported.lock().unwrap();
        *auto_pq_enabled = enabled;

        if *auto_pq_supported {
            if enabled {
                println!("Enable auto picture quality");
            } else {
                println!("Disable auto picture quality");
            }
            return Ok(());
        } else {
            return Err(ExceptionCode::UNSUPPORTED_OPERATION.into());
        }
    }

    fn isAutoSrSupported(&self) -> binder::Result<bool> {
        let auto_sr_supported = self.auto_sr_supported.lock().unwrap();
        Ok(*auto_sr_supported)
    }

    fn getAutoSrEnabled(&self) -> binder::Result<bool> {
        let auto_sr_enabled = self.auto_sr_enabled.lock().unwrap();
        Ok(*auto_sr_enabled)
    }

    fn setAutoSrEnabled(&self, enabled: bool) -> binder::Result<()> {
        let mut auto_sr_enabled = self.auto_sr_enabled.lock().unwrap();
        let auto_sr_supported = self.auto_sr_supported.lock().unwrap();
        *auto_sr_enabled = enabled;

        if *auto_sr_supported {
            if enabled {
                println!("Enable auto super resolution");
            } else {
                println!("Disable auto super resolution");
            }
            return Ok(());
        } else {
            return Err(ExceptionCode::UNSUPPORTED_OPERATION.into());
        }
    }

    fn isAutoAqSupported(&self) -> binder::Result<bool> {
        let auto_aq_supported = self.auto_aq_supported.lock().unwrap();
        Ok(*auto_aq_supported)
    }

    fn getAutoAqEnabled(&self) -> binder::Result<bool> {
        let auto_aq_enabled = self.auto_aq_enabled.lock().unwrap();
        Ok(*auto_aq_enabled)
    }

    fn setAutoAqEnabled(&self, enabled: bool) -> binder::Result<()> {
        let mut auto_aq_enabled = self.auto_aq_enabled.lock().unwrap();
        let auto_aq_supported = self.auto_aq_supported.lock().unwrap();
        *auto_aq_enabled = enabled;

        if *auto_aq_supported {
            if enabled {
                println!("Enable auto audio quality");
            } else {
                println!("Disable auto audio quality");
            }
            return Ok(());
        } else {
            return Err(ExceptionCode::UNSUPPORTED_OPERATION.into());
        }
    }

    fn getPictureProfileListener(&self) -> binder::Result<binder::Strong<dyn IPictureProfileChangedListener>> {
        println!("getPictureProfileListener");
        let listener = self.picture_profile_changed_listener.lock().unwrap();
        Ok(listener.clone().expect("NONE"))
    }

    fn setPictureProfileAdjustmentListener(
        &self,
        picture_profile_adjustment_listener: &Strong<dyn IPictureProfileAdjustmentListener>
    ) -> binder::Result<()> {
        println!("Received picture profile adjustment");
        let mut listener = self.picture_profile_adjustment_listener.lock().unwrap();
        *listener = Some(picture_profile_adjustment_listener.clone());
        Ok(())
    }

    fn sendDefaultPictureParameters(&self, _picture_parameters: &PictureParameters) -> binder::Result<()>{
        println!("Received picture parameters");
        Ok(())
    }

    fn setMutedColor(&self, color: i32) -> binder::Result<()> {
        println!("setMutedColor called with color: {}", color);
        Ok(())
    }

    fn setColorMuteEnabled(&self, enable: bool) -> binder::Result<()> {
        println!("setColorMuteEnabled called with enable: {}", enable);
        Ok(())
    }

    fn getSoundProfileListener(&self) -> binder::Result<binder::Strong<dyn ISoundProfileChangedListener>> {
        println!("getSoundProfileListener");
        let listener = self.sound_profile_changed_listener.lock().unwrap();
        listener.clone().ok_or(binder::StatusCode::UNKNOWN_ERROR.into())
    }

    fn setSoundProfileAdjustmentListener(
        &self,
        sound_profile_adjustment_listener: &Strong<dyn ISoundProfileAdjustmentListener>
    ) -> binder::Result<()> {
        println!("Received sound profile adjustment");
        let mut listener = self.sound_profile_adjustment_listener.lock().unwrap();
        *listener = Some(sound_profile_adjustment_listener.clone());
        Ok(())
    }

    fn sendDefaultSoundParameters(&self, _sound_parameters: &SoundParameters) -> binder::Result<()>{
        println!("Received sound parameters");
        Ok(())
    }

    fn getParamCaps(
            &self,
            param_names: &[ParameterName],
            caps: &mut Vec<ParamCapability>
    ) -> binder::Result<()> {
        println!("getParamCaps. len= {}", param_names.len());
        for name in param_names {
            caps.push(ParamCapability {
                name: name.clone(),
                isSupported: true,
                defaultValue: None,
                range: None,
                commonParamCapability: Some(CommonParamCapability { isMutable: true }),
            });
        }
        Ok(())
    }

    fn sendDefaultPictureProfile(
        &self,
        _picture_profile: &PictureProfile,
    ) -> binder::Result<()> {
        println!("Received default picture profile");
        Ok(())
    }

    fn sendDefaultSoundProfile(
        &self,
        _sound_profile: &SoundProfile,
    ) -> binder::Result<()> {
        println!("Received default sound profile with id");
        Ok(())
    }

    fn getVendorParamCaps(
            &self,
            param_names: &[VendorParameterIdentifier],
            caps: &mut Vec<VendorParamCapability>
    ) -> binder::Result<()> {
        println!("getVendorParamCaps. len= {}", param_names.len());
        for name in param_names {
            caps.push(VendorParamCapability {
                identifier: VendorParameterIdentifier { identifier: name.identifier.clone() },
                isSupported: true,
                defaultValue: None,
                range: None,
                commonParamCapability: Some(CommonParamCapability { isMutable: true }),
            });
        }
        Ok(())
    }

    fn getEqualizerCapabilities(&self) -> binder::Result<EqualizerCapabilities> {
        println!("HAL: getEqualizerCapabilities called");
        let caps = self.equalizer_capabilities.lock().unwrap();
        Ok((*caps).clone())
    }

    fn getEqualizerSettings(&self) -> binder::Result<EqualizerDetail> {
        println!("HAL: getEqualizerSettings called");
        let settings = self.equalizer_settings.lock().unwrap();
        Ok((*settings).clone())
    }

    fn setEqualizerSettings(&self, detail: &EqualizerDetail) -> binder::Result<()> {
        println!("HAL: setEqualizerSettings called");
        let mut settings = self.equalizer_settings.lock().unwrap();
        *settings = detail.clone();
        Ok(())
    }

    fn isDisplayTechnologySupported(
        &self,
        panel_technology: PanelTechnologyType,
    ) -> binder::Result<bool> {
        println!(
            "isDisplayTechnologySupported called with type: {:?}",
            panel_technology
        );
        match panel_technology {
            PanelTechnologyType::OLED => {
                let supported = self.oled_panel_supported.lock().unwrap();
                Ok(*supported)
            }
            _ => {
                Ok(false)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn test_picture_params_range() {
        let service = MediaQualityService::new();
        // Test valid values (including boundaries)
        let params_valid = PictureParameters {
            pictureParameters: vec![
                PictureParameter::MemcDeblur(0),
                PictureParameter::MemcDejudder(10),
            ],
            vendorPictureParameters: Default::default(),
        };
        assert!(service.sendDefaultPictureParameters(&params_valid).is_ok());

        // Test invalid value (too high)
        let params_invalid_high = PictureParameters {
            pictureParameters: vec![PictureParameter::MemcDeblur(11)],
            vendorPictureParameters: Default::default(),
        };
        assert!(service.sendDefaultPictureParameters(&params_invalid_high).is_err());

        // Test invalid value (too low)
        let params_invalid_low = PictureParameters {
            pictureParameters: vec![PictureParameter::MemcDejudder(-1)],
            vendorPictureParameters: Default::default(),
        };
        assert!(service.sendDefaultPictureParameters(&params_invalid_low).is_err());
    }

    #[test]
    fn test_mt_latency_us_range() {
        let service = MediaQualityService::new();

        // Test with a valid (non-negative) value
        let params_valid = SoundParameters {
            soundParameters: vec![SoundParameter::MtLatencyUs(500)],
            vendorSoundParameters: Default::default(),
        };
        assert!(service.sendDefaultSoundParameters(&params_valid).is_ok());

        // Test with another valid value (boundary case)
        let params_boundary = SoundParameters {
            soundParameters: vec![SoundParameter::MtLatencyUs(0)],
            vendorSoundParameters: Default::default(),
        };
        assert!(service.sendDefaultSoundParameters(&params_boundary).is_ok());

        // Test with an invalid (negative) value
        let params_invalid = SoundParameters {
            soundParameters: vec![SoundParameter::MtLatencyUs(-1)],
            vendorSoundParameters: Default::default(),
        };
        assert!(service.sendDefaultSoundParameters(&params_invalid).is_err());
    }
}
