// Copyright 2025, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Small utility to wait for hwcrypto service to be up

use anyhow::{/*Context,*/ Result};
use clap::Parser;
use log::info;
use std::{thread, time};

#[derive(Parser)]
/// Collection of CLI for trusty_security_vm_launcher
pub struct Args {
    /// Number of repetitions for the wait
    #[arg(long, default_value_t = 20)]
    number_repetitions: u32,

    /// Delay between repetitiond
    #[arg(long, default_value_t = 2)]
    delay_between_repetitions: u32,
}

fn main() -> Result<()> {
    let args = Args::parse();

    info!("Waiting for hwcrypto service");
    let delay = time::Duration::new(args.delay_between_repetitions.into(), 0);
    for _ in 0..args.number_repetitions {
        let hw_crypto_key = hwcryptohal_vts_test::get_hwcryptokey();
        if hw_crypto_key.is_ok() {
            break;
        }
        thread::sleep(delay);
    }
    Ok(())
}
