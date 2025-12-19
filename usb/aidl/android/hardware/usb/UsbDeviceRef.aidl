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
/**
 * A reference to a downstream USB device. This is used to link a downstream
 * USB device to its parent port. The rest of the device information can be
 * fetched from the {@code UsbService}.
 */
parcelable UsbDeviceRef {
    /**
     * The sysfs path of the USB device, which uniquely identifies it.
     * This path can be used to read more data from the file system (such as
     * idVendor and idProduct) and determine the device's position in the USB
     * hierarchy.
     *
     * e.g., "/sys/bus/usb/devices/3-3.5.1"
     * In this example, the device is connected to port 1 of a hub. This hub is
     * connected to port 5 of another hub, which is connected to port 3 of the
     * root hub on bus 3.
     */
    String sysfsPath;
    /**
     * The USB bus number.
     *
     * This corresponds to the 'busnum' sysfs attribute defined in the
     * Linux kernel: Documentation/ABI/stable/sysfs-bus-usb.
     *
     * Alongside devnum can be used to build the devfs path of the device. This
     * path is used in {@code UsbService} to identify the device.
     *
     * e.g., "/dev/bus/usb/003/014"
     */
    int busnum;
    /**
     * The USB device number.
     *
     * This corresponds to the 'devnum' sysfs attribute defined in the
     * Linux kernel: Documentation/ABI/stable/sysfs-bus-usb.
     */
    int devnum;
}
