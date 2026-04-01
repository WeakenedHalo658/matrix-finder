#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <x86intrin.h>  /* __rdtsc() — GCC/MinGW64      */
#include <cpuid.h>      /* __cpuid() — GCC built-in     */

#include "timing_detect.h"

/* Cycle counts above these values trigger the flags. */
#define THRESHOLD_ELEVATED  750ULL
#define THRESHOLD_DETECTED  3000ULL

/*
 * Take one timing sample:
 *   1. CPUID leaf 0  — serialises the pipeline, warms the branch predictor.
 *   2. RDTSC         — record t1.
 *   3. CPUID leaf 0  — the instruction under test; causes a VM exit if
 *                      a hypervisor is intercepting CPUID.
 *   4. RDTSC         — record t2.
 *
 * Returns t2 - t1, or 0 on counter wrap (discarded by the caller).
 */
static uint64_t measure_one(void)
{
    unsigned int eax, ebx, ecx, edx;
    uint64_t t1, t2;

    __cpuid(0, eax, ebx, ecx, edx);   /* serialise */
    t1 = __rdtsc();
    __cpuid(0, eax, ebx, ecx, edx);   /* timed CPUID */
    t2 = __rdtsc();

    return (t2 > t1) ? (t2 - t1) : 0;
}

static int cmp_u64(const void *a, const void *b)
{
    uint64_t ua = *(const uint64_t *)a;
    uint64_t ub = *(const uint64_t *)b;
    return (ua > ub) - (ua < ub);
}

void timing_detect(TimingResult *result)
{
    uint64_t samples[TIMING_SAMPLE_COUNT];

    memset(result, 0, sizeof(*result));

    /* Warm-up: a few unmeasured rounds let the CPU reach steady state. */
    for (int i = 0; i < 20; i++)
        measure_one();

    /* Collect samples. */
    for (int i = 0; i < TIMING_SAMPLE_COUNT; i++)
        samples[i] = measure_one();

    result->sample_count = TIMING_SAMPLE_COUNT;

    /* Sort ascending to derive min / median / max. */
    qsort(samples, TIMING_SAMPLE_COUNT, sizeof(samples[0]), cmp_u64);

    result->min_cycles    = samples[0];
    result->max_cycles    = samples[TIMING_SAMPLE_COUNT - 1];
    result->median_cycles = samples[TIMING_SAMPLE_COUNT / 2];

    result->elevated = (result->median_cycles >= THRESHOLD_ELEVATED) ? 1 : 0;
    result->detected = (result->median_cycles >= THRESHOLD_DETECTED) ? 1 : 0;
}

void timing_print(const TimingResult *result)
{
    printf("  Samples collected                    : %d\n",
           result->sample_count);
    printf("  CPUID overhead - min / median / max  : %llu / %llu / %llu cycles\n",
           (unsigned long long)result->min_cycles,
           (unsigned long long)result->median_cycles,
           (unsigned long long)result->max_cycles);
    printf("  Overhead elevated (>= 750 cy)        : %s\n",
           result->elevated ? "YES (ambiguous - VBS/Hyper-V or VM)" : "NO");
    printf("  VM detected via timing               : %s\n",
           result->detected ? "YES (>= 3000 cy)" : "NO");
}
