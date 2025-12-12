# Trusted HALs

This directory contains the AIDL interface definitions for services implemented
in a TEE and made available to Android protected VMs.

# VTS requirements

Trusted HALs are VINTF stable services and are declared in the device VINTF
manifest as such. Since they are implemented in environments outside of the
Android OS, the VTS tests for ensuring their compliance with their associated
manifest declarations have an extra requirement.

All top-level Trusted HAL services must add `ITrustedHalExt` (see `ext/`) as an
extension to their root interface. Note that this extension is not VINTF stable
and that is on purpose. This allows VTS tests to test that the binder library
being used to expose trusted HALs was built with the correct default *vendor*
stability guarantees.

Note that this requirement does not prohibit HAL implementers from adding their
own extensions to their service implementations if they wish. Only the extension
on the root Binder is reserved. Those who wish to add their own extensions can
do so by adding them as further extensions to the required `ITrustedHalExt`
extension.
