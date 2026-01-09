//
// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Attestation keys and certificates.
//!
//! Hard-coded keys and certs copied from system/keymaster/context/soft_attestation_cert.cpp
//! trusty/user/app/keymint/secure_storage_manager/software.rs also contains the same keys and certs

use kmr_common::{
    crypto::ec, crypto::rsa, crypto::CurveType, crypto::KeyMaterial, wire::keymint,
    wire::keymint::EcCurve, Error,
};
use kmr_ta::device::{RetrieveCertSigningInfo, SigningAlgorithm, SigningKeyType};

/// RSA attestation private key in PKCS#1 format.
///
/// Decoded contents (using [der2ascii](https://github.com/google/der-ascii)):
///
/// ```
/// SEQUENCE {
///   INTEGER { 0 }
///   INTEGER { `00c08323dc56881bb8302069f5b08561c6eebe7f05e2f5a842048abe8b47be76feaef25cf29b2afa3200141601429989a15fcfc6815eb363583c2fd2f20be4983283dd814b16d7e185417ae54abc296a3a6db5c004083b68c556c1f02339916419864d50b74d40aeca484c77356c895a0c275abfac499d5d7d2362f29c5e02e871` }
///   INTEGER { 65537 }
///   INTEGER { `00be860b0b99a802a6fb1a59438a7bb715065b09a36dc6e9cacc6bf3c02c34d7d79e94c6606428d88c7b7f6577c1cdea64074abe8e7286df1f0811dc9728260868de95d32efc96b6d084ff271a5f60defcc703e7a38e6e29ba9a3c5fc2c28076b6a896af1d34d78828ce9bddb1f34f9c9404430781298e201316725bbdbc993a41` }
///   INTEGER { `00e1c6d927646c0916ec36826d594983740c21f1b074c4a1a59867c669795c85d3dc464c5b929e94bfb34e0dcc5014b10f13341ab7fdd5f60414d2a326cad41cc5` }
///   INTEGER { `00da485997785cd5630fb0fd8c5254f98e538e18983aae9e6b7e6a5a7b5d343755b9218ebd40320d28387d789f76fa218bcc2d8b68a5f6418fbbeca5179ab3afbd` }
///   INTEGER { `50fefc32649559616ed6534e154509329d93a3d810dbe5bdb982292cf78bd8badb8020ae8d57f4b71d05386ffe9e9db271ca3477a34999db76f8e5ece9c0d49d` }
///   INTEGER { `15b74cf27cceff8bb36bf04d9d8346b09a2f70d2f4439b0f26ac7e03f7e9d1f77d4b915fd29b2823f03acb5d5200e0857ff2a803e93eee96d6235ce95442bc21` }
///   INTEGER { `0090a745da8970b2cd649660324228c5f82856ffd665ba9a85c8d60f1b8bee717ecd2c72eae01dad86ba7654d4cf45adb5f1f2b31d9f8122cfa5f1a5570f9b2d25` }
/// }
/// ```
const RSA_ATTEST_KEY: &str = concat!(
    "3082025d02010002818100c08323dc56881bb8302069f5b08561c6eebe7f05e2",
    "f5a842048abe8b47be76feaef25cf29b2afa3200141601429989a15fcfc6815e",
    "b363583c2fd2f20be4983283dd814b16d7e185417ae54abc296a3a6db5c00408",
    "3b68c556c1f02339916419864d50b74d40aeca484c77356c895a0c275abfac49",
    "9d5d7d2362f29c5e02e871020301000102818100be860b0b99a802a6fb1a5943",
    "8a7bb715065b09a36dc6e9cacc6bf3c02c34d7d79e94c6606428d88c7b7f6577",
    "c1cdea64074abe8e7286df1f0811dc9728260868de95d32efc96b6d084ff271a",
    "5f60defcc703e7a38e6e29ba9a3c5fc2c28076b6a896af1d34d78828ce9bddb1",
    "f34f9c9404430781298e201316725bbdbc993a41024100e1c6d927646c0916ec",
    "36826d594983740c21f1b074c4a1a59867c669795c85d3dc464c5b929e94bfb3",
    "4e0dcc5014b10f13341ab7fdd5f60414d2a326cad41cc5024100da485997785c",
    "d5630fb0fd8c5254f98e538e18983aae9e6b7e6a5a7b5d343755b9218ebd4032",
    "0d28387d789f76fa218bcc2d8b68a5f6418fbbeca5179ab3afbd024050fefc32",
    "649559616ed6534e154509329d93a3d810dbe5bdb982292cf78bd8badb8020ae",
    "8d57f4b71d05386ffe9e9db271ca3477a34999db76f8e5ece9c0d49d024015b7",
    "4cf27cceff8bb36bf04d9d8346b09a2f70d2f4439b0f26ac7e03f7e9d1f77d4b",
    "915fd29b2823f03acb5d5200e0857ff2a803e93eee96d6235ce95442bc210241",
    "0090a745da8970b2cd649660324228c5f82856ffd665ba9a85c8d60f1b8bee71",
    "7ecd2c72eae01dad86ba7654d4cf45adb5f1f2b31d9f8122cfa5f1a5570f9b2d",
    "25",
);

/// Attestation certificate corresponding to [`RSA_ATTEST_KEY`], signed by the key in
/// [`RSA_ATTEST_ROOT_CERT`].
///
/// Decoded contents:
///
/// ```
/// Certificate:
///     Data:
///         Version: 3 (0x2)
///         Serial Number: 4096 (0x1000)
///     Signature Algorithm: SHA256-RSA
///         Issuer: C=US, O=Google, Inc., OU=Android, L=Mountain View, ST=California
///         Validity:
///             Not Before: 2016-01-04 12:40:53 +0000 UTC
///             Not After : 2035-12-30 12:40:53 +0000 UTC
///         Subject: C=US, O=Google, Inc., OU=Android, ST=California, CN=Android Software Attestation Key
///         Subject Public Key Info:
///             Public Key Algorithm: rsaEncryption
///                 Public Key: (1024 bit)
///                 Modulus:
///                     c0:83:23:dc:56:88:1b:b8:30:20:69:f5:b0:85:61:
///                     c6:ee:be:7f:05:e2:f5:a8:42:04:8a:be:8b:47:be:
///                     76:fe:ae:f2:5c:f2:9b:2a:fa:32:00:14:16:01:42:
///                     99:89:a1:5f:cf:c6:81:5e:b3:63:58:3c:2f:d2:f2:
///                     0b:e4:98:32:83:dd:81:4b:16:d7:e1:85:41:7a:e5:
///                     4a:bc:29:6a:3a:6d:b5:c0:04:08:3b:68:c5:56:c1:
///                     f0:23:39:91:64:19:86:4d:50:b7:4d:40:ae:ca:48:
///                     4c:77:35:6c:89:5a:0c:27:5a:bf:ac:49:9d:5d:7d:
///                     23:62:f2:9c:5e:02:e8:71:
///                 Exponent: 65537 (0x10001)
///         X509v3 extensions:
///             X509v3 Authority Key Identifier:
///                 keyid:29faf1accc4dd24c96402775b6b0e932e507fe2e
///             X509v3 Subject Key Identifier:
///                 keyid:d40c101bf8cd63b9f73952b50e135ca6d7999386
///             X509v3 Key Usage: critical
///                 Digital Signature, Certificate Signing
///             X509v3 Basic Constraints: critical
///                 CA:true, pathlen:0
///     Signature Algorithm: SHA256-RSA
///          9e:2d:48:5f:8c:67:33:dc:1a:85:ad:99:d7:50:23:ea:14:ec:
///          43:b0:e1:9d:ea:c2:23:46:1e:72:b5:19:dc:60:22:e4:a5:68:
///          31:6c:0b:55:c4:e6:9c:a2:2d:9f:3a:4f:93:6b:31:8b:16:78:
///          16:0d:88:cb:d9:8b:cc:80:9d:84:f0:c2:27:e3:6b:38:f1:fd:
///          d1:e7:17:72:31:59:35:7d:96:f3:c5:7f:ab:9d:8f:96:61:26:
///          4f:b2:be:81:bb:0d:49:04:22:8a:ce:9f:f7:f5:42:2e:25:44:
///          fa:21:07:12:5a:83:b5:55:ad:18:82:f8:40:14:9b:9c:20:63:
///          04:7f:
/// ```
const RSA_ATTEST_CERT: &str = concat!(
    "308202b63082021fa00302010202021000300d06092a864886f70d01010b0500",
    "3063310b30090603550406130255533113301106035504080c0a43616c69666f",
    "726e69613116301406035504070c0d4d6f756e7461696e205669657731153013",
    "060355040a0c0c476f6f676c652c20496e632e3110300e060355040b0c07416e",
    "64726f6964301e170d3136303130343132343035335a170d3335313233303132",
    "343035335a3076310b30090603550406130255533113301106035504080c0a43",
    "616c69666f726e696131153013060355040a0c0c476f6f676c652c20496e632e",
    "3110300e060355040b0c07416e64726f69643129302706035504030c20416e64",
    "726f696420536f667477617265204174746573746174696f6e204b657930819f",
    "300d06092a864886f70d010101050003818d0030818902818100c08323dc5688",
    "1bb8302069f5b08561c6eebe7f05e2f5a842048abe8b47be76feaef25cf29b2a",
    "fa3200141601429989a15fcfc6815eb363583c2fd2f20be4983283dd814b16d7",
    "e185417ae54abc296a3a6db5c004083b68c556c1f02339916419864d50b74d40",
    "aeca484c77356c895a0c275abfac499d5d7d2362f29c5e02e8710203010001a3",
    "663064301d0603551d0e04160414d40c101bf8cd63b9f73952b50e135ca6d799",
    "9386301f0603551d2304183016801429faf1accc4dd24c96402775b6b0e932e5",
    "07fe2e30120603551d130101ff040830060101ff020100300e0603551d0f0101",
    "ff040403020284300d06092a864886f70d01010b0500038181009e2d485f8c67",
    "33dc1a85ad99d75023ea14ec43b0e19deac223461e72b519dc6022e4a568316c",
    "0b55c4e69ca22d9f3a4f936b318b1678160d88cbd98bcc809d84f0c227e36b38",
    "f1fdd1e717723159357d96f3c57fab9d8f9661264fb2be81bb0d4904228ace9f",
    "f7f5422e2544fa2107125a83b555ad1882f840149b9c2063047f",
);

/// Attestation self-signed root certificate holding the key that signed [`RSA_ATTEST_CERT`].
///
/// Decoded contents:
///
/// ```
/// Certificate:
///     Data:
///         Version: 3 (0x2)
///         Serial Number: 18416584322103887884 (0xff94d9dd9f07c80c)
///     Signature Algorithm: SHA256-RSA
///         Issuer: C=US, O=Google, Inc., OU=Android, L=Mountain View, ST=California
///         Validity:
///             Not Before: 2016-01-04 12:31:08 +0000 UTC
///             Not After : 2035-12-30 12:31:08 +0000 UTC
///         Subject: C=US, O=Google, Inc., OU=Android, L=Mountain View, ST=California
///         Subject Public Key Info:
///             Public Key Algorithm: rsaEncryption
///                 Public Key: (1024 bit)
///                 Modulus:
///                     a2:6b:ad:eb:6e:2e:44:61:ef:d5:0e:82:e6:b7:94:
///                     d1:75:23:1f:77:9b:63:91:63:ff:f7:aa:ff:0b:72:
///                     47:4e:c0:2c:43:ec:33:7c:d7:ac:ed:40:3e:8c:28:
///                     a0:66:d5:f7:87:0b:33:97:de:0e:b8:4e:13:40:ab:
///                     af:a5:27:bf:95:69:a0:31:db:06:52:65:f8:44:59:
///                     57:61:f0:bb:f2:17:4b:b7:41:80:64:c0:28:0e:8f:
///                     52:77:8e:db:d2:47:b6:45:e9:19:c8:e9:8b:c3:db:
///                     c2:91:3f:d7:d7:50:c4:1d:35:66:f9:57:e4:97:96:
///                     0b:09:ac:ce:92:35:85:9b:
///                 Exponent: 65537 (0x10001)
///         X509v3 extensions:
///             X509v3 Authority Key Identifier:
///                 keyid:29faf1accc4dd24c96402775b6b0e932e507fe2e
///             X509v3 Subject Key Identifier:
///                 keyid:29faf1accc4dd24c96402775b6b0e932e507fe2e
///             X509v3 Key Usage: critical
///                 Digital Signature, Certificate Signing
///             X509v3 Basic Constraints: critical
///                 CA:true
///     Signature Algorithm: SHA256-RSA
///          4f:72:f3:36:59:8d:0e:c1:b9:74:5b:31:59:f6:f0:8d:25:49:
///          30:9e:a3:1c:1c:29:d2:45:2d:20:b9:4d:5f:64:b4:e8:80:c7:
///          78:7a:9c:39:de:a8:b3:f5:bf:2f:70:5f:47:10:5c:c5:e6:eb:
///          4d:06:99:61:d2:ae:9a:07:ff:f7:7c:b8:ab:eb:9c:0f:24:07:
///          5e:b1:7f:ba:79:71:fd:4d:5b:9e:df:14:a9:fe:df:ed:7c:c0:
///          88:5d:f8:dd:9b:64:32:56:d5:35:9a:e2:13:f9:8f:ce:c1:7c:
///          dc:ef:a4:aa:b2:55:c3:83:a9:2e:fb:5c:f6:62:f5:27:52:17:
///          be:63:
/// ```
const RSA_ATTEST_ROOT_CERT: &str = concat!(
    "308202a730820210a003020102020900ff94d9dd9f07c80c300d06092a864886",
    "f70d01010b05003063310b30090603550406130255533113301106035504080c",
    "0a43616c69666f726e69613116301406035504070c0d4d6f756e7461696e2056",
    "69657731153013060355040a0c0c476f6f676c652c20496e632e3110300e0603",
    "55040b0c07416e64726f6964301e170d3136303130343132333130385a170d33",
    "35313233303132333130385a3063310b30090603550406130255533113301106",
    "035504080c0a43616c69666f726e69613116301406035504070c0d4d6f756e74",
    "61696e205669657731153013060355040a0c0c476f6f676c652c20496e632e31",
    "10300e060355040b0c07416e64726f696430819f300d06092a864886f70d0101",
    "01050003818d0030818902818100a26badeb6e2e4461efd50e82e6b794d17523",
    "1f779b639163fff7aaff0b72474ec02c43ec337cd7aced403e8c28a066d5f787",
    "0b3397de0eb84e1340abafa527bf9569a031db065265f844595761f0bbf2174b",
    "b7418064c0280e8f52778edbd247b645e919c8e98bc3dbc2913fd7d750c41d35",
    "66f957e497960b09acce9235859b0203010001a3633061301d0603551d0e0416",
    "041429faf1accc4dd24c96402775b6b0e932e507fe2e301f0603551d23041830",
    "16801429faf1accc4dd24c96402775b6b0e932e507fe2e300f0603551d130101",
    "ff040530030101ff300e0603551d0f0101ff040403020284300d06092a864886",
    "f70d01010b0500038181004f72f336598d0ec1b9745b3159f6f08d2549309ea3",
    "1c1c29d2452d20b94d5f64b4e880c7787a9c39dea8b3f5bf2f705f47105cc5e6",
    "eb4d069961d2ae9a07fff77cb8abeb9c0f24075eb17fba7971fd4d5b9edf14a9",
    "fedfed7cc0885df8dd9b643256d5359ae213f98fcec17cdcefa4aab255c383a9",
    "2efb5cf662f5275217be63",
);

/// EC attestation private key in `ECPrivateKey` format.
///
/// Decoded contents (using [der2ascii](https://github.com/google/der-ascii)):
///
/// ```
/// SEQUENCE {
///   INTEGER { 1 }
///   OCTET_STRING { `7c5f132f5c2824aff2287717a99559ac90d40e3daca757136ab5e7ab79e0669a` }
///   [0] {
///     # secp256r1
///     OBJECT_IDENTIFIER { 1.2.840.10045.3.1.7 }
///   }
///   [1] {
///     BIT_STRING { `00` `043e88029b9a9a4e57ff6ce540c67ec73bcf694c4d931e493ebfa67123482c00b156fa0473a6f00ee8702e8ed6a992a3c8b4a1aad6d325d53a31a0cecdd24962a6` }
///   }
/// }
/// ```
const EC_ATTEST_KEY: &str = concat!(
    "307702010104207c5f132f5c2824aff2287717a99559ac90d40e3daca757136a",
    "b5e7ab79e0669aa00a06082a8648ce3d030107a144034200043e88029b9a9a4e",
    "57ff6ce540c67ec73bcf694c4d931e493ebfa67123482c00b156fa0473a6f00e",
    "e8702e8ed6a992a3c8b4a1aad6d325d53a31a0cecdd24962a6",
);

/// Attestation certificate corresponding to [`EC_ATTEST_KEY`], signed by the key in
/// [`EC_ATTEST_ROOT_CERT`].
///
/// Decoded contents:
///
/// ```
/// Certificate:
///     Data:
///         Version: 3 (0x2)
///         Serial Number:
///             71:27:2d:e1:9b:3d:be:a9:10:b8:e3:4a:df:86:04:4e:61:12:23:47
///         Signature Algorithm: ecdsa-with-SHA256
///         Issuer: C=US, ST=California, L=Mountain View, O=Google, Inc., OU=Android, CN=Android Keystore Software Attestation Root
///         Validity
///             Not Before: Jan  9 23:10:04 2026 GMT
///             Not After : Dec 16 23:10:04 2125 GMT
///         Subject: C=US, ST=California, L=Mountain View, O=Google, Inc., OU=Android, CN=Android Keystore Software Attestation Intermediate
///         Subject Public Key Info:
///             Public Key Algorithm: id-ecPublicKey
///                 Public-Key: (256 bit)
///                 pub:
///                     04:3e:88:02:9b:9a:9a:4e:57:ff:6c:e5:40:c6:7e:
///                     c7:3b:cf:69:4c:4d:93:1e:49:3e:bf:a6:71:23:48:
///                     2c:00:b1:56:fa:04:73:a6:f0:0e:e8:70:2e:8e:d6:
///                     a9:92:a3:c8:b4:a1:aa:d6:d3:25:d5:3a:31:a0:ce:
///                     cd:d2:49:62:a6
///                 ASN1 OID: prime256v1
///                 NIST CURVE: P-256
///         X509v3 extensions:
///             X509v3 Subject Key Identifier:
///                 4A:51:29:21:12:C0:EB:34:C7:3A:94:CB:C9:A7:9B:8F:2E:34:3D:0C
///             X509v3 Authority Key Identifier:
///                 52:BA:C5:65:BE:DC:DD:D4:6B:F8:4F:55:C1:A7:92:BD:37:86:A9:AF
///             X509v3 Basic Constraints: critical
///                 CA:TRUE, pathlen:0
///             X509v3 Key Usage: critical
///                 Digital Signature, Certificate Sign
///     Signature Algorithm: ecdsa-with-SHA256
///     Signature Value:
///         30:44:02:20:34:5a:2e:d2:b1:0f:30:25:1d:64:e3:7f:9e:d0:
///         9a:ec:e1:c1:aa:a3:f7:17:d6:65:13:fe:91:00:11:8b:b7:ca:
///         02:20:54:8e:18:96:7d:46:52:33:1b:cc:c8:0d:2d:be:6c:72:
///         fc:52:e5:56:8b:c7:e0:a4:75:ff:b4:c3:5b:61:19:fc
/// ```
const EC_ATTEST_CERT: &str = concat!(
    "308202a33082024aa003020102021471272de19b3dbea910b8e34adf86044e61",
    "122347300a06082a8648ce3d040302308198310b300906035504061302555331",
    "13301106035504080c0a43616c69666f726e69613116301406035504070c0d4d",
    "6f756e7461696e205669657731153013060355040a0c0c476f6f676c652c2049",
    "6e632e3110300e060355040b0c07416e64726f69643133303106035504030c2a",
    "416e64726f6964204b657973746f726520536f66747761726520417474657374",
    "6174696f6e20526f6f743020170d3236303130393233313030345a180f323132",
    "35313231363233313030345a3081a0310b300906035504061302555331133011",
    "06035504080c0a43616c69666f726e69613116301406035504070c0d4d6f756e",
    "7461696e205669657731153013060355040a0c0c476f6f676c652c20496e632e",
    "3110300e060355040b0c07416e64726f6964313b303906035504030c32416e64",
    "726f6964204b657973746f726520536f66747761726520417474657374617469",
    "6f6e20496e7465726d6564696174653059301306072a8648ce3d020106082a86",
    "48ce3d030107034200043e88029b9a9a4e57ff6ce540c67ec73bcf694c4d931e",
    "493ebfa67123482c00b156fa0473a6f00ee8702e8ed6a992a3c8b4a1aad6d325",
    "d53a31a0cecdd24962a6a3663064301d0603551d0e041604144a51292112c0eb",
    "34c73a94cbc9a79b8f2e343d0c301f0603551d2304183016801452bac565bedc",
    "ddd46bf84f55c1a792bd3786a9af30120603551d130101ff040830060101ff02",
    "0100300e0603551d0f0101ff040403020284300a06082a8648ce3d0403020347",
    "0030440220345a2ed2b10f30251d64e37f9ed09aece1c1aaa3f717d66513fe91",
    "00118bb7ca0220548e18967d4652331bccc80d2dbe6c72fc52e5568bc7e0a475",
    "ffb4c35b6119fc",
);

/// Attestation self-signed root certificate holding the key that signed [`EC_ATTEST_CERT`].
///
/// Decoded contents:
///
/// ```
/// Certificate:
///     Data:
///         Version: 3 (0x2)
///         Serial Number:
///             19:2e:3a:11:4f:a0:d8:f6:58:be:a0:c6:d4:63:9e:cc:34:6e:e4:fb
///         Signature Algorithm: ecdsa-with-SHA256
///         Issuer: C=US, ST=California, L=Mountain View, O=Google, Inc., OU=Android, CN=Android Keystore Software Attestation Root
///         Validity
///             Not Before: Jan  9 23:07:55 2026 GMT
///             Not After : Dec 16 23:07:55 2125 GMT
///         Subject: C=US, ST=California, L=Mountain View, O=Google, Inc., OU=Android, CN=Android Keystore Software Attestation Root
///         Subject Public Key Info:
///             Public Key Algorithm: id-ecPublicKey
///                 Public-Key: (256 bit)
///                 pub:
///                     04:32:ab:e4:f6:0d:9c:57:84:8e:a3:d5:24:ae:50:
///                     87:e6:0e:82:4d:d9:60:ca:26:90:97:28:e9:0d:75:
///                     1f:f0:f8:d5:a3:db:e5:14:28:ed:ff:4c:2d:49:4d:
///                     8d:f9:f4:47:7e:fa:22:89:29:92:8a:fa:30:75:e6:
///                     fd:89:07:a3:e3
///                 ASN1 OID: prime256v1
///                 NIST CURVE: P-256
///         X509v3 extensions:
///             X509v3 Subject Key Identifier:
///                 52:BA:C5:65:BE:DC:DD:D4:6B:F8:4F:55:C1:A7:92:BD:37:86:A9:AF
///             X509v3 Authority Key Identifier:
///                 52:BA:C5:65:BE:DC:DD:D4:6B:F8:4F:55:C1:A7:92:BD:37:86:A9:AF
///             X509v3 Basic Constraints: critical
///                 CA:TRUE
///             X509v3 Key Usage: critical
///                 Digital Signature, Certificate Sign
///     Signature Algorithm: ecdsa-with-SHA256
///     Signature Value:
///         30:46:02:21:00:89:76:47:3f:5d:a0:24:dd:02:c9:25:d6:da:
///         a7:34:a8:e9:e3:7d:66:e8:73:45:6e:3b:05:c6:75:8d:b7:5a:
///         6a:02:21:00:85:8f:47:8e:cb:6d:08:4c:16:a4:5b:ed:62:f6:
///         21:56:52:39:57:36:fc:60:72:8d:d2:56:4b:90:eb:2b:55:0b
/// ```
const EC_ATTEST_ROOT_CERT: &str = concat!(
    "3082029a3082023fa0030201020214192e3a114fa0d8f658bea0c6d4639ecc34",
    "6ee4fb300a06082a8648ce3d040302308198310b300906035504061302555331",
    "13301106035504080c0a43616c69666f726e69613116301406035504070c0d4d",
    "6f756e7461696e205669657731153013060355040a0c0c476f6f676c652c2049",
    "6e632e3110300e060355040b0c07416e64726f69643133303106035504030c2a",
    "416e64726f6964204b657973746f726520536f66747761726520417474657374",
    "6174696f6e20526f6f743020170d3236303130393233303735355a180f323132",
    "35313231363233303735355a308198310b300906035504061302555331133011",
    "06035504080c0a43616c69666f726e69613116301406035504070c0d4d6f756e",
    "7461696e205669657731153013060355040a0c0c476f6f676c652c20496e632e",
    "3110300e060355040b0c07416e64726f69643133303106035504030c2a416e64",
    "726f6964204b657973746f726520536f66747761726520417474657374617469",
    "6f6e20526f6f743059301306072a8648ce3d020106082a8648ce3d0301070342",
    "000432abe4f60d9c57848ea3d524ae5087e60e824dd960ca26909728e90d751f",
    "f0f8d5a3dbe51428edff4c2d494d8df9f4477efa228929928afa3075e6fd8907",
    "a3e3a3633061301d0603551d0e0416041452bac565bedcddd46bf84f55c1a792",
    "bd3786a9af301f0603551d2304183016801452bac565bedcddd46bf84f55c1a7",
    "92bd3786a9af300f0603551d130101ff040530030101ff300e0603551d0f0101",
    "ff040403020284300a06082a8648ce3d04030203490030460221008976473f5d",
    "a024dd02c925d6daa734a8e9e37d66e873456e3b05c6758db75a6a022100858f",
    "478ecb6d084c16a45bed62f6215652395736fc60728dd2564b90eb2b550b",
);

/// Per-algorithm attestation certificate signing information.
pub struct CertSignAlgoInfo {
    key: KeyMaterial,
    chain: Vec<keymint::Certificate>,
}

/// Certificate signing information for all asymmetric key types.
pub struct CertSignInfo {
    rsa_info: CertSignAlgoInfo,
    ec_info: CertSignAlgoInfo,
}

impl CertSignInfo {
    /// Create a new cert signing impl.
    #[allow(clippy::new_without_default)]
    pub fn new() -> Self {
        CertSignInfo {
            rsa_info: CertSignAlgoInfo {
                key: KeyMaterial::Rsa(rsa::Key(hex::decode(RSA_ATTEST_KEY).unwrap()).into()),
                chain: vec![
                    keymint::Certificate {
                        encoded_certificate: hex::decode(RSA_ATTEST_CERT).unwrap(),
                    },
                    keymint::Certificate {
                        encoded_certificate: hex::decode(RSA_ATTEST_ROOT_CERT).unwrap(),
                    },
                ],
            },
            ec_info: CertSignAlgoInfo {
                key: KeyMaterial::Ec(
                    EcCurve::P256,
                    CurveType::Nist,
                    ec::Key::P256(ec::NistKey(hex::decode(EC_ATTEST_KEY).unwrap())).into(),
                ),
                chain: vec![
                    keymint::Certificate {
                        encoded_certificate: hex::decode(EC_ATTEST_CERT).unwrap(),
                    },
                    keymint::Certificate {
                        encoded_certificate: hex::decode(EC_ATTEST_ROOT_CERT).unwrap(),
                    },
                ],
            },
        }
    }
}

impl RetrieveCertSigningInfo for CertSignInfo {
    fn signing_key(&self, key_type: SigningKeyType) -> Result<KeyMaterial, Error> {
        Ok(match key_type.algo_hint {
            SigningAlgorithm::Rsa => self.rsa_info.key.clone(),
            SigningAlgorithm::Ec => self.ec_info.key.clone(),
        })
    }

    fn cert_chain(&self, key_type: SigningKeyType) -> Result<Vec<keymint::Certificate>, Error> {
        Ok(match key_type.algo_hint {
            SigningAlgorithm::Rsa => self.rsa_info.chain.clone(),
            SigningAlgorithm::Ec => self.ec_info.chain.clone(),
        })
    }
}
