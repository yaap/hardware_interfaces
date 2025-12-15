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
#include <keymint_support/openssl_utils.h>

#include <android-base/logging.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

namespace aidl::android::hardware::security::keymint {

// Create a wrapper function for BIO_free as its return type is int.
void BIO_free_void(BIO* p) {
    BIO_free(p);  // The integer return value is simply ignored.
}

typedef std::unique_ptr<BIO, UniquePtrDeleter<BIO, BIO_free_void>> BIO_Ptr;

bool aes256GcmEncrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& input,
                      const std::vector<uint8_t>& aad, const std::vector<uint8_t>& iv,
                      std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& tag) {
    if (key.size() != 32) {
        LOG(ERROR) << "Invalid key size. AES-256 requires a 32-byte key, but a key of "
                   << key.size() << " bytes was provided.";
        return false;
    }

    if (iv.size() != 12) {
        LOG(ERROR) << "Unsupported IV length: 12 bytes required, but " << iv.size()
                   << " bytes provided.";
        return false;
    }

    if (ciphertext.size() < input.size()) {
        LOG(ERROR) << "Invalid ciphertext size. The output ciphertext size " << ciphertext.size()
                   << " is less than the input size " << input.size();
    }

    const EVP_CIPHER* aesGcmCipher = EVP_aes_256_gcm();

    EVP_CIPHER_CTX_Ptr ctx(EVP_CIPHER_CTX_new());

    if (!EVP_EncryptInit_ex(ctx.get(), aesGcmCipher, nullptr /* engine */, key.data(), iv.data())) {
        LOG(ERROR) << "EVP_EncryptInit_ex failed.";
        return false;
    }
    if (!EVP_CIPHER_CTX_set_padding(ctx.get(), 0 /* no padding needed with GCM */)) {
        LOG(ERROR) << "EVP_CIPHER_CTX_set_padding failed.";
        return false;
    }

    // Update aad
    int len;
    if (!EVP_EncryptUpdate(ctx.get(), nullptr, &len, aad.data(), aad.size())) {
        LOG(ERROR) << "EVP_EncryptUpdate failed.";
        return false;
    }

    std::vector<uint8_t> outTmp(input.size());
    uint8_t* outPos = outTmp.data();
    int outLen;

    if (!EVP_EncryptUpdate(ctx.get(), outPos, &outLen, input.data(), input.size())) {
        LOG(ERROR) << "EVP_EncryptUpdate failed.";
        return false;
    }
    outPos += outLen;
    if (!EVP_EncryptFinal_ex(ctx.get(), outPos, &outLen)) {
        LOG(ERROR) << "EVP_EncryptFinal_ex failed.";
        return false;
    }
    outPos += outLen;
    if (outPos - outTmp.data() != static_cast<ssize_t>(input.size())) {
        LOG(ERROR) << "Expected the cipher size of " << input.size()
                   << " bytes, but received a cipher size of " << outTmp.size() << " bytes.";
        return false;
    }

    std::copy(outTmp.data(), outPos, ciphertext.begin());
    if (!EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data())) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl failed";
        return false;
    }
    return true;
}

bool generateEc256Pkcs8DerKey(std::vector<uint8_t>& output) {
    EVP_PKEY_Ptr pkey(EVP_PKEY_new());
    EC_KEY_Ptr ecKey(EC_KEY_new());
    if (ecKey.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed.";
    }

    EC_GROUP_Ptr group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (group.get() == nullptr) {
        LOG(ERROR) << "EC_GROUP_new_by_curve_name failed.";
        return false;
    }
#if !defined(OPENSSL_IS_BORINGSSL)
    if (!EC_GROUP_set_point_conversion_form(group.get(), POINT_CONVERSION_UNCOMPRESSED)) {
        LOG(ERROR) << "EC_GROUP_set_point_conversion_form failed.";
        return false;
    }
    if (!EC_GROUP_set_asn1_flag(group.get(), OPENSSL_EC_NAMED_CURVE)) {
        LOG(ERROR) << "EC_GROUP_set_asn1_flag failed.";
        return false;
    }
#endif

    if (EC_KEY_set_group(ecKey.get(), group.get()) != 1 || EC_KEY_generate_key(ecKey.get()) != 1 ||
        EC_KEY_check_key(ecKey.get()) < 0) {
        LOG(ERROR) << "Failed to generate EC key.";
        return false;
    }

    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ecKey.get()) != 1) {
        LOG(ERROR) << "EVP_PKEY_set1_EC_KEY failed.";
        return false;
    }

    BIO_Ptr bio(BIO_new(BIO_s_mem()));
    if (bio.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed on BIO_new.";
        return false;
    }

    // Write the key to the BIO in PKCS#8 DER format.
    // We pass NULL for encryption arguments as we want an unencrypted key.
    if (!i2d_PKCS8PrivateKey_bio(bio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr)) {
        LOG(ERROR) << "Error writing private key to BIO.";
        return false;
    }

    // Extract the data from the memory BIO into our buffer.
    BUF_MEM* bioBuf = nullptr;
    BIO_get_mem_ptr(bio.get(), &bioBuf);
    if (!bioBuf || bioBuf->length <= 0) {
        LOG(ERROR) << "Error getting data from BIO.";
        return false;
    }

    output.assign(bioBuf->data, bioBuf->data + bioBuf->length);
    return true;
}

bool generateRsa2048Pkcs8DerKey(std::vector<uint8_t>& output) {
    BIGNUM_Ptr exponent(BN_new());
    RSA_Ptr rsaKey(RSA_new());
    EVP_PKEY_Ptr pkey(EVP_PKEY_new());
    if (exponent.get() == nullptr || rsaKey.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed.";
        return false;
    }

    if (!BN_set_word(exponent.get(), RSA_F4) ||
        !RSA_generate_key_ex(rsaKey.get(), 2048, exponent.get(), nullptr /* callback */)) {
        LOG(ERROR) << "Failed to generate RSA key.";
        return false;
    }

    if (EVP_PKEY_set1_RSA(pkey.get(), rsaKey.get()) != 1) {
        LOG(ERROR) << "EVP_PKEY_set1_RSA failed.";
        return false;
    }

    BIO_Ptr bio(BIO_new(BIO_s_mem()));
    if (bio.get() == nullptr) {
        LOG(ERROR) << "Error creating memory BIO";
        return false;
    }

    // Write the key to the BIO in PKCS#8 DER format.
    // We pass NULL for encryption arguments as we want an unencrypted key.
    if (!i2d_PKCS8PrivateKey_bio(bio.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr)) {
        LOG(ERROR) << "Error writing private key to BIO";
        return false;
    }

    // Extract the data from the memory BIO into our buffer.
    BUF_MEM* bioBuf = nullptr;
    BIO_get_mem_ptr(bio.get(), &bioBuf);
    if (!bioBuf || bioBuf->length <= 0) {
        LOG(ERROR) << "Error getting data from BIO";
        return false;
    }

    output.assign(bioBuf->data, bioBuf->data + bioBuf->length);
    return true;
}

}  // namespace aidl::android::hardware::security::keymint