#ifndef REGISTRY_DETECT_H
#define REGISTRY_DETECT_H

#define REGISTRY_MAX_HITS  16
#define REGISTRY_KEY_MAX   128
#define REGISTRY_VENDOR_MAX 32

/*
 * A single VM-artifact registry key that was found to exist.
 */
typedef struct {
    char key_path[REGISTRY_KEY_MAX];
    char vendor[REGISTRY_VENDOR_MAX];
} RegistryHit;

/*
 * Result of the registry-based VM artifact scan.
 * Checks for VMware/VirtualBox/KVM/QEMU guest-side keys only —
 * deliberately excludes Hyper-V keys that are present on bare-metal
 * Windows when VBS/WSL2 is active.
 */
typedef struct {
    int          hit_count;
    RegistryHit  hits[REGISTRY_MAX_HITS];
    int          detected;   /* 1 if any VM-specific key was found */
} RegistryResult;

void registry_detect(RegistryResult *result);
void registry_print(const RegistryResult *result);

#endif /* REGISTRY_DETECT_H */
