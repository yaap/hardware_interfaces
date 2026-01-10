# NPU HAL

## Overview

This directory contains the interfaces and tests for the NPU HAL, which currently
provides the ability for Android to inform the NPU of application priorities and
receive status callbacks when work is occurring on the NPU.

## xTS

In order to test the HAL interfaces, the xTS (VTS, CTS, etc) tests need a way to
actually perform an inference on the NPU. This ability is typically provided via
vendor SDK, which is problematic for xTS. The solution is to have vendors provide a
command-line tool to perform a test inference which can be invoked by the xTS
tests.

The command-line tool, called `run-test-inference`, should be placed in
`/data/local/tmp` before running the VTS tests. When invoked, it must perform a
workload on the NPU. Ideally this is a ML/AI task, but this is not required.
Every invocation should execute the same workload. There is no minimum or maximum
duration specified, but it should be long enough that measurements are not
cumbersome, e.g. at least several hundred milliseconds.

Currently the tool accepts one option, `--job-priority`, which must be used to set
whatever job-based priority the vendor supports. The HAL definition of this
priority is an integer ranging from 0-1000, with 0 being the most-important value.
Vendors must convert this into whatever prioritization system they use when running
the test inference.

A scaffold for an example tool may be found under this directory in
`aidl/vts/test_inference_cli`. There is also an implementation for the Cuttlefish
HAL in `/device/google/cuttlefish/guest/hals/npu`.
