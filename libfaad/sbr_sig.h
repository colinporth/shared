#pragma once

//{{{  struct sbr_hfadj_info
typedef struct {
  real_t G_lim_boost[MAX_L_E][MAX_M];
  real_t Q_M_lim_boost[MAX_L_E][MAX_M];
  real_t S_M_boost[MAX_L_E][MAX_M];
  } sbr_hfadj_info;
//}}}

// dct
void dct4_kernel(real_t * in_real, real_t * in_imag, real_t * out_real, real_t * out_imag);

void DCT3_32_unscaled(real_t *y, real_t *x);
void DCT4_32(real_t *y, real_t *x);
void DST4_32(real_t *y, real_t *x);
void DCT2_32_unscaled(real_t *y, real_t *x);
void DCT4_16(real_t *y, real_t *x);
void DCT2_16_unscaled(real_t *y, real_t *x);

// hf
void hf_generation(sbr_info *sbr, qmf_t Xlow[MAX_NTSRHFG][64],
                   qmf_t Xhigh[MAX_NTSRHFG][64]
#ifdef SBR_LOW_POWER
                   ,real_t *deg
#endif
                   ,uint8_t ch);

uint8_t hf_adjustment(sbr_info *sbr, qmf_t Xsbr[MAX_NTSRHFG][64]
#ifdef SBR_LOW_POWER
                      ,real_t *deg
#endif
                      ,uint8_t ch);

// qmfa
qmfa_info* qmfa_init (uint8_t channels);
void qmfa_end (qmfa_info *qmfa);

qmfs_info* qmfs_init (uint8_t channels);
void qmfs_end (qmfs_info *qmfs);

void sbr_qmf_analysis_32 (sbr_info *sbr, qmfa_info *qmfa, const real_t *input,
                          qmf_t X[MAX_NTSRHFG][64], uint8_t offset, uint8_t kx);
void sbr_qmf_synthesis_32 (sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSRHFG][64], real_t *output);
void sbr_qmf_synthesis_64 (sbr_info *sbr, qmfs_info *qmfs, qmf_t X[MAX_NTSRHFG][64], real_t *output);
