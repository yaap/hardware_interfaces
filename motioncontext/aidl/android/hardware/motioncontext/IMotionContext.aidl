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

import android.hardware.motioncontext.IMotionContextCallback;
import android.hardware.motioncontext.IMotionContextClient;

/**
 * Interface for an offloaded Motion Context service. Clients of this service can subscribe to
 * receive events for various levels of motion, and may specify a "dwellTime" to offload simple
 * event filtering. This allows clients to leverage the benefits of the entire suite of available
 * motion context signals on the device while minimizing the power consumption for the client.
 */
@VintfStability
interface IMotionContext {
    /**
     * API for the client to register with the motion context service. Calling this function will
     * return an instance of IMotionContextClient that can be used to configure which motion events
     * the client is interested in receiving. These motion events will be delivered to the callback
     * provided to this function.
     *
     * @param callback The callback to receive motion events.
     *
     * @return The API for the client to interact with the motion context service.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_UNSUPPORTED_OPERATION if this functionality is unsupported.
     */
    IMotionContextClient registerClient(in IMotionContextCallback callback);
}
