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

package android.hardware.contexthub;

/**
 * Describes the level of support for shared data flows with offload endpoints.
 *
 * This type is defined to allow for a more detailed description of the support for this feature on
 * different devices, but for now it just contains a boolean for whether or not the feature is
 * at all supported.
 */
@VintfStability
parcelable SharedDataCapabilities {
    /** Whether shared data flows to/from offload endpoints are supported. */
    boolean dataFlowsSupported;
}