#include <stdio.h>

#include "cpuid_detect.h"

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

    printf("\n------------------------------------------------\n");
    printf("Analysis complete.\n");

    return 0;
}
