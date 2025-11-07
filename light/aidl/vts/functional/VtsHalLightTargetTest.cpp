/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "light_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android-base/logging.h>
#include <android/hardware/light/ILights.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <unistd.h>
#include <set>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::light::BrightnessMode;
using android::hardware::light::FlashMode;
using android::hardware::light::HwLight;
using android::hardware::light::HwLightEffect;
using android::hardware::light::HwLightState;
using android::hardware::light::ILights;
using android::hardware::light::InterpolationType;
using android::hardware::light::LightType;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

const std::set<LightType> kAllTypes{android::enum_range<LightType>().begin(),
                                    android::enum_range<LightType>().end()};

class LightsAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        lights = android::waitForDeclaredService<ILights>(String16(GetParam().c_str()));
        ASSERT_NE(lights, nullptr);
        ASSERT_TRUE(lights->getLights(&supportedLights).isOk());
    }

    sp<ILights> lights;
    std::vector<HwLight> supportedLights;

    virtual void TearDown() override {
        for (const HwLight& light : supportedLights) {
            HwLightState off;
            off.color = 0x00000000;
            off.flashMode = FlashMode::NONE;
            off.brightnessMode = BrightnessMode::USER;
            EXPECT_TRUE(lights->setLightState(light.id, off).isOk());
        }

        // must leave the device in a useable condition
        for (const HwLight& light : supportedLights) {
            if (light.type == LightType::BACKLIGHT) {
                HwLightState backlightOn;
                backlightOn.color = 0xFFFFFFFF;
                backlightOn.flashMode = FlashMode::TIMED;
                backlightOn.brightnessMode = BrightnessMode::USER;
                EXPECT_TRUE(lights->setLightState(light.id, backlightOn).isOk());
            }
        }
    }

    HwLightEffect buildEffect(int32_t lightId) {
        HwLightEffect effect;
        effect.lightId = lightId;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = 33;
        effect.interpolationType = InterpolationType::LINEAR;

        return effect;
    }
};

class LightsAidlV3 : public LightsAidl {
  public:
    virtual void SetUp() override {
        LightsAidl::SetUp();
        if (lights->getInterfaceVersion() < 3) {
            GTEST_SKIP() << "Skipping a v3 test on an older HAL.";
        }
    }
};

/**
 * Ensure all reported lights actually work.
 */
TEST_P(LightsAidl, TestSupported) {
    HwLightState whiteFlashing;
    whiteFlashing.color = 0xFFFFFFFF;
    whiteFlashing.flashMode = FlashMode::TIMED;
    whiteFlashing.flashOnMs = 100;
    whiteFlashing.flashOffMs = 50;
    whiteFlashing.brightnessMode = BrightnessMode::USER;
    for (const HwLight& light : supportedLights) {
        EXPECT_TRUE(lights->setLightState(light.id, whiteFlashing).isOk());
    }
}

/**
 * Ensure all reported lights have one of the supported types.
 */
TEST_P(LightsAidl, TestSupportedLightTypes) {
    for (const HwLight& light : supportedLights) {
        EXPECT_TRUE(kAllTypes.find(light.type) != kAllTypes.end());
    }
}

/**
 * Ensure all lights have a unique id.
 */
TEST_P(LightsAidl, TestUniqueIds) {
    std::set<int> ids;
    for (const HwLight& light : supportedLights) {
        EXPECT_TRUE(ids.find(light.id) == ids.end());
        ids.insert(light.id);
    }
}

/**
 * Ensure all lights have a unique ordinal for a given type.
 */
TEST_P(LightsAidl, TestUniqueOrdinalsForType) {
    std::map<int, std::set<int>> ordinalsByType;
    for (const HwLight& light : supportedLights) {
        auto& ordinals = ordinalsByType[(int)light.type];
        EXPECT_TRUE(ordinals.find(light.ordinal) == ordinals.end());
        ordinals.insert(light.ordinal);
    }
}

/**
 * Ensure EX_UNSUPPORTED_OPERATION is returned if LOW_PERSISTENCE is not supported.
 */
TEST_P(LightsAidl, TestLowPersistence) {
    HwLightState lowPersistence;
    lowPersistence.color = 0xFF123456;
    lowPersistence.flashMode = FlashMode::TIMED;
    lowPersistence.flashOnMs = 100;
    lowPersistence.flashOffMs = 50;
    lowPersistence.brightnessMode = BrightnessMode::LOW_PERSISTENCE;
    for (const HwLight& light : supportedLights) {
        Status status = lights->setLightState(light.id, lowPersistence);
        EXPECT_TRUE(status.isOk() || Status::EX_UNSUPPORTED_OPERATION == status.exceptionCode());
    }
}

/**
 * Ensure EX_UNSUPPORTED_OPERATION is returns for an invalid light id.
 */
TEST_P(LightsAidl, TestInvalidLightIdUnsupported) {
    int maxId = INT_MIN;
    for (const HwLight& light : supportedLights) {
        maxId = std::max(maxId, light.id);
    }

    Status status = lights->setLightState(maxId + 1, HwLightState());
    EXPECT_TRUE(status.exceptionCode() == Status::EX_UNSUPPORTED_OPERATION);
}

/**
 * Ensure a valid light effect is handled if supported, or the hal returns EX_UNSUPPORTED_OPERATION.
 */
TEST_P(LightsAidlV3, TestLightEffects) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = 33;
        effect.interpolationType = InterpolationType::LINEAR;

        Status status = lights->setLightEffects({effect});

        if (light.minUpdatePeriodMillis > 0) {
            EXPECT_TRUE(status.isOk());
        } else {
            EXPECT_EQ(Status::EX_UNSUPPORTED_OPERATION, status.exceptionCode());
        }
    }
}

/**
 * Ensure infinite effects are supported.
 */
TEST_P(LightsAidlV3, TestEffectsInfiniteIterations) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 0;
        effect.preemptive = false;
        effect.framePeriodMillis = 33;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_TRUE(status.isOk());
        }
    }
}

/**
 * Ensure EX_UNSUPPORTED_OPERATION is returned setting an effect for an invalid light id.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidLightIdUnsupported) {
    int maxId = INT_MIN;
    for (const HwLight& light : supportedLights) {
        maxId = std::max(maxId, light.id);
    }

    HwLightEffect effect;
    effect.lightId = maxId + 1;
    effect.frames = {5};
    effect.colors = {(int32_t)0xFFFFFFFF};
    effect.iterations = 1;
    effect.preemptive = false;
    effect.framePeriodMillis = 33;
    effect.interpolationType = InterpolationType::LINEAR;

    Status status = lights->setLightEffects({effect});

    EXPECT_EQ(Status::EX_UNSUPPORTED_OPERATION, status.exceptionCode());
}

/**
 * Ensure EX_ILLEGAL_ARGUMENT is returned when no frames are specified for the effect.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_emptyFrames) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = 33;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
        }
    }
}

/**
 * Ensure EX_ILLEGAL_ARGUMENT is returned when no colors have been specified for the effect.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_emptyColors) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = 33;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
        }
    }
}

/**
 * Ensure EX_ILLEGAL_ARGUMENT is returned when the number of frames and colors does not match.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_unevenArrays) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5, 10};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = 33;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
        }
    }
}

/**
 * Ensure EX_ILLEGAL_ARGUMENT is returned when the number of iterations is negative.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_negativeIterations) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = -1;
        effect.preemptive = false;
        effect.framePeriodMillis = 33;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
        }
    }
}

/**
 * Ensure EX_ILLEGAL_ARGUMENT is returned when the framerate is set to 0.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_ZeroFrameRate) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = 0;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
        }
    }
}

/**
 * Ensure EX_ILLEGAL_ARGUMENT is returned when the frame rate is negative.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_NegativeFrameRate) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = -10;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
        }
    }
}

/**
 * Ensure EX_ILLEGAL_ARGUMENT is returned when the requested frame rate exceeds the max frame rate
 * specified by the light.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_ExcessiveFrameRate) {
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.iterations = 1;
        effect.preemptive = false;
        effect.framePeriodMillis = light.minUpdatePeriodMillis - 1;
        effect.interpolationType = InterpolationType::LINEAR;

        if (light.minUpdatePeriodMillis > 0) {
            Status status = lights->setLightEffects({effect});
            EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
        }
    }
}

/**
 * Ensure an error is returned when multiple effects are specified and one is invalid.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_OneInvalidEffectInList) {
    std::vector<HwLightEffect> effects;

    // Fill the vector only with lights that support effects.
    for (const HwLight& light : supportedLights) {
        if (light.minUpdatePeriodMillis == 0) {
            continue;
        }

        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.preemptive = false;
        effect.framePeriodMillis = light.minUpdatePeriodMillis;
        effect.interpolationType = InterpolationType::LINEAR;
        effect.iterations = 1;

        // Make the second effect in the list invalid.
        if (effects.size() == 1) {
            effect.iterations = 0;
        }

        effects.push_back(effect);
    }

    if (effects.size() < 2) {
        // Nothing to validate if there is only one light.
        return;
    }

    Status status = lights->setLightEffects(effects);
    EXPECT_EQ(Status::EX_ILLEGAL_ARGUMENT, status.exceptionCode());
}

/**
 * Ensure an error is returned when multiple effects are specified and one ligth doesn't support it.
 */
TEST_P(LightsAidlV3, TestEffectsInvalidEffect_OneInvalidLightInList) {
    std::vector<HwLightEffect> effects;

    // Fill the vector with effects for all lights.
    for (const HwLight& light : supportedLights) {
        HwLightEffect effect;
        effect.lightId = light.id;
        effect.frames = {5};
        effect.colors = {(int32_t)0xFFFFFFFF};
        effect.preemptive = false;
        effect.framePeriodMillis = light.minUpdatePeriodMillis;
        effect.interpolationType = InterpolationType::LINEAR;
        effect.iterations = 1;

        effects.push_back(effect);
    }

    if (supportedLights.size() == effects.size()) {
        // Nothing to validate if all lights support effects
        return;
    }

    Status status = lights->setLightEffects(effects);
    EXPECT_EQ(Status::EX_UNSUPPORTED_OPERATION, status.exceptionCode());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LightsAidl);
INSTANTIATE_TEST_SUITE_P(Lights, LightsAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ILights::descriptor)),
                         android::PrintInstanceNameToString);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LightsAidlV3);
INSTANTIATE_TEST_SUITE_P(Lights, LightsAidlV3,
                         testing::ValuesIn(android::getAidlHalInstanceNames(ILights::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
