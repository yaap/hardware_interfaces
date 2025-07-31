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

//! VTS tests for IFactoryResetProtection

// Separate this use from the rest because rustfmt refuses to format it (too long?) and grouping
// it with the other use statements causes rustfmt to refuse to format them, too.
use android_hardware_security_factory_reset_protection::aidl::android::hardware::security::factory_reset_protection::IFactoryResetProtection::{
    IFactoryResetProtection, STATUS_FRP_IS_ACTIVE
};

use binder::{check_interface, is_declared, StatusCode, Strong};
use rdroidtest::{ignore_if, rdroidtest};
use std::sync::{Mutex, MutexGuard};

/// `GLOBAL_LOCK` is acquired at the beginning of each test and released at the end.  Its purpose
/// is to ensure the tests are serialized even if the testrunner tries to run them concurrently.
static GLOBAL_LOCK: Mutex<()> = Mutex::new(());

const FRP_SERVICE: &str =
    "android.hardware.security.factory_reset_protection.IFactoryResetProtection/default";

/// Because it's important to restore the FRP secret and deactivate FRP before exiting the tests,
/// the tests don't use the FRP HAL interface directly, but instead go through this wrapper, which
/// also implements [`Drop`], which attempts to restore the state.
struct FrpWrapper {
    frp: Strong<dyn IFactoryResetProtection>,

    /// Interior-mutable state for the wrapper.  We don't need the Mutex for synchronization,
    /// since everything runs single-threaded, but it makes Rust happy.
    state: Mutex<FrpWrapperState>,
}

struct FrpWrapperState {
    old_secret: [u8; 32],
    new_secret: Option<[u8; 32]>,
    is_active: bool,
}

impl FrpWrapper {
    /// Create a new FrpWrapper.  Also returns a [`MutexGuard`] referencing [`GLOBAL_LOCK`], which
    /// the calling test should hold until the end of the test, to ensure tests are serialized.
    fn new() -> (Self, MutexGuard<'static, ()>) {
        let frp: std::result::Result<Strong<dyn IFactoryResetProtection>, StatusCode> =
            check_interface(FRP_SERVICE);
        let frp = frp.expect("Failed to get IFactoryResetProtection interface");

        // TODO: Remove this.
        //
        // During normal operation, by the time we get here FRP should be inactive because
        // PDBService should have deactivated it.  But because PDBService integration isn't
        // complete, PDBService doesn't do that yet, so when we start up FRP is active if no tests
        // have been run.  If so, deactivate with default.
        if frp.isActive().expect("Failed to check if IFactoryResetProtection is active") {
            frp.deactivate(&[0; 32]).expect("Failed to deactivate IFactoryResetProtection");
        }

        let old_secret = frp.getSecret().unwrap();
        let is_active = frp.isActive().unwrap();
        let state = Mutex::new(FrpWrapperState { old_secret, new_secret: None, is_active });

        (Self { frp, state }, GLOBAL_LOCK.lock().unwrap())

    }

    fn is_active(&self) -> bool {
        self.state.lock().unwrap().is_active
    }

    fn old_secret(&self) -> [u8; 32] {
        self.state.lock().unwrap().old_secret
    }

    fn new_secret(&self) -> Option<[u8; 32]> {
        self.state.lock().unwrap().new_secret.clone()
    }

    /// Attempt to deactivate FRP
    fn try_deactivate(&self) {
        if !self.is_active() {
            return;
        }

        // Try new secret, if any.
        if let Some(secret) = self.new_secret() {
            if self.frp.deactivate(&secret).unwrap() {
                return;
            }
        }

        // Try old secret.
        if self.frp.deactivate(&self.old_secret()).unwrap() {
            return;
        }

        // Try default secret.
        assert!(self.frp.deactivate(&[0; 32]).unwrap())
    }
}

impl binder::Interface for FrpWrapper {}

impl IFactoryResetProtection for FrpWrapper {
    fn isActive(&self) -> binder::Result<bool> {
        self.frp.isActive()
    }

    fn activate(&self) -> binder::Result<()> {
        self.state.lock().unwrap().is_active = true;
        self.frp.activate()
    }

    fn deactivate(&self, secret: &[u8; 32]) -> binder::Result<bool> {
        match self.frp.deactivate(secret) {
            Ok(true) => {
                self.state.lock().unwrap().is_active = false;
                Ok(true)
            }
            other => other,
        }
    }

    fn setSecret(&self, secret: &[u8; 32]) -> binder::Result<()> {
        match self.frp.setSecret(secret) {
            Ok(()) => {
                self.state.lock().unwrap().new_secret = Some(*secret);
                Ok(())
            }
            other => other,
        }
    }

    fn getSecret(&self) -> binder::Result<[u8; 32]> {
        self.frp.getSecret()
    }

    fn storeData(&self, key: &str, data: &[u8]) -> binder::Result<()> {
        self.frp.storeData(key, data)
    }

    fn retrieveData(&self, key: &str) -> binder::Result<Vec<u8>> {
        self.frp.retrieveData(key)
    }

    fn deleteData(&self, key: &str) -> binder::Result<()> {
        self.frp.deleteData(key)
    }

    fn deleteAllData(&self) -> binder::Result<()> {
        self.frp.deleteAllData()
    }
}

impl Drop for FrpWrapper {
    fn drop(&mut self) {
        self.try_deactivate();
        if self.new_secret().is_some() {
            self.frp.setSecret(&self.old_secret()).unwrap();
        }
    }
}

/// Return true if the FRP service isn't declared.  This is used to ignore the FRP tests on devices
/// without the FRP HAL.
fn frp_service_not_declared() -> bool {
    !is_declared(FRP_SERVICE).expect("Failed to check for FRP service declaration.")
}

/// FRP should always be inactive when the tests start.
#[rdroidtest]
#[ignore_if(frp_service_not_declared())]
fn frp_is_inactive() {
    let (frp, _lock) = FrpWrapper::new();
    assert!(!frp.isActive().unwrap(), "FRP should not be active");
}

/// Test that deactivation works with the correct secret and doesn't work with the wrong secret.
#[rdroidtest]
#[ignore_if(frp_service_not_declared())]
fn deactivate_with_secret() {
    let (frp, _lock) = FrpWrapper::new();
    assert!(!frp.isActive().unwrap(), "FRP should not be active");

    let new_secret =
        frp.old_secret().iter().map(|b| b ^ 0xff).collect::<Vec<_>>().try_into().unwrap();
    frp.setSecret(&new_secret).unwrap();

    // Activate
    frp.activate().unwrap();
    assert!(frp.isActive().unwrap(), "FRP should be active after activate");

    // Attempt to deactivate with old secret, should fail.
    assert!(!frp.deactivate(&frp.old_secret()).unwrap(), "Deactivation should have failed");
    assert!(frp.isActive().unwrap(), "FRP should be active after incorrect deactivate");

    // Attempt to deactivate with new secret, should succeed.
    assert!(frp.deactivate(&new_secret).unwrap());
    assert!(!frp.isActive().unwrap(), "FRP should not be active after correct deactivate");
}

/// Verify that the secret cannot be read while FRP is active.
#[rdroidtest]
#[ignore_if(frp_service_not_declared())]
fn get_secret_while_active() {
    let (frp, _lock) = FrpWrapper::new();
    assert!(!frp.isActive().unwrap(), "FRP should not be active");

    frp.activate().unwrap();
    assert!(frp.isActive().unwrap(), "FRP should be active");

    match frp.getSecret() {
        Err(s) if s.service_specific_error() == STATUS_FRP_IS_ACTIVE => { /* success */ }
        Err(e) => panic!("Wrong error {:?}", e),
        Ok(secret) => panic!("Should have failed to get secret, got {:?}", secret),
    }
}

/// Verify that the secret cannot be changed while FRP is active.
#[rdroidtest]
#[ignore_if(frp_service_not_declared())]
fn update_secret_while_active() {
    let (frp, _lock) = FrpWrapper::new();
    assert!(!frp.isActive().unwrap(), "FRP should not be active");

    frp.activate().unwrap();
    assert!(frp.isActive().unwrap(), "FRP should be active");

    // Attempt to set secret.
    match frp.setSecret(&[1; 32]) {
        Err(s) if s.service_specific_error() == STATUS_FRP_IS_ACTIVE => { /* success */ }
        Err(e) => panic!("Wrong error: {:?}", e),
        Ok(()) => panic!("Should have failed to set secret"),
    }
}

rdroidtest::test_main!();
