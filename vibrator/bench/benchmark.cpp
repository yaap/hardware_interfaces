/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "benchmark/benchmark.h"

#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <future>

using ::benchmark::Counter;
using ::benchmark::Fixture;
using ::benchmark::kMicrosecond;
using ::benchmark::State;
using ::benchmark::internal::Benchmark;
using ::ndk::enum_range;

using namespace ::aidl::android::hardware::vibrator;
using namespace ::std::chrono_literals;

// Fixed number of iterations for benchmarks that trigger a vibration on the loop.
// They require slow cleanup to ensure a stable state on each run and less noisy metrics.
static constexpr auto VIBRATION_ITERATIONS = 500;

// Timeout to wait for vibration callback completion.
static constexpr auto VIBRATION_CALLBACK_TIMEOUT = 100ms;

// Max duration the vibrator can be turned on, in milliseconds.
static constexpr uint32_t MAX_ON_DURATION_MS = UINT16_MAX;

class VibratorBench : public Fixture {
  public:
    void SetUp(State& /*state*/) override {
        ABinderProcess_setThreadPoolMaxThreadCount(1);
        ABinderProcess_startThreadPool();
        auto serviceName = std::string(IVibrator::descriptor) + "/default";
        this->mVibrator = IVibrator::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(serviceName.c_str())));
    }

    void TearDown(State& /*state*/) override {
        if (mVibrator) {
            mVibrator->off();
            mVibrator->setExternalControl(false);
        }
    }

    static void DefaultConfig(Benchmark* b) { b->Unit(kMicrosecond); }

    static void DefaultArgs(Benchmark* /*b*/) { /* none */ }

  protected:
    std::shared_ptr<IVibrator> mVibrator;

    auto getOtherArg(const State& state, std::size_t index) const { return state.range(index + 0); }

    int32_t hasCapabilities(int32_t capabilities) {
        int32_t deviceCapabilities = 0;
        this->mVibrator->getCapabilities(&deviceCapabilities);
        return (deviceCapabilities & capabilities) == capabilities;
    }

    bool shouldSkipWithError(State& state, const ndk::ScopedAStatus&& status) {
        if (!status.isOk()) {
            state.SkipWithError(status.getMessage());
            return true;
        }
        return false;
    }

    void waitForComplete(std::future<void>& callbackFuture) {
        // Wait until the HAL has finished processing previous vibration before starting a new one,
        // so the HAL state is consistent on each run and metrics are less noisy. Some of the newest
        // HAL implementations are waiting on previous vibration cleanup and might be significantly
        // slower, so make sure we measure vibrations on a clean slate.
        if (callbackFuture.valid()) {
            callbackFuture.wait_for(VIBRATION_CALLBACK_TIMEOUT);
        }
    }

    static void SlowBenchConfig(Benchmark* b) { b->Iterations(VIBRATION_ITERATIONS); }
};

class SlowVibratorBench : public VibratorBench {
  public:
    static void DefaultConfig(Benchmark* b) {
        VibratorBench::DefaultConfig(b);
        SlowBenchConfig(b);
    }
};

class HalCallback : public BnVibratorCallback {
  public:
    HalCallback() = default;
    ~HalCallback() = default;

    ndk::ScopedAStatus onComplete() override {
        mPromise.set_value();
        return ndk::ScopedAStatus::ok();
    }

    std::future<void> getFuture() { return mPromise.get_future(); }

  private:
    std::promise<void> mPromise;
};

#define BENCHMARK_WRAPPER(fixt, test, code)           \
    BENCHMARK_DEFINE_F(fixt, test)                    \
    /* NOLINTNEXTLINE */                              \
    (State & state) {                                 \
        if (!mVibrator) {                             \
            state.SkipWithMessage("HAL unavailable"); \
            return;                                   \
        }                                             \
                                                      \
        code                                          \
    }                                                 \
    BENCHMARK_REGISTER_F(fixt, test)->Apply(fixt::DefaultConfig)->Apply(fixt::DefaultArgs)

BENCHMARK_WRAPPER(SlowVibratorBench, on, {
    auto ms = MAX_ON_DURATION_MS;

    for (auto _ : state) {
        auto cb = hasCapabilities(IVibrator::CAP_ON_CALLBACK)
                          ? ndk::SharedRefBase::make<HalCallback>()
                          : nullptr;
        // Grab the future before callback promise is destroyed by the HAL.
        auto cbFuture = cb ? cb->getFuture() : std::future<void>();

        // Test
        if (shouldSkipWithError(state, mVibrator->on(ms, cb))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(SlowVibratorBench, off, {
    auto ms = MAX_ON_DURATION_MS;

    for (auto _ : state) {
        auto cb = hasCapabilities(IVibrator::CAP_ON_CALLBACK)
                          ? ndk::SharedRefBase::make<HalCallback>()
                          : nullptr;
        // Grab the future before callback promise is destroyed by the HAL.
        auto cbFuture = cb ? cb->getFuture() : std::future<void>();

        // Setup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->on(ms, cb))) {
            return;
        }
        state.ResumeTiming();

        // Test
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(VibratorBench, getCapabilities, {
    int32_t capabilities = 0;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getCapabilities(&capabilities))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench, setAmplitude, {
    auto ms = MAX_ON_DURATION_MS;
    float amplitude = 1.0f;

    if (!hasCapabilities(IVibrator::CAP_AMPLITUDE_CONTROL)) {
        state.SkipWithMessage("amplitude control unavailable");
        return;
    }

    auto cb = hasCapabilities(IVibrator::CAP_ON_CALLBACK)
                      ? ndk::SharedRefBase::make<HalCallback>()
                      : nullptr;
    if (shouldSkipWithError(state, mVibrator->on(ms, cb))) {
        return;
    }

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->setAmplitude(amplitude))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench, setExternalControl, {
    if (!hasCapabilities(IVibrator::CAP_EXTERNAL_CONTROL)) {
        state.SkipWithMessage("external control unavailable");
        return;
    }

    for (auto _ : state) {
        // Test
        if (shouldSkipWithError(state, mVibrator->setExternalControl(true))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->setExternalControl(false))) {
            return;
        }
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(VibratorBench, setExternalAmplitude, {
    auto externalControl = static_cast<int32_t>(IVibrator::CAP_EXTERNAL_CONTROL);
    auto externalAmplitudeControl =
            static_cast<int32_t>(IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL);
    if (!hasCapabilities(externalControl | externalAmplitudeControl)) {
        state.SkipWithMessage("external amplitude control unavailable");
        return;
    }

    if (shouldSkipWithError(state, mVibrator->setExternalControl(true))) {
        return;
    }

    float amplitude = 1.0f;
    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->setAmplitude(amplitude))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench, getSupportedEffects, {
    std::vector<Effect> supportedEffects;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getSupportedEffects(&supportedEffects))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench, getSupportedAlwaysOnEffects, {
    if (!hasCapabilities(IVibrator::CAP_ALWAYS_ON_CONTROL)) {
        state.SkipWithMessage("always on control unavailable");
        return;
    }

    std::vector<Effect> supportedEffects;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getSupportedAlwaysOnEffects(&supportedEffects))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(VibratorBench, getSupportedPrimitives, {
    std::vector<CompositePrimitive> supportedPrimitives;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getSupportedPrimitives(&supportedPrimitives))) {
            return;
        }
    }
});

class EffectsVibratorBench : public VibratorBench {
  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Effect", "Strength"});
        for (const auto& effect : enum_range<Effect>()) {
            for (const auto& strength : enum_range<EffectStrength>()) {
                b->Args({static_cast<long>(effect), static_cast<long>(strength)});
            }
        }
    }

  protected:
    auto getEffect(const State& state) const {
        return static_cast<Effect>(this->getOtherArg(state, 0));
    }

    auto getStrength(const State& state) const {
        return static_cast<EffectStrength>(this->getOtherArg(state, 1));
    }

    bool isEffectSupported(const Effect& effect) {
        std::vector<Effect> supported;
        mVibrator->getSupportedEffects(&supported);
        return std::find(supported.begin(), supported.end(), effect) != supported.end();
    }

    bool isAlwaysOnEffectSupported(const Effect& effect) {
        std::vector<Effect> supported;
        mVibrator->getSupportedAlwaysOnEffects(&supported);
        return std::find(supported.begin(), supported.end(), effect) != supported.end();
    }
};

class SlowEffectsVibratorBench : public EffectsVibratorBench {
  public:
    static void DefaultConfig(Benchmark* b) {
        EffectsVibratorBench::DefaultConfig(b);
        SlowBenchConfig(b);
    }
};

BENCHMARK_WRAPPER(EffectsVibratorBench, alwaysOnEnable, {
    if (!hasCapabilities(IVibrator::CAP_ALWAYS_ON_CONTROL)) {
        state.SkipWithMessage("always on control unavailable");
        return;
    }

    int32_t id = 1;
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    if (!isAlwaysOnEffectSupported(effect)) {
        state.SkipWithMessage("always on effect unsupported");
        return;
    }

    for (auto _ : state) {
        // Test
        if (shouldSkipWithError(state, mVibrator->alwaysOnEnable(id, effect, strength))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->alwaysOnDisable(id))) {
            return;
        }
        state.ResumeTiming();
    }
});

BENCHMARK_WRAPPER(EffectsVibratorBench, alwaysOnDisable, {
    if (!hasCapabilities(IVibrator::CAP_ALWAYS_ON_CONTROL)) {
        state.SkipWithMessage("always on control unavailable");
        return;
    }

    int32_t id = 1;
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    if (!isAlwaysOnEffectSupported(effect)) {
        state.SkipWithMessage("always on effect unsupported");
        return;
    }

    for (auto _ : state) {
        // Setup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->alwaysOnEnable(id, effect, strength))) {
            return;
        }
        state.ResumeTiming();

        // Test
        if (shouldSkipWithError(state, mVibrator->alwaysOnDisable(id))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(SlowEffectsVibratorBench, perform, {
    auto effect = getEffect(state);
    auto strength = getStrength(state);

    if (!isEffectSupported(effect)) {
        state.SkipWithMessage("effect unsupported");
        return;
    }

    int32_t lengthMs = 0;

    for (auto _ : state) {
        auto cb = hasCapabilities(IVibrator::CAP_PERFORM_CALLBACK)
                          ? ndk::SharedRefBase::make<HalCallback>()
                          : nullptr;
        // Grab the future before callback promise is destroyed by the HAL.
        auto cbFuture = cb ? cb->getFuture() : std::future<void>();

        // Test
        if (shouldSkipWithError(state, mVibrator->perform(effect, strength, cb, &lengthMs))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

class PrimitivesVibratorBench : public VibratorBench {
  public:
    static void DefaultArgs(Benchmark* b) {
        b->ArgNames({"Primitive"});
        for (const auto& primitive : enum_range<CompositePrimitive>()) {
            b->Args({static_cast<long>(primitive)});
        }
    }

  protected:
    auto getPrimitive(const State& state) const {
        return static_cast<CompositePrimitive>(this->getOtherArg(state, 0));
    }

    bool isPrimitiveSupported(const CompositePrimitive& primitive) {
        std::vector<CompositePrimitive> supported;
        mVibrator->getSupportedPrimitives(&supported);
        return std::find(supported.begin(), supported.end(), primitive) != supported.end();
    }
};

class SlowPrimitivesVibratorBench : public PrimitivesVibratorBench {
  public:
    static void DefaultConfig(Benchmark* b) {
      PrimitivesVibratorBench::DefaultConfig(b);
      SlowBenchConfig(b);
    }
};

BENCHMARK_WRAPPER(PrimitivesVibratorBench, getCompositionDelayMax, {
    int32_t ms = 0;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getCompositionDelayMax(&ms))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(PrimitivesVibratorBench, getCompositionSizeMax, {
    int32_t size = 0;

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getCompositionSizeMax(&size))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(PrimitivesVibratorBench, getPrimitiveDuration, {
    if (!hasCapabilities(IVibrator::CAP_COMPOSE_EFFECTS)) {
        state.SkipWithMessage("compose effects unavailable");
        return;
    }

    auto primitive = getPrimitive(state);
    int32_t ms = 0;

    if (!isPrimitiveSupported(primitive)) {
        state.SkipWithMessage("primitive unsupported");
        return;
    }

    for (auto _ : state) {
        if (shouldSkipWithError(state, mVibrator->getPrimitiveDuration(primitive, &ms))) {
            return;
        }
    }
});

BENCHMARK_WRAPPER(SlowPrimitivesVibratorBench, compose, {
    if (!hasCapabilities(IVibrator::CAP_COMPOSE_EFFECTS)) {
        state.SkipWithMessage("compose effects unavailable");
        return;
    }

    CompositeEffect effect;
    effect.primitive = getPrimitive(state);
    effect.scale = 1.0f;
    effect.delayMs = 0;

    if (effect.primitive == CompositePrimitive::NOOP) {
        state.SkipWithMessage("skipping primitive NOOP");
        return;
    }
    if (!isPrimitiveSupported(effect.primitive)) {
        state.SkipWithMessage("primitive unsupported");
        return;
    }

    std::vector<CompositeEffect> effects;
    effects.push_back(effect);

    for (auto _ : state) {
        auto cb = ndk::SharedRefBase::make<HalCallback>();
        // Grab the future before callback promise is moved and destroyed by the HAL.
        auto cbFuture = cb->getFuture();

        // Test
        if (shouldSkipWithError(state, mVibrator->compose(effects, cb))) {
            return;
        }

        // Cleanup
        state.PauseTiming();
        if (shouldSkipWithError(state, mVibrator->off())) {
            return;
        }
        waitForComplete(cbFuture);
        state.ResumeTiming();
    }
});

BENCHMARK_MAIN();
