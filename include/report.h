#ifndef REPORT_H
#define REPORT_H

#include "cpuid_detect.h"
#include "registry_detect.h"
#include "smbios_detect.h"
#include "timing_detect.h"

/*
 * Per-signal contribution to the final confidence score.
 */
typedef struct {
    int registry_score;   /* 0 or 40 — registry VM artifacts        */
    int cpuid_score;      /* 0, 5, or 25 — hypervisor bit / vendor  */
    int smbios_score;     /* 0 or 20 — VM-specific SMBIOS strings   */
    int timing_score;     /* 0 or 10 — elevated RDTSC overhead      */
    int total;            /* sum, capped at 100                     */
} ScoreBreakdown;

/*
 * Final assessment verdict.
 */
typedef enum {
    VERDICT_BARE_METAL  = 0,   /* 0–20   */
    VERDICT_AMBIGUOUS   = 1,   /* 21–50  */
    VERDICT_VM_DETECTED = 2    /* 51–100 */
} Verdict;

/*
 * Aggregate report produced by the report engine.
 */
typedef struct {
    ScoreBreakdown score;
    Verdict        verdict;
    const char    *verdict_text;
} Report;

/*
 * Compute the confidence score from all detection layer results and
 * populate *report.  Never fails; all inputs must already be populated.
 */
void report_compute(Report              *report,
                    const CpuidResult   *cpuid,
                    const RegistryResult *reg,
                    const SmbiosResult  *smbios,
                    const TimingResult  *timing);

/* Print the final report to stdout. */
void report_print(const Report *report);

#endif /* REPORT_H */
