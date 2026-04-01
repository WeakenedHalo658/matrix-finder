#ifndef SMBIOS_DETECT_H
#define SMBIOS_DETECT_H

#define SMBIOS_STR_MAX 64

/*
 * Interesting SMBIOS fields extracted from the firmware table.
 * Empty string means the field was absent or had index 0.
 */
typedef struct {
    char bios_vendor[SMBIOS_STR_MAX];        /* Type 0, offset 04h */
    char sys_manufacturer[SMBIOS_STR_MAX];   /* Type 1, offset 04h */
    char sys_product[SMBIOS_STR_MAX];        /* Type 1, offset 05h */
    char sys_serial[SMBIOS_STR_MAX];         /* Type 1, offset 07h */
    char board_manufacturer[SMBIOS_STR_MAX]; /* Type 2, offset 04h */
    char board_product[SMBIOS_STR_MAX];      /* Type 2, offset 05h */
    char chassis_manufacturer[SMBIOS_STR_MAX]; /* Type 3, offset 04h */

    int  vm_string_hits;      /* count of strings matching known-VM patterns */
    int  generic_string_hits; /* count of placeholder/"not filled" strings    */
    int  detected;            /* 1 if evidence of virtualised hardware found  */
} SmbiosResult;

void smbios_detect(SmbiosResult *result);
void smbios_print(const SmbiosResult *result);

#endif /* SMBIOS_DETECT_H */
