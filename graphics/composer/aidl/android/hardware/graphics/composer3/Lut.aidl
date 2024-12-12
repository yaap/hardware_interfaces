/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

import android.hardware.graphics.composer3.LutProperties;

/**
 * LUT (Look-Up Table) Interface for Color Transformation.
 *
 * This interface allows the HWC (Hardware Composer) to define and communicate LUTs
 * with SurfaceFlinger.
 */

@VintfStability
parcelable Lut {
    /**
     * A handle to a memory region.
     * If the file descriptor is not set, this means that the HWC doesn't specify a Lut.
     *
     * When specifying a Lut, the HWC is required to follow the instructions as below:
     * 1. use `memfd_create` to create a shared memory segment
     *    with the size specified in lutProperties.
     * 2. use `mmap` to map the shared memory segment into its own virtual address space.
     *    PROT_READ/PROT_WRITE recommended for prot argument.
     *
     * For data precision, 32-bit float is used to specify a Lut by both the HWC and
     * the platform.
     *
     * For unflattening/flattening 3D Lut(s), the algorithm below should be observed
     * by both the HWC and the platform.
     * Assuming that we have a 3D array `ORIGINAL[WIDTH, HEIGHT, DEPTH]`, we would turn it into
     * `FLAT[WIDTH * HEIGHT * DEPTH]` by
     *
     * `FLAT[z + DEPTH * (y + HEIGHT * x)] = ORIGINAL[x, y, z]`
     */
    @nullable ParcelFileDescriptor pfd;

    /**
     * The properties of the Lut.
     */
    LutProperties lutProperties;
}
