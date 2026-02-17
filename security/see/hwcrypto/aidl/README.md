# HWCrypto AIDL Interfaces

This repository contains the AIDL interfaces for the HwCrypto HAL secure
service. HwCrypto provides an interface to a secure environment for performing
cryptographic operations and managing sensitive keys.

## Overview

HwCrypto supports device-bound key derivation, secure key sharing between
services, and batch processing of cryptographic commands.

## Key Features

-   **DICE-Bound Key Derivation**: Derive keys that are cryptographically bound
    to the device identity and the caller's software version (DICE policy).
-   **Command List Interface**: Execute a sequence of cryptographic operations
    (encryption, decryption, hashing, etc.) in a single IPC call for improved
    performance and reduced overhead.
-   **Opaque Keys**: Sensitive key material remains within the secure
    environment. Clients interact with keys through opaque handles
    (`IOpaqueKey`).
-   **Key Delegation and Sharing**: Securely share keys between different
    clients using shareable tokens protected by DICE policies.
-   **Memory Buffer Protection**: Associate keys with specific `ProtectionID`s
    to restrict their use to authorized memory regions (e.g., trusted video
    buffers).

## Entry Point

The primary entry point for the service is the
[IHwCryptoKey.aidl](android/hardware/security/see/hwcrypto/IHwCryptoKey.aidl)
interface. It provides methods for: - Deriving DICE-policy bound keys. -
Importing cleartext keys into the secure environment. - Retrieving keys from
pre-defined hardware slots. - Creating an `IHwCryptoOperations` factory for
batch processing.

## Documentation

For detailed information on specific components, refer to the following
documents:

-   [**HwCrypto DICE bound keys**](docs/hwcrypto_dice_bound_keys.md): Detailed
    explanation of how device keys are derived and bound to DICE policies.
-   [**Command List Interface**](docs/command_list_interface.md): Documentation
    on the command-based API for performing cryptographic operations.

## VTS Tests

The functionality of the HwCrypto HAL is verified through VTS (Vendor Test
Suite) tests located in the [vts](vts) directory. These tests cover both key
management and cryptographic operations.
