#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "smbios_detect.h"

/* ------------------------------------------------------------------ */
/* SMBIOS raw firmware table helpers                                    */
/* ------------------------------------------------------------------ */

/*
 * The 8-byte header prepended by Windows before the raw SMBIOS table.
 * Defined by GetSystemFirmwareTable('RSMB', 0, ...).
 */
#pragma pack(push, 1)
typedef struct {
    BYTE  Used20CallingMethod;
    BYTE  SMBIOSMajorVersion;
    BYTE  SMBIOSMinorVersion;
    BYTE  DmiRevision;
    DWORD Length;           /* byte count of SMBIOSTableData[] that follows */
} RawSMBIOSHeader;
#pragma pack(pop)

/*
 * Return the idx-th string (1-based) from the string section that follows
 * a formatted SMBIOS structure.  struct_ptr points to the start of the
 * structure; struct_len is the Length field (formatted area only).
 * Returns "" for index 0 or if the index is out of range.
 */
static const char *smbios_string(const BYTE *struct_ptr, BYTE struct_len,
                                  BYTE idx)
{
    if (idx == 0)
        return "";

    const char *p = (const char *)(struct_ptr + struct_len);

    for (BYTE i = 1; i < idx; i++) {
        if (*p == '\0')
            return "";          /* ran out of strings before reaching idx */
        while (*p != '\0')
            p++;
        p++;                    /* skip null terminator of this string */
    }

    return (*p == '\0') ? "" : p;
}

/* Safe copy that always NUL-terminates dst. */
static void safe_copy(char *dst, const char *src, size_t dst_size)
{
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

/* ------------------------------------------------------------------ */
/* String classification                                                */
/* ------------------------------------------------------------------ */

/*
 * Strings that indicate a real hypervisor's fabricated SMBIOS data.
 * Substring match (case-insensitive via manual lower not needed —
 * these are fixed known values reported verbatim by hypervisors).
 */
static const char *const vm_patterns[] = {
    /* VMware */
    "VMware",
    /* VirtualBox */
    "VirtualBox", "innotek GmbH",
    /* QEMU / KVM */
    "QEMU", "Bochs", "BOCHS",
    /* Xen */
    "Xen",
    /* Parallels */
    "Parallels",
    /* Red Hat (RHEV/oVirt) */
    "Red Hat",
    NULL
};

/*
 * Placeholder strings that OEMs and hypervisors use when they haven't
 * filled in real hardware identity — strong signal of virtualised HW.
 */
static const char *const generic_patterns[] = {
    "To Be Filled By O.E.M.",
    "Not Specified",
    "Not Present",
    "System Serial Number",
    "Default string",
    "None",
    "0000000000",    /* run of zeros common in QEMU/VBox serials */
    NULL
};

/* Returns 1 if s contains any of the substrings in the pattern list. */
static int matches_any(const char *s, const char *const *patterns)
{
    if (!s || *s == '\0')
        return 0;
    for (int i = 0; patterns[i] != NULL; i++) {
        if (strstr(s, patterns[i]) != NULL)
            return 1;
    }
    return 0;
}

/* Check a string, update counters, and store into dst. */
static void classify(const char *src, char *dst, size_t dst_size,
                     int *vm_hits, int *generic_hits)
{
    safe_copy(dst, src, dst_size);
    if (matches_any(src, vm_patterns))
        (*vm_hits)++;
    else if (matches_any(src, generic_patterns))
        (*generic_hits)++;
}

/* ------------------------------------------------------------------ */
/* Main detection routine                                               */
/* ------------------------------------------------------------------ */

void smbios_detect(SmbiosResult *result)
{
    memset(result, 0, sizeof(*result));

    /* Query required buffer size.
     * 0x52534D42 == 'R','S','M','B' — the SMBIOS firmware table provider ID. */
    DWORD buf_size = GetSystemFirmwareTable(0x52534D42u, 0, NULL, 0);
    if (buf_size == 0)
        return;

    BYTE *buf = (BYTE *)malloc(buf_size);
    if (!buf)
        return;

    if (GetSystemFirmwareTable(0x52534D42u, 0, buf, buf_size) != buf_size) {
        free(buf);
        return;
    }

    const RawSMBIOSHeader *hdr = (const RawSMBIOSHeader *)buf;
    const BYTE *table     = buf + sizeof(RawSMBIOSHeader);
    const BYTE *table_end = table + hdr->Length;

    const BYTE *p = table;

    while (p < table_end - 1) {
        BYTE type = p[0];
        BYTE len  = p[1];

        /* Sanity: length must be at least 4 (type + length + handle). */
        if (len < 4 || p + len > table_end)
            break;

        /* ---- Extract fields by structure type ---- */
        switch (type) {

        case 0: /* BIOS Information */
            if (len > 4) {
                const char *vendor = smbios_string(p, len, p[4]);
                classify(vendor,
                         result->bios_vendor, SMBIOS_STR_MAX,
                         &result->vm_string_hits, &result->generic_string_hits);
            }
            break;

        case 1: /* System Information */
            if (len > 4)
                classify(smbios_string(p, len, p[4]),
                         result->sys_manufacturer, SMBIOS_STR_MAX,
                         &result->vm_string_hits, &result->generic_string_hits);
            if (len > 5)
                classify(smbios_string(p, len, p[5]),
                         result->sys_product, SMBIOS_STR_MAX,
                         &result->vm_string_hits, &result->generic_string_hits);
            if (len > 7)
                classify(smbios_string(p, len, p[7]),
                         result->sys_serial, SMBIOS_STR_MAX,
                         &result->vm_string_hits, &result->generic_string_hits);
            break;

        case 2: /* Baseboard (Module) Information */
            if (len > 4)
                classify(smbios_string(p, len, p[4]),
                         result->board_manufacturer, SMBIOS_STR_MAX,
                         &result->vm_string_hits, &result->generic_string_hits);
            if (len > 5)
                classify(smbios_string(p, len, p[5]),
                         result->board_product, SMBIOS_STR_MAX,
                         &result->vm_string_hits, &result->generic_string_hits);
            break;

        case 3: /* System Enclosure / Chassis */
            if (len > 4)
                classify(smbios_string(p, len, p[4]),
                         result->chassis_manufacturer, SMBIOS_STR_MAX,
                         &result->vm_string_hits, &result->generic_string_hits);
            break;

        case 127: /* End-of-Table */
            goto done;

        default:
            break;
        }

        /* Advance past formatted area, then skip the string section
         * (one or more NUL-terminated strings ending with a double NUL). */
        const BYTE *str_ptr = p + len;
        while (str_ptr < table_end - 1 &&
               !(str_ptr[0] == 0 && str_ptr[1] == 0)) {
            str_ptr++;
        }
        p = str_ptr + 2;  /* skip the double-NUL terminator */
    }

done:
    free(buf);

    if (result->vm_string_hits > 0)
        result->detected = 1;
}

void smbios_print(const SmbiosResult *result)
{
    /* Always show the raw strings so the user can read the real hardware. */
    printf("  BIOS vendor                          : %s\n",
           *result->bios_vendor      ? result->bios_vendor      : "(empty)");
    printf("  System manufacturer                  : %s\n",
           *result->sys_manufacturer ? result->sys_manufacturer : "(empty)");
    printf("  System product                       : %s\n",
           *result->sys_product      ? result->sys_product      : "(empty)");
    printf("  System serial                        : %s\n",
           *result->sys_serial       ? result->sys_serial       : "(empty)");
    printf("  Board manufacturer                   : %s\n",
           *result->board_manufacturer ? result->board_manufacturer : "(empty)");
    printf("  Board product                        : %s\n",
           *result->board_product    ? result->board_product    : "(empty)");
    printf("  Chassis manufacturer                 : %s\n",
           *result->chassis_manufacturer ? result->chassis_manufacturer : "(empty)");
    printf("  VM-pattern string hits               : %d\n",
           result->vm_string_hits);
    printf("  Generic/placeholder string hits      : %d\n",
           result->generic_string_hits);
    printf("  VM detected via SMBIOS               : %s\n",
           result->detected ? "YES" : "NO");
}
