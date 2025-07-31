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

//! In-memory implementation of storage components for FRP.

use kmr_common::Error;
use kmr_ta::device::{FrpDataStorage, FrpSecretStorage};
use std::collections::HashMap;

/// A trivial memory-only "storage" implementation for FRP `SecretStorage`.
///
/// A proper implementation needs to persist the secret to secure storage that survives factory
/// reset, but this is sufficient for VTS.
pub struct InMemorySecretStorage {
    secret: [u8; 32],
}

impl InMemorySecretStorage {
    /// Create a new instance.
    pub fn new() -> Self {
        Self { secret: [0; 32] }
    }
}

impl FrpSecretStorage for InMemorySecretStorage {
    fn store_secret(&mut self, secret: [u8; 32]) -> Result<(), Error> {
        self.secret = secret;
        Ok(())
    }

    fn retrieve_secret(&self) -> Result<[u8; 32], Error> {
        Ok(self.secret)
    }
}

/// A trivial memory-only "storage" implementation for FRP `DataStorage`.
///
/// A proper implementation needs to persist the secret to storage that survives factory reset,
/// but this is sufficient for VTS.
pub struct InMemoryDataStorage {
    data: HashMap<String, Vec<u8>>
}

impl InMemoryDataStorage {
    /// Create a new instance.
    pub fn new() -> Self {
        Self { data: HashMap::new() }
    }
}

impl FrpDataStorage for InMemoryDataStorage {
    fn store_data(&mut self, key: &str, data: &[u8]) -> Result<(), Error> {
        self.data.insert(key.to_owned(), data.to_vec());
        Ok(())
    }

    fn retrieve_data(&self, key: &str) -> Result<Option<Vec<u8>>, Error> {
        Ok(self.data.get(key).cloned())
    }

    fn delete_data(&mut self, key: &str) -> Result<(), Error> {
        self.data.remove(key);
        Ok(())
    }

    fn clear(&mut self) -> Result<(),Error> {
        self.data.clear();
        Ok(())
    }
}
