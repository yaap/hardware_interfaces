
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

AIDL_DIR := $(LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_AIDL_STABLE := false

MODULE_AIDL_VERSION := 1

# TODO(b/462054024): The build system does not attach the version flag
# for unfrozen AIDLs, but VTS requires a valid version.
MODULE_AIDL_FLAGS := \
    --stability=vintf \
    --version $(MODULE_AIDL_VERSION)

MODULE_AIDL_LANGUAGE := rust

MODULE_CRATE_NAME := android_hardware_security_see_devicestate

MODULE_AIDL_PACKAGE := android/hardware/security/see/devicestate

MODULE_AIDLS := \
	$(AIDL_DIR)/$(MODULE_AIDL_PACKAGE)/IDeviceState.aidl \

include make/aidl.mk
