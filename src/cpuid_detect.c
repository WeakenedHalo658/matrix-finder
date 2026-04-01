#include <string.h>
#include <stdio.h>
#include <cpuid.h>

#include "cpuid_detect.h"

/* Known hypervisor CPUID vendor strings (leaf 0x40000000, EBX:ECX:EDX). */
typedef struct {
    const char *vendor_string; /* exactly 12 bytes, may contain embedded NULs */
    const char *vendor_name;
} VendorEntry;

static const VendorEntry known_vendors[] = {
    { "VMwareVMware",  "VMware"    },
    { "VBoxVBoxVBox",  "VirtualBox"},
    { "KVMKVMKVM\0\0\0", "KVM"    },  /* 9 chars + 3 NUL pad bytes */
    { "Microsoft Hv",  "Hyper-V"  },
    { "TCGTCGTCGTCG",  "QEMU/TCG" },
    { "XenVMMXenVMM",  "Xen"      },
    { NULL, NULL }
};

void cpuid_detect(CpuidResult *result)
{
    unsigned int eax, ebx, ecx, edx;

    memset(result, 0, sizeof(*result));

    /* --- Leaf 0x1: check hypervisor present bit (ECX[31]) --- */
    __cpuid(0x1, eax, ebx, ecx, edx);
    result->hypervisor_bit = (ecx >> 31) & 1;

    if (!result->hypervisor_bit) {
        strncpy(result->vendor_name, "None (bare metal)", VENDOR_NAME_MAX - 1);
        return;
    }

    /* --- Leaf 0x40000000: read hypervisor vendor string --- */
    /* The 12-byte vendor string is packed into EBX, ECX, EDX (4 bytes each). */
    __cpuid(0x40000000, eax, ebx, ecx, edx);

    memcpy(result->vendor_string,      &ebx, 4);
    memcpy(result->vendor_string + 4,  &ecx, 4);
    memcpy(result->vendor_string + 8,  &edx, 4);
    result->vendor_string[12] = '\0';

    /* Match against known vendors using memcmp (handles embedded NULs). */
    result->detected = 1;
    strncpy(result->vendor_name, "Unknown Hypervisor", VENDOR_NAME_MAX - 1);

    for (int i = 0; known_vendors[i].vendor_string != NULL; i++) {
        if (memcmp(result->vendor_string, known_vendors[i].vendor_string, 12) == 0) {
            strncpy(result->vendor_name, known_vendors[i].vendor_name,
                    VENDOR_NAME_MAX - 1);
            break;
        }
    }
}

void cpuid_print(const CpuidResult *result)
{
    printf("  Hypervisor bit (leaf 0x1  ECX[31]) : %s\n",
           result->hypervisor_bit ? "SET" : "NOT SET");

    if (result->hypervisor_bit) {
        /* Print vendor string with only printable bytes for safety. */
        printf("  Vendor string  (leaf 0x40000000)   : \"");
        for (int i = 0; i < 12; i++) {
            unsigned char c = (unsigned char)result->vendor_string[i];
            putchar((c >= 0x20 && c < 0x7f) ? c : '.');
        }
        printf("\"\n");
        printf("  Identified vendor                  : %s\n",
               result->vendor_name);
    }

    printf("  VM detected via CPUID              : %s\n",
           result->detected ? "YES" : "NO");
}
