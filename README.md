# MPIR Windows x64 binaries

`mpir.dll` is a proxy. chiavdf and other Windows callers link it, and at
process start it loads one of the variant DLLs from the same directory and
forwards the MPIR exports to that DLL.

## Files

| File | Role |
| --- | --- |
| `mpir.dll` | Proxy. Picks a variant and forwards about 600 exports. |
| `mpir_gc.dll` | Generic C. Works on every x64 CPU. |
| `mpir_sandybridge.dll` | AVX, with the OS saving YMM state. |
| `mpir_haswell.dll` | BMI2 (`MULX`) and AVX2. No ADX. |
| `mpir_broadwell.dll` | Broadwell build without AVX2. |
| `mpir_broadwell_avx.dll` | Broadwell AVX2 build. The image has AVX2 and leaves ADX unused. |
| `mpir_skylake_avx.dll` | BMI2, ADX (`ADCX`/`ADOX`), and AVX2. |
| `mpir_bulldozer.dll` | AMD family 0x15 model 1. |
| `mpir_piledriver.dll` | AMD family 0x15 models 2, 3, 16, 18, and 19. |
| `mpir.lib`, `mpir.exp` | Import library for the proxy. The export ordinals are stable. |
| `mpir.pdb` | Symbols for the proxy. |
| `mpir.h`, `mpirxx.h`, `gmp.h`, `gmpxx.h` | Public headers. |

Shipped variants leave `PDEP` and `PEXT` unused.

## Provenance

The variant DLLs were built in March 2021 by wjblanke from
[BrianGladman/mpir](https://github.com/BrianGladman/mpir) commit `800bbce9`
(`800bbce9f1dc17f4cffa046dbc5c230291bc974b`) with Visual Studio 2019. PDB
paths inside those DLLs point at `D:\Users\wjblanke\mpir\msvc\vs19\`.

The proxy was last rebuilt in pull request #3, commit `104142c`. Later
proxy rebuilds come from `.github/workflows/build-proxy.yml` on
`windows-latest` (see that run's `build-info.txt` for the MSVC toolset and
Windows SDK). That workflow rebuilds the proxy. The variant DLLs stay the March 2021 builds.

## How the proxy chooses a variant

`MPIR_PROXY_ARCH`, when set to one of the names below, wins. Any other
value is ignored and the table is used. The first matching row wins.

| CPU | DLL |
| --- | --- |
| AMD family 21 (0x15), model 1 | `bulldozer` |
| AMD family 21, models 2, 3, 16, 18, 19 | `piledriver` |
| Intel family 6, models 61, 71, 79, with AVX2 | `broadwell_avx` |
| Intel family 6, models 61, 71, 79 | `broadwell` |
| BMI2 + ADX + AVX2, and the OS saves YMM | `skylake_avx` |
| BMI2 + AVX2, and the OS saves YMM | `haswell` |
| AVX, and the OS saves YMM | `sandybridge` |
| anything else | `gc` |

YMM state is `XGETBV(0) & 6 == 6`, and it is only read when CPUID leaf 1
ECX reports `OSXSAVE`. AVX, AVX2, BMI2, and ADX come from leaf 1 ECX and
leaf 7 EBX.

The Bulldozer, Piledriver, and Broadwell rows are the previous model map.
They stay in front of the feature rows so those CPUs keep the DLL they
already loaded. Broadwell also has BMI2, ADX, and AVX2, which the feature
rows map to `skylake_avx`. Bulldozer has AVX, which the feature rows map
to `sandybridge`.

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
