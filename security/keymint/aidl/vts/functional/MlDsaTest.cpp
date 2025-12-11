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

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

namespace {

const std::map<MlDsaVariant, std::string> kOidString = {
        {MlDsaVariant::ML_DSA_65, ML_DSA_65_OID},
        {MlDsaVariant::ML_DSA_87, ML_DSA_87_OID},
};

const std::string kSeed =
        hex2str("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");

// PKCS#8 encoding of an ML-DSA-44 private key seed.
// From RFC 9881 section C.1.1.1
const std::string kSeed44Pkcs8 =
        hex2str("3034"                // SEQUENCE len x34 {
                "020100"              // INTEGER 0 (Version)
                "300b"                // SEQUENCE len 11 (privateKeyAlgorithm) {
                "0609"                // OBJECT_IDENTIFIER len 9
                "608648016503040311"  //  2.16.840.1.101.3.4.3.17
                // }
                "0422"  // OCTET STRING len 34
                "8020"  // tag 0 primitive len 32
                "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"  // seed value
        );

static const std::map<MlDsaVariant, std::string> kPkcs8SeedData = {
        // From RFC 9881 section C.1.2.1
        {MlDsaVariant::ML_DSA_65,
         hex2str("3034"                // SEQUENCE len x34 {
                 "020100"              // INTEGER 0 (Version)
                 "300b"                // SEQUENCE len 11 (privateKeyAlgorithm) {
                 "0609"                // OBJECT_IDENTIFIER len 9
                 "608648016503040312"  //  2.16.840.1.101.3.4.3.18
                 // }
                 "0422"  // OCTET STRING len 34
                 "8020"  // tag 0 primitive len 32
                 "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"  // seed value
                 )},
        // From RFC 9881 section C.1.3.1
        {MlDsaVariant::ML_DSA_87,
         hex2str("3034"                // SEQUENCE len x34 {
                 "020100"              // INTEGER 0 (Version)
                 "300b"                // SEQUENCE len 11 (privateKeyAlgorithm) {
                 "0609"                // OBJECT_IDENTIFIER len 9
                 "608648016503040313"  //  2.16.840.1.101.3.4.3.19
                 // }
                 "0422"  // OCTET STRING len 34
                 "8020"  // tag 0 primitive len 32
                 "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"  // seed
                                                                                     // value
                 )},
};

// PKCS#8 encoding of an ML-DSA-65 private key with both a seed and an (invalid) expanded key.
// Adapted from RFC 9881 section C.1.2.3
const std::string kBoth65Pkcs8 =
        hex2str("303c"                // SEQUENCE len x3c {
                "020100"              // INTEGER 0 (Version)
                "300b"                // SEQUENCE len 11 (privateKeyAlgorithm) {
                "0609"                // OBJECT_IDENTIFIER len 9
                "608648016503040312"  //  2.16.840.1.101.3.4.3.18
                // }
                "042a"  // OCTET STRING len 42 {
                "3028"  // SEQUENCE len 40
                "0420"  // OCTET STRING len 32
                "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"  // seed value
                "0404"      // OCTET STRING len 4 (invalid)
                "deadbeef"  // incorrect expanded key
        );

// PKCS#8 encoding of an ML-DSA-65 private key as a (large) expanded key.
// From RFC 9881 section C.1.2.2
const std::string kExpanded65Pkcs8 =
        "30820fd8"                // SEQUENCE
        "020100"                  // INTEGER 0 (Version)
        "300b"                    // SEQUENCE len 11 (privateKeyAlgorithm)
        "0609608648016503040312"  // 2.16.840.1.101.3.4.3.18
        // }
        "04820fc4"  // OCTET STRING len 4036
        "04820fc0"  // OCTET STRING len 4032
        "48683d91978e31eb3dddb8b0473482d2b88a5f625949fd8f58a561e696bd4c27"
        "d853fa69b8199023e8cd678dd9fabf9047646ffd0cb3cc7f795805a71e70d237"
        "1b0563e3cd3346149c8c9ebcf23b0a4e5a900eea9c6562790a7c63e38663daa2"
        "dddb6e480dc405a1e701948b74841ef5cc1c3f2bf327972e9510510cd5375ecc"
        "0855717711872221862381000424778061475007501717035504515125471838"
        "0461757222441088686086460127475671808706668643324441220436386675"
        "0282363424432205736410645554772275568143361462550820643768546875"
        "4353751068718333805475052580752818843811087260202008588301836113"
        "8282120617115787687888786437546016571550847188660727328806647418"
        "5676218031827664157824502564664311350436478012667314301166065586"
        "4718368863503847861101202356116137860785321240075478823043666116"
        "6042554182856053677856384344306326107707317842721411165303852768"
        "6746015082373532076610750468124806660303265231244540880031808876"
        "7217307182472151278011654474866172233380866064468352158420368011"
        "8021181833177354534881004486536743705772588334603842328568100604"
        "2604258456023568205183863843242122424564585867714572850478871718"
        "0618836086864156508116502646700608266227383172407257300727288620"
        "6675886826070640203303436631554642453456671873456583702250846856"
        "2880703670846237171006571758477870865553782235144677285673032287"
        "0014332061715845526632502651334777380355164313473510662751757402"
        "4688817067434681860176524533308721043434010322876351552650813077"
        "4544416815418363641120402687304367771280884635545300624581045836"
        "5124842780345166635843785601465115742321436685224777313450178362"
        "4205500064844712344088006047354057833363082106152252072488513486"
        "3706762258857126567347681646468425870812270550083832002320806634"
        "5336003346857247063554003577122752307142536874374570056643224482"
        "8520721833302053373340772780552530635250406733461318072807172483"
        "7763457318585160233344362516433816085877346242883007036585375500"
        "7552315037021324630437086806361503030043586357080211066473463522"
        "6203304380210852875783210788674808563474367342840584668414370055"
        "1087342644772112738473652647257714470417864426024711874081221660"
        "5847178137067680817058185585471363421075580163583585184403847110"
        "3387426282477413655442707346357775006625626842021246838646166460"
        "3122538884540084573446475447256054616684663088063827156328718384"
        "0652247681160662130330186802801384630505657238758365723230688046"
        "1226066516755705324132276735170801530016284601348877011188155713"
        "1546431170473288285636823455504186276563111168750510425441442785"
        "2211171788153685157447166255365583630250285576875327137103723705"
        "7147617136518412423664446641435205210851570333638602584266281481"
        "1054626817303875643321658856866363281340625401204088654788617165"
        "7623726234867030115115632050753502122108426531435567111525720106"
        "8536301505575860587843143132787880873847886378818138734261783885"
        "2466773350602115146423823268013544078347538553575283233518760115"
        "2134325773333655188615816168241842212230841448151201103024777242"
        "5443660677177076030145254035001838732377352650863571137344816052"
        "7745655373008583778503512111548062885018026813865205346801320724"
        "1803213005723864076427114101838525510632607104865176833828572762"
        "3545187350831328863766614263116750331125537641760314331772122344"
        "18a82e4f5c9ea0faf99eb04d78a7332711117c33f18eca21f8743376ada52198"
        "04a7ed9a5557fcd67a3550b3a4b8c588629c021475fa3d56d5d6cfbb1a09bda8"
        "d14de622ddff16d8bc99b14278a8af1d76bed157672dd9c32316f97e8daadef8"
        "d9da69586725567fb96b59990d4bf0bc9c195b90b74295f5675b24257c2710c1"
        "75b0153f2911328c2eb7abb9ad46e70a8b53c39ea642cee4b3cb42620e863ce8"
        "b650ce8adcd923721a1687023c673a8cbb6b03d51cd197e8c346ebadce93950f"
        "88cee201db9e320843e29f300d9a19500d70a4caf272c69e4eef69fbb8a55efd"
        "7ca2bed990d2d3b582848f9c45c2abc54cfc47d34f06c0ffa56fcd762ab9cba9"
        "146d7725218963b240d72b6d22c93171fbd47788b76e72042def0878d23df631"
        "a1a1e5a6027686de5b4a10e91069c8f2ba0259b04d6409da96567ca52da49702"
        "6e583a0ecefc1f01e6b988e21f9767a2b7e1672deb9a1e2a3fcc863aa91517c3"
        "34620601b4fe79730e934935f4b6fbc4e32695145c2b5f6a127fecc0a277451e"
        "bc3fd523444f9ee7c9c34534f356db544fc31c1bfde5f65c77ea2f7c2eae4c55"
        "ebaf104271c566fd4ebac71c7a62c74952817ae675504d9599b1b762b6aca168"
        "a83248c9d9adb0ceb1556e5759490bbc0c7900795ad72123038b662f64f106a9"
        "993681a25d59af7bc97a235be9284c5bc45a6c90cb1c2999c663d96b478e2307"
        "f85548957d65740e2673e9ebd1352829038f462b8fd3b5681da55c0252523853"
        "525ea0ad647e71ac2c5a8893e603ac97e56c04ceb2f26f5c5b4b6d94ab811380"
        "fd00f2208fe86535086aebfd35c29120624c04fbb6113929d9c556350253766c"
        "209fdba83c95fccd342a28099355d00bc863f4eef596eb0b42ebcc7c79491cce"
        "ae205ea0b8059fbb8a5726c5949d2b15e7e29c51fc9b02ee1a4fc357b5f1bef9"
        "c4add46a2a920c2fbf08a37eb1514bfa15110a4392a74c6f13c50c5cffd97531"
        "098d7cd23b60eb35c4a428b46c55386e1010c4ba7f70e4c7ecb7575f3063a71e"
        "84dfdcf09a58b2cdb0f99f27ed378610d25cbad7bfa6ba0d59189cfe88eab9b4"
        "6d7e6db0307eabe4198e99bd71f779ab66581e0912fc7b1d2585245e9a12687a"
        "975cd5e8e1dcc045d5f891c4c685db07cf81e77389b363eb6bdfe39b27ff84c9"
        "7eefee162e3b451fe6914719cb6436d855960ff915d7cea6adeafdfc1c05786c"
        "49f923a474ffdfc3153a06e6ed0b0ad220d72524434d5273c0aab6dde4e91476"
        "d581a2695a60de6d9f44d77aa08266e938eeb4a9597c9b64986059e49262a4ea"
        "b2454e14015ad0536c42733a5d77d7995c2a20446009ebfe5632c80c08ed2b97"
        "af35066489f597eb1b1f11f04f60e0c9040159c44ab3e60e0a15229d191228be"
        "d17bbc3ac939b3c67cee135f352c27216c9c31f72a3e87040c5f619306eb0b6c"
        "ca2a9ce7b22a1694d00ca9c05e315126457f26ce84f9617241860782f864b473"
        "d84017491902b1bdc8cdc5800dd46127fb80a71c095b473a562529b3b1e7e437"
        "e158a5f6666e9974d005b062c2309e6dce98f9b658c6e3f9a216d58c8c9142bd"
        "1c8c85a9da872ebbfad3fea9d9aba2b68c0e8f19c6ff5f00584d45daf9d6c9d6"
        "9ed04b8da8d687258b77807927612c530446fea7697ae3f926698929bc6a5a8c"
        "f3e2024c0f0c5ee57b5869bf981881caf9e3665fc7f7efc678929f87a56eaa42"
        "ea4d1ff6691822dd79a47096b776d1d8f01456e5873b0738406c382c573ae9cd"
        "e2d9e7f231b6cc5c676e7cf43963373013a58075381ff0949be084546d72e4f8"
        "a3e5fe4aa5091add234e2afe0030b1b663ae9d2d32410986b9402aaaf2465b74"
        "a5e2d0bc38e3a92bbddd8a1fed7b948c23cce6f8c08fe356835ba65b0f984068"
        "616ef48138efd89bf357a54d2ebbf376cbdcc69c5f1f61c64d2794bc06ccb9ab"
        "df66e25085d8c830e2ae3b0fe0f07a7af8b9320bf342970997d67d7c12593a8f"
        "bfade635aac53083a7022c47d5f77a52b57b598da9392ae6d86afc46fc064551"
        "81b9c75a646dc21f81e4bf213753de737fd2a140027920add35a223f9f5f4465"
        "ceb60c03ed0455a333a5cc83adbf43f1f42c2ccb8328c21c7ab7faed2b21cfad"
        "e2da55223aaab2af9b41c7332341746341b39aa2f43815650f5480511424cfa6"
        "901779c4d18b638cc0287aaaf31680338d20b17c7449fdc6a278a8d96a82ee4c"
        "4eca40125e2d65290071c7aef1be6a991598fb9d59512523bcd4b38c566b8e80"
        "a73ae333e134414327ef1d83c47c49dfe7936df1338a5e247787868fc84fdcb9"
        "5ac89c185c4bb5fd57b2338ac42b41c10a823df39624f36b15a2f067584e06ca"
        "2e08ccaff1618fe01dd06df3512e0b724dec8506da24215acacc2c51b82ad8d3"
        "02002fb41068b1da4f8bb147987b3516bad5dbddf01318fd3fa9bc43702ac498"
        "c719d95f2e841b622a5e4848a3c5c262959992ea7a7d72ca8a368028f497dfad"
        "93355cbb1bb9786d14ff2cf590317848f95856427110dda36f5192a816ce9c88"
        "16cc7bbfc804efc40085a3850b89f1e7fe5656dba410f906a97c32336c1ae7e8"
        "1737a83e087354e428da8538d948dbf5dfacb59dd2b5fd3bc803f4ba432c9a73"
        "9df2cfa9ed9484320f97edff1a48c6b86b3002cfb772dd5e562bc4c3d683ed96"
        "4b6199fa0514b0790d958095b7b85c6be875fbb559e1930146ccea63a388a194"
        "fe09c3dea03be52de27e901017afe809af630a7382bf5c4cd4d1b8f41579fb43"
        "48ede4ca05f4cd3f139a31b2544e516dbe4086b9bb4b2bed47e2d230982dd519"
        "2429d377b7c0745cc068e2f5a4aa04c7ff87209ed1259976a0fc9b25e9e851d4"
        "e3502c02c85d6dff029e211d01ebf0e9e7188d568f8437d813b0f122f2fb1760"
        "3b693ed9c38f17cfd50b815e6d9dfc0ed2ccf19f6399274a1420f235a59d8bf7"
        "24345e14e45d9e4be8934dfc3fa92678db61d7118bf53cb8a2225b335f7eae50"
        "e3f941237628db76d8ea38f77a72af3a26c81fe43523b335535a5d1db7c38f34"
        "1082bb5734d089e8ae309cfda3a0bcb5cd5b097113c8edf9616aa4f6e6631b91"
        "25276fb3f680a34341c3db668dc6cad45fc93b2708ca2af75ccce734fd191c50"
        "089dad53982fddae02531ff93e1f21ff395fc0a12874edf06b6f9647e95a7324"
        "586c71dfd91d901d621858190fecd00ccd110bbac59f96cb884c3c93994748a5"
        "6f41283bfc41fb89052153a894588c3cb9017f3d66326c985637e575acb81234"
        "6342654025d602de3ba940c19ac1a633dffda977b529b8013e19c1d6d0680f4d"
        "ae62c924450ae66aab82f21473061dab3d62b247f907e3551939ad3f5465e9d0"
        "8a82bfea17eea1b6b2b923757477f993000b2f43b70f28aaab1fe9a26ad1fd33"
        "61616c0b0e242fe76604b7033a1f30e97e28f526ca3c880fe2b8d9d1b0c9ff18"
        "8b31cb9d97425acab9b216d98a6ae355e583da71e8864ee3d16b0759796190ef"
        "545c1e62bfef92af6ca147b13244d6c892fc8ef223ab3f43f924c2f466097ee8";

const MlDsaVariant kVariants[] = {MlDsaVariant::ML_DSA_65, MlDsaVariant::ML_DSA_87};

}  // namespace

class MlDsaTest : public KeyMintAidlTestBase {
  public:
    void SetUp() {
        KeyMintAidlTestBase::SetUp();
        if (!MlDsaSupported()) {
            GTEST_SKIP() << "ML-DSA support not required";
        }
    }

    static AuthorizationSetBuilder KeyParams(MlDsaVariant variant) {
        return AuthorizationSetBuilder()
                .MlDsaSigningKey(variant)
                .Digest(Digest::NONE)
                .Authorization(TAG_NO_AUTH_REQUIRED)
                .SetDefaultValidity();
    }

    AuthorizationSetBuilder ImportParams() {
        return AuthorizationSetBuilder()
                .Authorization(TAG_ALGORITHM, Algorithm::ML_DSA)
                .SigningKey()
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
                result == ErrorCode::INVALID_ARGUMENT)
            << "result=" << result;
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
                result == ErrorCode::INVALID_ARGUMENT)
            << "result=" << result;
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
                    result == ErrorCode::INVALID_ARGUMENT)
                << "result=" << result;

        result = ImportKey(KeyParams(variant), KeyFormat::RAW, longSeed);
        EXPECT_NE(result, ErrorCode::OK);
        EXPECT_TRUE(result == ErrorCode::INVALID_INPUT_LENGTH ||
                    result == ErrorCode::UNSUPPORTED_KEY_SIZE ||
                    result == ErrorCode::INVALID_ARGUMENT)
                << "result=" << result;
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
                    result == ErrorCode::INVALID_ARGUMENT)
                << "result=" << result;
    }
}

TEST_P(MlDsaTest, ImportRawSeedUnknownVariant) {
    ErrorCode result = ImportKey(KeyParams(static_cast<MlDsaVariant>(44)), KeyFormat::RAW, kSeed);
    EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                result == ErrorCode::INVALID_ARGUMENT)
            << "result=" << result;
}

TEST_P(MlDsaTest, ImportWrappedRawSeed) {
    GTEST_SKIP() << "TODO: add test for import of a wrapped ML-DSA private key seed";
}

TEST_P(MlDsaTest, ImportPkcs8SeedUnspecifiedVariant) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        string data = kPkcs8SeedData.at(variant);
        ErrorCode result = ImportKey(ImportParams(), KeyFormat::PKCS8, data);
        EXPECT_EQ(result, ErrorCode::OK);
        ASSERT_GT(key_blob_.size(), 0U);

        CheckCommonParams(key_characteristics_, KeyOrigin::IMPORTED);
        CheckCharacteristics(key_blob_, key_characteristics_);

        ASSERT_GT(cert_chain_.size(), 0);
        EXPECT_TRUE(ChainSignaturesAreValid(cert_chain_));

        AuthorizationSet crypto_params = SecLevelAuthorizations(key_characteristics_);
        EXPECT_TRUE(crypto_params.Contains(TAG_ALGORITHM, Algorithm::ML_DSA));
        // Characteristics should inclkude the variant deduced from the PKCS#8 data.
        EXPECT_TRUE(crypto_params.Contains(TAG_ML_DSA_VARIANT, variant))
                << "Variant " << variant << "missing";

        // Check a signature against public key locally created from the seed.
        string message = "12345678901234567890123456789012";
        string signature = SignMessage(message, AuthorizationSetBuilder().Digest(Digest::NONE));
        DefaultSeedVerify(message, signature, variant);
    }
}

TEST_P(MlDsaTest, ImportPkcs8SeedMatchingVariant) {
    for (MlDsaVariant variant : kVariants) {
        SCOPED_TRACE(testing::Message() << variant);

        string data = kPkcs8SeedData.at(variant);
        ErrorCode result = ImportKey(ImportParams().Authorization(TAG_ML_DSA_VARIANT, variant),
                                     KeyFormat::PKCS8, data);
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

TEST_P(MlDsaTest, ImportPkcs8SeedMismatchVariant) {
    for (const auto& it : kPkcs8SeedData) {
        MlDsaVariant variant = it.first;
        SCOPED_TRACE(testing::Message() << variant);
        MlDsaVariant wrong = (variant == MlDsaVariant::ML_DSA_65) ? MlDsaVariant::ML_DSA_87
                                                                  : MlDsaVariant::ML_DSA_65;

        ErrorCode result = ImportKey(ImportParams().Authorization(TAG_ML_DSA_VARIANT, wrong),
                                     KeyFormat::PKCS8, it.second);
        EXPECT_EQ(result, ErrorCode::IMPORT_PARAMETER_MISMATCH);
    }
}

TEST_P(MlDsaTest, ImportPkcs8SeedUnsupportedVariant) {
    ErrorCode result = ImportKey(ImportParams(), KeyFormat::PKCS8, kSeed44Pkcs8);
    EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                result == ErrorCode::INVALID_ARGUMENT)
            << "result=" << result;
}

TEST_P(MlDsaTest, ImportPkcs8ExpandedFails) {
    ErrorCode result = ImportKey(ImportParams(), KeyFormat::PKCS8, kExpanded65Pkcs8);
    EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                result == ErrorCode::INVALID_ARGUMENT)
            << "result=" << result;
}

TEST_P(MlDsaTest, ImportPkcs8BothFails) {
    ErrorCode result = ImportKey(ImportParams(), KeyFormat::PKCS8, kBoth65Pkcs8);
    EXPECT_TRUE(result == ErrorCode::UNSUPPORTED_ML_DSA_VARIANT ||
                result == ErrorCode::INVALID_ARGUMENT)
            << "result=" << result;
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
