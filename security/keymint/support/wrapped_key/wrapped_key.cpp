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

#include <keymint_support/wrapped_key.h>

#include <android-base/logging.h>

#include <KeyMintUtils.h>
#include <keymaster/km_openssl/openssl_utils.h>

namespace aidl::android::hardware::security::keymint {
using keymaster::ASN1_INTEGER_Delete;
using keymaster::BIGNUM_Delete;
using keymaster::d2i_KM_WRAPPED_KEY;
using keymaster::d2i_KM_WRAPPED_KEY_DESCRIPTION;
using keymaster::i2d_KM_WRAPPED_KEY;
using keymaster::i2d_KM_WRAPPED_KEY_DESCRIPTION;
using keymaster::KM_AUTH_LIST;
using keymaster::KM_WRAPPED_KEY;
using keymaster::KM_WRAPPED_KEY_DESCRIPTION;
using keymaster::KM_WRAPPED_KEY_DESCRIPTION_free;
using keymaster::KM_WRAPPED_KEY_DESCRIPTION_new;
using keymaster::KM_WRAPPED_KEY_free;
using keymaster::KM_WRAPPED_KEY_new;
using keymaster::OpenSslObjectDeleter;
using keymaster::UniquePtr;

namespace {

DEFINE_OPENSSL_OBJECT_POINTER(KM_WRAPPED_KEY_DESCRIPTION)
DEFINE_OPENSSL_OBJECT_POINTER(KM_WRAPPED_KEY)

keymaster::AuthorizationSet aidlAuthorizationSet2KmAuthorizationSet(
        const AuthorizationSet& authorizationList) {
    return keymaster::AuthorizationSet(km_utils::aidlKeyParams2Km(authorizationList.vector_data()));
}

ErrorCode build_auth_list(const AuthorizationSet& authorizationList, KM_AUTH_LIST* record) {
    auto err = keymaster::build_auth_list(
            aidlAuthorizationSet2KmAuthorizationSet(authorizationList), record);
    if (err != KM_ERROR_OK) {
        LOG(ERROR) << "Failed to build the auth list.";
        return static_cast<ErrorCode>(err);
    }
    // keymaster::build_auth_list ignores the USER_SECURE_ID tag. so add it here manually.
    auto userSecureId = authorizationList.GetTagValue(TAG_USER_SECURE_ID);
    if (userSecureId.has_value()) {
        keymaster_key_param_t param = {.tag = KM_TAG_USER_SECURE_ID,
                                       .long_integer = static_cast<uint64_t>(*userSecureId)};
        keymaster::auth_list_add_param(param, nullptr /* ASN1_INTEGER_SET** */,
                                       &record->user_secure_id /* ASN1_INTEGER** */,
                                       nullptr /* ASN1_OCTET_STRING** */,
                                       nullptr /* ASN1_NULL** */);
    }
    return ErrorCode::OK;
}

}  // namespace

ErrorCode buildWrappedKeyDescription(const AuthorizationSet& authorizationList, KeyFormat keyFormat,
                                     std::vector<uint8_t>& derWrappedKeyDescription) {
    KM_WRAPPED_KEY_DESCRIPTION_Ptr wrappedKeyDescription(KM_WRAPPED_KEY_DESCRIPTION_new());
    if (!wrappedKeyDescription.get()) {
        LOG(ERROR) << "Memory allocation failed.";
        return ErrorCode::MEMORY_ALLOCATION_FAILED;
    }

    if (!ASN1_INTEGER_set(wrappedKeyDescription->key_format, static_cast<int>(keyFormat))) {
        LOG(ERROR) << "Failed to set the keyformat to key description structure.";
        return ErrorCode::UNKNOWN_ERROR;
    }

    auto err = build_auth_list(authorizationList, wrappedKeyDescription->auth_list);

    if (err != ErrorCode::OK) {
        return err;
    }

    int len = i2d_KM_WRAPPED_KEY_DESCRIPTION(wrappedKeyDescription.get(), nullptr);
    if (len < 0) {
        LOG(ERROR) << "i2d_KM_WRAPPED_KEY_DESCRIPTION failed to get the length.";
        return ErrorCode::UNKNOWN_ERROR;
    }
    derWrappedKeyDescription.resize(len);

    uint8_t* p = derWrappedKeyDescription.data();
    len = i2d_KM_WRAPPED_KEY_DESCRIPTION(wrappedKeyDescription.get(), &p);
    if (len < 0) {
        LOG(ERROR) << "i2d_KM_WRAPPED_KEY_DESCRIPTION failed.";
        return ErrorCode::UNKNOWN_ERROR;
    }
    return ErrorCode::OK;
}

ErrorCode buildWrappedKey(const std::vector<uint8_t>& transitKey, const std::vector<uint8_t>& iv,
                          const std::vector<uint8_t>& encryptedKey, const std::vector<uint8_t>& tag,
                          KeyFormat keyFormat, const AuthorizationSet& authSet,
                          std::vector<uint8_t>& derWrappedKey) {
    UniquePtr<KM_WRAPPED_KEY, KM_WRAPPED_KEY_Delete> wrappedKey(KM_WRAPPED_KEY_new());
    if (!wrappedKey.get()) {
        LOG(ERROR) << "Memory allocation failed.";
        return ErrorCode::MEMORY_ALLOCATION_FAILED;
    }

    if (!ASN1_OCTET_STRING_set(wrappedKey->transit_key, transitKey.data(), transitKey.size()) ||
        !ASN1_OCTET_STRING_set(wrappedKey->iv, iv.data(), iv.size()) ||
        !ASN1_OCTET_STRING_set(wrappedKey->secure_key, encryptedKey.data(), encryptedKey.size()) ||
        !ASN1_OCTET_STRING_set(wrappedKey->tag, tag.data(), tag.size()) ||
        !ASN1_INTEGER_set(wrappedKey->wrapped_key_description->key_format,
                          static_cast<int>(keyFormat))) {
        LOG(ERROR) << "Failed during setting SecureKeyWrapper fields.";
        return ErrorCode::UNKNOWN_ERROR;
    }

    auto err = build_auth_list(authSet, wrappedKey->wrapped_key_description->auth_list);
    if (err != ErrorCode::OK) {
        return err;
    }

    int len = i2d_KM_WRAPPED_KEY(wrappedKey.get(), nullptr);
    if (len < 0) {
        LOG(ERROR) << "i2d_KM_WRAPPED_KEY failed to get the length.";
        return ErrorCode::UNKNOWN_ERROR;
    }
    derWrappedKey.resize(len);

    uint8_t* p = derWrappedKey.data();
    len = i2d_KM_WRAPPED_KEY(wrappedKey.get(), &p);
    if (len < 0) {
        LOG(ERROR) << "i2d_KM_WRAPPED_KEY failed.";
        return ErrorCode::UNKNOWN_ERROR;
    }

    return ErrorCode::OK;
}

}  // namespace aidl::android::hardware::security::keymint
