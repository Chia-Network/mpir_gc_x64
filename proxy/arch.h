/* ISA of each variant DLL, and the tie-break that picks one.
   features is the instruction set that DLL executes. 0 is baseline x64.
   Vendor, family, and model only break ties. The first matching row wins.
   MPIR_PROXY_ARCH is applied by the proxy and does not consult this table.

   Disassembly of the shipped images:
   broadwell_avx and skylake_avx contain andn/blsr/blsi, shlx/shrx/sarx,
   and YMM ops. skylake_avx also contains mulx/adcx/adox.
   haswell contains mulx, popcnt, and YMM ops, and no andn/blsr/blsi.
   skylake_avx contains popcnt as well.
   sandybridge contains popcnt and no VEX.
   broadwell, bulldozer, piledriver, broadwell_avx, and gc contain no popcnt.
   broadwell, bulldozer, piledriver, and gc are baseline x64.
   Rows that name one DLL carry that DLL's mask. */

#ifndef MPIR_ARCH_H
#define MPIR_ARCH_H

#include <intrin.h>
#include <immintrin.h>
#include <stddef.h>
#include <string.h>

enum {
    FEAT_AVX2 = 1u << 1,
    FEAT_BMI1 = 1u << 2,
    FEAT_BMI2 = 1u << 3,
    FEAT_ADX = 1u << 4,
    /* XGETBV(0) & 6 == 6: the OS saves XMM and YMM. */
    FEAT_OS_YMM = 1u << 5,
    FEAT_POPCNT = 1u << 6
};

enum {
    CPUID1_ECX_POPCNT = 1u << 23,
    CPUID1_ECX_OSXSAVE = 1u << 27,
    CPUID7_EBX_BMI1 = 1u << 3,
    CPUID7_EBX_AVX2 = 1u << 5,
    CPUID7_EBX_BMI2 = 1u << 8,
    CPUID7_EBX_ADX = 1u << 19,
    XCR0_XMM_AND_YMM = 0x6
};

enum {
    VENDOR_ANY = 0,
    VENDOR_INTEL = 1,
    VENDOR_AMD = 2
};

struct MpirCpu {
    int vendor;
    int family;
    int model;
    unsigned features;
};

/* family or model < 0 matches any CPU. */
struct MpirArchRule {
    const char* arch;
    int vendor;
    int family;
    int model;
    unsigned features;
};

static const struct MpirArchRule kArchRules[] = {
    { "bulldozer", VENDOR_AMD, 21, 1, 0 },
    { "piledriver", VENDOR_AMD, 21, 2, 0 },
    { "piledriver", VENDOR_AMD, 21, 3, 0 },
    { "piledriver", VENDOR_AMD, 21, 16, 0 },
    { "piledriver", VENDOR_AMD, 21, 18, 0 },
    { "piledriver", VENDOR_AMD, 21, 19, 0 },
    { "broadwell_avx", VENDOR_INTEL, 6, 61, FEAT_BMI1 | FEAT_BMI2 | FEAT_AVX2 | FEAT_OS_YMM },
    { "broadwell_avx", VENDOR_INTEL, 6, 71, FEAT_BMI1 | FEAT_BMI2 | FEAT_AVX2 | FEAT_OS_YMM },
    { "broadwell_avx", VENDOR_INTEL, 6, 79, FEAT_BMI1 | FEAT_BMI2 | FEAT_AVX2 | FEAT_OS_YMM },
    { "broadwell", VENDOR_INTEL, 6, 61, 0 },
    { "broadwell", VENDOR_INTEL, 6, 71, 0 },
    { "broadwell", VENDOR_INTEL, 6, 79, 0 },
    { "skylake_avx", VENDOR_ANY, -1, -1, FEAT_POPCNT | FEAT_BMI1 | FEAT_BMI2 | FEAT_ADX | FEAT_AVX2 | FEAT_OS_YMM },
    { "haswell", VENDOR_ANY, -1, -1, FEAT_POPCNT | FEAT_BMI2 | FEAT_AVX2 | FEAT_OS_YMM },
    { "sandybridge", VENDOR_ANY, -1, -1, FEAT_POPCNT },
    { "gc", VENDOR_ANY, -1, -1, 0 },
};

static const size_t kArchRuleCount = sizeof(kArchRules) / sizeof(kArchRules[0]);

static void MpirReadCpu(struct MpirCpu* cpu) {
    int regs[4];
    int max_leaf;
    char vendor[13];
    unsigned eax;
    unsigned ecx;
    unsigned ebx;
    unsigned base_family;
    unsigned base_model;

    memset(cpu, 0, sizeof(*cpu));

    __cpuid(regs, 0);
    max_leaf = regs[0];
    memcpy(vendor + 0, &regs[1], 4);
    memcpy(vendor + 4, &regs[3], 4);
    memcpy(vendor + 8, &regs[2], 4);
    vendor[12] = '\0';
    if (strcmp(vendor, "GenuineIntel") == 0)
        cpu->vendor = VENDOR_INTEL;
    else if (strcmp(vendor, "AuthenticAMD") == 0)
        cpu->vendor = VENDOR_AMD;

    if (max_leaf < 1)
        return;

    __cpuid(regs, 1);
    eax = (unsigned)regs[0];
    ecx = (unsigned)regs[2];
    base_family = (eax >> 8) & 0xF;
    base_model = (eax >> 4) & 0xF;
    cpu->family = (int)base_family;
    cpu->model = (int)base_model;
    if (base_family == 0xF)
        cpu->family += (int)((eax >> 20) & 0xFF);
    if (base_family == 0x6 || base_family == 0xF)
        cpu->model += (int)((eax >> 12) & 0xF0);

    if (ecx & CPUID1_ECX_POPCNT)
        cpu->features |= FEAT_POPCNT;
    if (ecx & CPUID1_ECX_OSXSAVE) {
        unsigned __int64 xcr0 = _xgetbv(0);
        if ((xcr0 & XCR0_XMM_AND_YMM) == XCR0_XMM_AND_YMM)
            cpu->features |= FEAT_OS_YMM;
    }

    if (max_leaf >= 7) {
        __cpuidex(regs, 7, 0);
        ebx = (unsigned)regs[1];
        if (ebx & CPUID7_EBX_BMI1)
            cpu->features |= FEAT_BMI1;
        if (ebx & CPUID7_EBX_AVX2)
            cpu->features |= FEAT_AVX2;
        if (ebx & CPUID7_EBX_BMI2)
            cpu->features |= FEAT_BMI2;
        if (ebx & CPUID7_EBX_ADX)
            cpu->features |= FEAT_ADX;
    }
}

static int MpirRuleMatches(const struct MpirArchRule* rule, const struct MpirCpu* cpu) {
    if (rule->vendor != VENDOR_ANY && rule->vendor != cpu->vendor)
        return 0;
    if (rule->family >= 0 && rule->family != cpu->family)
        return 0;
    if (rule->model >= 0 && rule->model != cpu->model)
        return 0;
    return (cpu->features & rule->features) == rule->features;
}

static const char* MpirSelectArch(const struct MpirCpu* cpu) {
    size_t i;
    for (i = 0; i < kArchRuleCount; i++) {
        if (MpirRuleMatches(&kArchRules[i], cpu))
            return kArchRules[i].arch;
    }
    return "gc";
}

#endif
