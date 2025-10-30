# AuthMgr

The AuthMgr protocol authenticates and authorizes clients before they can
access trusted HALs, AIDL-defined services in trusted execution environments.
Version 1 was designed to allow applications running in a protected virtual
machine (pVM) to access services running in a TEE in ARM TrustZone. An
implementation of `IAuthMgrAuthorization` is referred to as an AuthMgr Backend.
An implementation of a client of the AuthMgr Backend is referred to as an
AuthMgr Frontend.


## Additional Requirements by Android Version

The comments on `IAuthMgrAuthorization` describe the requirements for implementing
an AuthMgr Backend (implementor of the interface) itself. There are some additional
requirements that are specific to Android release versions.

### Android 16
- If implementing `IAuthMgrAuthorization` in Android 16 only one AuthMgr Backend is
supported and dynamic service discovery is not supported. The AuthMgr Backend
service must be exposed on secure partition ID 0x8001 over VSOCK port 1.

- AuthMgr Front Ends must implement the "android.16" profile as described in the
[Android Profile for DICE](https://pigweed.googlesource.com/open-dice/+/HEAD/docs/android.md#versions)

## Using the instance-id to encode information about the VM instance

Originally, the instance-id was intended to play the following two important roles in the authmgr protocol.

1. detect and reject duplicated authentication attempts (from the same vm or an impersonating VM)
2. construct the file path to store the instance context (such as DICE policy and other meta data related to the VM), so that the instance context can be looked up via the instance-id in the subsequent authentication attempts.

To serve the above purposes, it was sufficient for the instance-id to be an opaque identifer. Therefore, as mentioned in the documentation of IAuthMgrAuthorization.aidl, instance-id could be either included in the DICE certificate chain or passed into the
`initAuthentication` method as an optional parameter, if instance-id is not included in the DICE certificate chain. DICE certificate chain includes the SHA256 hash of the instance-id according to the [DICE specification for guest vm](https://android.googlesource.com/platform/packages/modules/Virtualization/+/main/dice_for_avf_guest.cddl#33).

Given that instance-id is a vector of bytes, we wanted it to play a third role - which is: encoding and conveying certain useful information about the VM instance. This third role enforces the following requirements on the instance-id:

1. It needs to have a properly defined schema (see below) describing what and how the information about the VM instance is encoded, instead of being an opaque id.
2. It needs to be passed in via the `instanceIdentifier` parameter of the `initAuthentication` method, because if it is included in the DICE certificate chain, the instance-id is hashed and the semantics will be lost.

### Schema for encoding information about the VM instance in the instance-id:

- Size = 64 bytes
- 0 - 15 bytes = AVF (Android Virtualization Framework) reserved space

    - The most significant bit (MSB) of byte 0 indicates whether the VM is persistent. AuthMgr-BE identifies a persistent VM by checking whether the most significant bit of byte 0 is 1.

    - The remaining 7 bits of byte 0, along with all of bytes 1, 2, and 3, specify the partition where the VM resides.

        - persistent, system VM: 0x80 0x00 0x00 0x00
        - non-persistent, system VM: 0x77 0x77 0x77 0x77
        - persistent, vendor VM: 0xff 0xff 0xff 0xff
        - non-persistent, vendor VM: not yet supported

    - All system and vendor VMs should set 4-15th bytes to zero to facilitate future usage.

- 16 - 31 bytes: primary UUID for a VM

    - A constant 16 byte UUID that identifies the VM type, for e.g.

        - Security VM: 6c07e3dc-79e1-45a3-936d-b5d73a978f41
        - Test VM: 07590497-b72d-4434-ab36-51442e130536

- 32 - 63 bytes:

    - For Security VM, this space is filled with zeros
    - For Test VMs, the first 16 bytes can be used to include a secondary UUID to identify a specific test VM instance.

        For e.g. by default in Trusty, all test VMs are deleted upon reboot of AuthMgr-BE. If a particular test VM needs to survive a reboot, the secondary UUID should be known to AuthMgr-BE for it to be excluded from being deleted upon reboot.