#include <stdio.h>

#include "cpuid_detect.h"
#include "registry_detect.h"
#include "smbios_detect.h"
#include "timing_detect.h"

int main(void)
{
    printf("================================================\n");
    printf("  Ghost Detector - VM/Environment Analyzer\n");
    printf("================================================\n\n");

    /* --- Milestone 2: CPUID Detection Layer --- */
    printf("[CPUID Detection Layer]\n");
    CpuidResult cpuid_result;
    cpuid_detect(&cpuid_result);
    cpuid_print(&cpuid_result);

    printf("\n");

    /* --- Milestone 3: Registry Artifact Scan --- */
    printf("[Registry Detection Layer]\n");
    RegistryResult reg_result;
    registry_detect(&reg_result);
    registry_print(&reg_result);

    printf("\n");

    /* --- Milestone 3: SMBIOS Hardware String Scan --- */
    printf("[SMBIOS Detection Layer]\n");
    SmbiosResult smbios_result;
    smbios_detect(&smbios_result);
    smbios_print(&smbios_result);

    /* --- Milestone 4: RDTSC Timing Detection Layer --- */
    printf("[Timing Detection Layer]\n");
    TimingResult timing_result;
    timing_detect(&timing_result);
    timing_print(&timing_result);

    printf("\n------------------------------------------------\n");
    printf("Analysis complete.\n");

    return 0;
}
