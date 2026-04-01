# matrix-finder

> Are you living in a simulation?

`matrix-finder` is a Windows CLI that detects whether the current machine is a VM or hypervisor guest by probing four independent hardware and OS signals, then aggregating them into a 0–100 confidence score.

---

## Why this exists

Games run anti-cheat software that bans or block players who appear to be inside a virtual machine. The same problem bites Linux users running Windows games under Wine/Proton inside a KVM guest: the hypervisor fingerprint leaks through and the game refuses to start. Knowing *which* signals get you flagged, and how strongly, lets you understand (or argue with) that decision — the original motivation for this kind of tooling.

More broadly, VM detection is a standard first step in malware sandboxing, reverse engineering setups, and security research environments where you want to know what kind of execution environment you're actually in.

---

## Detection layers

| Layer | Source | Weight | What it checks |
|-------|--------|--------|----------------|
| Registry | `HKLM` key scan | 40 | VM-specific registry artifacts left by VirtualBox, VMware, QEMU, Hyper-V guest additions |
| CPUID | `CPUID` instruction | 5–25 | Hypervisor present bit (ECX[31] of leaf 0x1) + vendor string from leaf 0x40000000. Microsoft Hv (VBS/WSL2 on bare metal) scores 5; any other vendor scores 25 |
| SMBIOS | Windows firmware API | 20 | BIOS/system manufacturer strings that contain VM-specific identifiers (e.g. "VBOX", "VMware", "QEMU") |
| RDTSC timing | `RDTSC` instruction | 10 | Repeated timestamp reads to measure VM-exit overhead. A weak signal — elevated latency on real hardware with VBS is a known false positive |

Scores are additive and capped at 100:

- **0–20** — Bare metal (possibly with hypervisor security features like VBS)
- **21–50** — Ambiguous — hypervisor present but inconclusive
- **51–100** — VM guest detected

---

## Build & run

Requires GCC (MinGW64) and GNU Make. Both are expected on `PATH`.

```bash
make          # compile to matrix-finder.exe
make run      # compile and run
make test     # build and run the mock-based unit test suite
make clean    # remove build artifacts
```

---

## Example output

Run on a bare-metal Windows 11 machine with Virtualization Based Security (VBS) enabled:

```
================================================
  Ghost Detector - VM/Environment Analyzer
================================================

[CPUID Detection Layer]
  Hypervisor bit : SET
  Vendor string  : Hyper-V
  Verdict        : Hypervisor present (Microsoft Hv -- VBS/WSL2 on bare metal is common)

[Registry Detection Layer]
  Keys scanned   : 12
  Hits           : 0
  Verdict        : No VM registry artifacts found

[SMBIOS Detection Layer]
  Strings scanned: 6
  VM string hits : 0
  Verdict        : No VM-specific SMBIOS strings found

[Timing Detection Layer]
  Median delta   : 312 cycles
  Threshold      : 200 cycles
  Verdict        : Elevated RDTSC overhead detected

================================================
  FINAL REPORT
================================================
  Signal breakdown:
    Registry artifacts   : 0 / 40
    CPUID vendor/bit     : 5 (max 25 non-MS / 5 bit-only)
    SMBIOS VM strings    : 0 / 20
    RDTSC timing         : 10 / 10
  ------------------------------------------------
  Confidence score       : 15 / 100
  Verdict                : Bare metal (possibly with hypervisor security features)
================================================
```

---

## Tech stack

- **Language:** C (C99), no C++ or external libraries
- **Compiler:** GCC 13 via MinGW64
- **OS APIs:** `windows.h`, `advapi32` (registry), `NtQuerySystemInformation` (SMBIOS)
- **Testing:** hand-rolled mock layer that replaces real hardware reads with fixed fixture data
  
---

## Milestone history

| Commit | Milestone | Description |
|--------|-----------|-------------|
| `540d8d9` | 1 | Project scaffold — Makefile, directory layout, empty stubs |
| `ffe854a` | 2 | CPUID detection layer — hypervisor bit and vendor string |
| `33e3172` | 3 | Registry and SMBIOS detection layers via Windows API |
| `ee7479c` | 4 | RDTSC timing detection layer |
| `207f94e` | 5 | Report engine — weighted confidence score and verdict |
| `878b5a6` | 6 | Mock-based unit tests and error handling |
| `853f510` | 7 | CI/CD GitHub Actions workflow (build + test on push) |
