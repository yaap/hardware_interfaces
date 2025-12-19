#include "InferenceRunner.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <android-base/logging.h>

namespace aidl::android::hardware::npu {

const std::filesystem::path kInferenceToolPaths[] = {
        "/data/local/tmp/run-test-inference",
        "/apex/com.android.hardware.npu/bin/run-test-inference"  // cuttlefish
};

InferenceRunner::InferenceRunner() {
    toolPath_ = std::string();
    for (auto path : kInferenceToolPaths) {
        if (std::filesystem::exists(path)) {
            toolPath_ = path;
            break;
        }
    }
}

bool InferenceRunner::runInference(const InferenceOptions& options) {
    if (!checkToolExists()) {
        return false;
    }

    std::string command = toolPath_ + " --job-priority " + std::to_string(options.priority);
    if (options.originalUid >= 0) {
        command += " --original-uid " + std::to_string(options.originalUid);
    }

    if (options.uid >= 0) {
        command = std::string("su ") + std::to_string(options.uid) + " " + command;
    }

    return std::system(command.c_str()) == 0;
}

std::future<bool> InferenceRunner::runInferenceAsync(const InferenceOptions& options) {
    return std::async(std::launch::async, [this, options] { return runInference(options); });
}

bool InferenceRunner::checkToolExists() {
    if (toolPath_.empty()) {
        return false;
    }
    return std::filesystem::exists(toolPath_);
}

}  // namespace aidl::android::hardware::npu