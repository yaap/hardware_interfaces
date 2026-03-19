/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.weaver;

import android.hardware.weaver.WeaverConfig;
import android.hardware.weaver.WeaverReadResponse;

/**
 * Weaver provides secure persistent storage of secret values that may only be
 * read if the corresponding key has been presented.
 *
 * The storage must be secure, as the device's user authentication and
 * encryption rely on the security of these values. The cardinality of the
 * domains of the key and value must be suitably large such that they cannot be
 * easily guessed. The recommended key and value sizes are at least 16 bytes.
 *
 * Weaver is structured as an array of slots, each containing a key-value pair.
 * Slots are uniquely identified by an ID in the range [0, `getConfig().slots`).
 * The recommended number of slots is at least 64.
 */
@VintfStability
interface IWeaver {
    /**
     * Retrieves the config information for this implementation of Weaver.
     *
     * The config is static i.e. every invocation returns the same information.
     *
     * @return config data for this implementation of Weaver if status is OK,
     *         otherwise undefined.
     */
    WeaverConfig getConfig();

    /**
     * Don't use these constants. Use the WeaverReadStatus enum values instead.
     *
     * Background: in Weaver AIDL v1, read() was intended to report these
     * statuses via the Binder exception code. However, that design was broken
     * because it didn't allow returning the timeout along with the error.
     * Weaver AIDL v2 fixed this by adding the WeaverReadStatus enum to
     * WeaverReadResponse. So, the enum should be used now, and this separate
     * set of definitions remains only for AIDL backwards compatibility.
     */
    const int STATUS_FAILED = 1;
    const int STATUS_INCORRECT_KEY = 2;
    const int STATUS_THROTTLE = 3;

    /**
     * Attempts to retrieve the value stored in the identified slot.
     *
     * Throttling must be used to limit the frequency of failed read attempts.
     * Throttling must be applied on a per-slot basis, so that a successful read
     * from one slot doesn't reset the throttling state of any other slot. The
     * recommended throttling policy, which is a map from failure count to
     * starting timeout, can be found in the reference code.
     *
     * Each slot's failure count must be stored persistently, so that a reboot
     * doesn't bypass the throttling policy. If an implementation has a secure
     * timer that runs while the device is powered off, then it may preserve the
     * elapsed time across reboots as well. If an implementation doesn't do
     * that, then after reboot it must instead implicitly reset every slot's
     * timeout to the starting timeout for its current failure count.
     *
     * If read() is called on a slot that has a remaining timeout, then it must
     * return a WeaverReadResponse containing WeaverReadStatus.THROTTLE, an
     * empty value, and the remaining timeout. It MUST NOT reveal the actual
     * value, reveal any information about whether the key is correct, or modify
     * the slot's failure count.
     *
     * If read() is called on a slot that has no timeout remaining (i.e., either
     * the slot's timeout has expired or it has never had a timeout), then it
     * first increments and persists the failure count for that slot. If it is
     * unable to persist the incremented failure count, then read() returns a
     * WeaverReadResponse containing WeaverReadStatus.FAILED, an empty value,
     * and a zero timeout. Otherwise:
     *
     *    - If the key is correct, read() resets the slot's failure count to
     *      zero and returns a WeaverReadResponse containing
     *      WeaverReadStatus.OK, the slot's value, and a zero timeout. This is
     *      the only case in which the slot's value is returned.
     *
     *    - If the key is incorrect, read() returns a WeaverReadResponse
     *      containing WeaverReadStatus.INCORRECT_KEY, an empty value, and the
     *      next timeout. Note that the next timeout depends on the throttling
     *      policy and may be either zero or a positive value.
     *
     * To mitigate timing attacks, key comparison must be done in constant time.
     *
     * Implementations MUST NOT consolidate the two updates of the failure
     * counter in the OK case into one. Doing so would require that the key be
     * checked before updating the failure count, opening a race condition that
     * allows a key to be tested without the attempt being counted.
     *
     * @param slotId of the slot to read from.
     * @param key that is stored in the slot.
     * @return The WeaverReadResponse for this read request. If the status is OK,
     * value is set to the value in the slot and timeout is 0. Otherwise, value is
     * empty and timeout is set accordingly.
     */
    WeaverReadResponse read(in int slotId, in byte[] key);

    /**
     * Overwrites the identified slot with the provided key and value, rendering
     * the previous contents of the slot permanently unrecoverable.
     *
     * To remain idempotent, the new key and value are written regardless of the
     * current state of the slot, and there should be no timeout afterward.
     *
     * Service status return:
     *
     * OK if the write was successfully completed.
     * FAILED if the write was unsuccessful.
     *
     * @param slotId of the slot to write to.
     * @param key to write to the slot.
     * @param value to write to the slot.
     */
    void write(in int slotId, in byte[] key, in byte[] value);

    /**
     * Conveys a hint that a read or write probably will occur within a few
     * seconds, e.g. due to the user starting to enter their PIN.
     * Implementations of this method should check if the secure hardware is in
     * a low-power state where it isn't yet ready to process reads and writes.
     * If it is, the secure hardware should be transitioned to a state where it
     * is ready to process reads and writes, so that upcoming reads and/or
     * writes can be processed more quickly. This should be done asynchronously,
     * i.e. this method shouldn't wait for completion of the warm-up.
     *
     * Note that there is no corresponding cool-down method that explicitly
     * tells the HAL that a low-power state should be entered again.
     * Implementations should re-enter a low-power state automatically after the
     * secure hardware has been idle for a certain amount of time. The
     * recommended idle timeout is 5 seconds.
     *
     * This method can be implemented as a no-op if it isn't applicable for the
     * implementation, e.g. if the implementation doesn't have power states.
     *
     * Reads and writes may still occur without a preceding call to warmUp(), so
     * implementations mustn't rely on warmUp() having occurred in order to
     * service reads and writes. However, it's expected that read and write
     * latency may be higher when no warm-up was done.
     *
     * It isn't guaranteed that warmUp() will actually be followed by any read
     * or write. For example, the user may start entering a PIN (triggering a
     * call to warmUp()) and then abandon their attempt.
     */
    oneway void warmUp();

    /**
     * Gets the currently remaining throttling timeout, in milliseconds, for the
     * identified slot.
     *
     * If the implementation cannot provide this information, throw
     * EX_UNSUPPORTED_OPERATION. However, implementations are recommended to
     * provide this information so that the lock screen can accurately show the
     * remaining timeout in all cases.
     *
     * @param slotId of the slot to retrieve the timeout for
     * @return A positive number that gives the remaining timeout in
     * milliseconds, or 0 if there is no remaining timeout (including both the
     * case where there never was a timeout and the case where there originally
     * was a timeout but it has expired).
     * @throws EX_ILLEGAL_ARGUMENT if the slot ID is invalid
     * @throws EX_UNSUPPORTED_OPERATION if this method is not supported
     */
    long getTimeout(in int slotId);
}
