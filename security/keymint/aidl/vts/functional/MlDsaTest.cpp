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

#define LOG_TAG "keymint_1_test"
#include <cutils/log.h>

#include <iostream>
#include <optional>

#include <openssl/mldsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>

#define KEYMINT_HAL_V4
#define KEYMINT_HAL_V5
#include <keymint_support/authorization_set.h>
#include <keymint_support/key_param_output.h>
#include <keymint_support/openssl_utils.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

namespace {

const std::map<MlDsaVariant, std::string> kOidString = {
        {MlDsaVariant::ML_DSA_65, ML_DSA_65_OID},
        {MlDsaVariant::ML_DSA_87, ML_DSA_87_OID},
};

const std::string kSeed =
        hex2str("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");

// From draft-ietf-lamps-dilithium-certificates-12 section C.1.2.1
const std::string kSeed65Pkcs8 =
        hex2str("3034"                // SEQUENCE len x34 {
                "020100"              // INTEGER 0 (Version)
                "300b"                // SEQUENCE len 11 (privateKeyAlgorithm) {
                "0609"                // OBJECT_IDENTIFIER len 9
                "608648016503040312"  //  2.16.840.1.101.3.4.3.18
                // }
                "0422"  // OCTET STRING len 34
                "8020"  // tag 0 primitive len 32
                "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"  // seed value
        );

// TODO(b/395069628): enable ML-DSA-87 tests when reference implementation updated
const MlDsaVariant kVariants[] = {MlDsaVariant::ML_DSA_65, /* MlDsaVariant::ML_DSA_87 */};

}  // namespace

class MlDsaTest : public KeyMintAidlTestBase {
  public:
    void SetUp() {
        KeyMintAidlTestBase::SetUp();
        if (!MlDsaSupported()) {
            GTEST_SKIP() << "ML-DSA support not required";
        }
    }

    bool MlDsaSupported() {
        // StrongBox never supports ML-DSA.
        if (SecLevel() == SecurityLevel::STRONGBOX) {
            return false;
        }
        // ML-DSA was included in version 5 of the KeyMint interface.
        return AidlVersion() >= 5;
    }

    static AuthorizationSetBuilder KeyParams(MlDsaVariant variant) {
        return AuthorizationSetBuilder()
                .MlDsaSigningKey(variant)
                .Digest(Digest::NONE)
                .Authorization(TAG_NO_AUTH_REQUIRED)
                .SetDefaultValidity();
    }

    // Verify a signature using a key locally generated from the fixed default seed.
    static void DefaultSeedVerify(const std::string& message, const std::string& signature,
                                  MlDsaVariant variant) {
        if (variant == MlDsaVariant::ML_DSA_65) {
            MLDSA65_private_key key;
            ASSERT_EQ(MLDSA65_private_key_from_seed(
                              &key, reinterpret_cast<const uint8_t*>(kSeed.data()), kSeed.size()),
                      1);
            MLDSA65_public_key pubkey;
            ASSERT_EQ(MLDSA65_public_from_private(&pubkey, &key), 1);
            EXPECT_EQ(
                    MLDSA65_verify(&pubkey, reinterpret_cast<const uint8_t*>(signature.data()),
                                   signature.size(),
                                   reinterpret_cast<const uint8_t*>(message.data()), message.size(),
                                   /* context= */ nullptr,
                                   /* context_len=*/0),
                    1);
        } else {
            MLDSA87_private_key key;
            ASSERT_EQ(MLDSA87_private_key_from_seed(
                              &key, reinterpret_cast<const uint8_t*>(kSeed.data()), kSeed.size()),
                      1);
            MLDSA87_public_key pubkey;
            ASSERT_EQ(MLDSA87_public_from_private(&pubkey, &key), 1);
            EXPECT_EQ(
                    MLDSA87_verify(&pubkey, reinterpret_cast<const uint8_t*>(signature.data()),
                                   signature.size(),
                                   reinterpret_cast<const uint8_t*>(message.data()), message.size(),
                                   /* context= */ nullptr,
                                   /* context_len=*/0),
                    1);
        }
    }

    void LocalVerifyMlDsa(const std::string& message, const std::string& signature,
                          MlDsaVariant variant) {
        // Parse the certificate and retrieve the public key.
        ASSERT_GT(cert_chain_.size(), 0);
        X509_Ptr cert(parse_cert_blob(cert_chain_[0].encodedCertificate));
        ASSERT_NE(cert.get(), nullptr);

        SubjectPublicKeyInfo info;
        extract_spki(cert.get(), &info);
        EXPECT_EQ(info.oid, kOidString.find(variant)->second);

        LocalVerifyMlDsaRaw(message, signature, variant, info.pubkey);
    }
};

TEST_P(MlDsaTest, KeyGeneration) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ErrorCode result = GenerateKey(KeyParams(variant), &key_blob, &key_characteristics);
        ASSERT_EQ(result, ErrorCode::OK);
        KeyBlobDeleter deleter(keymint_, key_blob);

        ASSERT_GT(key_blob.size(), 0U);

        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        ASSERT_GT(cert_chain_.size(), 0);
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);

        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::ML_DSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_ML_DSA_VARIANT, variant))
                << "Variant " << variant << "missing";
    }
}

TEST_P(MlDsaTest, GenerateWithAttestation) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        auto challenge = "hello";
        auto app_id = "foo";
        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ErrorCode result = GenerateKey(
                KeyParams(variant).AttestationChallenge(challenge).AttestationApplicationId(app_id),
                &key_blob, &key_characteristics);
        ASSERT_EQ(result, ErrorCode::OK);
        KeyBlobDeleter deleter(keymint_, key_blob);

        ASSERT_GT(key_blob.size(), 0U);

        CheckBaseParams(key_characteristics);
        CheckCharacteristics(key_blob, key_characteristics);

        ASSERT_GT(cert_chain_.size(), 0);
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics);
        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::ML_DSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_ML_DSA_VARIANT, variant))
                << "Variant " << variant << "missing";

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(key_characteristics);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), challenge, app_id,  //
                                              sw_enforced, hw_enforced, SecLevel(),
                                              cert_chain_[0].encodedCertificate));
    }
}

TEST_P(MlDsaTest, KeyGenerationFailNoVariant) {
    ErrorCode result = GenerateKey(AuthorizationSetBuilder()
                                           .Authorization(TAG_ALGORITHM, Algorithm::ML_DSA)
                                           .SigningKey()
                                           .Digest(Digest::NONE)
                                           .SetDefaultValidity());
    EXPECT_NE(result, ErrorCode::OK);
    EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                result == ErrorCode::INVALID_ARGUMENT);
}

TEST_P(MlDsaTest, KeyGenerationFailDualPurpose) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        // Can't combine signing and attestation.
        ErrorCode result =
                GenerateKey(KeyParams(variant).Authorization(TAG_PURPOSE, KeyPurpose::ATTEST_KEY));
        EXPECT_EQ(result, ErrorCode::INCOMPATIBLE_PURPOSE);
    }
}

TEST_P(MlDsaTest, KeyGenerationFailUnknownVariant) {
    ErrorCode result = GenerateKey(KeyParams(static_cast<MlDsaVariant>(44)));
    EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                result == ErrorCode::INVALID_ARGUMENT);
}

TEST_P(MlDsaTest, SignOneShot) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        ErrorCode result = GenerateKey(KeyParams(variant));
        ASSERT_EQ(result, ErrorCode::OK);

        string message = "12345678901234567890123456789012";
        string signature = SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE));
        LocalVerifyMlDsa(message, signature, variant);
    }
}

TEST_P(MlDsaTest, SignIncremental) {
    auto params = AuthorizationSetBuilder().Digest(Digest::NONE);
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        vector<uint8_t> key_blob;
        vector<KeyCharacteristics> key_characteristics;
        ErrorCode result = GenerateKey(KeyParams(variant), &key_blob, &key_characteristics);
        ASSERT_EQ(result, ErrorCode::OK);
        KeyBlobDeleter deleter(keymint_, key_blob);

        string chunk1 = "1234567890";
        string chunk2 = "123456789012";

        AuthorizationSet out_params;
        ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::SIGN, key_blob, params, &out_params));
        EXPECT_TRUE(out_params.empty());

        string output;
        result = Update(chunk1, &output);
        EXPECT_EQ(result, ErrorCode::OK);
        EXPECT_EQ(output.size(), 0);
        result = Update(chunk2, &output);
        EXPECT_EQ(result, ErrorCode::OK);
        EXPECT_EQ(output.size(), 0);

        string message = chunk1 + chunk2;
        string signature;
        EXPECT_EQ(ErrorCode::OK, Finish({}, &signature));

        LocalVerifyMlDsa(message, signature, variant);
    }
}

TEST_P(MlDsaTest, SignFailWithWrongDigest) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        ErrorCode result = GenerateKey(KeyParams(variant));
        ASSERT_EQ(result, ErrorCode::OK);

        string message = "12345678901234567890123456789012";
        AuthorizationSet out_params;
        result = Begin(KeyPurpose::SIGN, key_blob_,
                       AuthorizationSetBuilder().Digest(Digest::SHA_2_256), &out_params);
        EXPECT_EQ(result, ErrorCode::UNSUPPORTED_DIGEST);
    }
}

TEST_P(MlDsaTest, ImportRawSeed) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        ErrorCode result = ImportKey(KeyParams(variant), KeyFormat::RAW, kSeed);
        EXPECT_EQ(result, ErrorCode::OK);
        ASSERT_GT(key_blob_.size(), 0U);

        CheckCommonParams(key_characteristics_, KeyOrigin::IMPORTED);
        CheckCharacteristics(key_blob_, key_characteristics_);

        ASSERT_GT(cert_chain_.size(), 0);
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics_);
        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::ML_DSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_ML_DSA_VARIANT, variant))
                << "Variant " << variant << "missing";

        // Check a signature against public key locally created from the seed.
        string message = "12345678901234567890123456789012";
        string signature = SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE));
        DefaultSeedVerify(message, signature, variant);
    }
}

TEST_P(MlDsaTest, ImportRawSeedWrongLen) {
    std::string shortSeed = kSeed.substr(0, 30);
    std::string longSeed = kSeed + "x";
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        ErrorCode result = ImportKey(KeyParams(variant), KeyFormat::RAW, shortSeed);
        EXPECT_NE(result, ErrorCode::OK);
        EXPECT_TRUE(result == ErrorCode::INVALID_INPUT_LENGTH ||
                    result == ErrorCode::UNSUPPORTED_KEY_SIZE ||
                    result == ErrorCode::INVALID_ARGUMENT);

        result = ImportKey(KeyParams(variant), KeyFormat::RAW, longSeed);
        EXPECT_NE(result, ErrorCode::OK);
        EXPECT_TRUE(result == ErrorCode::INVALID_INPUT_LENGTH ||
                    result == ErrorCode::UNSUPPORTED_KEY_SIZE ||
                    result == ErrorCode::INVALID_ARGUMENT);
    }
}

TEST_P(MlDsaTest, ImportRawSeedUnspecifiedVariant) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        auto params = AuthorizationSetBuilder()
                              .Authorization(TAG_ALGORITHM, Algorithm::ML_DSA)
                              .SigningKey()
                              .Digest(Digest::NONE)
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .SetDefaultValidity();
        ErrorCode result = ImportKey(params, KeyFormat::RAW, kSeed);
        EXPECT_NE(result, ErrorCode::OK);
        EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                    result == ErrorCode::INVALID_ARGUMENT);
    }
}

TEST_P(MlDsaTest, ImportRawSeedUnknownVariant) {
    ErrorCode result = ImportKey(KeyParams(static_cast<MlDsaVariant>(44)), KeyFormat::RAW, kSeed);
    EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                result == ErrorCode::INVALID_ARGUMENT);
}

TEST_P(MlDsaTest, ImportPkcs8Fails) {
    ErrorCode result =
            ImportKey(KeyParams(MlDsaVariant::ML_DSA_65), KeyFormat::PKCS8, kSeed65Pkcs8);
    EXPECT_EQ(result, ErrorCode::UNSUPPORTED_KEY_FORMAT);
}

TEST_P(MlDsaTest, ImportWrappedRawSeed) {
    GTEST_SKIP() << "TODO: add test for import of a wrapped ML-DSA private key seed";
}

TEST_P(MlDsaTest, AttestToEcdsaKey) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        // Create an ML-DSA ATTEST_KEY.
        auto params = AuthorizationSetBuilder()
                              .MlDsaKey(variant)
                              .Authorization(TAG_PURPOSE, KeyPurpose::ATTEST_KEY)
                              .Digest(Digest::NONE)
                              .Authorization(TAG_NO_AUTH_REQUIRED)
                              .SetDefaultValidity();
        AttestationKey attest_key;
        attest_key.issuerSubjectName = make_name_from_str("Android Keystore Key");
        vector<KeyCharacteristics> attest_key_chars;
        ErrorCode result = GenerateKey(params, &attest_key.keyBlob, &attest_key_chars);
        ASSERT_EQ(result, ErrorCode::OK);
        KeyBlobDeleter deleter(keymint_, attest_key.keyBlob);

        CheckCommonParams(attest_key_chars, KeyOrigin::GENERATED);
        CheckCharacteristics(attest_key.keyBlob, attest_key_chars);
        ASSERT_GT(cert_chain_.size(), 0);
        AuthorizationSet crypto_params = SecLevelAuthorizations(attest_key_chars);
        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::ML_DSA));
        EXPECT_TRUE(crypto_params.Contains(TAG_ML_DSA_VARIANT, variant))
                << "Variant " << variant << "missing";

        // Generate an ECDSA P256 key that is attested to by ML-DSA.
        auto challenge = "foo";
        auto app_id = "bar";
        vector<uint8_t> attested_key_blob;
        vector<KeyCharacteristics> attested_key_characteristics;
        vector<Certificate> attested_key_chain;
        ASSERT_EQ(ErrorCode::OK, GenerateKey(AuthorizationSetBuilder()
                                                     .EcdsaSigningKey(EcCurve::P_256)
                                                     .Authorization(TAG_NO_AUTH_REQUIRED)
                                                     .AttestationChallenge(challenge)
                                                     .AttestationApplicationId(app_id)
                                                     .SetDefaultValidity(),
                                             attest_key, &attested_key_blob,
                                             &attested_key_characteristics, &attested_key_chain));
        KeyBlobDeleter attested_deleter(keymint_, attested_key_blob);

        ASSERT_GT(attested_key_chain.size(), 0);

        // Attestation by itself is not valid (last entry is not self-signed).
        EXPECT_FALSE(ChainSignaturesAreValid(attested_key_chain));

        // Appending the attest_key chain to the attested_key_chain should yield a valid chain.
        attested_key_chain.push_back(cert_chain_[0]);
        EXPECT_EQ(attested_key_chain.size(), 2);
        EXPECT_TRUE(ChainSignaturesAreValid(attested_key_chain));

        AuthorizationSet hw_enforced = HwEnforcedAuthorizations(attested_key_characteristics);
        AuthorizationSet sw_enforced = SwEnforcedAuthorizations(attested_key_characteristics);
        EXPECT_TRUE(verify_attestation_record(AidlVersion(), challenge, app_id,  //
                                              sw_enforced, hw_enforced, SecLevel(),
                                              attested_key_chain[0].encodedCertificate));
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(MlDsaTest);

class StrongBoxMlDsaTest : public KeyMintAidlTestBase {
  public:
    void SetUp() {
        KeyMintAidlTestBase::SetUp();
        if (SecLevel() != SecurityLevel::STRONGBOX) {
            GTEST_SKIP() << "StrongBox-specific test";
        }
    }
};

TEST_P(StrongBoxMlDsaTest, KeyGenerationFails) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        ErrorCode result = GenerateKey(MlDsaTest::KeyParams(variant));
        EXPECT_NE(result, ErrorCode::OK) << "StrongBox should reject ML-DSA";
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(StrongBoxMlDsaTest);

}  // namespace aidl::android::hardware::security::keymint::test
