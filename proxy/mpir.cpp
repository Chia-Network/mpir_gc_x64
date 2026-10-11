#include <windows.h>
#include <shlwapi.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "arch.h"

#define MPIRPATHLEN 4096

/* mProcs slots. runasm.asm jumps through mProcs[i]. The first real export
   is slot 6, which mpir.def publishes as ordinal 7. Slots 0..5 are unused.
   Forwarder bodies live in runasm.asm; this file only fills the slots. */
enum { kForwardCount = 600 };

HINSTANCE mHinst = 0, mHinstDLL = 0;
LPCSTR mImportNames[] = {
    "",
    "",
    "",
    "",
    "",
    "",
    "__combine_limbs",
    "__fermat_to_mpz",
    "__gmp_0",
    "__gmp_allocate_func",
    "__gmp_asprintf",
    "__gmp_asprintf_final",
    "__gmp_asprintf_memory",
    "__gmp_asprintf_reps",
    "__gmp_assert_fail",
    "__gmp_assert_header",
    "__gmp_bits_per_limb",
    "__gmp_default_allocate",
    "__gmp_default_fp_limb_precision",
    "__gmp_default_free",
    "__gmp_default_reallocate",
    "__gmp_digit_value_tab",
    "__gmp_divide_by_zero",
    "__gmp_doprnt",
    "__gmp_doprnt_integer",
    "__gmp_doprnt_mpf2",
    "__gmp_doscan",
    "__gmp_errno",
    "__gmp_exception",
    "__gmp_extract_double",
    "__gmp_fib_table",
    "__gmp_fprintf",
    "__gmp_free_func",
    "__gmp_fscanf",
    "__gmp_get_memory_functions",
    "__gmp_init_primesieve",
    "__gmp_invalid_operation",
    "__gmp_jacobi_table",
    "__gmp_junk",
    "__gmp_modlimb_invert_table",
    "__gmp_nextprime",
    "__gmp_primesieve",
    "__gmp_printf",
    "__gmp_randclear",
    "__gmp_randinit_default",
    "__gmp_randinit_lc_2exp",
    "__gmp_randinit_lc_2exp_size",
    "__gmp_randinit_mt",
    "__gmp_randinit_mt_noseed",
    "__gmp_randinit_set",
    "__gmp_rands",
    "__gmp_rands_initialized",
    "__gmp_randseed",
    "__gmp_randseed_ui",
    "__gmp_reallocate_func",
    "__gmp_replacement_vsnprintf",
    "__gmp_scanf",
    "__gmp_set_memory_functions",
    "__gmp_snprintf",
    "__gmp_sprintf",
    "__gmp_sqrt_of_negative",
    "__gmp_sscanf",
    "__gmp_tmp_reentrant_alloc",
    "__gmp_tmp_reentrant_free",
    "__gmp_urandomb_ui",
    "__gmp_urandomm_ui",
    "__gmp_vasprintf",
    "__gmp_version",
    "__gmp_vfprintf",
    "__gmp_vfscanf",
    "__gmp_vprintf",
    "__gmp_vscanf",
    "__gmp_vsnprintf",
    "__gmp_vsprintf",
    "__gmp_vsscanf",
    "__gmpf_abs",
    "__gmpf_add",
    "__gmpf_add_ui",
    "__gmpf_ceil",
    "__gmpf_clear",
    "__gmpf_clears",
    "__gmpf_cmp",
    "__gmpf_cmp_d",
    "__gmpf_cmp_si",
    "__gmpf_cmp_ui",
    "__gmpf_cmp_z",
    "__gmpf_div",
    "__gmpf_div_2exp",
    "__gmpf_div_ui",
    "__gmpf_dump",
    "__gmpf_eq",
    "__gmpf_fits_si_p",
    "__gmpf_fits_sint_p",
    "__gmpf_fits_slong_p",
    "__gmpf_fits_sshort_p",
    "__gmpf_fits_ui_p",
    "__gmpf_fits_uint_p",
    "__gmpf_fits_ulong_p",
    "__gmpf_fits_ushort_p",
    "__gmpf_floor",
    "__gmpf_get_2exp_d",
    "__gmpf_get_d",
    "__gmpf_get_d_2exp",
    "__gmpf_get_default_prec",
    "__gmpf_get_prec",
    "__gmpf_get_si",
    "__gmpf_get_str",
    "__gmpf_get_ui",
    "__gmpf_init",
    "__gmpf_init2",
    "__gmpf_init_set",
    "__gmpf_init_set_d",
    "__gmpf_init_set_si",
    "__gmpf_init_set_str",
    "__gmpf_init_set_ui",
    "__gmpf_inits",
    "__gmpf_inp_str",
    "__gmpf_integer_p",
    "__gmpf_mul",
    "__gmpf_mul_2exp",
    "__gmpf_mul_ui",
    "__gmpf_neg",
    "__gmpf_out_str",
    "__gmpf_pow_ui",
    "__gmpf_random2",
    "__gmpf_reldiff",
    "__gmpf_rrandomb",
    "__gmpf_set",
    "__gmpf_set_d",
    "__gmpf_set_default_prec",
    "__gmpf_set_prec",
    "__gmpf_set_prec_raw",
    "__gmpf_set_q",
    "__gmpf_set_si",
    "__gmpf_set_str",
    "__gmpf_set_ui",
    "__gmpf_set_z",
    "__gmpf_size",
    "__gmpf_sqrt",
    "__gmpf_sqrt_ui",
    "__gmpf_sub",
    "__gmpf_sub_ui",
    "__gmpf_swap",
    "__gmpf_trunc",
    "__gmpf_ui_div",
    "__gmpf_ui_sub",
    "__gmpf_urandomb",
    "__gmpn_add",
    "__gmpn_add_1",
    "__gmpn_add_err1_n",
    "__gmpn_add_err2_n",
    "__gmpn_add_n",
    "__gmpn_addadd_n",
    "__gmpn_addmul_1",
    "__gmpn_addmul_2",
    "__gmpn_addsub_n",
    "__gmpn_and_n",
    "__gmpn_andn_n",
    "__gmpn_bases",
    "__gmpn_bc_set_str",
    "__gmpn_bdivmod",
    "__gmpn_binvert",
    "__gmpn_binvert_itch",
    "__gmpn_clz_tab",
    "__gmpn_cmp",
    "__gmpn_com_n",
    "__gmpn_copyd",
    "__gmpn_copyi",
    "__gmpn_dc_bdiv_q",
    "__gmpn_dc_bdiv_q_n",
    "__gmpn_dc_bdiv_qr",
    "__gmpn_dc_bdiv_qr_n",
    "__gmpn_dc_div_q",
    "__gmpn_dc_div_qr",
    "__gmpn_dc_div_qr_n",
    "__gmpn_dc_divappr_q",
    "__gmpn_dc_set_str",
    "__gmpn_div_2expmod_2expp1",
    "__gmpn_divexact",
    "__gmpn_divexact_1",
    "__gmpn_divexact_by3c",
    "__gmpn_divexact_byff",
    "__gmpn_divexact_byfobm1",
    "__gmpn_divisible_p",
    "__gmpn_divrem",
    "__gmpn_divrem_1",
    "__gmpn_divrem_2",
    "__gmpn_divrem_euclidean_qr_1",
    "__gmpn_divrem_euclidean_qr_2",
    "__gmpn_divrem_euclidean_r_1",
    "__gmpn_divrem_hensel_qr_1",
    "__gmpn_divrem_hensel_qr_1_1",
    "__gmpn_divrem_hensel_qr_1_2",
    "__gmpn_divrem_hensel_r_1",
    "__gmpn_divrem_hensel_rsh_qr_1",
    "__gmpn_divrem_hensel_rsh_qr_1_preinv",
    "__gmpn_dump",
    "__gmpn_fib2_ui",
    "__gmpn_gcd",
    "__gmpn_gcd_1",
    "__gmpn_gcd_subdiv_step",
    "__gmpn_gcdext",
    "__gmpn_gcdext_1",
    "__gmpn_gcdext_hook",
    "__gmpn_gcdext_lehmer_n",
    "__gmpn_get_d",
    "__gmpn_get_str",
    "__gmpn_hamdist",
    "__gmpn_hgcd",
    "__gmpn_hgcd2",
    "__gmpn_hgcd2_jacobi",
    "__gmpn_hgcd_appr",
    "__gmpn_hgcd_appr_itch",
    "__gmpn_hgcd_itch",
    "__gmpn_hgcd_jacobi",
    "__gmpn_hgcd_matrix_adjust",
    "__gmpn_hgcd_matrix_init",
    "__gmpn_hgcd_matrix_mul",
    "__gmpn_hgcd_matrix_mul_1",
    "__gmpn_hgcd_matrix_update_q",
    "__gmpn_hgcd_mul_matrix1_vector",
    "__gmpn_hgcd_reduce",
    "__gmpn_hgcd_reduce_itch",
    "__gmpn_hgcd_step",
    "__gmpn_inv_div_q",
    "__gmpn_inv_div_qr",
    "__gmpn_inv_div_qr_n",
    "__gmpn_inv_divappr_q",
    "__gmpn_inv_divappr_q_n",
    "__gmpn_invert",
    "__gmpn_invert_trunc",
    "__gmpn_ior_n",
    "__gmpn_iorn_n",
    "__gmpn_is_invert",
    "__gmpn_jacobi_2",
    "__gmpn_jacobi_base",
    "__gmpn_jacobi_n",
    "__gmpn_kara_mul_n",
    "__gmpn_kara_sqr_n",
    "__gmpn_lshift",
    "__gmpn_matrix22_mul",
    "__gmpn_matrix22_mul1_inverse_vector",
    "__gmpn_matrix22_mul_itch",
    "__gmpn_matrix22_mul_strassen",
    "__gmpn_mod_1",
    "__gmpn_mod_1_1",
    "__gmpn_mod_1_2",
    "__gmpn_mod_1_3",
    "__gmpn_mod_1_k",
    "__gmpn_mod_34lsub1",
    "__gmpn_modexact_1c_odd",
    "__gmpn_mul",
    "__gmpn_mul_1",
    "__gmpn_mul_2expmod_2expp1",
    "__gmpn_mul_basecase",
    "__gmpn_mul_fft",
    "__gmpn_mul_fft_main",
    "__gmpn_mul_mfa_trunc_sqrt2",
    "__gmpn_mul_n",
    "__gmpn_mul_trunc_sqrt2",
    "__gmpn_mulhigh_n",
    "__gmpn_mullow_basecase",
    "__gmpn_mullow_n",
    "__gmpn_mullow_n_basecase",
    "__gmpn_mulmid",
    "__gmpn_mulmid_basecase",
    "__gmpn_mulmid_n",
    "__gmpn_mulmod_2expm1",
    "__gmpn_mulmod_2expp1_basecase",
    "__gmpn_mulmod_Bexpp1",
    "__gmpn_mulmod_Bexpp1_fft",
    "__gmpn_mulmod_bnm1",
    "__gmpn_nand_n",
    "__gmpn_nior_n",
    "__gmpn_normmod_2expp1",
    "__gmpn_nsumdiff_n",
    "__gmpn_perfect_square_p",
    "__gmpn_popcount",
    "__gmpn_pow_1",
    "__gmpn_powlo",
    "__gmpn_powm",
    "__gmpn_preinv_divrem_1",
    "__gmpn_preinv_mod_1",
    "__gmpn_random",
    "__gmpn_random2",
    "__gmpn_randomb",
    "__gmpn_redc_1",
    "__gmpn_redc_2",
    "__gmpn_redc_n",
    "__gmpn_rootrem",
    "__gmpn_rootrem_basecase",
    "__gmpn_rrandom",
    "__gmpn_rsh_divrem_hensel_qr_1",
    "__gmpn_rsh_divrem_hensel_qr_1_1",
    "__gmpn_rsh_divrem_hensel_qr_1_2",
    "__gmpn_rshift",
    "__gmpn_sb_bdiv_q",
    "__gmpn_sb_bdiv_qr",
    "__gmpn_sb_div_q",
    "__gmpn_sb_div_qr",
    "__gmpn_sb_divappr_q",
    "__gmpn_scan0",
    "__gmpn_scan1",
    "__gmpn_set_str",
    "__gmpn_set_str_compute_powtab",
    "__gmpn_sizeinbase",
    "__gmpn_sqr",
    "__gmpn_sqr_basecase",
    "__gmpn_sqrtrem",
    "__gmpn_sub",
    "__gmpn_sub_1",
    "__gmpn_sub_err1_n",
    "__gmpn_sub_err2_n",
    "__gmpn_sub_n",
    "__gmpn_subadd_n",
    "__gmpn_submul_1",
    "__gmpn_sumdiff_n",
    "__gmpn_tdiv_q",
    "__gmpn_tdiv_qr",
    "__gmpn_toom32_mul",
    "__gmpn_toom3_interpolate",
    "__gmpn_toom3_mul",
    "__gmpn_toom3_mul_n",
    "__gmpn_toom3_sqr_n",
    "__gmpn_toom42_mul",
    "__gmpn_toom42_mulmid",
    "__gmpn_toom4_interpolate",
    "__gmpn_toom4_mul",
    "__gmpn_toom4_mul_n",
    "__gmpn_toom4_sqr_n",
    "__gmpn_toom53_mul",
    "__gmpn_toom8_sqr_n",
    "__gmpn_toom8h_mul",
    "__gmpn_toom_couple_handling",
    "__gmpn_toom_eval_dgr3_pm1",
    "__gmpn_toom_eval_dgr3_pm2",
    "__gmpn_toom_eval_pm1",
    "__gmpn_toom_eval_pm2",
    "__gmpn_toom_eval_pm2exp",
    "__gmpn_toom_eval_pm2rexp",
    "__gmpn_toom_interpolate_16pts",
    "__gmpn_urandomb",
    "__gmpn_urandomm",
    "__gmpn_xnor_n",
    "__gmpn_xor_n",
    "__gmpn_zero",
    "__gmpn_zero_p",
    "__gmpq_abs",
    "__gmpq_add",
    "__gmpq_canonicalize",
    "__gmpq_clear",
    "__gmpq_clears",
    "__gmpq_cmp",
    "__gmpq_cmp_si",
    "__gmpq_cmp_ui",
    "__gmpq_cmp_z",
    "__gmpq_div",
    "__gmpq_div_2exp",
    "__gmpq_equal",
    "__gmpq_get_d",
    "__gmpq_get_den",
    "__gmpq_get_num",
    "__gmpq_get_str",
    "__gmpq_init",
    "__gmpq_inits",
    "__gmpq_inp_str",
    "__gmpq_inv",
    "__gmpq_mul",
    "__gmpq_mul_2exp",
    "__gmpq_neg",
    "__gmpq_out_str",
    "__gmpq_set",
    "__gmpq_set_d",
    "__gmpq_set_den",
    "__gmpq_set_f",
    "__gmpq_set_num",
    "__gmpq_set_si",
    "__gmpq_set_str",
    "__gmpq_set_ui",
    "__gmpq_set_z",
    "__gmpq_sub",
    "__gmpq_swap",
    "__gmpz_2fac_ui",
    "__gmpz_abs",
    "__gmpz_add",
    "__gmpz_add_ui",
    "__gmpz_addmul",
    "__gmpz_addmul_ui",
    "__gmpz_and",
    "__gmpz_aorsmul_1",
    "__gmpz_array_init",
    "__gmpz_bin_ui",
    "__gmpz_bin_uiui",
    "__gmpz_cdiv_q",
    "__gmpz_cdiv_q_2exp",
    "__gmpz_cdiv_q_ui",
    "__gmpz_cdiv_qr",
    "__gmpz_cdiv_qr_ui",
    "__gmpz_cdiv_r",
    "__gmpz_cdiv_r_2exp",
    "__gmpz_cdiv_r_ui",
    "__gmpz_cdiv_ui",
    "__gmpz_clear",
    "__gmpz_clears",
    "__gmpz_clrbit",
    "__gmpz_cmp",
    "__gmpz_cmp_d",
    "__gmpz_cmp_si",
    "__gmpz_cmp_ui",
    "__gmpz_cmpabs",
    "__gmpz_cmpabs_d",
    "__gmpz_cmpabs_ui",
    "__gmpz_com",
    "__gmpz_combit",
    "__gmpz_congruent_2exp_p",
    "__gmpz_congruent_p",
    "__gmpz_congruent_ui_p",
    "__gmpz_divexact",
    "__gmpz_divexact_gcd",
    "__gmpz_divexact_ui",
    "__gmpz_divisible_2exp_p",
    "__gmpz_divisible_p",
    "__gmpz_divisible_ui_p",
    "__gmpz_dump",
    "__gmpz_export",
    "__gmpz_fac_ui",
    "__gmpz_fdiv_q",
    "__gmpz_fdiv_q_2exp",
    "__gmpz_fdiv_q_ui",
    "__gmpz_fdiv_qr",
    "__gmpz_fdiv_qr_ui",
    "__gmpz_fdiv_r",
    "__gmpz_fdiv_r_2exp",
    "__gmpz_fdiv_r_ui",
    "__gmpz_fdiv_ui",
    "__gmpz_fib2_ui",
    "__gmpz_fib_ui",
    "__gmpz_fits_si_p",
    "__gmpz_fits_sint_p",
    "__gmpz_fits_slong_p",
    "__gmpz_fits_sshort_p",
    "__gmpz_fits_ui_p",
    "__gmpz_fits_uint_p",
    "__gmpz_fits_ulong_p",
    "__gmpz_fits_ushort_p",
    "__gmpz_gcd",
    "__gmpz_gcd_ui",
    "__gmpz_gcdext",
    "__gmpz_get_2exp_d",
    "__gmpz_get_d",
    "__gmpz_get_d_2exp",
    "__gmpz_get_si",
    "__gmpz_get_str",
    "__gmpz_get_sx",
    "__gmpz_get_ui",
    "__gmpz_get_ux",
    "__gmpz_getlimbn",
    "__gmpz_hamdist",
    "__gmpz_import",
    "__gmpz_init",
    "__gmpz_init2",
    "__gmpz_init_set",
    "__gmpz_init_set_d",
    "__gmpz_init_set_si",
    "__gmpz_init_set_str",
    "__gmpz_init_set_sx",
    "__gmpz_init_set_ui",
    "__gmpz_init_set_ux",
    "__gmpz_inits",
    "__gmpz_inp_raw",
    "__gmpz_inp_str",
    "__gmpz_inp_str_nowhite",
    "__gmpz_invert",
    "__gmpz_ior",
    "__gmpz_jacobi",
    "__gmpz_kronecker_si",
    "__gmpz_kronecker_ui",
    "__gmpz_lcm",
    "__gmpz_lcm_ui",
    "__gmpz_likely_prime_p",
    "__gmpz_limbs_finish",
    "__gmpz_limbs_modify",
    "__gmpz_limbs_read",
    "__gmpz_limbs_write",
    "__gmpz_lucnum2_ui",
    "__gmpz_lucnum_ui",
    "__gmpz_mfac_uiui",
    "__gmpz_miller_rabin",
    "__gmpz_millerrabin",
    "__gmpz_mod",
    "__gmpz_mul",
    "__gmpz_mul_2exp",
    "__gmpz_mul_si",
    "__gmpz_mul_ui",
    "__gmpz_n_pow_ui",
    "__gmpz_neg",
    "__gmpz_next_prime_candidate",
    "__gmpz_nextprime",
    "__gmpz_nthroot",
    "__gmpz_oddfac_1",
    "__gmpz_out_raw",
    "__gmpz_out_str",
    "__gmpz_perfect_power_p",
    "__gmpz_perfect_square_p",
    "__gmpz_popcount",
    "__gmpz_pow_ui",
    "__gmpz_powm",
    "__gmpz_powm_ui",
    "__gmpz_primorial_ui",
    "__gmpz_probab_prime_p",
    "__gmpz_probable_prime_p",
    "__gmpz_prodlimbs",
    "__gmpz_realloc",
    "__gmpz_realloc2",
    "__gmpz_remove",
    "__gmpz_roinit_n",
    "__gmpz_root",
    "__gmpz_rootrem",
    "__gmpz_rrandomb",
    "__gmpz_scan0",
    "__gmpz_scan1",
    "__gmpz_set",
    "__gmpz_set_d",
    "__gmpz_set_f",
    "__gmpz_set_q",
    "__gmpz_set_si",
    "__gmpz_set_str",
    "__gmpz_set_sx",
    "__gmpz_set_ui",
    "__gmpz_set_ux",
    "__gmpz_setbit",
    "__gmpz_si_kronecker",
    "__gmpz_size",
    "__gmpz_sizeinbase",
    "__gmpz_sqrt",
    "__gmpz_sqrtrem",
    "__gmpz_sub",
    "__gmpz_sub_ui",
    "__gmpz_submul",
    "__gmpz_submul_ui",
    "__gmpz_swap",
    "__gmpz_tdiv_q",
    "__gmpz_tdiv_q_2exp",
    "__gmpz_tdiv_q_ui",
    "__gmpz_tdiv_qr",
    "__gmpz_tdiv_qr_ui",
    "__gmpz_tdiv_r",
    "__gmpz_tdiv_r_2exp",
    "__gmpz_tdiv_r_ui",
    "__gmpz_tdiv_ui",
    "__gmpz_trial_division",
    "__gmpz_tstbit",
    "__gmpz_ui_kronecker",
    "__gmpz_ui_pow_ui",
    "__gmpz_ui_sub",
    "__gmpz_urandomb",
    "__gmpz_urandomm",
    "__gmpz_xor",
    "__mpir_butterfly_lshB",
    "__mpir_butterfly_rshB",
    "__mpir_fft_adjust",
    "__mpir_fft_adjust_limbs",
    "__mpir_fft_adjust_sqrt2",
    "__mpir_fft_butterfly",
    "__mpir_fft_butterfly_sqrt2",
    "__mpir_fft_butterfly_twiddle",
    "__mpir_fft_combine_bits",
    "__mpir_fft_mfa_trunc_sqrt2",
    "__mpir_fft_mfa_trunc_sqrt2_inner",
    "__mpir_fft_mfa_trunc_sqrt2_outer",
    "__mpir_fft_mulmod_2expp1",
    "__mpir_fft_naive_convolution_1",
    "__mpir_fft_negacyclic",
    "__mpir_fft_radix2",
    "__mpir_fft_radix2_twiddle",
    "__mpir_fft_split_bits",
    "__mpir_fft_split_limbs",
    "__mpir_fft_trunc",
    "__mpir_fft_trunc1",
    "__mpir_fft_trunc1_twiddle",
    "__mpir_fft_trunc_sqrt2",
    "__mpir_ifft_butterfly",
    "__mpir_ifft_butterfly_sqrt2",
    "__mpir_ifft_butterfly_twiddle",
    "__mpir_ifft_mfa_trunc_sqrt2",
    "__mpir_ifft_mfa_trunc_sqrt2_outer",
    "__mpir_ifft_negacyclic",
    "__mpir_ifft_radix2",
    "__mpir_ifft_radix2_twiddle",
    "__mpir_ifft_trunc",
    "__mpir_ifft_trunc1",
    "__mpir_ifft_trunc1_twiddle",
    "__mpir_ifft_trunc_sqrt2",
    "__mpir_revbin",
    "__mpir_version",
    "mpir_is_likely_prime_BPSW",
    "mpir_sqrt",
    "mpz_inp_raw_m",
    "mpz_inp_raw_p",
    "mpz_out_raw_m",
};

extern "C" UINT_PTR mProcs[kForwardCount] = { 0 };
static_assert(sizeof(mImportNames) / sizeof(mImportNames[0]) == kForwardCount,
    "mImportNames and mProcs must stay in lockstep with runasm.asm");

static void DebugLine(const char* line) {
    char flag[2];
    HANDLE err;
    DWORD wrote;

    if (GetEnvironmentVariableA("MPIR_PROXY_DEBUG", flag, sizeof(flag)) != 1 || flag[0] != '1')
        return;
    err = GetStdHandle(STD_ERROR_HANDLE);
    if (err == NULL || err == INVALID_HANDLE_VALUE)
        return;
    WriteFile(err, line, lstrlenA(line), &wrote, NULL);
}

static int ArchToUtf8(const wchar_t* arch, char* buf, int cap) {
    return WideCharToMultiByte(CP_UTF8, 0, arch, -1, buf, cap, NULL, NULL) > 0;
}

static const char* ArchByName(const wchar_t* name) {
    char ascii[32];
    size_t i;

    for (i = 0; name[i] != L'\0'; i++) {
        if (i + 1 >= sizeof(ascii) || name[i] > 127)
            return NULL;
        ascii[i] = (char)name[i];
    }
    ascii[i] = '\0';
    for (i = 0; i < kArchRuleCount; i++) {
        if (_stricmp(ascii, kArchRules[i].arch) == 0)
            return kArchRules[i].arch;
    }
    return NULL;
}

static void DebugIgnoredOverride(const wchar_t* name) {
    char utf8[64];
    char line[160];

    if (name != NULL && ArchToUtf8(name, utf8, (int)sizeof(utf8))) {
        sprintf_s(line, sizeof(line), "mpir proxy: ignoring unknown MPIR_PROXY_ARCH=%s\n", utf8);
        DebugLine(line);
        return;
    }
    DebugLine("mpir proxy: ignoring unknown MPIR_PROXY_ARCH\n");
}

static const char* SelectArch(void) {
    wchar_t override_arch[32];
    DWORD n;
    struct MpirCpu cpu;
    const char* chosen;

    n = GetEnvironmentVariableW(L"MPIR_PROXY_ARCH", override_arch, 32);
    if (n >= 32) {
        DebugIgnoredOverride(NULL);
    } else if (n > 0) {
        chosen = ArchByName(override_arch);
        if (chosen)
            return chosen;
        DebugIgnoredOverride(override_arch);
    }

    MpirReadCpu(&cpu);
    return MpirSelectArch(&cpu);
}

static HMODULE LoadArchDll(const WCHAR* dir, const char* arch) {
    WCHAR path[MPIRPATHLEN];
    WCHAR wide[32];
    size_t i;

    for (i = 0; arch[i] != '\0'; i++) {
        if (i + 1 >= sizeof(wide) / sizeof(wide[0])) {
            SetLastError(ERROR_INVALID_NAME);
            return NULL;
        }
        wide[i] = (unsigned char)arch[i];
    }
    wide[i] = L'\0';
    if (swprintf_s(path, MPIRPATHLEN, L"%s\\mpir_%s.dll", dir, wide) < 0) {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return NULL;
    }
    return LoadLibraryW(path);
}

static void DebugSelected(const char* arch) {
    char line[128];
    sprintf_s(line, sizeof(line), "mpir proxy: selected %s\n", arch);
    DebugLine(line);
}

static void DebugLoadFailed(const char* arch, DWORD err, const char* suffix) {
    char line[256];
    sprintf_s(line, sizeof(line),
        "mpir proxy: failed to load mpir_%s.dll (Win32 error %lu)%s\n",
        arch, err, suffix);
    DebugLine(line);
}

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID) {
    WCHAR dir[MPIRPATHLEN];
    DWORD got;
    const char* arch;
    int i;

    if (fdwReason == DLL_PROCESS_ATTACH) {
        mHinst = hinstDLL;
        got = GetModuleFileNameW(mHinst, dir, MPIRPATHLEN);
        if (got == 0 || got >= MPIRPATHLEN)
            return FALSE;
        if (!PathRemoveFileSpecW(dir))
            return FALSE;

        arch = SelectArch();
        mHinstDLL = LoadArchDll(dir, arch);
        if (!mHinstDLL) {
            DWORD err = GetLastError();
            if (strcmp(arch, "gc") != 0) {
                DebugLoadFailed(arch, err, ", falling back to gc");
                arch = "gc";
                mHinstDLL = LoadArchDll(dir, arch);
                if (!mHinstDLL)
                    err = GetLastError();
            }
            if (!mHinstDLL) {
                DebugLoadFailed(arch, err, "");
                return FALSE;
            }
        }

        DebugSelected(arch);
        for (i = 0; i < kForwardCount; i++)
            mProcs[i] = (UINT_PTR)GetProcAddress(mHinstDLL, mImportNames[i]);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        if (mHinstDLL) {
            FreeLibrary(mHinstDLL);
            mHinstDLL = NULL;
        }
    }
    return TRUE;
}
