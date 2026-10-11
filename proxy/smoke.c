/* Loads mpir.dll from this exe's directory and checks mpz_mul / mpz_add.

   --plan prints the CPU, the arch proxy/arch.h selects (AUTO), and which
   variant DLLs this CPU can execute. RUN means the CPU has every bit in
   that DLL's mask. Vendor, family, and model break ties for AUTO.
   The default mode calls through the proxy (honouring MPIR_PROXY_ARCH
   if it is set). */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"

/* Win64 __mpz_struct: two ints, then an unsigned long long pointer. */
struct Mpz {
    int alloc;
    int size;
    unsigned long long* limbs;
};

typedef struct Mpz MpzT[1];

struct Cpu {
    struct MpirCpu hw;
    char brand[49];
    char vendor[13];
    int stepping;
    unsigned leaf1_ecx;
    unsigned leaf7_ebx;
    unsigned long long xcr0;
    int xcr0_valid;
};

static const char kA[] = "123456789012345678901234567890";
static const char kB[] = "987654321098765432109876543210";
static const char kMul[] = "121932631137021795226185032733622923332237463801111263526900";
static const char kAdd[] = "1111111110111111111011111111100";

static void ReadCpu(struct Cpu* cpu) {
    int regs[4];
    unsigned eax;
    int max_leaf;
    int brand_regs[12];
    size_t n;

    memset(cpu, 0, sizeof(*cpu));
    memcpy(cpu->brand, "unknown", 8);
    MpirReadCpu(&cpu->hw);

    __cpuid(regs, 0);
    max_leaf = regs[0];
    memcpy(cpu->vendor + 0, &regs[1], 4);
    memcpy(cpu->vendor + 4, &regs[3], 4);
    memcpy(cpu->vendor + 8, &regs[2], 4);
    cpu->vendor[12] = '\0';

    if (max_leaf >= 1) {
        __cpuid(regs, 1);
        eax = (unsigned)regs[0];
        cpu->leaf1_ecx = (unsigned)regs[2];
        cpu->stepping = (int)(eax & 0xF);
        if (cpu->leaf1_ecx & CPUID1_ECX_OSXSAVE) {
            cpu->xcr0 = _xgetbv(0);
            cpu->xcr0_valid = 1;
        }
    }

    if (max_leaf >= 7) {
        __cpuidex(regs, 7, 0);
        cpu->leaf7_ebx = (unsigned)regs[1];
    }

    __cpuid(regs, 0x80000000);
    if ((unsigned)regs[0] >= 0x80000004) {
        __cpuid(brand_regs + 0, 0x80000002);
        __cpuid(brand_regs + 4, 0x80000003);
        __cpuid(brand_regs + 8, 0x80000004);
        memcpy(cpu->brand, brand_regs, 48);
        cpu->brand[48] = '\0';
        n = strlen(cpu->brand);
        while (n > 0 && cpu->brand[n - 1] == ' ')
            cpu->brand[--n] = '\0';
    }
}

static void PrintCpu(const struct Cpu* cpu) {
    printf("brand: %s\n", cpu->brand);
    printf("vendor: %s\n", cpu->vendor);
    printf("family: %d\n", cpu->hw.family);
    printf("model: %d\n", cpu->hw.model);
    printf("stepping: %d\n", cpu->stepping);
    printf("leaf1_ecx: 0x%08x\n", cpu->leaf1_ecx);
    printf("leaf7_ebx: 0x%08x\n", cpu->leaf7_ebx);
    if (cpu->xcr0_valid)
        printf("xcr0: 0x%I64x\n", cpu->xcr0);
    else
        printf("xcr0: unavailable\n");
    printf("POPCNT: %d\n", (cpu->hw.features & FEAT_POPCNT) != 0);
    printf("AVX: %d\n", (cpu->hw.features & FEAT_AVX) != 0);
    printf("OSXSAVE: %d\n", (cpu->leaf1_ecx & CPUID1_ECX_OSXSAVE) != 0);
    printf("AVX2: %d\n", (cpu->hw.features & FEAT_AVX2) != 0);
    printf("BMI1: %d\n", (cpu->hw.features & FEAT_BMI1) != 0);
    printf("BMI2: %d\n", (cpu->hw.features & FEAT_BMI2) != 0);
    printf("ADX: %d\n", (cpu->hw.features & FEAT_ADX) != 0);
    printf("OS_YMM: %d\n", (cpu->hw.features & FEAT_OS_YMM) != 0);
}

static int Plan(void) {
    struct Cpu cpu;
    size_t i;
    ReadCpu(&cpu);
    PrintCpu(&cpu);
    printf("AUTO %s\n", MpirSelectArch(&cpu.hw));
    printf("RUN auto\n");
    for (i = 0; i < kArchRuleCount; i++) {
        size_t j;
        int seen = 0;
        unsigned mask;
        for (j = 0; j < i; j++) {
            if (strcmp(kArchRules[j].arch, kArchRules[i].arch) == 0)
                seen = 1;
        }
        if (seen)
            continue;
        mask = kArchRules[i].features;
        if ((cpu.hw.features & mask) == mask)
            printf("RUN %s\n", kArchRules[i].arch);
        else
            printf("SKIP %s\n", kArchRules[i].arch);
    }
    return 0;
}

static HMODULE LoadProxy(void) {
    char path[MAX_PATH];
    char* slash;
    DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return NULL;
    slash = strrchr(path, '\\');
    if (slash == NULL)
        return NULL;
    if (strcpy_s(slash + 1, (size_t)(MAX_PATH - (slash + 1 - path)), "mpir.dll") != 0)
        return NULL;
    return LoadLibraryA(path);
}

static FARPROC Need(HMODULE mod, const char* name) {
    FARPROC proc = GetProcAddress(mod, name);
    if (proc == NULL) {
        printf("missing export %s (%lu)\n", name, GetLastError());
        exit(1);
    }
    return proc;
}

static int Exercise(void) {
    HMODULE mod;
    typedef void (*fn_init)(MpzT);
    typedef int (*fn_set_str)(MpzT, const char*, int);
    typedef void (*fn_bin)(MpzT, MpzT, MpzT);
    typedef char* (*fn_get_str)(char*, int, MpzT);
    typedef void (*fn_clear)(MpzT);
    fn_init init;
    fn_set_str set_str;
    fn_bin mul;
    fn_bin add;
    fn_get_str get_str;
    fn_clear clear;
    struct Mpz a[1], b[1], c[1];
    char* text;
    char request[64];
    DWORD n;

    if (sizeof(struct Mpz) != 16) {
        printf("unexpected mpz layout\n");
        return 1;
    }

    n = GetEnvironmentVariableA("MPIR_PROXY_ARCH", request, sizeof(request));
    if (n == 0 || n >= sizeof(request))
        printf("request: auto\n");
    else
        printf("request: %s\n", request);

    mod = LoadProxy();
    if (mod == NULL) {
        printf("LoadLibrary mpir.dll failed (%lu)\n", GetLastError());
        return 1;
    }

#pragma warning(push)
#pragma warning(disable: 4191) /* FARPROC to a typed function pointer */
    init = (fn_init)Need(mod, "__gmpz_init");
    set_str = (fn_set_str)Need(mod, "__gmpz_set_str");
    mul = (fn_bin)Need(mod, "__gmpz_mul");
    add = (fn_bin)Need(mod, "__gmpz_add");
    get_str = (fn_get_str)Need(mod, "__gmpz_get_str");
    clear = (fn_clear)Need(mod, "__gmpz_clear");
#pragma warning(pop)

    init(a);
    init(b);
    init(c);
    if (set_str(a, kA, 10) != 0 || set_str(b, kB, 10) != 0) {
        printf("mpz_set_str failed\n");
        return 1;
    }

    mul(c, a, b);
    text = get_str(NULL, 10, c);
    if (text == NULL) {
        printf("mpz_get_str failed after mul\n");
        return 1;
    }
    printf("mul %s\n", text);
    if (strcmp(text, kMul) != 0) {
        printf("expected mul %s\n", kMul);
        free(text);
        return 1;
    }
    free(text);

    add(c, a, b);
    text = get_str(NULL, 10, c);
    if (text == NULL) {
        printf("mpz_get_str failed after add\n");
        return 1;
    }
    printf("add %s\n", text);
    if (strcmp(text, kAdd) != 0) {
        printf("expected add %s\n", kAdd);
        free(text);
        return 1;
    }
    free(text);

    clear(a);
    clear(b);
    clear(c);
    FreeLibrary(mod);
    printf("ok\n");
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 2 && strcmp(argv[1], "--plan") == 0)
        return Plan();
    if (argc != 1) {
        printf("usage: smoke [--plan]\n");
        return 2;
    }
    return Exercise();
}
