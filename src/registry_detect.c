#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "registry_detect.h"

/*
 * Registry keys that exist only on true VM guests.
 * Bare-metal Windows with Hyper-V VBS/WSL2 does NOT have these.
 * Each entry targets one specific hypervisor product.
 */
typedef struct {
    HKEY        root;
    const char *subkey;
    const char *vendor;
} RegProbe;

static const RegProbe probes[] = {
    /* VMware Tools install */
    { HKEY_LOCAL_MACHINE,
      "SOFTWARE\\VMware, Inc.\\VMware Tools",
      "VMware" },
    /* VMware Tools (32-bit view on 64-bit OS) */
    { HKEY_LOCAL_MACHINE,
      "SOFTWARE\\WOW6432Node\\VMware, Inc.\\VMware Tools",
      "VMware" },
    /* VMware SVGA driver service */
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\vm3dmp",
      "VMware" },
    /* VMware mouse / pointing device */
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\vmmouse",
      "VMware" },

    /* VirtualBox Guest Additions */
    { HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Oracle\\VirtualBox Guest Additions",
      "VirtualBox" },
    { HKEY_LOCAL_MACHINE,
      "SOFTWARE\\WOW6432Node\\Oracle\\VirtualBox Guest Additions",
      "VirtualBox" },
    /* VirtualBox guest kernel driver */
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
      "VirtualBox" },
    /* VirtualBox shared-folders driver */
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\VBoxSF",
      "VirtualBox" },

    /* QEMU Guest Agent (qemu-ga) */
    { HKEY_LOCAL_MACHINE,
      "SOFTWARE\\QEMU",
      "QEMU" },
    { HKEY_LOCAL_MACHINE,
      "SOFTWARE\\WOW6432Node\\QEMU",
      "QEMU" },
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\QEMU-GA",
      "QEMU" },

    /* KVM / VirtIO drivers (Red Hat) — present on KVM guests running Windows */
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\viostor",
      "KVM/VirtIO" },
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\vioinput",
      "KVM/VirtIO" },
    { HKEY_LOCAL_MACHINE,
      "SYSTEM\\CurrentControlSet\\Services\\NetKVM",
      "KVM/VirtIO" },
    { HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Red Hat\\RHEV Agents",
      "KVM/RHEV" },

    { NULL, NULL, NULL }  /* sentinel */
};

void registry_detect(RegistryResult *result)
{
    memset(result, 0, sizeof(*result));

    for (int i = 0; probes[i].subkey != NULL; i++) {
        if (result->hit_count >= REGISTRY_MAX_HITS)
            break;

        HKEY hkey;
        LONG rc = RegOpenKeyExA(probes[i].root, probes[i].subkey,
                                0, KEY_READ, &hkey);
        if (rc == ERROR_SUCCESS) {
            RegCloseKey(hkey);

            RegistryHit *hit = &result->hits[result->hit_count++];
            strncpy(hit->key_path, probes[i].subkey, REGISTRY_KEY_MAX - 1);
            strncpy(hit->vendor,   probes[i].vendor,  REGISTRY_VENDOR_MAX - 1);
            result->detected = 1;
        }
    }
}

void registry_print(const RegistryResult *result)
{
    printf("  VM-specific registry keys found      : %d\n", result->hit_count);

    for (int i = 0; i < result->hit_count; i++) {
        printf("    [%s] HKLM\\%s\n",
               result->hits[i].vendor,
               result->hits[i].key_path);
    }

    printf("  VM detected via registry             : %s\n",
           result->detected ? "YES" : "NO");
}
