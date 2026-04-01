#ifndef CPUID_DETECT_H
#define CPUID_DETECT_H

#define VENDOR_NAME_MAX 64

/*
 * Result of the CPUID-based hypervisor detection pass.
 * All fields are populated by cpuid_detect().
 */
typedef struct {
    int  hypervisor_bit;          /* 1 if CPUID leaf 0x1 ECX[31] is set      */
    char vendor_string[13];       /* Raw 12-byte vendor string + NUL          */
    char vendor_name[VENDOR_NAME_MAX]; /* Human-readable name, e.g. "VMware" */
    int  detected;                /* 1 if a hypervisor was positively detected */
} CpuidResult;

/* Populate *result with CPUID findings. Never returns an error; on CPUs
 * that don't support a leaf the corresponding fields are zeroed/empty. */
void cpuid_detect(CpuidResult *result);

/* Print a human-readable summary of *result to stdout. */
void cpuid_print(const CpuidResult *result);

#endif /* CPUID_DETECT_H */
