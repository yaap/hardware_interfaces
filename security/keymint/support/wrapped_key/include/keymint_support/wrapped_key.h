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
#pragma once

#include <aidl/android/hardware/security/keymint/ErrorCode.h>
#include <aidl/android/hardware/security/keymint/KeyFormat.h>

#include <keymaster/wrapped_key.h>
#include <keymint_support/authorization_set.h>

namespace aidl::android::hardware::security::keymint {

/**
 * @brief Builds a `KM_WRAPPED_KEY_DESCRIPTION` structure and serializes it to a DER-encoded byte
 * vector.
 *
 * This function constructs a wrapped key description and then converts that object into a standard
 * **PKCS#8 DER** format byte vector.
 *
 * @param authorizationList A reference to the `AuthorizationSet` containing the properties and
 * authorizations of the key to be wrapped.
 * @param keyFormat The format of the key to be wrapped (e.g., raw, PKCS#8).
 * @param derWrappedKeyDescription A reference to a `std::vector<uint8_t>` where the resulting
 * DER-encoded key description will be stored.
 * @return Returns `ErrorCode::OK` on success, or an appropriate `ErrorCode` if a failure occurs.
 */
ErrorCode buildWrappedKeyDescription(const AuthorizationSet& authorizationList, KeyFormat keyFormat,
                                     std::vector<uint8_t>& derWrappedKeyDescription);

/**
 * @brief Constructs a wrapped key structure and serializes it to a DER-encoded byte vector.
 *
 * This function takes the components of a wrapped key (encrypted key, IV, tag, etc.),
 * assembles them into a `KM_WRAPPED_KEY` structure, and then encodes the entire
 * structure into a binary DER format.
 *
 * @param transitKey The public key used to encrypt the key-wrapping key.
 * @param iv The initialization vector (IV) used for symmetric encryption of the secure key.
 * @param encryptedKey The encrypted key payload.
 * @param tag The authentication tag for the encrypted key.
 * @param keyFormat The format of the key being wrapped (e.g., PKCS#8).
 * @param authSet The authorization list (metadata and properties) for the key.
 * @param derWrappedKey A reference to a `std::vector<uint8_t>` where the resulting
 * DER-encoded wrapped key will be stored.
 * @return Returns `ErrorCode::OK` on success, or an appropriate `ErrorCode` on failure.
 */
ErrorCode buildWrappedKey(const std::vector<uint8_t>& transitKey, const std::vector<uint8_t>& iv,
                          const std::vector<uint8_t>& encryptedKey, const std::vector<uint8_t>& tag,
                          KeyFormat keyFormat, const AuthorizationSet& authSet,
                          std::vector<uint8_t>& derWrappedKey);

}  // namespace aidl::android::hardware::security::keymint
