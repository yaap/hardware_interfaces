/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <android_bpf_defs.h>
#include <linux/bpf.h>         // struct __sk_buff
#include <linux/netlink.h>     // struct nlmsghdr
#include <stdint.h>

static long (*bpf_skb_load_bytes)(const struct __sk_buff* skb, int off, void* to, int len) =
    (void*)BPF_FUNC_skb_load_bytes;

#define LENGTH_0SUBSYSTEM_EQUAL 11          // length of "\0SUBSYSTEM="
#define LENGTH_POWER_SUPPLY0 13             // length of "power_supply\0"

// Hexadecimal constants representing the strings, assumes little endian format
#define HEX_0SUBSYST 0x5453595342555300uLL  // "\0SUBSYST"
#define HEX_EM_EQUAL 0x00000000003d4d45uLL  // "EM=" + null padding

#define HEX_POWER_SU 0x75735f7265776f70uLL  // "power_su"
#define HEX_PPLY0    0x00000000796c7070uLL  // "pply\0" + null padding

DEFINE_BPF_PROG("skfilter/power_supply", AID_ROOT, AID_SYSTEM, skfilter_power_supply)
(struct __sk_buff* skb) {
    // The first character matched is a '\0'. Starting right past the netlink message header
    // is fine since the SUBSYSTEM= text never occurs at the start, as the kernel always
    // adds ACTION=%s DEVPATH=%s SUBSYSTEM=%s first and null terminates each.
    //
    // See also the kobject_uevent_env() implementation:
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/lib/kobject_uevent.c?#n473
    //
    // The upper bound of this loop has been chosen not to exceed the maximum
    // number of instructions in a BPF program (BPF loops are unrolled).
    #define LENGTH_PREFIX 16 // length of "ACTION=\0DEVPATH="
    for (uint64_t i = sizeof(struct nlmsghdr) + LENGTH_PREFIX; i < 256; ++i) {

        // 16 bytes is big enough to cover both
        uint64_t buf[2] = {};

        if (bpf_skb_load_bytes(skb, i, &buf, LENGTH_0SUBSYSTEM_EQUAL))
            break;  // too far -> SUBSYSTEM= not found

        uint64_t v = (buf[0] ^ HEX_0SUBSYST) | (buf[1] ^ HEX_EM_EQUAL);
        asm ("" : "+r" (v));  // prevent compiler optimizing ^ | ^ into != || !=
        if (v) continue;  // no match -> try again at next position

        i += LENGTH_0SUBSYSTEM_EQUAL;

        // No need to zero buffer as second string is not shorter than first

        if (bpf_skb_load_bytes(skb, i, &buf, LENGTH_POWER_SUPPLY0))
            return 0;  // too far -> cannot match -> drop

        v = (buf[0] ^ HEX_POWER_SU) | (buf[1] ^ HEX_PPLY0);
        asm ("" : "+r" (v));  // prevent compiler optimizing ^ | ^ into != || !=
        if (v) return 0;  // did not match -> drop packet

        return skb->len;  // did match -> accept packet
    }

    // The SUBSYSTEM= text has not been found in the bytes that have been
    // examined: let the user space software perform filtering.
    return skb->len;
}

LICENSE("Apache 2.0");
CRITICAL("healthd");
