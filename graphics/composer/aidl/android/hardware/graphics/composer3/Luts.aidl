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
parcelable Luts {
    /**
     * A handle to a memory region.
     * If the file descriptor is not set, this means that the HWC doesn't specify a Lut.
     *
     * When specifying a Lut, the HWC is required to follow the instructions as below:
     * 1. use `ashmem_create_region` to create a shared memory segment
     *    with the size specified in lutProperties.
     * 2. use `mmap` to map the shared memory segment into its own virtual address space.
     *    PROT_READ/PROT_WRITE recommended for prot argument.
     *
     * For data precision, 32-bit float is used to specify a Lut by both the HWC and
     * the platform.
     *
     * For 1D LUTs:
     * -   Values should also be normalized for fixed-point pixel formats.
     * -   Floating-point pixel formats and extended-range buffers are currently unsupported.
     *
     * For 3D LUT buffers:
     * -   Values must be normalized to the range [0.0, 1.0], inclusive. 1.0 is the maximum panel luminance.
     * -   If N is the dimension, the data is organized in RGB order: R0, R1, ..., RN, G0, G1, ..., GN, B0, B1, ..., BN.
     */
    @nullable ParcelFileDescriptor pfd;

    /**
     * The offsets store the starting point of each Lut memory of the Lut buffer.
     *
     * Multiple Luts can be packed into one same `pfd`, and `offsets` is used to pinpoint
     * the starting point of each Lut.
     *
     * `offsets` should be valid unless an invalid `pfd` is provided.
     */
    @nullable int[] offsets;

    /**
     * The properties list of the Luts.
     *
     * The number of sampling key inside should only be one.
     */
    LutProperties[] lutProperties;
}
