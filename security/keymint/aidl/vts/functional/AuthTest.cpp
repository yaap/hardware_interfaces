/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "KeyMintAidlTestBase.h"

#include <aidl/android/hardware/gatekeeper/GatekeeperEnrollResponse.h>
#include <aidl/android/hardware/gatekeeper/GatekeeperVerifyResponse.h>
#include <aidl/android/hardware/gatekeeper/IGatekeeper.h>
#include <aidl/android/hardware/security/secureclock/ISecureClock.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <keymint_support/key_param_output.h>
#include <keymint_support/openssl_utils.h>
#include <keymint_support/wrapped_key.h>
#include <openssl/rand.h>
#include <vendorsupport/api_level.h>

using aidl::android::hardware::gatekeeper::GatekeeperEnrollResponse;
using aidl::android::hardware::gatekeeper::GatekeeperVerifyResponse;
using aidl::android::hardware::gatekeeper::IGatekeeper;
using aidl::android::hardware::security::keymint::HardwareAuthToken;
using aidl::android::hardware::security::secureclock::ISecureClock;

#include <android/hardware/gatekeeper/1.0/IGatekeeper.h>
#include <android/hardware/gatekeeper/1.0/types.h>
#include <gatekeeper/password_handle.h>  // for password_handle_t
#include <hardware/hw_auth_token.h>

using ::android::sp;
using IHidlGatekeeper = ::android::hardware::gatekeeper::V1_0::IGatekeeper;
using HidlGatekeeperResponse = ::android::hardware::gatekeeper::V1_0::GatekeeperResponse;
using HidlGatekeeperStatusCode = ::android::hardware::gatekeeper::V1_0::GatekeeperStatusCode;

namespace aidl::android::hardware::security::keymint::test {

class AuthTest : public KeyMintAidlTestBase {
  public:
    void SetUp() {
        KeyMintAidlTestBase::SetUp();

        // Find the default Gatekeeper instance.
        string gk_name = string(IGatekeeper::descriptor) + "/default";
        if (AServiceManager_isDeclared(gk_name.c_str())) {
            // Enroll a user with AIDL Gatekeeper.
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(gk_name.c_str()));
            gk_ = IGatekeeper::fromBinder(binder);
        } else {
            // Prior to Android U, Gatekeeper was HIDL not AIDL and so may not be present.
            // Try to enroll user with HIDL Gatekeeper instead.
            string gk_name = "default";
            hidl_gk_ = IHidlGatekeeper::getService(gk_name.c_str());
            if (hidl_gk_ == nullptr) {
                std::cerr << "No HIDL Gatekeeper instance for '" << gk_name << "' found.\n";
                return;
            }
            std::cerr << "No AIDL Gatekeeper instance for '" << gk_name << "' found, using HIDL.\n";
        }

        // If the device needs timestamps, find the default ISecureClock instance.
        if (timestamp_token_required_) {
            string clock_name = string(ISecureClock::descriptor) + "/default";
            if (AServiceManager_isDeclared(clock_name.c_str())) {
                ::ndk::SpAIBinder binder(AServiceManager_waitForService(clock_name.c_str()));
                clock_ = ISecureClock::fromBinder(binder);
            } else {
                std::cerr << "No ISecureClock instance for '" << clock_name << "' found.\n";
            }
        }

        // Enroll a password for a user.
        uid_ = 10001;
        password_ = "correcthorsebatterystaple";
        std::optional<GatekeeperEnrollResponse> rsp = doEnroll(password_);
        ASSERT_TRUE(rsp.has_value());
        sid_ = rsp->secureUserId;
        handle_ = rsp->data;
    }

    void TearDown() {
        if (gk_ == nullptr) return;
        gk_->deleteUser(uid_);
        if (alt_uid_ != 0) {
            gk_->deleteUser(alt_uid_);
        }
    }

    bool GatekeeperAvailable() { return (gk_ != nullptr) || (hidl_gk_ != nullptr); }

    std::optional<GatekeeperEnrollResponse> doEnroll(uint32_t uid,
                                                     const std::vector<uint8_t>& newPwd,
                                                     const std::vector<uint8_t>& curHandle = {},
                                                     const std::vector<uint8_t>& curPwd = {}) {
        if (gk_ != nullptr) {
            while (true) {
                GatekeeperEnrollResponse rsp;
                Status status = gk_->enroll(uid, curHandle, curPwd, newPwd, &rsp);
                if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                    status.getServiceSpecificError() == IGatekeeper::ERROR_RETRY_TIMEOUT) {
                    sleep(1);
                    continue;
                }
                if (status.isOk()) {
                    return std::move(rsp);
                } else {
                    GTEST_LOG_(ERROR) << "doEnroll(AIDL) failed: " << status;
                    return std::nullopt;
                }
            }
        } else if (hidl_gk_ != nullptr) {
            while (true) {
                HidlGatekeeperResponse rsp;
                auto status = hidl_gk_->enroll(
                        uid, curHandle, curPwd, newPwd,
                        [&rsp](const HidlGatekeeperResponse& cbRsp) { rsp = cbRsp; });
                if (!status.isOk()) {
                    GTEST_LOG_(ERROR) << "doEnroll(HIDL) failed";
                    return std::nullopt;
                }
                if (rsp.code == HidlGatekeeperStatusCode::ERROR_RETRY_TIMEOUT) {
                    sleep(1);
                    continue;
                }
                if (rsp.code != HidlGatekeeperStatusCode::STATUS_OK) {
                    GTEST_LOG_(ERROR) << "doEnroll(HIDL) failed with " << int(rsp.code);
                    return std::nullopt;
                }
                // "Parse" the returned data to get at the secure user ID.
                if (rsp.data.size() != sizeof(::gatekeeper::password_handle_t)) {
                    GTEST_LOG_(ERROR)
                            << "HAL returned password handle of invalid length " << rsp.data.size();
                    return std::nullopt;
                }
                const ::gatekeeper::password_handle_t* handle =
                        reinterpret_cast<const ::gatekeeper::password_handle_t*>(rsp.data.data());

                // Translate HIDL response to look like an AIDL response.
                GatekeeperEnrollResponse aidl_rsp;
                aidl_rsp.statusCode = IGatekeeper::STATUS_OK;
                aidl_rsp.data = rsp.data;
                aidl_rsp.secureUserId = handle->user_id;
                return aidl_rsp;
            }
        } else {
            return std::nullopt;
        }
    }

    std::optional<GatekeeperEnrollResponse> doEnroll(uint32_t uid, const string& newPwd,
                                                     const std::vector<uint8_t>& curHandle = {},
                                                     const string& curPwd = {}) {
        return doEnroll(uid, std::vector<uint8_t>(newPwd.begin(), newPwd.end()), curHandle,
                        std::vector<uint8_t>(curPwd.begin(), curPwd.end()));
    }
    std::optional<GatekeeperEnrollResponse> doEnroll(const string& newPwd) {
        return doEnroll(uid_, newPwd);
    }

    std::optional<HardwareAuthToken> doVerify(uint32_t uid, uint64_t challenge,
                                              const std::vector<uint8_t>& handle,
                                              const std::vector<uint8_t>& pwd) {
        if (gk_ != nullptr) {
            while (true) {
                GatekeeperVerifyResponse rsp;
                Status status = gk_->verify(uid, challenge, handle, pwd, &rsp);
                if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                    status.getServiceSpecificError() == IGatekeeper::ERROR_RETRY_TIMEOUT) {
                    sleep(1);
                    continue;
                }
                if (status.isOk()) {
                    return rsp.hardwareAuthToken;
                } else {
                    GTEST_LOG_(ERROR) << "doVerify(AIDL) failed: " << status;
                    return std::nullopt;
                }
            }
        } else if (hidl_gk_ != nullptr) {
            while (true) {
                HidlGatekeeperResponse rsp;
                auto status = hidl_gk_->verify(
                        uid, challenge, handle, pwd,
                        [&rsp](const HidlGatekeeperResponse& cbRsp) { rsp = cbRsp; });
                if (!status.isOk()) {
                    GTEST_LOG_(ERROR) << "doVerify(HIDL) failed";
                    return std::nullopt;
                }
                if (rsp.code == HidlGatekeeperStatusCode::ERROR_RETRY_TIMEOUT) {
                    sleep(1);
                    continue;
                }
                if (rsp.code != HidlGatekeeperStatusCode::STATUS_OK) {
                    GTEST_LOG_(ERROR) << "doVerify(HIDL) failed with " << int(rsp.code);
                    return std::nullopt;
                }
                // "Parse" the returned data to get auth token contents.
                if (rsp.data.size() != sizeof(hw_auth_token_t)) {
                    GTEST_LOG_(ERROR) << "Incorrect size of AuthToken payload.";
                    return std::nullopt;
                }
                const hw_auth_token_t* hwAuthToken =
                        reinterpret_cast<const hw_auth_token_t*>(rsp.data.data());
                HardwareAuthToken authToken;
                authToken.timestamp.milliSeconds = betoh64(hwAuthToken->timestamp);
                authToken.challenge = hwAuthToken->challenge;
                authToken.userId = hwAuthToken->user_id;
                authToken.authenticatorId = hwAuthToken->authenticator_id;
                authToken.authenticatorType = static_cast<HardwareAuthenticatorType>(
                        betoh32(hwAuthToken->authenticator_type));
                authToken.mac.assign(&hwAuthToken->hmac[0], &hwAuthToken->hmac[32]);
                return authToken;
            }
        } else {
            return std::nullopt;
        }
    }
    std::optional<HardwareAuthToken> doVerify(uint32_t uid, uint64_t challenge,
                                              const std::vector<uint8_t>& handle,
                                              const string& pwd) {
        return doVerify(uid, challenge, handle, std::vector<uint8_t>(pwd.begin(), pwd.end()));
    }
    std::optional<HardwareAuthToken> doVerify(uint64_t challenge,
                                              const std::vector<uint8_t>& handle,
                                              const string& pwd) {
        return doVerify(uid_, challenge, handle, pwd);
    }

    // Variants of the base class methods but with authentication information included.
    string ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                          const string& message, const AuthorizationSet& in_params,
                          AuthorizationSet* out_params, const HardwareAuthToken& hat) {
        AuthorizationSet begin_out_params;
        ErrorCode result = Begin(operation, key_blob, in_params, out_params, hat);
        EXPECT_EQ(ErrorCode::OK, result);
        if (result != ErrorCode::OK) {
            return "";
        }

        std::optional<secureclock::TimeStampToken> time_token = std::nullopt;
        if (timestamp_token_required_ && clock_ != nullptr) {
            // Ask a secure clock instance for a timestamp, including the per-op challenge.
            secureclock::TimeStampToken token;
            EXPECT_EQ(ErrorCode::OK,
                      GetReturnErrorCode(clock_->generateTimeStamp(challenge_, &token)));
            time_token = token;
        }

        string output;
        EXPECT_EQ(ErrorCode::OK, Finish(message, {} /* signature */, &output, hat, time_token));
        return output;
    }

    string EncryptMessage(const vector<uint8_t>& key_blob, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params,
                          const HardwareAuthToken& hat) {
        SCOPED_TRACE("EncryptMessage");
        return ProcessMessage(key_blob, KeyPurpose::ENCRYPT, message, in_params, out_params, hat);
    }

    string DecryptMessage(const vector<uint8_t>& key_blob, const string& ciphertext,
                          const AuthorizationSet& params, const HardwareAuthToken& hat) {
        SCOPED_TRACE("DecryptMessage");
        AuthorizationSet out_params;
        string plaintext =
                ProcessMessage(key_blob, KeyPurpose::DECRYPT, ciphertext, params, &out_params, hat);
        EXPECT_TRUE(out_params.empty());
        return plaintext;
    }

    string SignMessage(const vector<uint8_t>& key_blob, const string& message,
                       const AuthorizationSet& in_params, AuthorizationSet* out_params,
                       const HardwareAuthToken& hat) {
        SCOPED_TRACE("SignMessage");
        return ProcessMessage(key_blob, KeyPurpose::SIGN, message, in_params, out_params, hat);
    }

  protected:
    std::shared_ptr<IGatekeeper> gk_;
    sp<IHidlGatekeeper> hidl_gk_;
    std::shared_ptr<ISecureClock> clock_;
    string password_;
    uint32_t uid_;
    int64_t sid_;
    uint32_t alt_uid_;
    int64_t alt_sid_;
    std::vector<uint8_t> handle_;
};

// Test use of a key that requires user-authentication within recent history.
TEST_P(AuthTest, TimeoutAuthentication) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }
    if (timestamp_token_required_ && clock_ == nullptr) {
        GTEST_SKIP() << "Device requires timestamps and no ISecureClock available";
    }

    // Create an AES key that requires authentication within the last 3 seconds.
    const uint32_t timeout_secs = 3;
    auto builder = AuthorizationSetBuilder()
                           .AesEncryptionKey(256)
                           .BlockMode(BlockMode::ECB)
                           .Padding(PaddingMode::PKCS7)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::PASSWORD)
                           .Authorization(TAG_AUTH_TIMEOUT, timeout_secs);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    // Attempt to use the AES key without authentication.
    const string message = "Hello World!";
    AuthorizationSet out_params;
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));

    // Verify to get a HAT, arbitrary challenge.
    const uint64_t challenge = 42;
    const std::optional<HardwareAuthToken> hat = doVerify(challenge, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);

    // Adding the auth token makes it possible to use the AES key.
    const string ciphertext = EncryptMessage(keyblob, message, params, &out_params, hat.value());
    const string plaintext = DecryptMessage(keyblob, ciphertext, params, hat.value());
    EXPECT_EQ(message, plaintext);

    // Altering a single bit in the MAC means no auth.
    HardwareAuthToken dodgy_hat = hat.value();
    ASSERT_GT(dodgy_hat.mac.size(), 0);
    dodgy_hat.mac[0] ^= 0x01;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params, dodgy_hat));

    // Wait for long enough that the hardware auth token expires.
    sleep(timeout_secs + 1);

    auto begin_result = Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params, hat);
    if (begin_result == ErrorCode::OK) {
        // If begin() succeeds despite the out-of-date HAT, that must mean that the KeyMint
        // device doesn't have its own clock.  In that case, it only detects timeout via a
        // timestamp token provided on update()/finish()
        ASSERT_TRUE(timestamp_token_required_);

        secureclock::TimeStampToken time_token;
        EXPECT_EQ(ErrorCode::OK,
                  GetReturnErrorCode(clock_->generateTimeStamp(challenge_, &time_token)));

        string output;
        EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
                  Finish(message, {} /* signature */, &output, hat, time_token));
    } else {
        // The KeyMint implementation may have its own clock that can immediately detect timeout.
        ASSERT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED, begin_result);
    }
}

// Test use of a key that requires user-authentication within recent history, but where
// the `TimestampToken` provided to the device is unrelated to the in-progress operation.
TEST_P(AuthTest, TimeoutAuthenticationIncorrectTimestampToken) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }
    if (!timestamp_token_required_) {
        GTEST_SKIP() << "Test only applies to devices with no secure clock";
    }
    if (clock_ == nullptr) {
        GTEST_SKIP() << "Device requires timestamps and no ISecureClock available";
    }

    // Create an AES key that requires authentication within the last 3 seconds.
    const uint32_t timeout_secs = 3;
    auto builder = AuthorizationSetBuilder()
                           .AesEncryptionKey(256)
                           .BlockMode(BlockMode::ECB)
                           .Padding(PaddingMode::PKCS7)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::PASSWORD)
                           .Authorization(TAG_AUTH_TIMEOUT, timeout_secs);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    // Verify to get a HAT, arbitrary challenge.
    const uint64_t challenge = 42;
    const std::optional<HardwareAuthToken> hat = doVerify(challenge, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);

    // KeyMint implementation has no clock, so only detects timeout via timestamp token provided
    // on update()/finish().  However, for this test we ensure that that the timestamp token has a
    // *different* challenge value.
    const string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    AuthorizationSet out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params, hat));

    secureclock::TimeStampToken time_token;
    EXPECT_EQ(ErrorCode::OK,
              GetReturnErrorCode(clock_->generateTimeStamp(challenge_ + 1, &time_token)));
    string output;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Finish(message, {} /* signature */, &output, hat, time_token));
}

// Test use of a key with multiple USER_SECURE_ID values.  For variety, use an EC signing key
// generated with attestation.
TEST_P(AuthTest, TimeoutAuthenticationMultiSid) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }
    if (timestamp_token_required_ && clock_ == nullptr) {
        GTEST_SKIP() << "Device requires timestamps and no ISecureClock available";
    }

    // Enroll a password for a second user.
    alt_uid_ = 20001;
    const string alt_password = "correcthorsebatterystaple2";
    std::optional<GatekeeperEnrollResponse> rsp = doEnroll(alt_uid_, alt_password);
    ASSERT_TRUE(rsp.has_value());
    alt_sid_ = rsp->secureUserId;
    const std::vector<uint8_t> alt_handle = rsp->data;

    // Create an attested EC key that requires authentication within the last 3 seconds from either
    // secure ID. Also allow any authenticator type.
    const uint32_t timeout_secs = 3;
    auto builder = AuthorizationSetBuilder()
                           .EcdsaSigningKey(EcCurve::P_256)
                           .Digest(Digest::NONE)
                           .Digest(Digest::SHA_2_256)
                           .SetDefaultValidity()
                           .AttestationChallenge("challenge")
                           .AttestationApplicationId("app_id")
                           .Authorization(TAG_USER_SECURE_ID, alt_sid_)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::ANY)
                           .Authorization(TAG_AUTH_TIMEOUT, timeout_secs);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    auto result = GenerateKey(builder, &keyblob, &key_characteristics);
    ASSERT_EQ(ErrorCode::OK, result);

    // Verify first user to get a HAT that should work.
    const uint64_t challenge = 42;
    const std::optional<HardwareAuthToken> hat = doVerify(uid_, challenge, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);

    const string message = "Hello World!";
    auto params = AuthorizationSetBuilder().Digest(Digest::SHA_2_256);
    AuthorizationSet out_params;
    const string signature = SignMessage(keyblob, message, params, &out_params, hat.value());

    // Verify second user to get a HAT that should work.
    const uint64_t alt_challenge = 43;
    const std::optional<HardwareAuthToken> alt_hat =
            doVerify(alt_uid_, alt_challenge, alt_handle, alt_password);
    ASSERT_TRUE(alt_hat.has_value());
    EXPECT_EQ(alt_hat->userId, alt_sid_);

    const string alt_signature =
            SignMessage(keyblob, message, params, &out_params, alt_hat.value());
}

// Test use of a key that requires an auth token for each action on the operation, with
// a per-operation challenge value included.
TEST_P(AuthTest, AuthPerOperation) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }

    // Create an AES key that requires authentication per-action.
    auto builder = AuthorizationSetBuilder()
                           .AesEncryptionKey(256)
                           .BlockMode(BlockMode::ECB)
                           .Padding(PaddingMode::PKCS7)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::PASSWORD);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    // Attempt to use the AES key without authentication fails after begin.
    const string message = "Hello World!";
    AuthorizationSet out_params;
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    string output;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED, Finish(message, {} /* signature */, &output));

    // Verify to get a HAT, but with an arbitrary challenge.
    const uint64_t unrelated_challenge = 42;
    const std::optional<HardwareAuthToken> unrelated_hat =
            doVerify(unrelated_challenge, handle_, password_);
    ASSERT_TRUE(unrelated_hat.has_value());
    EXPECT_EQ(unrelated_hat->userId, sid_);

    // Attempt to use the AES key with an unrelated authentication fails after begin.
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params, unrelated_hat.value()));
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Finish(message, {} /* signature */, &output, unrelated_hat.value()));

    // Now get a HAT with the challenge from an in-progress operation.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    const std::optional<HardwareAuthToken> hat = doVerify(challenge_, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Finish(message, {} /* signature */, &ciphertext, hat.value()));

    // Altering a single bit in the MAC means no auth.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    std::optional<HardwareAuthToken> dodgy_hat = doVerify(challenge_, handle_, password_);
    ASSERT_TRUE(dodgy_hat.has_value());
    EXPECT_EQ(dodgy_hat->userId, sid_);
    ASSERT_GT(dodgy_hat->mac.size(), 0);
    dodgy_hat->mac[0] ^= 0x01;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Finish(message, {} /* signature */, &ciphertext, dodgy_hat.value()));
}

// Test use of a key with large message that requires an auth token for each action on the
// operation, with a per-operation challenge value included.
TEST_P(AuthTest, AuthPerOperationLargeMessage) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }
    if (SecLevel() == SecurityLevel::STRONGBOX &&
        get_vendor_api_level() <= AVendorSupport_getVendorApiLevelOf(__ANDROID_API_V__)) {
        // Certain Strongbox implementations incorrectly handle Auth-bound key
        // operations with large input messages, leading to VERIFICATION_FAILED
        // error. This test is skipped for these older implementations.
        GTEST_SKIP() << "Skip test on StrongBox device with vendor-api-level <= __ANDROID_API_V__";
    }

    // Create an AES key that requires authentication per-action.
    auto builder = AuthorizationSetBuilder()
                           .AesEncryptionKey(256)
                           .BlockMode(BlockMode::CBC)
                           .Padding(PaddingMode::PKCS7)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::PASSWORD);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    vector<vector<size_t>> chunk_sizes = {
            {2048},
            {2048, 0},
            {1024, 1024},
            {1024, 1024, 0},
            {512, 512, 512, 512},
            {512, 512, 512, 512, 0},
    };
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::CBC).Padding(PaddingMode::PKCS7);

    for (const auto& chunks : chunk_sizes) {
        AuthorizationSet out_params;
        ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
        auto iv = out_params.GetTagValue(TAG_NONCE);
        EXPECT_TRUE(iv);

        // Get a HAT with the challenge from an in-progress operation.
        auto hat = doVerify(challenge_, handle_, password_);
        ASSERT_TRUE(hat.has_value());
        ASSERT_EQ(hat->userId, sid_);

        // Encrypt in the given chunk sizes.
        string message;
        string ciphertext;
        for (int i = 0; i < chunks.size(); i++) {
            SCOPED_TRACE(testing::Message()
                         << "chunk[" << i << "] of " << chunks.size() << ", size " << chunks[i]);
            string msg_chunk = string(chunks[i], 'A');
            string ct_chunk;
            if (i < chunks.size() - 1) {
                // Pass in n-1 chunks to `update()`
                std::cerr << "Update(size=" << chunks[i] << ")\n";
                ASSERT_EQ(ErrorCode::OK, Update(msg_chunk, &ct_chunk, hat.value(), {}));
            } else {
                // Pass the final chunk to `finish()`
                std::cerr << "Finish(size=" << chunks[i] << ")\n";
                ASSERT_EQ(ErrorCode::OK,
                          Finish(msg_chunk, {} /* signature */, &ct_chunk, hat.value()));
            }
            message += msg_chunk;
            ciphertext += ct_chunk;
        }

        auto params_iv = AuthorizationSetBuilder().Authorizations(params).Authorization(TAG_NONCE,
                                                                                        iv->get());
        out_params.Clear();
        // Decrypt in one go
        ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::DECRYPT, keyblob, params_iv, &out_params));
        hat = doVerify(challenge_, handle_, password_);
        ASSERT_TRUE(hat.has_value());
        ASSERT_EQ(hat->userId, sid_);

        string plaintext;
        ASSERT_EQ(ErrorCode::OK, Finish(ciphertext, {} /* signature */, &plaintext, hat.value()));

        EXPECT_EQ(message, plaintext);
    }
}

// Test use of a key that requires an auth token for each action on the operation, with
// a per-operation challenge value included, with multiple secure IDs allowed.
TEST_P(AuthTest, AuthPerOperationMultiSid) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }

    // Enroll a password for a second user.
    alt_uid_ = 20001;
    const string alt_password = "correcthorsebatterystaple2";
    std::optional<GatekeeperEnrollResponse> rsp = doEnroll(alt_uid_, alt_password);
    ASSERT_TRUE(rsp.has_value());
    alt_sid_ = rsp->secureUserId;
    const std::vector<uint8_t> alt_handle = rsp->data;

    // Create an AES key that requires authentication per-action.
    auto builder = AuthorizationSetBuilder()
                           .AesEncryptionKey(256)
                           .BlockMode(BlockMode::ECB)
                           .Padding(PaddingMode::PKCS7)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_SECURE_ID, alt_sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::ANY);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    // Get a HAT for first user with the challenge from an in-progress operation.
    const string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    AuthorizationSet out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    const std::optional<HardwareAuthToken> hat = doVerify(uid_, challenge_, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Finish(message, {} /* signature */, &ciphertext, hat.value()));

    // Get a HAT for second user with the challenge from an in-progress operation.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    const std::optional<HardwareAuthToken> alt_hat =
            doVerify(alt_uid_, challenge_, alt_handle, alt_password);
    ASSERT_TRUE(alt_hat.has_value());
    EXPECT_EQ(alt_hat->userId, alt_sid_);
    string alt_ciphertext;
    EXPECT_EQ(ErrorCode::OK, Finish(message, {} /* signature */, &ciphertext, alt_hat.value()));
}

// Test use of a key that requires an auth token for each action on the operation, but
// which gets passed a HAT of the wrong type
TEST_P(AuthTest, AuthPerOperationWrongAuthType) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }

    // Create an AES key that requires authentication per-action, but with no valid authenticator
    // types.
    auto builder =
            AuthorizationSetBuilder()
                    .AesEncryptionKey(256)
                    .BlockMode(BlockMode::ECB)
                    .Padding(PaddingMode::PKCS7)
                    .Authorization(TAG_USER_SECURE_ID, sid_)
                    .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::FINGERPRINT);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    // Get a HAT with the challenge from an in-progress operation.
    const string message = "Hello World!";
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    AuthorizationSet out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    const std::optional<HardwareAuthToken> hat = doVerify(challenge_, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);

    // Should fail because auth type doesn't (can't) match.
    string ciphertext;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Finish(message, {} /* signature */, &ciphertext, hat.value()));
}

// Test use of a imported wrapped key that requires an auth token for each action on the operation,
// with a per-operation challenge value included.
TEST_P(AuthTest, ImportWrappedKeyAuthPerOperation) {
    if (!GatekeeperAvailable()) {
        GTEST_SKIP() << "No Gatekeeper available";
    }
    if (SecLevel() == SecurityLevel::STRONGBOX &&
        get_vendor_api_level() <= AVendorSupport_getVendorApiLevelOf(__ANDROID_API_V__)) {
        GTEST_SKIP() << "Skip test on StrongBox device with vendor-api-level <= __ANDROID_API_V__ ";
    }

    // Construct a SecureKeyWrapper as explained below:
    // 1. Generate Keys:
    //     - Create a wrapped key (the key to be securely transported).
    //     - Generate a wrapping key, which must be an RSA 2048-bit key with the purpose WRAP_KEY.
    //     - Generate a transient, 32-byte AES key to encrypt the wrapped key.
    // 2. Define Key Description:
    //     - Create a KeyDescription structure for the wrapped key. This structure defines the
    //     wrapped key's format and its authorization list.
    // 3. Encrypt the Wrapped Key:
    //     - Generate a random IV.
    //     - Encrypt the wrapped key (from step 1) with the transient AES key (from step 1) using
    //     the AES-GCM algorithm.
    //     - Use the DER-encoded KeyDescription (from step 2) as the Additional Authenticated Data
    //     (AAD) for this encryption. This binds the key data to its properties, ensuring their
    //     integrity.
    // 4. Secure the Transient Key:
    //     - Generate a random, 32-byte masking key.
    //     - XOR the transient AES key with this masking key.
    //     - Encrypt the result with the RSA wrapping key (from step 1) using the RSA OAEP MGF1 SHA1
    //     digest algorithm. This final output is the encrypted transport key.
    // 5. Assemble the Wrapper:
    //     - Construct the final SecureKeyWrapper structure. This structure contains the encrypted
    //     transport key, the IV from the AES-GCM encryption, the KeyDescription, the encrypted
    //     wrapped key, and the authentication tag generated by the AES-GCM process.

    vector<Algorithm> algorithms = {Algorithm::RSA, Algorithm::EC, Algorithm::HMAC, Algorithm::AES,
                                    Algorithm::TRIPLE_DES};
    for (auto alg : algorithms) {
        KeyFormat key_format;
        std::vector<uint8_t> wrapped_key;
        // This guide explains how to combine USER_SECURE_ID and USER_AUTH_TYPE in a KeyDescription
        // for a WrappedKey.
        //
        // The USER_SECURE_ID tag determines which Secure ID (SID) to use, either the
        // password SID or the biometric SID. The USER_AUTH_TYPE tag specifies the
        // required authentication method to authorize the key's use.
        //
        // -------------------------------------------------------------------------------------
        //
        // Password or Biometric (Flex-Auth Key)
        //
        // A key that can be authenticated by either a password or a biometric scan. This is useful
        // for maximum user flexibility.
        // USER_SECURE_ID: HardwareAuthenticatorType::PASSWORD
        // USER_AUTH_TYPE: HardwareAuthenticatorType::ANY
        //
        // -------------------------------------------------------------------------------------
        //
        // Biometric-Only (Persistent)
        //
        // A key that is authenticated by a biometric scan only and will NOT be invalidated if new
        // biometric data is enrolled. This is achieved by linking the key to the password SID.
        // USER_SECURE_ID: HardwareAuthenticatorType::PASSWORD
        // USER_AUTH_TYPE: HardwareAuthenticatorType::FINGERPRINT
        //
        // -------------------------------------------------------------------------------------
        //
        // Biometric-Only (Invalidates on New Enrollment)
        //
        // A key that is authenticated by a biometric scan only and WILL be invalidated if new
        // biometric data is enrolled. This is achieved by linking the key to the biometric SID.
        // USER_SECURE_ID: HardwareAuthenticatorType::FINGERPRINT
        // USER_AUTH_TYPE: HardwareAuthenticatorType::FINGERPRINT
        //
        // -------------------------------------------------------------------------------------
        //
        // Password-Only
        //
        // A key that can only be authenticated with a password.
        // USER_SECURE_ID: HardwareAuthenticatorType::PASSWORD
        // USER_AUTH_TYPE: HardwareAuthenticatorType::PASSWORD

        // For this test we are testing only the first combination as we cannot test the biometric
        // authentication in VTS.
        auto wrapped_key_desc =
                AuthorizationSetBuilder()
                        .Padding(PaddingMode::NONE)
                        .Authorization(TAG_USER_SECURE_ID,
                                       static_cast<int64_t>(HardwareAuthenticatorType::PASSWORD))
                        .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::ANY)
                        .SetDefaultValidity();

        SCOPED_TRACE(testing::Message() << "Algorithm-" << alg);
        switch (alg) {
            case Algorithm::RSA:
                wrapped_key_desc.RsaSigningKey(2048, 65537).Digest(Digest::NONE);
                EXPECT_TRUE(generateRsa2048Pkcs8DerKey(wrapped_key));
                key_format = KeyFormat::PKCS8;
                break;
            case Algorithm::EC:
                wrapped_key_desc.EcdsaSigningKey(EcCurve::P_256).Digest(Digest::NONE);
                EXPECT_TRUE(generateEc256Pkcs8DerKey(wrapped_key));
                key_format = KeyFormat::PKCS8;
                break;
            case Algorithm::HMAC:
                wrapped_key_desc.HmacKey(128)
                        .Digest(Digest::SHA_2_256)
                        .Authorization(TAG_MIN_MAC_LENGTH, 128);
                wrapped_key.resize(16);
                RAND_bytes(wrapped_key.data(), 16);
                key_format = KeyFormat::RAW;
                break;
            case Algorithm::AES:
                wrapped_key_desc.AesEncryptionKey(256).BlockMode(BlockMode::CBC);
                wrapped_key.resize(32);
                RAND_bytes(wrapped_key.data(), 32);
                key_format = KeyFormat::RAW;
                break;
            case Algorithm::TRIPLE_DES:
                wrapped_key_desc.TripleDesEncryptionKey(168).BlockMode(BlockMode::ECB);
                wrapped_key.resize(24);
                RAND_bytes(wrapped_key.data(), 24);
                key_format = KeyFormat::RAW;
                break;
            default:
                ADD_FAILURE() << "Invalid Algorithm " << uint32_t(alg);
                continue;
        }

        size_t aes_gcm_tag_len = 16;
        size_t aes_gcm_iv_len = 12;
        size_t aes_gcm_key_size = 32;
        size_t masking_key_size = 32;

        std::vector<uint8_t> transport_key(aes_gcm_key_size);
        std::vector<uint8_t> masking_key(masking_key_size);
        std::vector<uint8_t> iv(aes_gcm_iv_len);
        std::vector<uint8_t> rsa_2048_wrapping_key;

        RAND_bytes(transport_key.data(), aes_gcm_key_size);
        RAND_bytes(masking_key.data(), masking_key_size);
        RAND_bytes(iv.data(), aes_gcm_iv_len);

        // create KeyDescription in ASN.1 DER encoded format
        std::vector<uint8_t> der_wrapped_key_description;
        EXPECT_EQ(ErrorCode::OK, buildWrappedKeyDescription(wrapped_key_desc, key_format,
                                                            der_wrapped_key_description));

        std::vector<uint8_t> encrypted_wrapped_key(wrapped_key.size());
        std::vector<uint8_t> tag(aes_gcm_tag_len);
        // Encrypt the wrapped key with transport_key
        aes256GcmEncrypt(transport_key, wrapped_key, der_wrapped_key_description, iv,
                         encrypted_wrapped_key, tag);
        // XOR transport key with masking key
        std::transform(transport_key.begin(), transport_key.end(), masking_key.begin(),
                       transport_key.begin(),
                       [](uint8_t transport_key_byte, uint8_t masking_key_byte) {
                           return transport_key_byte ^ masking_key_byte;
                       });
        // Build Authorization list for wrapping key which includes WRAP_KEY as purpose.
        auto wrapping_key_desc = AuthorizationSetBuilder()
                                         .RsaEncryptionKey(2048, 65537)
                                         .Digest(Digest::SHA_2_256)
                                         .Padding(PaddingMode::RSA_OAEP)
                                         .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                         .SetDefaultValidity();

        // Generate a RSA 2048 key as Wrapping key
        ASSERT_EQ(ErrorCode::OK,
                  GenerateKey(AuthorizationSetBuilder()
                                      .Authorization(TAG_NO_AUTH_REQUIRED)
                                      .RsaEncryptionKey(2048, 65537)
                                      .Authorization(TAG_PURPOSE, KeyPurpose::WRAP_KEY)
                                      .Padding(PaddingMode::RSA_OAEP)
                                      .Digest(Digest::SHA_2_256)
                                      .SetDefaultValidity()));

        // Encrypt the masked transport key with wrapping key
        auto params =
                AuthorizationSetBuilder().Digest(Digest::SHA_2_256).Padding(PaddingMode::RSA_OAEP);
        string encrypted_transport_key =
                LocalRsaEncryptMessage({transport_key.begin(), transport_key.end()}, params);

        // Build SecureKeyWrapper in ASN.1 DER encoded format
        std::vector<uint8_t> der_wrapped_key;
        EXPECT_EQ(ErrorCode::OK,
                  buildWrappedKey({encrypted_transport_key.begin(), encrypted_transport_key.end()},
                                  iv, encrypted_wrapped_key, tag, key_format, wrapped_key_desc,
                                  der_wrapped_key));

        int64_t biometric_sid = 24;
        ASSERT_EQ(ErrorCode::OK, ImportWrappedKey(der_wrapped_key, key_blob_, masking_key,
                                                  params.vector_data(), sid_, biometric_sid));
        AuthorizationSet auths;
        for (auto& entry : key_characteristics_) {
            auths.push_back(AuthorizationSet(entry.authorizations));
        }
        // Tag::USER_SECURE_ID must be included in the key characteristics
        ASSERT_TRUE(auths.Contains(TAG_USER_SECURE_ID, sid_));

        // Block aligned input message
        const string message = "1234567890123456";
        auto begin_params = AuthorizationSetBuilder().Padding(PaddingMode::NONE);
        AuthorizationSet out_params;
        KeyPurpose key_purpose;
        switch (alg) {
            case Algorithm::RSA:
                // fall through
            case Algorithm::EC:
                begin_params.Digest(Digest::NONE);
                key_purpose = KeyPurpose::SIGN;
                break;
            case Algorithm::HMAC:
                begin_params.Digest(Digest::SHA_2_256).Authorization(TAG_MAC_LENGTH, 160);
                key_purpose = KeyPurpose::SIGN;
                break;
            case Algorithm::AES:
                begin_params.BlockMode(BlockMode::CBC);
                key_purpose = KeyPurpose::ENCRYPT;
                break;
            case Algorithm::TRIPLE_DES:
                begin_params.BlockMode(BlockMode::ECB);
                key_purpose = KeyPurpose::ENCRYPT;
                break;
            default:
                ADD_FAILURE() << "Invalid Algorithm " << uint32_t(alg);
                continue;
        }
        // The imported wrapped key is treated as 'auth-per-operation' because AUTH_TIMEOUT is not
        // specified.
        EXPECT_EQ(ErrorCode::OK, Begin(key_purpose, begin_params, &out_params));
        const std::optional<HardwareAuthToken> hat = doVerify(uid_, challenge_, handle_, password_);
        ASSERT_TRUE(hat.has_value());
        EXPECT_EQ(hat->userId, sid_);
        string ciphertext;
        string signature;
        EXPECT_EQ(ErrorCode::OK, Finish(message, signature, &ciphertext, hat.value()));
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(AuthTest);

}  // namespace aidl::android::hardware::security::keymint::test
