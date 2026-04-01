/*
 * Milestone 6: Mock-based unit tests for all detection layers.
 *
 * Strategy: the *_detect() functions call hardware/OS APIs and cannot be
 * mocked without linker tricks.  Instead we test the pure-logic paths:
 *   - cpuid_classify_vendor()    — vendor string matching (no __cpuid call)
 *   - smbios_is_vm_string()      — SMBIOS pattern matching (no WinAPI call)
 *   - smbios_is_generic_string() — placeholder pattern matching
 *   - report_compute()           — scoring and verdict logic
 * For registry, timing, and SMBIOS we construct mock result structs and
 * drive report_compute() to verify scoring thresholds.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "cpuid_detect.h"
#include "registry_detect.h"
#include "smbios_detect.h"
#include "timing_detect.h"
#include "report.h"

/* ------------------------------------------------------------------ */
/* Minimal test framework                                               */
/* ------------------------------------------------------------------ */

static int g_pass = 0;
static int g_fail = 0;

static void check(const char *name, int ok)
{
    if (ok) {
        printf("  PASS  %s\n", name);
        g_pass++;
    } else {
        printf("  FAIL  %s\n", name);
        g_fail++;
    }
}

#define CHECK(name, expr)  check((name), (int)(expr))

/* ------------------------------------------------------------------ */
/* CPUID vendor string parsing                                          */
/* ------------------------------------------------------------------ */

static void test_cpuid_vendor_parsing(void)
{
    printf("\n[CPUID vendor parsing]\n");
    CpuidResult r;

    /* --- Known vendors --- */
    memset(&r, 0, sizeof(r));
    memcpy(r.vendor_string, "VMwareVMware", 12);
    cpuid_classify_vendor(&r);
    CHECK("VMware vendor string -> name='VMware'",   strcmp(r.vendor_name, "VMware") == 0);
    CHECK("VMware -> detected=1",                    r.detected == 1);

    memset(&r, 0, sizeof(r));
    memcpy(r.vendor_string, "VBoxVBoxVBox", 12);
    cpuid_classify_vendor(&r);
    CHECK("VirtualBox vendor string -> name='VirtualBox'",
          strcmp(r.vendor_name, "VirtualBox") == 0);

    memset(&r, 0, sizeof(r));
    memcpy(r.vendor_string, "KVMKVMKVM\0\0\0", 12);  /* embedded NULs */
    cpuid_classify_vendor(&r);
    CHECK("KVM vendor string (with NUL padding) -> name='KVM'",
          strcmp(r.vendor_name, "KVM") == 0);

    memset(&r, 0, sizeof(r));
    memcpy(r.vendor_string, "Microsoft Hv", 12);
    cpuid_classify_vendor(&r);
    CHECK("Microsoft Hv vendor string -> name='Hyper-V'",
          strcmp(r.vendor_name, "Hyper-V") == 0);

    memset(&r, 0, sizeof(r));
    memcpy(r.vendor_string, "TCGTCGTCGTCG", 12);
    cpuid_classify_vendor(&r);
    CHECK("QEMU/TCG vendor string -> name='QEMU/TCG'",
          strcmp(r.vendor_name, "QEMU/TCG") == 0);

    memset(&r, 0, sizeof(r));
    memcpy(r.vendor_string, "XenVMMXenVMM", 12);
    cpuid_classify_vendor(&r);
    CHECK("Xen vendor string -> name='Xen'",
          strcmp(r.vendor_name, "Xen") == 0);

    /* --- Unknown vendor --- */
    memset(&r, 0, sizeof(r));
    memcpy(r.vendor_string, "SomeOtherHyp", 12);
    cpuid_classify_vendor(&r);
    CHECK("Unknown vendor string -> name='Unknown Hypervisor'",
          strcmp(r.vendor_name, "Unknown Hypervisor") == 0);
    CHECK("Unknown vendor -> detected=1", r.detected == 1);

    /* --- Hypervisor bit not set -> report gives cpuid_score=0 --- */
    {
        CpuidResult  cpuid   = {0};
        RegistryResult reg   = {0};
        SmbiosResult smbios  = {0};
        TimingResult timing  = {0};
        Report report;
        /* hypervisor_bit=0, detected=0 — bit not set on this machine */
        report_compute(&report, &cpuid, &reg, &smbios, &timing);
        CHECK("No hypervisor bit -> cpuid_score=0", report.score.cpuid_score == 0);
    }
}

/* ------------------------------------------------------------------ */
/* Registry detection                                                   */
/* ------------------------------------------------------------------ */

static void test_registry(void)
{
    printf("\n[Registry detection]\n");
    CpuidResult  cpuid  = {0};
    SmbiosResult smbios = {0};
    TimingResult timing = {0};
    RegistryResult reg;
    Report report;

    /* No VM keys present */
    memset(&reg, 0, sizeof(reg));
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("Zero registry hits -> registry_score=0", report.score.registry_score == 0);
    CHECK("Zero registry hits -> detected=0",       reg.detected == 0);

    /* One VM key found (e.g. VMware Tools) */
    memset(&reg, 0, sizeof(reg));
    reg.hit_count = 1;
    reg.detected  = 1;
    strncpy(reg.hits[0].key_path,
            "SOFTWARE\\VMware, Inc.\\VMware Tools", REGISTRY_KEY_MAX - 1);
    strncpy(reg.hits[0].vendor, "VMware", REGISTRY_VENDOR_MAX - 1);
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("Registry hit -> registry_score=40", report.score.registry_score == 40);
    CHECK("Registry hit -> detected=1",        reg.detected == 1);
}

/* ------------------------------------------------------------------ */
/* SMBIOS string classification                                         */
/* ------------------------------------------------------------------ */

static void test_smbios(void)
{
    printf("\n[SMBIOS string classification]\n");

    /* --- Real hardware strings should not match --- */
    CHECK("'Dell Inc.' is not a VM string",         smbios_is_vm_string("Dell Inc.") == 0);
    CHECK("'Dell Inc.' is not a generic string",    smbios_is_generic_string("Dell Inc.") == 0);
    CHECK("'ASUSTeK Computer' is not a VM string",  smbios_is_vm_string("ASUSTeK Computer") == 0);

    /* --- Known VM strings --- */
    CHECK("'VMware, Inc.' is a VM string",          smbios_is_vm_string("VMware, Inc.") == 1);
    CHECK("'VMware Virtual Platform' is a VM string",
          smbios_is_vm_string("VMware Virtual Platform") == 1);
    CHECK("'VirtualBox' is a VM string",            smbios_is_vm_string("VirtualBox") == 1);
    CHECK("'innotek GmbH' is a VM string",          smbios_is_vm_string("innotek GmbH") == 1);
    CHECK("'QEMU' is a VM string",                  smbios_is_vm_string("QEMU") == 1);
    CHECK("'Bochs' is a VM string",                 smbios_is_vm_string("Bochs") == 1);
    CHECK("'Red Hat' is a VM string",               smbios_is_vm_string("Red Hat") == 1);

    /* --- Generic placeholder strings --- */
    CHECK("'To Be Filled By O.E.M.' is generic",
          smbios_is_generic_string("To Be Filled By O.E.M.") == 1);
    CHECK("'Not Specified' is generic",             smbios_is_generic_string("Not Specified") == 1);
    CHECK("'Default string' is generic",            smbios_is_generic_string("Default string") == 1);
    CHECK("'System Serial Number' is generic",
          smbios_is_generic_string("System Serial Number") == 1);
    CHECK("'0000000000' is generic",                smbios_is_generic_string("0000000000") == 1);

    /* --- Report scoring with mocked SMBIOS state --- */
    {
        CpuidResult  cpuid  = {0};
        RegistryResult reg  = {0};
        TimingResult timing = {0};
        SmbiosResult smbios;
        Report report;

        /* Real hardware: no VM hits */
        memset(&smbios, 0, sizeof(smbios));
        strncpy(smbios.sys_manufacturer, "Dell Inc.", SMBIOS_STR_MAX - 1);
        smbios.vm_string_hits    = 0;
        smbios.detected          = 0;
        report_compute(&report, &cpuid, &reg, &smbios, &timing);
        CHECK("Real SMBIOS strings -> smbios_score=0", report.score.smbios_score == 0);

        /* VMware SMBIOS strings */
        memset(&smbios, 0, sizeof(smbios));
        strncpy(smbios.sys_manufacturer, "VMware, Inc.", SMBIOS_STR_MAX - 1);
        smbios.vm_string_hits = 1;
        smbios.detected       = 1;
        report_compute(&report, &cpuid, &reg, &smbios, &timing);
        CHECK("VM SMBIOS strings -> smbios_score=20", report.score.smbios_score == 20);

        /* Generic placeholders only (no vm_string_hits) */
        memset(&smbios, 0, sizeof(smbios));
        strncpy(smbios.sys_manufacturer, "To Be Filled By O.E.M.", SMBIOS_STR_MAX - 1);
        smbios.vm_string_hits    = 0;
        smbios.generic_string_hits = 2;
        smbios.detected          = 0;
        report_compute(&report, &cpuid, &reg, &smbios, &timing);
        CHECK("Generic-only SMBIOS (no vm hits) -> smbios_score=0",
              report.score.smbios_score == 0);
    }
}

/* ------------------------------------------------------------------ */
/* Timing thresholds                                                    */
/* ------------------------------------------------------------------ */

static void test_timing(void)
{
    printf("\n[Timing thresholds]\n");
    CpuidResult  cpuid  = {0};
    RegistryResult reg  = {0};
    SmbiosResult smbios = {0};
    TimingResult timing;
    Report report;

    /* Below threshold (< 750 cy) */
    memset(&timing, 0, sizeof(timing));
    timing.median_cycles = 400;
    timing.min_cycles    = 200;
    timing.max_cycles    = 600;
    timing.sample_count  = TIMING_SAMPLE_COUNT;
    timing.elevated      = 0;
    timing.detected      = 0;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("400 cy median -> timing_score=0",           report.score.timing_score == 0);
    CHECK("400 cy median -> VERDICT_BARE_METAL",       report.verdict == VERDICT_BARE_METAL);

    /* Elevated (750 <= median < 3000) */
    memset(&timing, 0, sizeof(timing));
    timing.median_cycles = 1000;
    timing.sample_count  = TIMING_SAMPLE_COUNT;
    timing.elevated      = 1;
    timing.detected      = 0;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("1000 cy median -> timing_score=10",         report.score.timing_score == 10);
    CHECK("1000 cy median -> elevated=1, detected=0",
          timing.elevated == 1 && timing.detected == 0);

    /* Strongly elevated (>= 3000) */
    memset(&timing, 0, sizeof(timing));
    timing.median_cycles = 5000;
    timing.sample_count  = TIMING_SAMPLE_COUNT;
    timing.elevated      = 1;
    timing.detected      = 1;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("5000 cy median -> timing_score=10 (elevated path)", report.score.timing_score == 10);
    CHECK("5000 cy median -> detected=1",                      timing.detected == 1);
}

/* ------------------------------------------------------------------ */
/* Report scoring and verdict thresholds                                */
/* ------------------------------------------------------------------ */

static void test_report_scoring(void)
{
    printf("\n[Report scoring and verdicts]\n");
    CpuidResult  cpuid;
    RegistryResult reg;
    SmbiosResult smbios;
    TimingResult timing;
    Report report;

    /* All signals absent -> score=0, bare metal */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("All absent -> score=0",             report.score.total == 0);
    CHECK("All absent -> VERDICT_BARE_METAL",  report.verdict == VERDICT_BARE_METAL);

    /* Timing only (score=10) -> still bare metal */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    timing.elevated = 1;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("Timing elevated only -> score=10",           report.score.total == 10);
    CHECK("Timing elevated only -> VERDICT_BARE_METAL", report.verdict == VERDICT_BARE_METAL);

    /* Hyper-V bit only (score=5) -> bare metal */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    cpuid.hypervisor_bit = 1;
    strncpy(cpuid.vendor_name, "Hyper-V", VENDOR_NAME_MAX - 1);
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("Hyper-V bit only -> cpuid_score=5",          report.score.cpuid_score == 5);
    CHECK("Hyper-V bit only -> score=5",                report.score.total == 5);
    CHECK("Hyper-V bit only -> VERDICT_BARE_METAL",     report.verdict == VERDICT_BARE_METAL);

    /* Non-MS hypervisor CPUID (score=25) -> ambiguous */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    cpuid.hypervisor_bit = 1;
    strncpy(cpuid.vendor_name, "VMware", VENDOR_NAME_MAX - 1);
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("VMware CPUID vendor only -> cpuid_score=25", report.score.cpuid_score == 25);
    CHECK("VMware CPUID vendor only -> VERDICT_AMBIGUOUS",
          report.verdict == VERDICT_AMBIGUOUS);

    /* Boundary: score=20 -> bare metal */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    smbios.vm_string_hits = 1;   /* smbios_score=20 */
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("score=20 boundary -> VERDICT_BARE_METAL",  report.verdict == VERDICT_BARE_METAL);

    /* Score=25 (SMBIOS 20 + Hyper-V bit 5) -> ambiguous */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    smbios.vm_string_hits = 1;
    cpuid.hypervisor_bit  = 1;
    strncpy(cpuid.vendor_name, "Hyper-V", VENDOR_NAME_MAX - 1);
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("score=25 (SMBIOS+HV-bit) -> VERDICT_AMBIGUOUS",
          report.verdict == VERDICT_AMBIGUOUS);

    /* Boundary: score=50 (registry 40 + timing 10) -> ambiguous */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    reg.detected    = 1;
    timing.elevated = 1;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("score=50 boundary -> VERDICT_AMBIGUOUS", report.verdict == VERDICT_AMBIGUOUS);

    /* Score=65 (registry 40 + CPUID vendor 25) -> VM detected */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    reg.detected         = 1;
    cpuid.hypervisor_bit = 1;
    strncpy(cpuid.vendor_name, "VMware", VENDOR_NAME_MAX - 1);
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("score=65 -> VERDICT_VM_DETECTED", report.verdict == VERDICT_VM_DETECTED);

    /* Registry + SMBIOS (40+20=60) -> VM detected */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    reg.detected          = 1;
    smbios.vm_string_hits = 1;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("Registry+SMBIOS -> score=60",            report.score.total == 60);
    CHECK("Registry+SMBIOS -> VERDICT_VM_DETECTED", report.verdict == VERDICT_VM_DETECTED);

    /* All signals present: 40+25+20+10=95 */
    memset(&cpuid,  0, sizeof(cpuid));
    memset(&reg,    0, sizeof(reg));
    memset(&smbios, 0, sizeof(smbios));
    memset(&timing, 0, sizeof(timing));
    reg.detected          = 1;
    cpuid.hypervisor_bit  = 1;
    strncpy(cpuid.vendor_name, "VMware", VENDOR_NAME_MAX - 1);
    smbios.vm_string_hits = 1;
    timing.elevated       = 1;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("All signals -> score=95",            report.score.total == 95);
    CHECK("All signals -> VERDICT_VM_DETECTED", report.verdict == VERDICT_VM_DETECTED);
}

/* ------------------------------------------------------------------ */
/* Error handling: graceful degradation when layers fail               */
/* ------------------------------------------------------------------ */

static void test_error_handling(void)
{
    printf("\n[Error handling - graceful degradation]\n");
    CpuidResult  cpuid  = {0};
    RegistryResult reg  = {0};
    SmbiosResult smbios = {0};
    TimingResult timing = {0};
    Report report;

    /*
     * Registry access denied: all RegOpenKeyExA calls return non-SUCCESS.
     * The layer silently skips each key -> hit_count=0, detected=0.
     */
    memset(&reg, 0, sizeof(reg));
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("Registry all-denied (0 hits) -> registry_score=0",
          report.score.registry_score == 0);
    CHECK("Registry all-denied -> VERDICT_BARE_METAL",
          report.verdict == VERDICT_BARE_METAL);

    /*
     * SMBIOS query fails: GetSystemFirmwareTable returns 0.
     * smbios_detect() returns immediately with all fields zeroed.
     */
    memset(&smbios, 0, sizeof(smbios));
    CHECK("SMBIOS query failure -> vm_string_hits=0",   smbios.vm_string_hits == 0);
    CHECK("SMBIOS query failure -> detected=0",         smbios.detected == 0);
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("SMBIOS query failure -> smbios_score=0",     report.score.smbios_score == 0);

    /*
     * Timing samples are inconsistent: a few extreme outliers push max very
     * high, but the median (what the layer uses) stays low.
     * Verify that outliers do not trigger a false positive.
     */
    memset(&timing, 0, sizeof(timing));
    timing.median_cycles = 300;    /* well below 750 threshold */
    timing.min_cycles    =  50;
    timing.max_cycles    = 80000;  /* single wild outlier */
    timing.sample_count  = TIMING_SAMPLE_COUNT;
    timing.elevated      = 0;      /* median governs, not max */
    timing.detected      = 0;
    report_compute(&report, &cpuid, &reg, &smbios, &timing);
    CHECK("Inconsistent timing (low median, high outlier) -> timing_score=0",
          report.score.timing_score == 0);
    CHECK("Inconsistent timing -> VERDICT_BARE_METAL",
          report.verdict == VERDICT_BARE_METAL);

    /*
     * Partial CPUID: hypervisor bit set but vendor string is all-zero
     * (some minimal hypervisors don't implement leaf 0x40000000).
     * Should resolve to "Unknown Hypervisor" and score=25.
     */
    {
        CpuidResult partial = {0};
        partial.hypervisor_bit = 1;
        memset(partial.vendor_string, 0, 12);   /* all-zero vendor */
        cpuid_classify_vendor(&partial);
        CHECK("All-zero vendor string -> 'Unknown Hypervisor'",
              strcmp(partial.vendor_name, "Unknown Hypervisor") == 0);
        CHECK("All-zero vendor string -> detected=1", partial.detected == 1);

        memset(&reg,    0, sizeof(reg));
        memset(&smbios, 0, sizeof(smbios));
        memset(&timing, 0, sizeof(timing));
        report_compute(&report, &partial, &reg, &smbios, &timing);
        CHECK("Unknown hypervisor bit -> cpuid_score=25 (non-MS)",
              report.score.cpuid_score == 25);
    }
}

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("================================================\n");
    printf("  Ghost Detector - Unit Tests (Milestone 6)\n");
    printf("================================================\n");

    test_cpuid_vendor_parsing();
    test_registry();
    test_smbios();
    test_timing();
    test_report_scoring();
    test_error_handling();

    printf("\n================================================\n");
    printf("  Results: %d passed, %d failed\n", g_pass, g_fail);
    printf("================================================\n");

    return (g_fail > 0) ? 1 : 0;
}
