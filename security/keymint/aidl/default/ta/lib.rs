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

//! Local in-process implementation of the KeyMint TA. This is insecure and should
//! only be used for testing purposes.

// This crate is `std` using, but some of the code uses macros from a `no_std` world.
extern crate alloc;

use kmr_common::crypto;
use kmr_crypto_boring::rng::BoringRng;
use kmr_ta::device::{
    BootloaderDone, CsrSigningAlgorithm, FrpDataStorage, FrpSecretStorage, Implementation,
    TrustedPresenceUnsupported,
};
use kmr_ta::{HardwareInfo, KeyMintTa, RpcInfo, RpcInfoV3};
use kmr_wire::keymint::SecurityLevel;
use kmr_wire::rpc::MINIMUM_SUPPORTED_KEYS_IN_CSR;
use log::info;

pub mod attest;
pub mod clock;
pub mod frp;
pub mod rpc;
pub mod soft;

/// Build a set of crypto trait implementations based around BoringSSL and the standard library
/// clock.
pub fn boringssl_crypto_impls() -> crypto::Implementation {
    let rng = BoringRng;
    let clock = clock::StdClock::new();
    kmr_crypto_boring::implementation(Box::new(rng), Box::new(clock))
}

/// Build a [`kmr_ta::KeyMintTa`] instance for nonsecure use.
pub fn build_ta() -> kmr_ta::KeyMintTa {
    let rpc_sign_algo = CsrSigningAlgorithm::EdDSA;
    build_ta_with(
        Box::new(soft::RpcArtifacts::new(soft::Derive::default(), rpc_sign_algo)),
        Some(Box::new(frp::InMemorySecretStorage::new())),
        Some(Box::new(frp::InMemoryDataStorage::new())),
    )
}

/// Build a [`kmr_ta::KeyMintTa`] instance for nonsecure use, including some specified trait
/// implementations.
pub fn build_ta_with(
    rpc: Box<dyn kmr_ta::device::RetrieveRpcArtifacts>,
    frp_secret_storage: Option<Box<dyn FrpSecretStorage>>,
    frp_data_storage: Option<Box<dyn FrpDataStorage>>,
) -> kmr_ta::KeyMintTa {
    info!("Building NON-SECURE KeyMint Rust TA");
    let hw_info = HardwareInfo {
        version_number: 1,
        security_level: SecurityLevel::TrustedEnvironment,
        impl_name: "Rust reference implementation",
        author_name: "Google",
        unique_id: "NON-SECURE KeyMint TA",
    };
    let rpc_info_v3 = RpcInfoV3 {
        author_name: "Google",
        unique_id: "NON-SECURE KeyMint TA",
        fused: false,
        supported_num_of_keys_in_csr: MINIMUM_SUPPORTED_KEYS_IN_CSR,
    };

    let sign_info = attest::CertSignInfo::new();
    let keys: Box<dyn kmr_ta::device::RetrieveKeyMaterial> = Box::new(soft::Keys);
    let dev = Implementation {
        keys,
        sign_info: Some(Box::new(sign_info)),
        // HAL populates attestation IDs from properties.
        attest_ids: None,
        sdd_mgr: None,
        // `BOOTLOADER_ONLY` keys not supported.
        bootloader: Box::new(BootloaderDone),
        // `STORAGE_KEY` keys not supported.
        sk_wrapper: None,
        // `TRUSTED_USER_PRESENCE_REQUIRED` keys not supported
        tup: Box::new(TrustedPresenceUnsupported),
        // No support for converting previous implementation's keyblobs.
        legacy_key: None,
        rpc,
        frp_secret_storage,
        frp_data_storage,
    };
    KeyMintTa::new(hw_info, RpcInfo::V3(rpc_info_v3), boringssl_crypto_impls(), dev)
}
