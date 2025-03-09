#include <gtest/gtest.h>
#include "hwcryptokeyimpl.h"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(HwCryptoHalDelegator, keyPolicyCppToNdk) {
    cpp_hwcrypto::KeyPolicy cppPolicy = cpp_hwcrypto::KeyPolicy();
    cppPolicy.keyType = cpp_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING;
    cppPolicy.usage = cpp_hwcrypto::types::KeyUse::DECRYPT;
    cppPolicy.keyLifetime = cpp_hwcrypto::types::KeyLifetime::PORTABLE;
    cppPolicy.keyManagementKey = false;
    cppPolicy.keyPermissions.push_back(
            cpp_hwcrypto::types::KeyPermissions::ALLOW_PORTABLE_KEY_WRAPPING);
    ndk_hwcrypto::KeyPolicy ndkPolicy = android::trusty::hwcryptohalservice::convertKeyPolicy<
            ndk_hwcrypto::KeyPolicy, cpp_hwcrypto::KeyPolicy>(cppPolicy);
    EXPECT_EQ(ndkPolicy.keyType, ndk_hwcrypto::types::KeyType::AES_128_CBC_PKCS7_PADDING);
    EXPECT_EQ(ndkPolicy.usage, ndk_hwcrypto::types::KeyUse::DECRYPT);
    EXPECT_EQ(ndkPolicy.keyLifetime, ndk_hwcrypto::types::KeyLifetime::PORTABLE);
    EXPECT_EQ(ndkPolicy.keyManagementKey, false);
    EXPECT_EQ(ndkPolicy.keyPermissions.size(), 1ul);
    EXPECT_EQ(ndkPolicy.keyPermissions[0],
              ndk_hwcrypto::types::KeyPermissions::ALLOW_PORTABLE_KEY_WRAPPING);
}

TEST(HwCryptoHalDelegator, keyPolicyNdkToCpp) {
    ndk_hwcrypto::KeyPolicy ndkPolicy = ndk_hwcrypto::KeyPolicy();
    ndkPolicy.keyType = ndk_hwcrypto::types::KeyType::AES_128_CTR;
    ndkPolicy.usage = ndk_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT;
    ndkPolicy.keyLifetime = ndk_hwcrypto::types::KeyLifetime::HARDWARE;
    ndkPolicy.keyManagementKey = true;
    ndkPolicy.keyPermissions.push_back(
            ndk_hwcrypto::types::KeyPermissions::ALLOW_EPHEMERAL_KEY_WRAPPING);
    ndkPolicy.keyPermissions.push_back(
            ndk_hwcrypto::types::KeyPermissions::ALLOW_HARDWARE_KEY_WRAPPING);
    cpp_hwcrypto::KeyPolicy cppPolicy = android::trusty::hwcryptohalservice::convertKeyPolicy<
            cpp_hwcrypto::KeyPolicy, ndk_hwcrypto::KeyPolicy>(ndkPolicy);
    EXPECT_EQ(cppPolicy.keyType, cpp_hwcrypto::types::KeyType::AES_128_CTR);
    EXPECT_EQ(cppPolicy.usage, cpp_hwcrypto::types::KeyUse::ENCRYPT_DECRYPT);
    EXPECT_EQ(cppPolicy.keyLifetime, cpp_hwcrypto::types::KeyLifetime::HARDWARE);
    EXPECT_EQ(cppPolicy.keyManagementKey, true);
    EXPECT_EQ(cppPolicy.keyPermissions.size(), 2ul);
    EXPECT_EQ(cppPolicy.keyPermissions[0],
              cpp_hwcrypto::types::KeyPermissions::ALLOW_EPHEMERAL_KEY_WRAPPING);
    EXPECT_EQ(cppPolicy.keyPermissions[1],
              cpp_hwcrypto::types::KeyPermissions::ALLOW_HARDWARE_KEY_WRAPPING);
}