/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.motioncontext;

import android.hardware.motioncontext.ErrorCode;
import android.hardware.motioncontext.MotionEvent;

@VintfStability
oneway interface IMotionContextCallback {
    /**
     * Callback when an error is received.
     *
     * @param errorCode The error code received.
     * @param cookie The cookie from the client when making the request that resulted in this error.
     */
    void onMotionContextError(in ErrorCode errorCode, in int cookie);

    /**
     * Callback when a motion event is received.
     *
     * @param motionEvent The motion event received.
     */
    void onMotionEvent(in MotionEvent motionEvent);
}
