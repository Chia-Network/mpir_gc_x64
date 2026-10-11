# MPIR Windows x64 binaries

`mpir.dll` is a proxy. chiavdf and other Windows callers link it, and at
process start it loads one of the variant DLLs from the same directory and
forwards the MPIR exports to that DLL.

## Files

| File | Role |
| --- | --- |
| `mpir.dll` | Proxy. Picks a variant and forwards about 600 exports. |
| `mpir_gc.dll` | Generic C. Works on every x64 CPU. |
| `mpir_sandybridge.dll` | `POPCNT` and baseline x64. The image has no VEX. |
| `mpir_haswell.dll` | `POPCNT`, BMI2 (`MULX`), and AVX2. The image has no BMI1 and no ADX. |
| `mpir_broadwell.dll` | Baseline x64. Used for Broadwell models when `broadwell_avx` cannot run. |
| `mpir_broadwell_avx.dll` | BMI1 (`ANDN`/`BLSR`/`BLSI`), BMI2 (`SHLX`/`SHRX`/`SARX`), and AVX2. No ADX and no `POPCNT`. |
| `mpir_skylake_avx.dll` | `POPCNT`, BMI1, BMI2, ADX (`ADCX`/`ADOX`), and AVX2. |
| `mpir_bulldozer.dll` | Baseline x64. AMD family 0x15 model 1. |
| `mpir_piledriver.dll` | Baseline x64. AMD family 0x15 models 2, 3, 16, 18, and 19. |
| `mpir.lib`, `mpir.exp` | Import library for the proxy. The export ordinals are stable. |
| `mpir.pdb` | Symbols for the proxy. |
| `mpir.h`, `mpirxx.h`, `gmp.h`, `gmpxx.h` | Public headers. |

Shipped variants leave `PDEP` and `PEXT` unused.

## Provenance

The variant DLLs were built in March 2021 by wjblanke from
[BrianGladman/mpir](https://github.com/BrianGladman/mpir) commit `800bbce9`
(`800bbce9f1dc17f4cffa046dbc5c230291bc974b`) with Visual Studio 2019. PDB
paths inside those DLLs point at `D:\Users\wjblanke\mpir\msvc\vs19\`.

The proxy in this tree (`mpir.dll` and `mpir.pdb`) was built by
[build-proxy run 38173287500](https://github.com/Chia-Network/mpir_gc_x64/actions/runs/38173287500)
on `windows-latest` image `20260925.250.1`. That job used MSBuild 18.10.1
from Visual Studio 18 Enterprise, platform toolset v142, and Windows SDK
10.0.26100.0, configuration Release|x64. The sources it compiled were the
pull-request head `3c68682`. The previous proxy build was pull request #3,
commit `104142c`. The variant DLLs stay the March 2021 builds.

## How the proxy chooses a variant

`MPIR_PROXY_ARCH`, when set to one of the names below, wins and skips
the mask. Any other value is ignored and the table is used. The table
lives in `proxy/arch.h`, which the proxy and `proxy/smoke.c` both include.

`features` on every row is the ISA that DLL executes. Vendor, family, and
model only break ties. The first matching row wins. A Broadwell whose OS
does not save YMM misses `broadwell_avx` and loads the baseline
`broadwell` DLL.

| CPU | DLL |
| --- | --- |
| AMD family 21 (0x15), model 1 | `bulldozer` (baseline x64) |
| AMD family 21, models 2, 3, 16, 18, 19 | `piledriver` (baseline x64) |
| Intel family 6, models 61, 71, 79, with BMI1 + BMI2 + AVX2, and the OS saves YMM | `broadwell_avx` |
| Intel family 6, models 61, 71, 79 | `broadwell` (baseline x64) |
| `POPCNT` + BMI1 + BMI2 + ADX + AVX2, and the OS saves YMM | `skylake_avx` |
| `POPCNT` + BMI2 + AVX2, and the OS saves YMM | `haswell` |
| `POPCNT` | `sandybridge` |
| anything else | `gc` |

YMM state is `XGETBV(0) & 6 == 6`, and it is only read when CPUID leaf 1
ECX reports `OSXSAVE`. `POPCNT` is leaf 1 ECX bit 23. AVX is leaf 1 ECX bit 28.
BMI1 (bit 3), AVX2 (bit 5), BMI2 (bit 8), and ADX (bit 19) are leaf 7 subleaf 0 EBX.

Bulldozer and Piledriver stay in front of the feature rows. Those images
are baseline x64, so a family 0x15 model 1 part keeps `bulldozer` even
though that CPU has `POPCNT`. Broadwell stays in front of `skylake_avx`.
A Broadwell that has BMI1, BMI2, AVX2, and OS YMM keeps `broadwell_avx`
(that image has no ADX). The same models without that ISA keep `broadwell`.

If the chosen DLL fails to load, the proxy loads `mpir_gc.dll`.

## Environment variables

`MPIR_PROXY_ARCH`  
One of `gc`, `sandybridge`, `haswell`, `broadwell`, `broadwell_avx`,
`skylake_avx`, `bulldozer`, `piledriver`. Used for benchmarks and as an
escape hatch. The name check is case-insensitive.

`MPIR_PROXY_DEBUG`  
Set to `1` to print the loaded arch on stderr:

```text
mpir proxy: selected skylake_avx
```

## Rebuilding the proxy

From a Visual Studio x64 developer prompt, in the repo root:

```text
msbuild proxy\mpir.vcxproj /p:Configuration=Release /p:Platform=x64
```

The project file asks for the v142 toolset (Visual Studio 2019). If that
toolset is not installed, pass the one you have, for example
`/p:PlatformToolset=v143`. The output is `proxy\x64\Release\mpir.dll` and
`mpir.pdb` (the file name may be `MPIR.dll`; Windows treats that as the
same file).

CI does this on `windows-latest`: it keeps v142 when the runner has it,
otherwise it picks the newest installed x64 toolset, compares export
ordinals and names with the committed `mpir.dll`, and runs `proxy\smoke.c`
beside the variant DLLs. `mpir.lib` and `mpir.exp` stay as they are while
that export list is unchanged.
