/*
 * Copyright 2025 The Android Open Source Project
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
package android.hardware.security.see.devicestate;

/*
 * Interface for querying the device's manufacturing and provisioning state.
 *
 * This interface acts as a guardrail for Trusted Applications (TAs) to ensure
 * that critical provisioning data is only written to secure storage during
 * the manufacturing phase.
 *
 * While this interface is general-purpose for any TA requiring provisioning
 * controls, a primary example is the "KeyMint in VM" feature. In that context,
 * this interface ensures that Device ID data is only written to the KeyMint
 * secure storage backend during manufacturing, preventing subsequent overwrites.
 */
@VintfStability
interface IDeviceState {
    /**
     * Checks if the device is currently in a manufacturing state where
     * provisioning of sensitive data is permitted.
     *
     * This method allows TAs to determine if they should accept sensitive data
     * provisioning.
     *
     * Example Usage:
     * This is used to gate the ability for the OEM to provision device-specific
     * ID data to the KeyMint secure storage backend when KeyMint is running
     * in a VM.
     *
     * Lifecycle & Responsibility:
     *
     * - Factory Phase (True): Vendors/OEMs must ensure this returns true
     * only during the manufacturing phase at the factory. In this state,
     * provisioning data to TAs is allowed.
     *
     * - Post-Factory/Consumer Phase (False): Once the device leaves the
     * factory, the Vendor implementation must ensure this returns false.
     * In this state, provisioning must be blocked to prevent overwriting or
     * tampering.
     *
     * @return true if the device is in the factory manufacturing phase and
     * provisioning is authorized; false otherwise.
     */
    boolean provisioningAllowed();
}
