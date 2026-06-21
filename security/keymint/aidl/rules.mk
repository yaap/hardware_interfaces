# Copyright (C) 2025 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_AIDL_STABLE := true

MODULE_AIDL_VERSION := 4

AIDL_DIR = $(LOCAL_DIR)/aidl_api/android.hardware.security.keymint/$(MODULE_AIDL_VERSION)
SECURECLOCK_AIDL_DIR := hardware/interfaces/security/secureclock/aidl

MODULE_CRATE_NAME := android_hardware_security_keymint

MODULE_AIDL_LANGUAGE := rust

MODULE_AIDL_PACKAGE := android/hardware/security/keymint

MODULE_AIDL_INCLUDES := \
	-I $(AIDL_DIR) \
	-I $(SECURECLOCK_AIDL_DIR) \

MODULE_AIDL_FLAGS := \
	--stability=vintf \

MODULE_AIDLS := \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/Algorithm.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/AttestationKey.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/BeginResult.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/BlockMode.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/Certificate.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/Digest.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/EcCurve.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/ErrorCode.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/HardwareAuthenticatorType.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/HardwareAuthToken.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/IKeyMintDevice.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/IKeyMintOperation.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyCharacteristics.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyCreationResult.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyFormat.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyMintHardwareInfo.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyOrigin.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyParameter.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyParameterValue.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/KeyPurpose.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/PaddingMode.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/SecurityLevel.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/Tag.aidl     \
    $(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/TagType.aidl     \

MODULE_AIDL_RUST_DEPS := \
	android_hardware_security_secureclock

MODULE_LIBRARY_DEPS := \
	hardware/interfaces/security/secureclock/aidl \

include make/aidl.mk
