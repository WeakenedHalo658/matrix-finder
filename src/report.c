#include <stdio.h>
#include <string.h>

#include "report.h"

/* Scoring weights (must match CLAUDE.md spec). */
#define SCORE_REGISTRY      40
#define SCORE_CPUID_VENDOR  25   /* non-Microsoft hypervisor vendor */
#define SCORE_SMBIOS        20
#define SCORE_TIMING        10
#define SCORE_CPUID_BIT      5   /* hypervisor bit only (Microsoft Hv) */

void report_compute(Report               *report,
                    const CpuidResult    *cpuid,
                    const RegistryResult *reg,
                    const SmbiosResult   *smbios,
                    const TimingResult   *timing)
{
    memset(report, 0, sizeof(*report));

    /* Registry: strongest signal — no false positives on real hardware. */
    if (reg->detected)
        report->score.registry_score = SCORE_REGISTRY;

    /*
     * CPUID: differentiate between a non-Microsoft vendor (strong signal)
     * and Microsoft Hv which is common on bare-metal Windows with VBS/WSL2.
     */
    if (cpuid->hypervisor_bit) {
        int is_microsoft_hv = (strcmp(cpuid->vendor_name, "Hyper-V") == 0);
        report->score.cpuid_score = is_microsoft_hv ? SCORE_CPUID_BIT
                                                     : SCORE_CPUID_VENDOR;
    }

    /* SMBIOS: only count VM-specific strings, not generic placeholders. */
    if (smbios->vm_string_hits > 0)
        report->score.smbios_score = SCORE_SMBIOS;

    /* Timing: weak signal — elevated overhead, too many false positives. */
    if (timing->elevated)
        report->score.timing_score = SCORE_TIMING;

    report->score.total = report->score.registry_score
                        + report->score.cpuid_score
                        + report->score.smbios_score
                        + report->score.timing_score;

    if (report->score.total > 100)
        report->score.total = 100;

    /* Classify. */
    if (report->score.total <= 20) {
        report->verdict      = VERDICT_BARE_METAL;
        report->verdict_text = "Bare metal (possibly with hypervisor security features)";
    } else if (report->score.total <= 50) {
        report->verdict      = VERDICT_AMBIGUOUS;
        report->verdict_text = "Ambiguous -- hypervisor present but inconclusive";
    } else {
        report->verdict      = VERDICT_VM_DETECTED;
        report->verdict_text = "VM guest detected";
    }
}

void report_print(const Report *report)
{
    printf("================================================\n");
    printf("  FINAL REPORT\n");
    printf("================================================\n");
    printf("  Signal breakdown:\n");
    printf("    Registry artifacts   : %d / %d\n",
           report->score.registry_score, SCORE_REGISTRY);
    printf("    CPUID vendor/bit     : %d (max %d non-MS / %d bit-only)\n",
           report->score.cpuid_score, SCORE_CPUID_VENDOR, SCORE_CPUID_BIT);
    printf("    SMBIOS VM strings    : %d / %d\n",
           report->score.smbios_score, SCORE_SMBIOS);
    printf("    RDTSC timing         : %d / %d\n",
           report->score.timing_score, SCORE_TIMING);
    printf("  ------------------------------------------------\n");
    printf("  Confidence score       : %d / 100\n", report->score.total);
    printf("  Verdict                : %s\n", report->verdict_text);
    printf("================================================\n");
}
