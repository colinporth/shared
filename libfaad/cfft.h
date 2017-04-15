#pragma once

typedef struct {
  uint16_t n;
  uint16_t ifac[15];
  complex_t *work;
  complex_t *tab;
  } cfft_info;


void cfftf (cfft_info *cfft, complex_t *c);
void cfftb (cfft_info *cfft, complex_t *c);
cfft_info* cffti(uint16_t n);
void cfftu (cfft_info *cfft);
