#ifndef TIMING_DETECT_H
#define TIMING_DETECT_H

#include <stdint.h>

#define TIMING_SAMPLE_COUNT 100

/*
 * Result of the RDTSC-based VM exit overhead timing pass.
 *
 * Method: bracket a serializing CPUID instruction between two RDTSC reads,
 * collect TIMING_SAMPLE_COUNT samples, and report the median.  A hypervisor
 * must intercept CPUID and service the VM exit, adding measurable latency.
 *
 * Thresholds (cycles at ~3-5 GHz):
 *   < 750   → normal bare-metal range
 *   750–2999 → elevated; consistent with Hyper-V VBS on bare metal or a
 *              light hypervisor (NOT a reliable VM indicator on its own)
 *   ≥ 3000  → strongly elevated; typical of full-guest VMware/VBox/QEMU
 */
typedef struct {
    uint64_t median_cycles;  /* median CPUID round-trip in TSC cycles   */
    uint64_t min_cycles;     /* lowest single sample                    */
    uint64_t max_cycles;     /* highest single sample                   */
    int      sample_count;   /* always TIMING_SAMPLE_COUNT on success   */
    int      elevated;       /* 1 if median >= 750  (ambiguous signal)  */
    int      detected;       /* 1 if median >= 3000 (strong VM signal)  */
} TimingResult;

/* Populate *result with RDTSC timing findings. */
void timing_detect(TimingResult *result);

/* Print a human-readable summary of *result to stdout. */
void timing_print(const TimingResult *result);

#endif /* TIMING_DETECT_H */
