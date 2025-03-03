/*
 * Copyright (C) 2024 The Android Open Source Project
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

//! VTS test library for HwCrypto functionality.
//! It provides the base clases necessaries to write HwCrypto VTS tests

use anyhow::{Context, Result};
use binder::{ExceptionCode, FromIBinder, IntoBinderResult, ParcelFileDescriptor};
use rpcbinder::RpcSession;
use vsock::VsockStream;
use std::os::fd::{FromRawFd, IntoRawFd};
use std::fs::File;
use std::io::Read;
use rustutils::system_properties;
use android_hardware_security_see_hwcrypto::aidl::android::hardware::security::see::hwcrypto::IHwCryptoKey::IHwCryptoKey;

const HWCRYPTO_SERVICE_PORT: u32 = 4;

/// Local function to connect to service
pub fn connect_service<T: FromIBinder + ?Sized>(
    cid: u32,
    port: u32,
) -> Result<binder::Strong<T>, binder::StatusCode> {
    RpcSession::new().setup_preconnected_client(|| {
        let mut stream = VsockStream::connect_with_cid_port(cid, port).ok()?;
        let mut buffer = [0];
        let _ = stream.read(&mut buffer);
        // SAFETY: ownership is transferred from stream to f
        let f = unsafe { File::from_raw_fd(stream.into_raw_fd()) };
        Some(ParcelFileDescriptor::new(f).into_raw_fd())
    })
}

/// Get a HwCryptoKey binder service object
pub fn get_hwcryptokey() -> Result<binder::Strong<dyn IHwCryptoKey>, binder::Status> {
    let cid = system_properties::read("trusty.test_vm.vm_cid")
        .context("couldn't get vm cid")
        .or_binder_exception(ExceptionCode::ILLEGAL_STATE)?
        .ok_or(ExceptionCode::ILLEGAL_STATE)?
        .parse::<u32>()
        .context("couldn't parse vm cid")
        .or_binder_exception(ExceptionCode::ILLEGAL_ARGUMENT)?;
    Ok(connect_service(cid, HWCRYPTO_SERVICE_PORT)?)
}
