#ifndef NPU_INFERENCERUNNER_H
#define NPU_INFERENCERUNNER_H

#include <future>

#include <utils/RefBase.h>

namespace aidl::android::hardware::npu {

const int DEFAULT_PRIORITY = 500;
const int DEFAULT_ORIGINAL_UID = -1;

struct InferenceOptions {
    int priority = DEFAULT_PRIORITY;
    int originalUid = DEFAULT_ORIGINAL_UID;
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
