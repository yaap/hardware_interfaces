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
#define LOG_TAG "ambient_backlight_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/tv/mediaquality/AmbientBacklightEvent.h>
#include <aidl/android/hardware/tv/mediaquality/AmbientBacklightSettings.h>
#include <aidl/android/hardware/tv/mediaquality/BnMediaQualityCallback.h>
#include <aidl/android/hardware/tv/mediaquality/BnPictureProfileAdjustmentListener.h>
#include <aidl/android/hardware/tv/mediaquality/BnSoundProfileAdjustmentListener.h>
#include <aidl/android/hardware/tv/mediaquality/IMediaQuality.h>
#include <aidl/android/hardware/tv/mediaquality/PictureParameter.h>
#include <aidl/android/hardware/tv/mediaquality/PictureParameters.h>
#include <aidl/android/hardware/tv/mediaquality/PictureProfile.h>
#include <aidl/android/hardware/tv/mediaquality/SoundParameter.h>
#include <aidl/android/hardware/tv/mediaquality/SoundParameters.h>
#include <aidl/android/hardware/tv/mediaquality/SoundProfile.h>
#include <aidl/android/hardware/tv/mediaquality/StreamStatus.h>

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <log/log.h>
#include <future>

using aidl::android::hardware::graphics::common::PixelFormat;
using aidl::android::hardware::tv::mediaquality::AmbientBacklightEvent;
using aidl::android::hardware::tv::mediaquality::AmbientBacklightSettings;
using aidl::android::hardware::tv::mediaquality::AmbientBacklightSource;
using aidl::android::hardware::tv::mediaquality::BnMediaQualityCallback;
using aidl::android::hardware::tv::mediaquality::BnPictureProfileAdjustmentListener;
using aidl::android::hardware::tv::mediaquality::BnSoundProfileAdjustmentListener;
using aidl::android::hardware::tv::mediaquality::IMediaQuality;
using aidl::android::hardware::tv::mediaquality::ParamCapability;
using aidl::android::hardware::tv::mediaquality::PictureParameter;
using aidl::android::hardware::tv::mediaquality::PictureParameters;
using aidl::android::hardware::tv::mediaquality::PictureProfile;
using aidl::android::hardware::tv::mediaquality::SoundParameter;
using aidl::android::hardware::tv::mediaquality::SoundParameters;
using aidl::android::hardware::tv::mediaquality::SoundProfile;
using aidl::android::hardware::tv::mediaquality::StreamStatus;
using aidl::android::hardware::tv::mediaquality::VendorParamCapability;
using aidl::android::hardware::tv::mediaquality::VendorParameterIdentifier;
using android::ProcessState;
using android::String16;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

void validateParameterRange0To100(int value) {
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 100);
}

void validateParameterRange0To2047(int value) {
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 2047);
}

void validateColorTemperature(int value) {
    EXPECT_GE(value, -100);
    EXPECT_LE(value, 100);
}

void validatePictureParameter(const PictureParameter& param) {
    switch (param.getTag()) {
        case PictureParameter::Tag::brightness: {
            ALOGD("[validatePictureParameter] validate brightness value");
            float value = param.get<PictureParameter::Tag::brightness>();
            EXPECT_TRUE(value >= 0.0f && value <= 1.0f);
            break;
        }
        case PictureParameter::Tag::contrast: {
            ALOGD("[validatePictureParameter] validate contrast value");
            int value = param.get<PictureParameter::Tag::contrast>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::sharpness: {
            ALOGD("[validatePictureParameter] validate sharpness value");
            int value = param.get<PictureParameter::Tag::sharpness>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::saturation: {
            ALOGD("[validatePictureParameter] validate saturation value");
            int value = param.get<PictureParameter::Tag::saturation>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::hue: {
            ALOGD("[validatePictureParameter] validate hue value");
            int value = param.get<PictureParameter::Tag::hue>();
            EXPECT_GE(value, -50);
            EXPECT_LE(value, 50);
            break;
        }
        case PictureParameter::Tag::colorTunerBrightness: {
            ALOGD("[validatePictureParameter] validate colorTunerBrightness value");
            int value = param.get<PictureParameter::Tag::colorTunerBrightness>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturation: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturation value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturation>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHue: {
            ALOGD("[validatePictureParameter] validate colorTunerHue value");
            int value = param.get<PictureParameter::Tag::colorTunerHue>();
            EXPECT_GE(value, -50);
            EXPECT_LE(value, 50);
            break;
        }
        case PictureParameter::Tag::colorTunerRedOffset: {
            ALOGD("[validatePictureParameter] validate colorTunerRedOffset value");
            int value = param.get<PictureParameter::Tag::colorTunerRedOffset>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerGreenOffset: {
            ALOGD("[validatePictureParameter] validate colorTunerGreenOffset value");
            int value = param.get<PictureParameter::Tag::colorTunerGreenOffset>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerBlueOffset: {
            ALOGD("[validatePictureParameter] validate colorTunerBlueOffset value");
            int value = param.get<PictureParameter::Tag::colorTunerBlueOffset>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerRedGain: {
            ALOGD("[validatePictureParameter] validate colorTunerRedGain value");
            int value = param.get<PictureParameter::Tag::colorTunerRedGain>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerGreenGain: {
            ALOGD("[validatePictureParameter] validate colorTunerGreenGain value");
            int value = param.get<PictureParameter::Tag::colorTunerGreenGain>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerBlueGain: {
            ALOGD("[validatePictureParameter] validate colorTunerBlueGain value");
            int value = param.get<PictureParameter::Tag::colorTunerBlueGain>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::panelInitMaxLuminceNits: {
            ALOGD("[validatePictureParameter] validate panelInitMaxLuminceNits value");
            int value = param.get<PictureParameter::Tag::panelInitMaxLuminceNits>();
            EXPECT_GE(value, 0);
            EXPECT_LE(value, 10000);
            break;
        }
        case PictureParameter::Tag::colorTemperatureRedGain: {
            ALOGD("[validatePictureParameter] validate colorTemperatureRedGain value");
            int value = param.get<PictureParameter::Tag::colorTemperatureRedGain>();
            validateColorTemperature(value);
            break;
        }
        case PictureParameter::Tag::colorTemperatureGreenGain: {
            ALOGD("[validatePictureParameter] validate colorTemperatureGreenGain value");
            int value = param.get<PictureParameter::Tag::colorTemperatureGreenGain>();
            validateColorTemperature(value);
            break;
        }
        case PictureParameter::Tag::colorTemperatureBlueGain: {
            ALOGD("[validatePictureParameter] validate colorTemperatureBlueGain value");
            int value = param.get<PictureParameter::Tag::colorTemperatureBlueGain>();
            validateColorTemperature(value);
            break;
        }
        case PictureParameter::Tag::colorTemperatureRedOffset: {
            ALOGD("[validatePictureParameter] validate ccolorTemperatureRedOffset value");
            int value = param.get<PictureParameter::Tag::colorTemperatureRedOffset>();
            validateColorTemperature(value);
            break;
        }
        case PictureParameter::Tag::colorTemperatureGreenOffset: {
            ALOGD("[validatePictureParameter] validate colorTemperatureGreenOffset value");
            int value = param.get<PictureParameter::Tag::colorTemperatureGreenOffset>();
            validateColorTemperature(value);
            break;
        }
        case PictureParameter::Tag::colorTemperatureBlueOffset: {
            ALOGD("[validatePictureParameter] validate colorTemperatureBlueOffset value");
            int value = param.get<PictureParameter::Tag::colorTemperatureBlueOffset>();
            validateColorTemperature(value);
            break;
        }
        case PictureParameter::Tag::elevenPointRed: {
            ALOGD("[validatePictureParameter] validate elevenPointRed value");
            std::array<int, 11> elevenPointValues =
                    param.get<PictureParameter::Tag::elevenPointRed>();
            for (int value : elevenPointValues) {
                validateParameterRange0To100(value);
            }
            break;
        }
        case PictureParameter::Tag::elevenPointGreen: {
            ALOGD("[validatePictureParameter] validate elevenPointGreen value");
            std::array<int, 11> elevenPointValues =
                    param.get<PictureParameter::Tag::elevenPointGreen>();
            for (int value : elevenPointValues) {
                validateParameterRange0To100(value);
            }
            break;
        }
        case PictureParameter::Tag::elevenPointBlue: {
            ALOGD("[validatePictureParameter] validate elevenPointBlue value");
            std::array<int, 11> elevenPointValues =
                    param.get<PictureParameter::Tag::elevenPointBlue>();
            for (int value : elevenPointValues) {
                validateParameterRange0To100(value);
            }
            break;
        }
        case PictureParameter::Tag::osdRedGain: {
            ALOGD("[validatePictureParameter] validate osdRedGain value");
            int value = param.get<PictureParameter::Tag::osdRedGain>();
            validateParameterRange0To2047(value);
            break;
        }
        case PictureParameter::Tag::osdGreenGain: {
            ALOGD("[validatePictureParameter] validate osdGreenGain value");
            int value = param.get<PictureParameter::Tag::osdGreenGain>();
            validateParameterRange0To2047(value);
            break;
        }
        case PictureParameter::Tag::osdBlueGain: {
            ALOGD("[validatePictureParameter] validate osdBlueGain value");
            int value = param.get<PictureParameter::Tag::osdBlueGain>();
            validateParameterRange0To2047(value);
            break;
        }
        case PictureParameter::Tag::osdRedOffset: {
            ALOGD("[validatePictureParameter] validate osdRedOffset value");
            int value = param.get<PictureParameter::Tag::osdRedOffset>();
            validateParameterRange0To2047(value);
            break;
        }
        case PictureParameter::Tag::osdGreenOffset: {
            ALOGD("[validatePictureParameter] validate osdGreenOffset value");
            int value = param.get<PictureParameter::Tag::osdGreenOffset>();
            validateParameterRange0To2047(value);
            break;
        }
        case PictureParameter::Tag::osdBlueOffset: {
            ALOGD("[validatePictureParameter] validate osdBlueOffset value");
            int value = param.get<PictureParameter::Tag::osdBlueOffset>();
            validateParameterRange0To2047(value);
            break;
        }
        case PictureParameter::Tag::osdHue: {
            ALOGD("[validatePictureParameter] validate osdHue value");
            int value = param.get<PictureParameter::Tag::osdHue>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::osdSaturation: {
            ALOGD("[validatePictureParameter] validate osdSaturation value");
            int value = param.get<PictureParameter::Tag::osdSaturation>();
            EXPECT_GE(value, 0);
            EXPECT_LE(value, 255);
            break;
        }
        case PictureParameter::Tag::osdContrast: {
            ALOGD("[validatePictureParameter] validate osdContrast value");
            int value = param.get<PictureParameter::Tag::osdContrast>();
            validateParameterRange0To2047(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHueRed: {
            ALOGD("[validatePictureParameter] validate colorTunerHueRed value");
            int value = param.get<PictureParameter::Tag::colorTunerHueRed>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHueGreen: {
            ALOGD("[validatePictureParameter] validate colorTunerHueGreen value");
            int value = param.get<PictureParameter::Tag::colorTunerHueGreen>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHueBlue: {
            ALOGD("[validatePictureParameter] validate colorTunerHueBlue value");
            int value = param.get<PictureParameter::Tag::colorTunerHueBlue>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHueCyan: {
            ALOGD("[validatePictureParameter] validate colorTunerHueCyan value");
            int value = param.get<PictureParameter::Tag::colorTunerHueCyan>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHueMagenta: {
            ALOGD("[validatePictureParameter] validate colorTunerHueMagenta value");
            int value = param.get<PictureParameter::Tag::colorTunerHueMagenta>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHueYellow: {
            ALOGD("[validatePictureParameter] validate colorTunerHueYellow value");
            int value = param.get<PictureParameter::Tag::colorTunerHueYellow>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerHueFlesh: {
            ALOGD("[validatePictureParameter] validate colorTunerHueFlesh value");
            int value = param.get<PictureParameter::Tag::colorTunerHueFlesh>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturationRed: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturationRed value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturationRed>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturationGreen: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturationGreen value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturationGreen>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturationBlue: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturationBlue value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturationBlue>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturationCyan: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturationCyan value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturationCyan>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturationMagenta: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturationMagenta value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturationMagenta>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturationYellow: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturationYellow value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturationYellow>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerSaturationFlesh: {
            ALOGD("[validatePictureParameter] validate colorTunerSaturationFlesh value");
            int value = param.get<PictureParameter::Tag::colorTunerSaturationFlesh>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerLuminanceRed: {
            ALOGD("[validatePictureParameter] validate colorTunerLuminanceRed value");
            int value = param.get<PictureParameter::Tag::colorTunerLuminanceRed>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerLuminanceGreen: {
            ALOGD("[validatePictureParameter] validate colorTunerLuminanceGreen value");
            int value = param.get<PictureParameter::Tag::colorTunerLuminanceGreen>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerLuminanceBlue: {
            ALOGD("[validatePictureParameter] validate colorTunerLuminanceBlue value");
            int value = param.get<PictureParameter::Tag::colorTunerLuminanceBlue>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerLuminanceCyan: {
            ALOGD("[validatePictureParameter] validate colorTunerLuminanceCyan value");
            int value = param.get<PictureParameter::Tag::colorTunerLuminanceCyan>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerLuminanceMagenta: {
            ALOGD("[validatePictureParameter] validate colorTunerLuminanceMagenta value");
            int value = param.get<PictureParameter::Tag::colorTunerLuminanceMagenta>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerLuminanceYellow: {
            ALOGD("[validatePictureParameter] validate colorTunerLuminanceYellow value");
            int value = param.get<PictureParameter::Tag::colorTunerLuminanceYellow>();
            validateParameterRange0To100(value);
            break;
        }
        case PictureParameter::Tag::colorTunerLuminanceFlesh: {
            ALOGD("[validatePictureParameter] validate colorTunerLuminanceFlesh value");
            int value = param.get<PictureParameter::Tag::colorTunerLuminanceFlesh>();
            validateParameterRange0To100(value);
            break;
        }
        default:
            ALOGD("Those parameters don't need to check.");
            break;
    }
}

void validateSoundParameter(const SoundParameter& param) {
    switch (param.getTag()) {
        case SoundParameter::Tag::balance: {
            ALOGD("[validateSoundParameter] validate balance value");
            int value = param.get<SoundParameter::Tag::balance>();
            EXPECT_GE(value, -50);
            EXPECT_LE(value, 50);
            break;
        }
        case SoundParameter::Tag::bass: {
            ALOGD("[validateSoundParameter] validate bass value");
            int value = param.get<SoundParameter::Tag::bass>();
            validateParameterRange0To100(value);
            break;
        }
        case SoundParameter::Tag::treble: {
            ALOGD("[validateSoundParameter] validate treble value");
            int value = param.get<SoundParameter::Tag::treble>();
            validateParameterRange0To100(value);
            break;
        }
        case SoundParameter::Tag::speakersDelayMs: {
            ALOGD("[validateSoundParameter] validate speakersDelayMs value");
            int value = param.get<SoundParameter::Tag::speakersDelayMs>();
            EXPECT_GE(value, 0);
            EXPECT_LE(value, 250);
            break;
        }
        case SoundParameter::Tag::digitalOutputDelayMs: {
            ALOGD("[validateSoundParameter] validate digitalOutputDelayMs value");
            int value = param.get<SoundParameter::Tag::digitalOutputDelayMs>();
            EXPECT_GE(value, 0);
            EXPECT_LE(value, 250);
            break;
        }
        default:
            ALOGD("Those parameters don't need to check.");
            break;
    }
}

class MediaQualityCallback : public BnMediaQualityCallback {
  public:
    explicit MediaQualityCallback(
            const std::function<void(const AmbientBacklightEvent& event)>& on_hal_event_cb)
        : on_hal_event_cb_(on_hal_event_cb) {}
    ScopedAStatus notifyAmbientBacklightEvent(const AmbientBacklightEvent& event) override {
        on_hal_event_cb_(event);
        return ScopedAStatus::ok();
    }

  private:
    std::function<void(const AmbientBacklightEvent& event)> on_hal_event_cb_;
};

class PictureProfileAdjustmentListener : public BnPictureProfileAdjustmentListener {
  public:
    explicit PictureProfileAdjustmentListener(
            const std::function<void(const PictureProfile& pictureProfile)>&
                    on_hal_picture_profile_adjust)
        : on_hal_picture_profile_adjust_(on_hal_picture_profile_adjust) {}

    ScopedAStatus onPictureProfileAdjusted(const PictureProfile& pictureProfile) override {
        for (const auto& param : pictureProfile.parameters.pictureParameters) {
            validatePictureParameter(param);
        }
        on_hal_picture_profile_adjust_(pictureProfile);
        return ScopedAStatus::ok();
    }

    ScopedAStatus onParamCapabilityChanged(int64_t, const std::vector<ParamCapability>&) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onVendorParamCapabilityChanged(int64_t,
                                                 const std::vector<VendorParamCapability>&) {
        return ScopedAStatus::ok();
    }

    ScopedAStatus requestPictureParameters(int64_t) { return ScopedAStatus::ok(); }

    ScopedAStatus onStreamStatusChanged(int64_t, StreamStatus) { return ScopedAStatus::ok(); }

  private:
    std::function<void(const PictureProfile& pictureProfile)> on_hal_picture_profile_adjust_;
};

class SoundProfileAdjustmentListener : public BnSoundProfileAdjustmentListener {
  public:
    explicit SoundProfileAdjustmentListener(
            const std::function<void(const SoundProfile& soundProfile)>&
                    on_hal_sound_profile_adjust)
        : on_hal_sound_profile_adjust_(on_hal_sound_profile_adjust) {}

    ScopedAStatus onSoundProfileAdjusted(const SoundProfile& soundProfile) override {
        for (const auto& param : soundProfile.parameters.soundParameters) {
            validateSoundParameter(param);
        }
        on_hal_sound_profile_adjust_(soundProfile);
        return ScopedAStatus::ok();
    }

    ScopedAStatus onParamCapabilityChanged(int64_t, const std::vector<ParamCapability>&) override {
        return ScopedAStatus::ok();
    }

    ScopedAStatus onVendorParamCapabilityChanged(int64_t,
                                                 const std::vector<VendorParamCapability>&) {
        return ScopedAStatus::ok();
    }

    ScopedAStatus requestSoundParameters(int64_t) { return ScopedAStatus::ok(); }

  private:
    std::function<void(const SoundProfile& soundProfile)> on_hal_sound_profile_adjust_;
};

class MediaQualityAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        mediaquality = IMediaQuality::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(mediaquality, nullptr);
    }
    std::shared_ptr<IMediaQuality> mediaquality;
};

TEST_P(MediaQualityAidl, TestSetAmbientBacklightDetectionEnabled) {
    std::promise<void> open_cb_promise;
    std::future<void> open_cb_future{open_cb_promise.get_future()};
    std::shared_ptr<MediaQualityCallback> callback =
            ndk::SharedRefBase::make<MediaQualityCallback>([&open_cb_promise](auto event) {
                EXPECT_EQ(event.getTag(), AmbientBacklightEvent::Tag::enabled);
                EXPECT_EQ(event.template get<AmbientBacklightEvent::Tag::enabled>(), true);
                open_cb_promise.set_value();
                return ScopedAStatus::ok();
            });
    ASSERT_OK(mediaquality->setAmbientBacklightCallback(callback));
    ASSERT_OK(mediaquality->setAmbientBacklightDetectionEnabled(true));
    std::chrono::milliseconds timeout{10000};
    EXPECT_EQ(open_cb_future.wait_for(timeout), std::future_status::ready);
}

TEST_P(MediaQualityAidl, TestGetAmbientBacklightDetectionEnabled) {
    bool enabled;
    ASSERT_OK(mediaquality->getAmbientBacklightDetectionEnabled(&enabled));
}

TEST_P(MediaQualityAidl, TestSetMediaQualityCallback) {
    std::shared_ptr<MediaQualityCallback> callback = ndk::SharedRefBase::make<MediaQualityCallback>(
            [](auto /* event */) { return ScopedAStatus::ok(); });
    ASSERT_OK(mediaquality->setAmbientBacklightCallback(callback));
}

TEST_P(MediaQualityAidl, TestGetPictureProfileChangedListener) {
    std::shared_ptr<::aidl::android::hardware::tv::mediaquality::IPictureProfileChangedListener>
            aidlListener;
    mediaquality->getPictureProfileListener(&aidlListener);
}

TEST_P(MediaQualityAidl, TestGetSoundProfileChangedListener) {
    std::shared_ptr<::aidl::android::hardware::tv::mediaquality::ISoundProfileChangedListener>
            aidlListener;
    mediaquality->getSoundProfileListener(&aidlListener);
}

TEST_P(MediaQualityAidl, TestSetPictureProfileAdjustmentListener) {
    std::shared_ptr<PictureProfileAdjustmentListener> listener =
            ndk::SharedRefBase::make<PictureProfileAdjustmentListener>(
                    [](auto /*picture profile*/) { return ScopedAStatus::ok(); });
    ASSERT_OK(mediaquality->setPictureProfileAdjustmentListener(listener));
}

TEST_P(MediaQualityAidl, TestSendDefaultPictureParameters) {
    PictureParameters pictureParameters;
    std::vector<PictureParameter> picParams;

    PictureParameter brightnessParam;
    brightnessParam.set<PictureParameter::Tag::brightness>(0.5f);
    picParams.push_back(brightnessParam);

    PictureParameter contrastParam;
    contrastParam.set<PictureParameter::Tag::contrast>(50);
    picParams.push_back(contrastParam);

    pictureParameters.pictureParameters = picParams;
    ASSERT_OK(mediaquality->sendDefaultPictureParameters(pictureParameters));
}

TEST_P(MediaQualityAidl, TestSetSoundProfileAdjustmentListener) {
    std::shared_ptr<SoundProfileAdjustmentListener> listener =
            ndk::SharedRefBase::make<SoundProfileAdjustmentListener>(
                    [](auto /*sound profile*/) { return ScopedAStatus::ok(); });
    ASSERT_OK(mediaquality->setSoundProfileAdjustmentListener(listener));
}

TEST_P(MediaQualityAidl, TestSendDefaultSoundParameters) {
    SoundParameters soundParameters;
    std::vector<SoundParameter> soundParams;

    SoundParameter balanceParam;
    balanceParam.set<SoundParameter::Tag::balance>(50);
    soundParams.push_back(balanceParam);

    SoundParameter bassParam;
    bassParam.set<SoundParameter::Tag::bass>(50);
    soundParams.push_back(bassParam);

    soundParameters.soundParameters = soundParams;
    ASSERT_OK(mediaquality->sendDefaultSoundParameters(soundParameters));
}

TEST_P(MediaQualityAidl, TestSetAmbientBacklightDetector) {
    AmbientBacklightSettings in_settings = {
            .uid = 1,
            .source = AmbientBacklightSource::VIDEO,
            .colorFormat = PixelFormat::RGB_888,
            .hZonesNumber = 32,
            .vZonesNumber = 20,
            .hasLetterbox = true,
            .colorThreshold = 0,
    };
    ASSERT_OK(mediaquality->setAmbientBacklightDetector(in_settings));
}

TEST_P(MediaQualityAidl, TestIsAutoPqSupported) {
    bool supported;
    ASSERT_OK(mediaquality->isAutoPqSupported(&supported));
}

TEST_P(MediaQualityAidl, TestGetAutoPqEnabled) {
    bool enabled;
    ASSERT_OK(mediaquality->getAutoPqEnabled(&enabled));
}

TEST_P(MediaQualityAidl, TestSetAutoPqEnabled) {
    ASSERT_OK(mediaquality->setAutoPqEnabled(true));
}

TEST_P(MediaQualityAidl, TestIsAutoSrSupported) {
    bool supported;
    ASSERT_OK(mediaquality->isAutoSrSupported(&supported));
}

TEST_P(MediaQualityAidl, TestGetAutoSrEnabled) {
    bool enabled;
    ASSERT_OK(mediaquality->getAutoSrEnabled(&enabled));
}

TEST_P(MediaQualityAidl, TestSetAutoSrEnabled) {
    ASSERT_OK(mediaquality->setAutoSrEnabled(true));
}

TEST_P(MediaQualityAidl, TestIsAutoAqSupported) {
    bool supported;
    ASSERT_OK(mediaquality->isAutoAqSupported(&supported));
}

TEST_P(MediaQualityAidl, TestGetAutoAqEnabled) {
    bool enabled;
    ASSERT_OK(mediaquality->getAutoAqEnabled(&enabled));
}

TEST_P(MediaQualityAidl, TestSetAutoAqEnabled) {
    ASSERT_OK(mediaquality->setAutoAqEnabled(true));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MediaQualityAidl);

INSTANTIATE_TEST_SUITE_P(
        PerInstance, MediaQualityAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(IMediaQuality::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
