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

package android.hardware.usb;

@VintfStability
@Backing(type="int")
/**
 * Indicates the type of charger as defined by the Battery Charging Specification,
 * Revision 1.2 (BC 1.2)
 */
enum Bc12Type {
    /* Unknown BC 1.2 type */
    UNKNOWN = 0,
    /**
     * SDP (Standard Downstream Port) as defined by BC 1.2
     */
    SDP = 1,
    /**
     * CDP (Charging Downstream Port) as defined by BC 1.2
     */
    CDP = 2,
    /**
     * DCP (Dedicated Charging Port) as defined by BC 1.2
     */
    DCP = 3,
}
