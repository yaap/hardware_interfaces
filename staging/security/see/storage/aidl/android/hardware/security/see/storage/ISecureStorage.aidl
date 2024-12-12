/*
 * Copyright 2024 The Android Open Source Project
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
package android.hardware.security.see.storage;

import android.hardware.security.see.storage.Filesystem;
import android.hardware.security.see.storage.IStorageSession;

/**
 * Interface for the Secure Storage HAL
 *
 * Creates sessions which can be used to access storage.
 */
interface ISecureStorage {
    const int ERR_UNSUPPORTED_PROPERTIES = 1;
    const int ERR_NOT_FOUND = 2;
    const int ERR_ALREADY_EXISTS = 3;
    const int ERR_BAD_TRANSACTION = 4;
    const int ERR_AB_UPDATE_IN_PROGRESS = 5;
    const int ERR_FS_TAMPERED = 6;

    /**
     * Starts a storage session for a filesystem.
     *
     * @filesystem:
     *     The minimum filesystem properties requested.
     *
     * May return service-specific errors:
     *   - ERR_UNSUPPORTED_PROPERTIES if no filesystems exist which meet the minimum requested
     *       requirements
     */
    IStorageSession startSession(in Filesystem filesystem);
}
