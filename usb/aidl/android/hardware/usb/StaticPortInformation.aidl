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

import android.hardware.usb.Capability;
import android.hardware.usb.ConnectorType;
import android.hardware.usb.DisplayLinkSpeed;
import android.hardware.usb.PhysicalLocation;
import android.hardware.usb.PortDataRole;
import android.hardware.usb.PortPowerRole;
import android.hardware.usb.UsbSpeed;

/**
 * Static information about a USB port.
 *
 * This information is expected to be stable for the lifetime of the device.
 */
@VintfStability
parcelable StaticPortInformation {
    /**
     * Unique index for the port.
     *   * for Type-C ports: `port<port_number>` - matching the port name in
     *     PortStatus.aidl
     *   * for Type-A ports: `port<port_number>` - the number is assigned starting
     *     from the highest Type-C port number + 1 in an order based on the
     *     physical location:
     *       Panel (Top->Bottom->Left->Right->Front->Back) ->
     *       Vertical position (Upper->Center->Lower) ->
     *       Horizontal position (Left->Center->Right)
     */
    String portName = "";

    /**
     * Sysfs path associated with the port - unique for this port.
     *   * for Type-C ports: /sys/class/typec/port<port_number>.
     *   * for Type-A ports: /sys/bus/usb/devices/<device_address>.
     *       Note: the device address is the one of the highest speed port linked
     *       to the Type-A port.
     *
     * Note: For Type-A ports, the directory for this path exists only when a device is connected
     * to the port.
     */
    String sysfsPath = "";

    /**
     * The type of connector for this port (e.g., Type-A, Type-C).
     */
    ConnectorType connectorType = ConnectorType.UNKNOWN;

    /**
     * Physical location of the port on the device. This is based on ACPI's
     * _PLD (Physical Location of Device) information, describing which panel
     * the port is on (e.g., front, left) and its position on that panel.
     */
    PhysicalLocation physicalLocation;

    /**
     * A list of capabilities supported by this port, such as whether it
     * supports device mode, debug accessory mode, etc.
     */
    Capability[] capabilities = {};

    /**
     * A list of power roles (source, sink) supported by this port.
     * For Type-A ports this is always {PortPowerRole.SOURCE}.
     */
    PortPowerRole[] powerRolesSupported = {PortPowerRole.NONE};

    /**
     * A list of data roles (host, device) supported by this port.
     * For Type-A ports this is always {PortDataRole.HOST}.
     */
    PortDataRole[] dataRolesSupported = {PortDataRole.NONE};

    /**
     * The sysfs paths of the USB host ports that are linked to this port.
     * A port can be linked to multiple host ports (e.g., for USB2 and USB3).
     * e.g., "/sys/bus/usb/devices/3-1"
     *
     * Note: the directories exist only when a device is connected to the port.
     */
    String[] linkedHostUSBPortPaths = {};

    /**
     * For Type-C ports only. The sysfs paths of the USB gadget/device controllers
     * linked to this port.
     * e.g., "/sys/class/udc/<controller_name>"
     */
    String[] linkedDeviceUSBPortPaths = {};

    /**
     * For Type-C ports only. The sysfs paths for alternate modes like DisplayPort.
     * e.g. "/sys/class/drm/card0/card0-DP-1"
     */
    String[] linkedDisplayPaths = {};

    /**
     * For Type-C ports only. The sysfs paths for USB4/Thunderbolt ports.
     * e.g., "/sys/bus/thunderbolt/devices/1-0/usb4_port3"
     */
    String[] linkedUsb4TbtPaths = {};

    /**
     * For Type-C ports that can sink power only. The sysfs paths of the power
     * supply entries that are linked to this port.
     * e.g., "/sys/class/power_supply/<power_supply_name>"
     */
    String[] linkedPowerSupplyPaths = {};

    /**
     * A list of the USB data speeds supported by this port as dictated by the root hubs it's
     * connected to. This does not include USB4/Thunderbolt speeds.
     */
    UsbSpeed[] usbRootHubSpeedsSupported = {};

    /**
     * For Type-C ports only. A list of the USB4/Thunderbolt data speeds supported by this port.
     */
    UsbSpeed[] usb4TbtSpeedsSupported = {};

    /**
     * For Type-C ports only. A list of the display link speeds (DisplayPort alternate mode)
     * supported by this port.
     */
    DisplayLinkSpeed[] displayLinkSpeedsSupported = {};
}
