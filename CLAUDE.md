# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
make        # build
make run    # build and run
make clean  # clean
```

GCC lives at `/e/CodingEnvironment/MSYS2/mingw64/bin/gcc` (on PATH via mingw64/bin).  
`make` is at `E:\CodingEnvironment\MSYS2\usr\bin\make` (on system PATH).

Object files go to `build/`, binary is `matrix-finder.exe` in the project root.

## Architecture

The tool is structured as independent detection layers that each produce a result struct, then feed into a central report engine (Milestone 5). Each layer is a `.c`/`.h` pair under `src/` and `include/`.

| Milestone | Module | What it detects |
|-----------|--------|-----------------|
| 2 (done)  | `cpuid_detect` | Hypervisor bit (CPUID leaf 0x1 ECX[31]) + vendor string (leaf 0x40000000) |
| 3         | `registry_detect`, `smbios_detect` | VM registry keys; fake SMBIOS hardware strings via Windows API |
| 4         | `timing_detect` | RDTSC timing anomalies from VM exit overhead |
| 5         | `report` | Aggregates all layer results into a 0–100 confidence score with per-signal breakdown |
| 6         | `tests/` | Mock-based unit tests for each layer |

`main.c` is intentionally thin: it calls each layer's `_detect()` function, then passes all results to the report engine.

## Constraints

- C only (no C++/C#), compiled with `gcc` (MinGW64) — no MSVC.
- Windows API (`windows.h`) for registry and SMBIOS; no external libraries.
- Each milestone ends with a Git commit.
