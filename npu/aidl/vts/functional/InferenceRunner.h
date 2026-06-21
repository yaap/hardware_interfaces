#ifndef NPU_INFERENCERUNNER_H
#define NPU_INFERENCERUNNER_H

#include <future>

#include <utils/RefBase.h>

namespace aidl::android::hardware::npu {

const int DEFAULT_PRIORITY = 500;

struct InferenceOptions {
    int priority = DEFAULT_PRIORITY;
    std::optional<int> originalUid = std::nullopt;
    std::optional<int> uid = std::nullopt;
};

class InferenceRunner : public ::android::RefBase {
  public:
    InferenceRunner();

    bool runInference(const InferenceOptions& options = InferenceOptions());
    std::future<bool> runInferenceAsync(const InferenceOptions& options = InferenceOptions());

    bool checkToolExists();

  private:
    std::string toolPath_;
};

}  // namespace aidl::android::hardware::npu

#endif  // NPU_INFERENCERUNNER_H
