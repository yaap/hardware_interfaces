/**
 * Copyright (c) 2025, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TestLayer.h"

namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test {

void TestLayer::write(ComposerClientWriter& writer) {
    writer.setLayerDisplayFrame(mDisplay, mLayer, mDisplayFrame);
    writer.setLayerSourceCrop(mDisplay, mLayer, mSourceCrop);
    writer.setLayerZOrder(mDisplay, mLayer, mZOrder);
    writer.setLayerSurfaceDamage(mDisplay, mLayer, mSurfaceDamage);
    writer.setLayerTransform(mDisplay, mLayer, mTransform);
    writer.setLayerPlaneAlpha(mDisplay, mLayer, mAlpha);
    writer.setLayerBlendMode(mDisplay, mLayer, mBlendMode);
    writer.setLayerBrightness(mDisplay, mLayer, mBrightness);
    writer.setLayerDataspace(mDisplay, mLayer, mDataspace);
    if (mLutsSupported) {
        Luts luts{
                .pfd = ::ndk::ScopedFileDescriptor(dup(mLuts.pfd.get())),
                .offsets = mLuts.offsets,
                .lutProperties = mLuts.lutProperties,
        };
        writer.setLayerLuts(mDisplay, mLayer, luts);
    }
}

}  // namespace aidl::android::hardware::graphics::composer3::libhwc_aidl_test