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

package android.hardware.security.factory_reset_protection;

/**
 * IFactoryResetProtection is the interface to a trusted application (TA), running in an
 * isolated environment that is secure from arbitrary compromise of the Android system and
 * kernel. The purpose of this trusted application is to render devices that were reset to a
 * factory-clean state by an unauthorized user useless, to reduce the value of stolen Android
 * devices while preserving the ability of authorized parties to reset devices.
 *
 * Factory Reset Protection (FRP) is in one of two states at any given time: active or inactive, and
 * the current state is held in isolated environment RAM.  While FRP is in the active state, various
 * system and isolated environment functions will not operate correctly, preventing the device from
 * being used normally.  In order to restore normal operation, FRP must be moved to the inactive
 * state, or "deactivated".
 *
 * Unless the bootloader reports that FRP should be disabled, at every boot FRP starts in the active
 * state, restricting device functionality until it is deactivated.  If the bootloader reports
 * that FRP should be disabled, the FRP TA must set the FRP secret to the default value at startup.
 * Note that the mechanism the bootloader uses to report whether FRP should be disabled is
 * implementation-defined.  Implementors are recommended to use a mechanism similar to that
 * used by bootloaders to report the lock state to KeyMint.
 *
 * FRP deactivation
 * ================
 *
 * Deactivation is done by calling the `deactivate()` method, and passing the correct FRP "secret",
 * which is a 32-byte value.  The secret can be set when the TA is in the inactive state by calling
 * the `setFrpSecret()` method.  If the `setFrpSecret()` method has never been called, the FRP
 * secret defaults to all zeros.
 *
 * The IFactoryResetProtection client must arrange to have the secret available for presentation in
 * various contexts.  For example, a plaintext copy may be stored in /data in a location accessible
 * only to the client, for use during a normal boot.  This copy would be destroyed by an
 * unauthorized factory reset, so the client should keep other copies encrypted in ways that can be
 * decrypted with the participation of an authorized party.
 *
 * FRP secret storage
 * ==================
 *
 * To provide deactivation as described above, IFactoryResetProtection implementations must provide
 * secure storage for the FRP secret.  This storage must have the following properties:
 *
 * o   Security: The storage must provide privacy and integrity.  It must not be possible for an
 *     attacker to read or modify the value.  On typical Android TEEs this can be achieved by
 *     storing the data in RPMB.  It is acceptable if the attacker can block reading of the FRP
 *     secret, but the IFactoryResetProtection implementation must not allow deactivation if it
 *     cannot read the secret.
 * o   Durability:  The storage must survive factory reset, as well as being generally reliable.
 *     Loss or corruption of the FRP secret could permanently disable a device.
 *
 * FRP data storage
 * ================
 *
 * IFactoryResetProtection implementations must provide their client with some secure storage, to be
 * used to store copies of the encrypted FRP secrets and related data.  This storage does not
 * require strong security guarantees, since it is not sensitive. It does need to be durable: It
 * must survive factory reset and be generally reliable.
 *
 * KeyMint interaction
 * ===================
 *
 * It is strongly recommended that the IFactoryResetProtection implementation run in the same
 * isolated environment as the default IKeyMintDevice implementation.
 */
@VintfStability
interface IFactoryResetProtection {
    /**
     * Application-specific error code returned by IFactoryResetProtection methods to indicate a
     * general error.
     */
    const int STATUS_FAILED = 1;

    /**
     * Application-specific error code returned by IFactoryResetProtection methods to indicate
     * that the requested operation failed because FRP is active.
     */
    const int STATUS_FRP_IS_ACTIVE = 2;

    /**
     * Application-specific error code returned by IFactoryResetProtection methods to indicate
     * that the requested operation failed because input arguments were invalid.
     */
    const int STATUS_ILLEGAL_ARGUMENT = 3;

    /**
     * Application-specific error code returned by IFactoryResetProtection methods to indicate
     * that the requested operation failed because it is unsupported.
     */
    const int STATUS_UNSUPPORTED = 4;

    /**
     * Returns true if FRP is active. FRP is activated automatically by every reboot, i.e. the
     * FRP trusted app must always start in `active` state, so this method must return true
     * after every boot, until `deactivate()` is called with the correct secret.
     */
    boolean isActive();

    /**
     * Activates FRP, causing `isActive()` to return `true`. The only way to exit the active state
     * is by calling `deactivate()`, passing the FRP secret set with `setFrpSecret()`.
     *
     * This method is generally used only for testing.  In normal operation, FRP is activated
     * automatically by every reboot.
     */
    void activate();

    /**
     * Deactivates FRP iff the caller provides the correct secret.  If the value provided in the
     * `frpSecret` parameter matches the secret stored with `setFrpSecret()` FRP is deactivated
     * (`isActive()` will return false).
     *
     * SECURITY REQUIREMENT: Implementations must take care to prevent side-channel leakage of the
     * FRP secret.  In particular, a constant-time comparison function must be used.
     *
     * @param candidateSecret is the caller-provided value that must match the secret stored
     *        with `setFrpSecret()`.
     *
     * @return `true` if the secrets matched, `false` if they did not.
     */
    boolean deactivate(in byte[32] candidateSecret);

    /**
     * Stores a new FRP secret, replacing the existing one.
     *
     * If called when FRP is in the `active` state, this method does not change the FRP secret,
     * instead failing status `STATUS_FRP_IS_ACTIVE`.
     *
     * SECURITY REQUIREMENT: The FRP secret must be stored in a location that cannot be read or
     * modified by Android userspace or kernel.
     *
     * RELIABILITY REQUIREMENT: The FRP secret must be fully written to secure storage before
     * this method returns.  If for some reason the write fails, `STATUS_FAILED` must be
     * returned and the previous secret must not be changed.
     */
    void setSecret(in byte[32] newSecret);

    /**
     * Retrieves the FRP secret.
     *
     * If called when FRP is in the `active` state, this method does not return the FRP secret,
     * instead failing with the error code `STATUS_FRP_IS_ACTIVE`.  If for some reason the read
     * fails `STATUS_FAILED` must be returned.
     *
     * This method is generally used only for testing.
     */
    byte[32] getSecret();

    /**
     * Store FRP data in a simple key/value store.
     *
     * The IFactoryResetProtection client stores various pieces of information to enable authorized
     * deactivation of FRP.  The HAL should allow for this information to be stored in TEE secure
     * storage.
     *
     * This feature must allow storage of up to 128 key/value pairs, with a maximum total data size
     * (keys and values) of 16 KiB.  If a storage request would cause either limit to be exceeded,
     * the implementation may return `STATUS_ILLEGAL_ARGUMENT`.  If the specified key is longer than
     * 16 bytes in the UTF-8 encoding, the implementation must return STATUS_ILLEGAL_ARGUMENT.
     *
     * @param key A non-null String indicating what kind of data the associated value is.  Limited
     *        to 16 bytes in UTF-8 encoding.
     *
     * @param value A non-null byte array containing the associated value.
     */
    void storeData(in String key, in byte[] value);

    /**
     * Retrieve FRP data stored with `storeData`.
     *
     * See `storeData` for motivation and limitations.
     *
     * If the specified key was not used to store data, this method must return
     * `STATUS_ILLEGAL_ARGUMENT`.  If for some other reason the read fails, `STATUS_FAILED`
     * must be returned.
     *
     * @param key A non-null String indicating the data to retrieve.
     */
    byte[] retrieveData(in String key);

    /**
     * Delete FRP data stored with `storeData`.
     *
     * See `storeData` for motivation and limitations.
     *
     * If the deletion fails, this method must return `STATUS_FAILED`.
     *
     * @param key A non-null String indicating the data to delete.
     */
    void deleteData(in String key);

    /**
     * Delete all data stored with `storeData`.
     *
     * See `storeData` for motivation and limitations.
     *
     * If the deletion fails, this method must return `STATUS_FAILED`.
     *
     */
    void deleteAllData();
}
