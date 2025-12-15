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

#pragma once

#include <iostream>
#include <vector>

#include <aidl/android/hardware/security/keymint/Digest.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

namespace aidl::android::hardware::security::keymint {

template <typename T, void (*F)(T*)>
struct UniquePtrDeleter {
    void operator()(T* p) const { F(p); }
};

typedef UniquePtrDeleter<EVP_PKEY, EVP_PKEY_free> EVP_PKEY_Delete;

#define MAKE_OPENSSL_PTR_TYPE(type) \
    typedef std::unique_ptr<type, UniquePtrDeleter<type, type##_free>> type##_Ptr;

MAKE_OPENSSL_PTR_TYPE(ASN1_OBJECT)
MAKE_OPENSSL_PTR_TYPE(BN_CTX)
MAKE_OPENSSL_PTR_TYPE(EVP_CIPHER_CTX)
MAKE_OPENSSL_PTR_TYPE(EC_GROUP)
MAKE_OPENSSL_PTR_TYPE(EC_KEY)
MAKE_OPENSSL_PTR_TYPE(EC_POINT)
MAKE_OPENSSL_PTR_TYPE(EVP_PKEY)
MAKE_OPENSSL_PTR_TYPE(EVP_PKEY_CTX)
MAKE_OPENSSL_PTR_TYPE(RSA)
MAKE_OPENSSL_PTR_TYPE(X509)
MAKE_OPENSSL_PTR_TYPE(X509_NAME)

typedef std::unique_ptr<BIGNUM, UniquePtrDeleter<BIGNUM, BN_free>> BIGNUM_Ptr;

inline const EVP_MD* openssl_digest(Digest digest) {
    switch (digest) {
        case Digest::NONE:
            return nullptr;
        case Digest::MD5:
            return EVP_md5();
        case Digest::SHA1:
            return EVP_sha1();
        case Digest::SHA_2_224:
            return EVP_sha224();
        case Digest::SHA_2_256:
            return EVP_sha256();
        case Digest::SHA_2_384:
            return EVP_sha384();
        case Digest::SHA_2_512:
            return EVP_sha512();
    }
    return nullptr;
}

/**
 * @brief Encrypts data using AES-256 in Galois/Counter Mode (GCM).
 *
 * This function performs authenticated encryption on the input data using AES-GCM
 * with a 256-bit key.
 * The function takes the plaintext to be encrypted, a secret key, an initialization vector (IV),
 * and optional associated authenticated data (AAD) as input. It produces the ciphertext and an
 * authentication tag as output. The tag is used to verify the integrity and authenticity of the
 * ciphertext and AAD during decryption.
 *
 * @param key The 256-bit (32-byte) secret key used for encryption.
 * @param input The plaintext data to be encrypted.
 * @param aad The optional associated authenticated data. This data is authenticated but not
 * encrypted.
 * @param iv The initialization vector (IV) used for the encryption process. For GCM, the IV should
 * be unique for each encryption operation under the same key. Callers must ensure the IV length is
 * 12 bytes; other sizes are not supported.
 * @param ciphertext A reference to a `std::vector<uint8_t>` where the resulting ciphertext will be
 * stored. The caller must ensure this vector is pre-allocated with a size at least equal to the
 * input Length.
 * @param tag A reference to a `std::vector<uint8_t>` where the generated authentication tag will be
 * stored. The caller must ensure this vector is pre-sized to the required length (16 bytes for
 * GCM).
 * @return Returns `true` on successful encryption, `false` otherwise.
 */
bool aes256GcmEncrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& input,
                      const std::vector<uint8_t>& aad, const std::vector<uint8_t>& iv,
                      std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& tag);

/**
 * @brief Generates an Elliptic Curve private key on the P-256 curve.
 *
 * This function uses OpenSSL to generate a new EC key pair, specifically for the
 * `prime256v1` curve (also known as secp256r1 or NIST P-256). It then encodes
 * the generated **private key** into a **PKCS#8 DER** format.
 *
 * @param output A reference to a `std::vector<uint8_t>` where the
 * generated private key in PKCS#8 DER format will be stored.
 *
 * @return Returns `true` on successful key generation and encoding, `false` otherwise.
 */
bool generateEc256Pkcs8DerKey(std::vector<uint8_t>& output);

/**
 * @brief Generates an RSA private key with a key size of 2048 bits.
 *
 * This function utilizes OpenSSL to generate a new RSA key pair. The key size is
 * set to 2048 bits. The public exponent is set to `RSA_F4` (65537).
 * The generated **private key** is then encoded into **PKCS#8 DER** format.
 *
 * @param output A reference to a `std::vector<uint8_t>` where the
 * generated private key in PKCS#8 DER format will be stored.
 *
 * @return Returns `true` on successful key generation and encoding, `false` otherwise.
 */
bool generateRsa2048Pkcs8DerKey(std::vector<uint8_t>& output);

}  // namespace aidl::android::hardware::security::keymint
