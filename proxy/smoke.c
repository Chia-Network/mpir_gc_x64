/* Loads mpir.dll from this exe's directory and checks mpz_mul / mpz_add.

   --plan prints the CPU and which MPIR_PROXY_ARCH values this CPU can
   execute. The default mode calls through the proxy (honouring
   MPIR_PROXY_ARCH if it is set). */

#include <windows.h>
#include <intrin.h>
#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    FEAT_AVX = 1u << 0,
    FEAT_AVX2 = 1u << 1,
    FEAT_BMI2 = 1u << 2,
    FEAT_ADX = 1u << 3,
    FEAT_OS_YMM = 1u << 4
};

enum {
    CPUID1_ECX_OSXSAVE = 1u << 27,
    CPUID1_ECX_AVX = 1u << 28,
    CPUID7_EBX_AVX2 = 1u << 5,
    CPUID7_EBX_BMI2 = 1u << 8,
    CPUID7_EBX_ADX = 1u << 19,
    XCR0_XMM_AND_YMM = 0x6
};

enum {
    VENDOR_OTHER = 0,
    VENDOR_INTEL = 1,
    VENDOR_AMD = 2
};

/* Win64 __mpz_struct: two ints, then an unsigned long long pointer. */
struct Mpz {
    int alloc;
    int size;
    unsigned long long* limbs;
};

typedef struct Mpz MpzT[1];

struct Cpu {
    char brand[49];
    char vendor[13];
    int vendor_id;
    int family;
    int model;
    int stepping;
    unsigned leaf1_ecx;
    unsigned leaf7_ebx;
    unsigned long long xcr0;
    int xcr0_valid;
    unsigned features;
};

/* What this CPU can execute. Bulldozer and Piledriver stay tied to family
   0x15 because those DLLs are tuned for that family. broadwell_avx needs
   AVX2 and OS YMM state; its image has no ADX. */
struct SmokeRow {
    const char* arch;
    unsigned features;
    int vendor;
    int family;
    int model;
};

static const struct SmokeRow kRows[] = {
    { "gc", 0, -1, -1, -1 },
    { "sandybridge", FEAT_AVX | FEAT_OS_YMM, -1, -1, -1 },
    { "haswell", FEAT_BMI2 | FEAT_AVX2 | FEAT_OS_YMM, -1, -1, -1 },
    { "broadwell", FEAT_AVX | FEAT_OS_YMM, -1, -1, -1 },
    { "broadwell_avx", FEAT_AVX2 | FEAT_OS_YMM, -1, -1, -1 },
    { "skylake_avx", FEAT_BMI2 | FEAT_ADX | FEAT_AVX2 | FEAT_OS_YMM, -1, -1, -1 },
    { "bulldozer", 0, VENDOR_AMD, 21, 1 },
    { "piledriver", 0, VENDOR_AMD, 21, 2 },
    { "piledriver", 0, VENDOR_AMD, 21, 3 },
    { "piledriver", 0, VENDOR_AMD, 21, 16 },
    { "piledriver", 0, VENDOR_AMD, 21, 18 },
    { "piledriver", 0, VENDOR_AMD, 21, 19 },
};

static const char kA[] = "123456789012345678901234567890";
static const char kB[] = "987654321098765432109876543210";
static const char kMul[] = "121932631137021795226185032733622923332237463801111263526900";
static const char kAdd[] = "1111111110111111111011111111100";

static void ReadCpu(struct Cpu* cpu) {
    int regs[4];
    unsigned eax;
    unsigned base_family;
    unsigned base_model;
    int max_leaf;
    int brand_regs[12];
    size_t n;

    memset(cpu, 0, sizeof(*cpu));
    memcpy(cpu->brand, "unknown", 8);

    __cpuid(regs, 0);
    max_leaf = regs[0];
    memcpy(cpu->vendor + 0, &regs[1], 4);
    memcpy(cpu->vendor + 4, &regs[3], 4);
    memcpy(cpu->vendor + 8, &regs[2], 4);
    cpu->vendor[12] = '\0';
    if (strcmp(cpu->vendor, "GenuineIntel") == 0)
        cpu->vendor_id = VENDOR_INTEL;
    else if (strcmp(cpu->vendor, "AuthenticAMD") == 0)
        cpu->vendor_id = VENDOR_AMD;

    if (max_leaf >= 1) {
        __cpuid(regs, 1);
        eax = (unsigned)regs[0];
        cpu->leaf1_ecx = (unsigned)regs[2];
        cpu->stepping = (int)(eax & 0xF);
        base_family = (eax >> 8) & 0xF;
        base_model = (eax >> 4) & 0xF;
        cpu->family = (int)base_family;
        cpu->model = (int)base_model;
        if (base_family == 0xF)
            cpu->family += (int)((eax >> 20) & 0xFF);
        if (base_family == 0x6 || base_family == 0xF)
            cpu->model += (int)((eax >> 12) & 0xF0);
        if (cpu->leaf1_ecx & CPUID1_ECX_AVX)
            cpu->features |= FEAT_AVX;
        if (cpu->leaf1_ecx & CPUID1_ECX_OSXSAVE) {
            cpu->xcr0 = _xgetbv(0);
            cpu->xcr0_valid = 1;
            if ((cpu->xcr0 & XCR0_XMM_AND_YMM) == XCR0_XMM_AND_YMM)
                cpu->features |= FEAT_OS_YMM;
        }
    }

    if (max_leaf >= 7) {
        __cpuidex(regs, 7, 0);
        cpu->leaf7_ebx = (unsigned)regs[1];
        if (cpu->leaf7_ebx & CPUID7_EBX_AVX2)
            cpu->features |= FEAT_AVX2;
        if (cpu->leaf7_ebx & CPUID7_EBX_BMI2)
            cpu->features |= FEAT_BMI2;
        if (cpu->leaf7_ebx & CPUID7_EBX_ADX)
            cpu->features |= FEAT_ADX;
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
    printf("family: %d\n", cpu->family);
    printf("model: %d\n", cpu->model);
    printf("stepping: %d\n", cpu->stepping);
    printf("leaf1_ecx: 0x%08x\n", cpu->leaf1_ecx);
    printf("leaf7_ebx: 0x%08x\n", cpu->leaf7_ebx);
    if (cpu->xcr0_valid)
        printf("xcr0: 0x%I64x\n", cpu->xcr0);
    else
        printf("xcr0: unavailable\n");
    printf("AVX: %d\n", (cpu->features & FEAT_AVX) != 0);
    printf("OSXSAVE: %d\n", (cpu->leaf1_ecx & CPUID1_ECX_OSXSAVE) != 0);
    printf("AVX2: %d\n", (cpu->features & FEAT_AVX2) != 0);
    printf("BMI2: %d\n", (cpu->features & FEAT_BMI2) != 0);
    printf("ADX: %d\n", (cpu->features & FEAT_ADX) != 0);
    printf("OS_YMM: %d\n", (cpu->features & FEAT_OS_YMM) != 0);
}

static int RowMatches(const struct SmokeRow* row, const struct Cpu* cpu) {
    if (row->vendor >= 0 && row->vendor != cpu->vendor_id)
        return 0;
    if (row->family >= 0 && row->family != cpu->family)
        return 0;
    if (row->model >= 0 && row->model != cpu->model)
        return 0;
    if ((cpu->features & row->features) != row->features)
        return 0;
    return 1;
}

static int ArchSupported(const struct Cpu* cpu, const char* arch) {
    size_t i;
    for (i = 0; i < sizeof(kRows) / sizeof(kRows[0]); i++) {
        if (strcmp(kRows[i].arch, arch) == 0 && RowMatches(&kRows[i], cpu))
            return 1;
    }
    return 0;
}

static int Plan(void) {
    struct Cpu cpu;
    size_t i;
    ReadCpu(&cpu);
    PrintCpu(&cpu);
    printf("RUN auto\n");
    for (i = 0; i < sizeof(kRows) / sizeof(kRows[0]); i++) {
        size_t j;
        int seen = 0;
        for (j = 0; j < i; j++) {
            if (strcmp(kRows[j].arch, kRows[i].arch) == 0)
                seen = 1;
        }
        if (seen)
            continue;
        if (ArchSupported(&cpu, kRows[i].arch))
            printf("RUN %s\n", kRows[i].arch);
        else
            printf("SKIP %s\n", kRows[i].arch);
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
