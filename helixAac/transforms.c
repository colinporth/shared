#include "aaccommon.h"

// fft
#define NUM_FFT_SIZES 2
static const int nfftTab[NUM_FFT_SIZES] = {64, 512};
static const int nfftlog2Tab[NUM_FFT_SIZES] = {6, 9};

#define SQRT1_2 0x5a82799a  /* sqrt(1/2) in Q31 */
#define swapcplx(p0,p1) t = p0; t1 = *(&(p0)+1); p0 = p1; *(&(p0)+1) = *(&(p1)+1); p1 = t; *(&(p1)+1) = t1

//{{{
/* Twiddle tables for FFT
 * format = Q30
 *
 * for (k = 4; k <= N/4; k <<= 1) {
 *   for (j = 0; j < k; j++) {
 *     double wr1, wi1, wr2, wi2, wr3, wi3;
 *
 *     wr1 = cos(1.0 * M_PI * j / (2*k));
 *     wi1 = sin(1.0 * M_PI * j / (2*k));
 *     wr1 = (wr1 + wi1);
 *     wi1 = -wi1;
 *
 *     wr2 = cos(2.0 * M_PI * j / (2*k));
 *     wi2 = sin(2.0 * M_PI * j / (2*k));
 *     wr2 = (wr2 + wi2);
 *     wi2 = -wi2;
 *
 *     wr3 = cos(3.0 * M_PI * j / (2*k));
 *     wi3 = sin(3.0 * M_PI * j / (2*k));
 *     wr3 = (wr3 + wi3);
 *     wi3 = -wi3;
 *
 *     if (k & 0xaaaaaaaa) {
 *       w_odd[iodd++] = (float)wr2;
 *       w_odd[iodd++] = (float)wi2;
 *       w_odd[iodd++] = (float)wr1;
 *       w_odd[iodd++] = (float)wi1;
 *       w_odd[iodd++] = (float)wr3;
 *       w_odd[iodd++] = (float)wi3;
 *     } else {
 *       w_even[ieven++] = (float)wr2;
 *       w_even[ieven++] = (float)wi2;
 *       w_even[ieven++] = (float)wr1;
 *       w_even[ieven++] = (float)wi1;
 *       w_even[ieven++] = (float)wr3;
 *       w_even[ieven++] = (float)wi3;
 *     }
 *   }
 * }
 */
//}}}
//{{{
const int twidTabOdd[8*6 + 32*6 + 128*6]  = {
  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x539eba45, 0xe7821d59,
  0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59,
  0x539eba45, 0xc4df2862, 0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x22a2f4f8, 0xc4df2862,
  0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862,
  0xac6145bb, 0x187de2a7, 0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x45f704f7, 0xf9ba1651,
  0x43103085, 0xfcdc1342, 0x48b2b335, 0xf69bf7c9, 0x4b418bbe, 0xf383a3e2, 0x45f704f7, 0xf9ba1651,
  0x4fd288dc, 0xed6bf9d1, 0x4fd288dc, 0xed6bf9d1, 0x48b2b335, 0xf69bf7c9, 0x553805f2, 0xe4a2eff6,
  0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x569cc31b, 0xe1d4a2c8,
  0x4da1fab5, 0xf0730342, 0x5a6690ae, 0xd5052d97, 0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1,
  0x5a12e720, 0xce86ff2a, 0x5a12e720, 0xd76619b6, 0x51d1dc80, 0xea70658a, 0x57cc15bc, 0xc91af976,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x5a12e720, 0xce86ff2a,
  0x553805f2, 0xe4a2eff6, 0x4da1fab5, 0xc1eb0209, 0x58c542c5, 0xcac933ae, 0x569cc31b, 0xe1d4a2c8,
  0x45f704f7, 0xc04ee4b8, 0x569cc31b, 0xc78e9a1d, 0x57cc15bc, 0xdf18f0ce, 0x3cc85709, 0xc013bc39,
  0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060, 0x4fd288dc, 0xc2c17d52,
  0x5987b08a, 0xd9e01006, 0x26b2a794, 0xc3bdbdf6, 0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6,
  0x1a4608ab, 0xc78e9a1d, 0x45f704f7, 0xc04ee4b8, 0x5a6690ae, 0xd5052d97, 0x0d47d096, 0xcc983f70,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x396b3199, 0xc04ee4b8,
  0x5a6690ae, 0xd09441bb, 0xf2b82f6a, 0xd9e01006, 0x3248d382, 0xc13ad060, 0x5a12e720, 0xce86ff2a,
  0xe5b9f755, 0xe1d4a2c8, 0x2aaa7c7f, 0xc2c17d52, 0x5987b08a, 0xcc983f70, 0xd94d586c, 0xea70658a,
  0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x1a4608ab, 0xc78e9a1d,
  0x57cc15bc, 0xc91af976, 0xc337a8f7, 0xfcdc1342, 0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d,
  0xba08fb09, 0x0645e9af, 0x08df1a8c, 0xce86ff2a, 0x553805f2, 0xc6250a18, 0xb25e054b, 0x0f8cfcbe,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xf720e574, 0xd76619b6,
  0x51d1dc80, 0xc3bdbdf6, 0xa833ea44, 0x20e70f32, 0xee57aa21, 0xdc71898d, 0x4fd288dc, 0xc2c17d52,
  0xa5ed18e0, 0x2899e64a, 0xe5b9f755, 0xe1d4a2c8, 0x4da1fab5, 0xc1eb0209, 0xa5996f52, 0x2f6bbe45,
  0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52, 0xd5558381, 0xed6bf9d1,
  0x48b2b335, 0xc0b15502, 0xaac7fa0e, 0x39daf5e8, 0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8,
  0xb02d7724, 0x3d3e82ae, 0xc694ce67, 0xf9ba1651, 0x43103085, 0xc013bc39, 0xb74d4ccb, 0x3f4eaafe,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x418d2621, 0xfe6deaa1,
  0x40c7d2bd, 0xff36f170, 0x424ff28f, 0xfda4f351, 0x43103085, 0xfcdc1342, 0x418d2621, 0xfe6deaa1,
  0x4488e37f, 0xfb4ab7db, 0x4488e37f, 0xfb4ab7db, 0x424ff28f, 0xfda4f351, 0x46aa0d6d, 0xf8f21e8e,
  0x45f704f7, 0xf9ba1651, 0x43103085, 0xfcdc1342, 0x48b2b335, 0xf69bf7c9, 0x475a5c77, 0xf82a6c6a,
  0x43cdd89a, 0xfc135231, 0x4aa22036, 0xf4491311, 0x48b2b335, 0xf69bf7c9, 0x4488e37f, 0xfb4ab7db,
  0x4c77a88e, 0xf1fa3ecb, 0x49ffd417, 0xf50ef5de, 0x454149fc, 0xfa824bfd, 0x4e32a956, 0xefb047f2,
  0x4b418bbe, 0xf383a3e2, 0x45f704f7, 0xf9ba1651, 0x4fd288dc, 0xed6bf9d1, 0x4c77a88e, 0xf1fa3ecb,
  0x46aa0d6d, 0xf8f21e8e, 0x5156b6d9, 0xeb2e1dbe, 0x4da1fab5, 0xf0730342, 0x475a5c77, 0xf82a6c6a,
  0x52beac9f, 0xe8f77acf, 0x4ec05432, 0xeeee2d9d, 0x4807eb4b, 0xf7630799, 0x5409ed4b, 0xe6c8d59c,
  0x4fd288dc, 0xed6bf9d1, 0x48b2b335, 0xf69bf7c9, 0x553805f2, 0xe4a2eff6, 0x50d86e6d, 0xebeca36c,
  0x495aada2, 0xf5d544a7, 0x56488dc5, 0xe28688a4, 0x51d1dc80, 0xea70658a, 0x49ffd417, 0xf50ef5de,
  0x573b2635, 0xe0745b24, 0x52beac9f, 0xe8f77acf, 0x4aa22036, 0xf4491311, 0x580f7b19, 0xde6d1f65,
  0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x5471e2e6, 0xe61086bc,
  0x4bde1089, 0xf2beafed, 0x595c3e2a, 0xda8249b4, 0x553805f2, 0xe4a2eff6, 0x4c77a88e, 0xf1fa3ecb,
  0x59d438e5, 0xd8a00bae, 0x55f104dc, 0xe3399167, 0x4d0e4de2, 0xf136580d, 0x5a2d0957, 0xd6cb76c9,
  0x569cc31b, 0xe1d4a2c8, 0x4da1fab5, 0xf0730342, 0x5a6690ae, 0xd5052d97, 0x573b2635, 0xe0745b24,
  0x4e32a956, 0xefb047f2, 0x5a80baf6, 0xd34dcdb4, 0x57cc15bc, 0xdf18f0ce, 0x4ec05432, 0xeeee2d9d,
  0x5a7b7f1a, 0xd1a5ef90, 0x584f7b58, 0xddc29958, 0x4f4af5d1, 0xee2cbbc1, 0x5a56deec, 0xd00e2639,
  0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1, 0x5a12e720, 0xce86ff2a, 0x592d59da, 0xdb25f566,
  0x50570819, 0xecabef3d, 0x59afaf4c, 0xcd110216, 0x5987b08a, 0xd9e01006, 0x50d86e6d, 0xebeca36c,
  0x592d59da, 0xcbacb0bf, 0x59d438e5, 0xd8a00bae, 0x5156b6d9, 0xeb2e1dbe, 0x588c1404, 0xca5a86c4,
  0x5a12e720, 0xd76619b6, 0x51d1dc80, 0xea70658a, 0x57cc15bc, 0xc91af976, 0x5a43b190, 0xd6326a88,
  0x5249daa2, 0xe9b38223, 0x56eda1a0, 0xc7ee77b3, 0x5a6690ae, 0xd5052d97, 0x52beac9f, 0xe8f77acf,
  0x55f104dc, 0xc6d569be, 0x5a7b7f1a, 0xd3de9156, 0x53304df6, 0xe83c56cf, 0x54d69714, 0xc5d03118,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x5a7b7f1a, 0xd1a5ef90,
  0x5409ed4b, 0xe6c8d59c, 0x5249daa2, 0xc402a33c, 0x5a6690ae, 0xd09441bb, 0x5471e2e6, 0xe61086bc,
  0x50d86e6d, 0xc33aee27, 0x5a43b190, 0xcf89e3e8, 0x54d69714, 0xe55937d5, 0x4f4af5d1, 0xc2884e6e,
  0x5a12e720, 0xce86ff2a, 0x553805f2, 0xe4a2eff6, 0x4da1fab5, 0xc1eb0209, 0x59d438e5, 0xcd8bbb6d,
  0x55962bc0, 0xe3edb628, 0x4bde1089, 0xc1633f8a, 0x5987b08a, 0xcc983f70, 0x55f104dc, 0xe3399167,
  0x49ffd417, 0xc0f1360b, 0x592d59da, 0xcbacb0bf, 0x56488dc5, 0xe28688a4, 0x4807eb4b, 0xc0950d1d,
  0x58c542c5, 0xcac933ae, 0x569cc31b, 0xe1d4a2c8, 0x45f704f7, 0xc04ee4b8, 0x584f7b58, 0xc9edeb50,
  0x56eda1a0, 0xe123e6ad, 0x43cdd89a, 0xc01ed535, 0x57cc15bc, 0xc91af976, 0x573b2635, 0xe0745b24,
  0x418d2621, 0xc004ef3f, 0x573b2635, 0xc8507ea7, 0x57854ddd, 0xdfc606f1, 0x3f35b59d, 0xc0013bd3,
  0x569cc31b, 0xc78e9a1d, 0x57cc15bc, 0xdf18f0ce, 0x3cc85709, 0xc013bc39, 0x55f104dc, 0xc6d569be,
  0x580f7b19, 0xde6d1f65, 0x3a45e1f7, 0xc03c6a07, 0x553805f2, 0xc6250a18, 0x584f7b58, 0xddc29958,
  0x37af354c, 0xc07b371e, 0x5471e2e6, 0xc57d965d, 0x588c1404, 0xdd196538, 0x350536f1, 0xc0d00db6,
  0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060, 0x52beac9f, 0xc449d892,
  0x58fb0568, 0xdbcb0cce, 0x2f7afdfc, 0xc1bb5a11, 0x51d1dc80, 0xc3bdbdf6, 0x592d59da, 0xdb25f566,
  0x2c9caf6c, 0xc2517e31, 0x50d86e6d, 0xc33aee27, 0x595c3e2a, 0xda8249b4, 0x29aee694, 0xc2fd08a9,
  0x4fd288dc, 0xc2c17d52, 0x5987b08a, 0xd9e01006, 0x26b2a794, 0xc3bdbdf6, 0x4ec05432, 0xc2517e31,
  0x59afaf4c, 0xd93f4e9e, 0x23a8fb93, 0xc4935b3c, 0x4da1fab5, 0xc1eb0209, 0x59d438e5, 0xd8a00bae,
  0x2092f05f, 0xc57d965d, 0x4c77a88e, 0xc18e18a7, 0x59f54bee, 0xd8024d59, 0x1d719810, 0xc67c1e18,
  0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6, 0x1a4608ab, 0xc78e9a1d, 0x49ffd417, 0xc0f1360b,
  0x5a2d0957, 0xd6cb76c9, 0x17115bc0, 0xc8b4ab32, 0x48b2b335, 0xc0b15502, 0x5a43b190, 0xd6326a88,
  0x13d4ae08, 0xc9edeb50, 0x475a5c77, 0xc07b371e, 0x5a56deec, 0xd59afadb, 0x10911f04, 0xcb39edca,
  0x45f704f7, 0xc04ee4b8, 0x5a6690ae, 0xd5052d97, 0x0d47d096, 0xcc983f70, 0x4488e37f, 0xc02c64a6,
  0x5a72c63b, 0xd4710883, 0x09f9e6a1, 0xce0866b8, 0x43103085, 0xc013bc39, 0x5a7b7f1a, 0xd3de9156,
  0x06a886a0, 0xcf89e3e8, 0x418d2621, 0xc004ef3f, 0x5a80baf6, 0xd34dcdb4, 0x0354d741, 0xd11c3142,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x3e68fb62, 0xc004ef3f,
  0x5a80baf6, 0xd2317756, 0xfcab28bf, 0xd4710883, 0x3cc85709, 0xc013bc39, 0x5a7b7f1a, 0xd1a5ef90,
  0xf9577960, 0xd6326a88, 0x3b1e5335, 0xc02c64a6, 0x5a72c63b, 0xd11c3142, 0xf606195f, 0xd8024d59,
  0x396b3199, 0xc04ee4b8, 0x5a6690ae, 0xd09441bb, 0xf2b82f6a, 0xd9e01006, 0x37af354c, 0xc07b371e,
  0x5a56deec, 0xd00e2639, 0xef6ee0fc, 0xdbcb0cce, 0x35eaa2c7, 0xc0b15502, 0x5a43b190, 0xcf89e3e8,
  0xec2b51f8, 0xddc29958, 0x341dbfd3, 0xc0f1360b, 0x5a2d0957, 0xcf077fe1, 0xe8eea440, 0xdfc606f1,
  0x3248d382, 0xc13ad060, 0x5a12e720, 0xce86ff2a, 0xe5b9f755, 0xe1d4a2c8, 0x306c2624, 0xc18e18a7,
  0x59f54bee, 0xce0866b8, 0xe28e67f0, 0xe3edb628, 0x2e88013a, 0xc1eb0209, 0x59d438e5, 0xcd8bbb6d,
  0xdf6d0fa1, 0xe61086bc, 0x2c9caf6c, 0xc2517e31, 0x59afaf4c, 0xcd110216, 0xdc57046d, 0xe83c56cf,
  0x2aaa7c7f, 0xc2c17d52, 0x5987b08a, 0xcc983f70, 0xd94d586c, 0xea70658a, 0x28b1b544, 0xc33aee27,
  0x595c3e2a, 0xcc217822, 0xd651196c, 0xecabef3d, 0x26b2a794, 0xc3bdbdf6, 0x592d59da, 0xcbacb0bf,
  0xd3635094, 0xeeee2d9d, 0x24ada23d, 0xc449d892, 0x58fb0568, 0xcb39edca, 0xd0850204, 0xf136580d,
  0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x2092f05f, 0xc57d965d,
  0x588c1404, 0xca5a86c4, 0xcafac90f, 0xf5d544a7, 0x1e7de5df, 0xc6250a18, 0x584f7b58, 0xc9edeb50,
  0xc850cab4, 0xf82a6c6a, 0x1c6427a9, 0xc6d569be, 0x580f7b19, 0xc9836582, 0xc5ba1e09, 0xfa824bfd,
  0x1a4608ab, 0xc78e9a1d, 0x57cc15bc, 0xc91af976, 0xc337a8f7, 0xfcdc1342, 0x1823dc7d, 0xc8507ea7,
  0x57854ddd, 0xc8b4ab32, 0xc0ca4a63, 0xff36f170, 0x15fdf758, 0xc91af976, 0x573b2635, 0xc8507ea7,
  0xbe72d9df, 0x0192155f, 0x13d4ae08, 0xc9edeb50, 0x56eda1a0, 0xc7ee77b3, 0xbc322766, 0x03ecadcf,
  0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d, 0xba08fb09, 0x0645e9af, 0x0f7944a7, 0xcbacb0bf,
  0x56488dc5, 0xc730e997, 0xb7f814b5, 0x089cf867, 0x0d47d096, 0xcc983f70, 0x55f104dc, 0xc6d569be,
  0xb6002be9, 0x0af10a22, 0x0b145041, 0xcd8bbb6d, 0x55962bc0, 0xc67c1e18, 0xb421ef77, 0x0d415013,
  0x08df1a8c, 0xce86ff2a, 0x553805f2, 0xc6250a18, 0xb25e054b, 0x0f8cfcbe, 0x06a886a0, 0xcf89e3e8,
  0x54d69714, 0xc5d03118, 0xb0b50a2f, 0x11d3443f, 0x0470ebdc, 0xd09441bb, 0x5471e2e6, 0xc57d965d,
  0xaf279193, 0x14135c94, 0x0238a1c6, 0xd1a5ef90, 0x5409ed4b, 0xc52d3d18, 0xadb6255e, 0x164c7ddd,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xfdc75e3a, 0xd3de9156,
  0x53304df6, 0xc4935b3c, 0xab2968ec, 0x1aa6c82b, 0xfb8f1424, 0xd5052d97, 0x52beac9f, 0xc449d892,
  0xaa0efb24, 0x1cc66e99, 0xf9577960, 0xd6326a88, 0x5249daa2, 0xc402a33c, 0xa9125e60, 0x1edc1953,
  0xf720e574, 0xd76619b6, 0x51d1dc80, 0xc3bdbdf6, 0xa833ea44, 0x20e70f32, 0xf4ebafbf, 0xd8a00bae,
  0x5156b6d9, 0xc37b2b6a, 0xa773ebfc, 0x22e69ac8, 0xf2b82f6a, 0xd9e01006, 0x50d86e6d, 0xc33aee27,
  0xa6d2a626, 0x24da0a9a, 0xf086bb59, 0xdb25f566, 0x50570819, 0xc2fd08a9, 0xa65050b4, 0x26c0b162,
  0xee57aa21, 0xdc71898d, 0x4fd288dc, 0xc2c17d52, 0xa5ed18e0, 0x2899e64a, 0xec2b51f8, 0xddc29958,
  0x4f4af5d1, 0xc2884e6e, 0xa5a92114, 0x2a650525, 0xea0208a8, 0xdf18f0ce, 0x4ec05432, 0xc2517e31,
  0xa58480e6, 0x2c216eaa, 0xe7dc2383, 0xe0745b24, 0x4e32a956, 0xc21d0eb8, 0xa57f450a, 0x2dce88aa,
  0xe5b9f755, 0xe1d4a2c8, 0x4da1fab5, 0xc1eb0209, 0xa5996f52, 0x2f6bbe45, 0xe39bd857, 0xe3399167,
  0x4d0e4de2, 0xc1bb5a11, 0xa5d2f6a9, 0x30f8801f, 0xe1821a21, 0xe4a2eff6, 0x4c77a88e, 0xc18e18a7,
  0xa62bc71b, 0x32744493, 0xdf6d0fa1, 0xe61086bc, 0x4bde1089, 0xc1633f8a, 0xa6a3c1d6, 0x33de87de,
  0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52, 0xdb525dc3, 0xe8f77acf,
  0x4aa22036, 0xc114ccb9, 0xa7f084e7, 0x367c9a7e, 0xd94d586c, 0xea70658a, 0x49ffd417, 0xc0f1360b,
  0xa8c4d9cb, 0x37af8159, 0xd74e4abc, 0xebeca36c, 0x495aada2, 0xc0d00db6, 0xa9b7723b, 0x38cf1669,
  0xd5558381, 0xed6bf9d1, 0x48b2b335, 0xc0b15502, 0xaac7fa0e, 0x39daf5e8, 0xd3635094, 0xeeee2d9d,
  0x4807eb4b, 0xc0950d1d, 0xabf612b5, 0x3ad2c2e8, 0xd177fec6, 0xf0730342, 0x475a5c77, 0xc07b371e,
  0xad415361, 0x3bb6276e, 0xcf93d9dc, 0xf1fa3ecb, 0x46aa0d6d, 0xc063d405, 0xaea94927, 0x3c84d496,
  0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8, 0xb02d7724, 0x3d3e82ae, 0xcbe2402d, 0xf50ef5de,
  0x454149fc, 0xc03c6a07, 0xb1cd56aa, 0x3de2f148, 0xca155d39, 0xf69bf7c9, 0x4488e37f, 0xc02c64a6,
  0xb3885772, 0x3e71e759, 0xc850cab4, 0xf82a6c6a, 0x43cdd89a, 0xc01ed535, 0xb55ddfca, 0x3eeb3347,
  0xc694ce67, 0xf9ba1651, 0x43103085, 0xc013bc39, 0xb74d4ccb, 0x3f4eaafe, 0xc4e1accb, 0xfb4ab7db,
  0x424ff28f, 0xc00b1a20, 0xb955f293, 0x3f9c2bfb, 0xc337a8f7, 0xfcdc1342, 0x418d2621, 0xc004ef3f,
  0xbb771c81, 0x3fd39b5a, 0xc197049e, 0xfe6deaa1, 0x40c7d2bd, 0xc0013bd3, 0xbdb00d71, 0x3ff4e5e0,
};
//}}}
//{{{
const int twidTabEven[4*6 + 16*6 + 64*6]  = {
  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x5a82799a, 0xd2bec333,
  0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333,
  0x00000000, 0xd2bec333, 0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x4b418bbe, 0xf383a3e2,
  0x45f704f7, 0xf9ba1651, 0x4fd288dc, 0xed6bf9d1, 0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2,
  0x58c542c5, 0xdc71898d, 0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1, 0x5a12e720, 0xce86ff2a,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x58c542c5, 0xcac933ae,
  0x569cc31b, 0xe1d4a2c8, 0x45f704f7, 0xc04ee4b8, 0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d,
  0x3248d382, 0xc13ad060, 0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6, 0x1a4608ab, 0xc78e9a1d,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x3248d382, 0xc13ad060,
  0x5a12e720, 0xce86ff2a, 0xe5b9f755, 0xe1d4a2c8, 0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae,
  0xcdb72c7e, 0xf383a3e2, 0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d, 0xba08fb09, 0x0645e9af,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xee57aa21, 0xdc71898d,
  0x4fd288dc, 0xc2c17d52, 0xa5ed18e0, 0x2899e64a, 0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060,
  0xa73abd3b, 0x3536cc52, 0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8, 0xb02d7724, 0x3d3e82ae,

  0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x40000000, 0x00000000, 0x43103085, 0xfcdc1342,
  0x418d2621, 0xfe6deaa1, 0x4488e37f, 0xfb4ab7db, 0x45f704f7, 0xf9ba1651, 0x43103085, 0xfcdc1342,
  0x48b2b335, 0xf69bf7c9, 0x48b2b335, 0xf69bf7c9, 0x4488e37f, 0xfb4ab7db, 0x4c77a88e, 0xf1fa3ecb,
  0x4b418bbe, 0xf383a3e2, 0x45f704f7, 0xf9ba1651, 0x4fd288dc, 0xed6bf9d1, 0x4da1fab5, 0xf0730342,
  0x475a5c77, 0xf82a6c6a, 0x52beac9f, 0xe8f77acf, 0x4fd288dc, 0xed6bf9d1, 0x48b2b335, 0xf69bf7c9,
  0x553805f2, 0xe4a2eff6, 0x51d1dc80, 0xea70658a, 0x49ffd417, 0xf50ef5de, 0x573b2635, 0xe0745b24,
  0x539eba45, 0xe7821d59, 0x4b418bbe, 0xf383a3e2, 0x58c542c5, 0xdc71898d, 0x553805f2, 0xe4a2eff6,
  0x4c77a88e, 0xf1fa3ecb, 0x59d438e5, 0xd8a00bae, 0x569cc31b, 0xe1d4a2c8, 0x4da1fab5, 0xf0730342,
  0x5a6690ae, 0xd5052d97, 0x57cc15bc, 0xdf18f0ce, 0x4ec05432, 0xeeee2d9d, 0x5a7b7f1a, 0xd1a5ef90,
  0x58c542c5, 0xdc71898d, 0x4fd288dc, 0xed6bf9d1, 0x5a12e720, 0xce86ff2a, 0x5987b08a, 0xd9e01006,
  0x50d86e6d, 0xebeca36c, 0x592d59da, 0xcbacb0bf, 0x5a12e720, 0xd76619b6, 0x51d1dc80, 0xea70658a,
  0x57cc15bc, 0xc91af976, 0x5a6690ae, 0xd5052d97, 0x52beac9f, 0xe8f77acf, 0x55f104dc, 0xc6d569be,
  0x5a82799a, 0xd2bec333, 0x539eba45, 0xe7821d59, 0x539eba45, 0xc4df2862, 0x5a6690ae, 0xd09441bb,
  0x5471e2e6, 0xe61086bc, 0x50d86e6d, 0xc33aee27, 0x5a12e720, 0xce86ff2a, 0x553805f2, 0xe4a2eff6,
  0x4da1fab5, 0xc1eb0209, 0x5987b08a, 0xcc983f70, 0x55f104dc, 0xe3399167, 0x49ffd417, 0xc0f1360b,
  0x58c542c5, 0xcac933ae, 0x569cc31b, 0xe1d4a2c8, 0x45f704f7, 0xc04ee4b8, 0x57cc15bc, 0xc91af976,
  0x573b2635, 0xe0745b24, 0x418d2621, 0xc004ef3f, 0x569cc31b, 0xc78e9a1d, 0x57cc15bc, 0xdf18f0ce,
  0x3cc85709, 0xc013bc39, 0x553805f2, 0xc6250a18, 0x584f7b58, 0xddc29958, 0x37af354c, 0xc07b371e,
  0x539eba45, 0xc4df2862, 0x58c542c5, 0xdc71898d, 0x3248d382, 0xc13ad060, 0x51d1dc80, 0xc3bdbdf6,
  0x592d59da, 0xdb25f566, 0x2c9caf6c, 0xc2517e31, 0x4fd288dc, 0xc2c17d52, 0x5987b08a, 0xd9e01006,
  0x26b2a794, 0xc3bdbdf6, 0x4da1fab5, 0xc1eb0209, 0x59d438e5, 0xd8a00bae, 0x2092f05f, 0xc57d965d,
  0x4b418bbe, 0xc13ad060, 0x5a12e720, 0xd76619b6, 0x1a4608ab, 0xc78e9a1d, 0x48b2b335, 0xc0b15502,
  0x5a43b190, 0xd6326a88, 0x13d4ae08, 0xc9edeb50, 0x45f704f7, 0xc04ee4b8, 0x5a6690ae, 0xd5052d97,
  0x0d47d096, 0xcc983f70, 0x43103085, 0xc013bc39, 0x5a7b7f1a, 0xd3de9156, 0x06a886a0, 0xcf89e3e8,
  0x40000000, 0xc0000000, 0x5a82799a, 0xd2bec333, 0x00000000, 0xd2bec333, 0x3cc85709, 0xc013bc39,
  0x5a7b7f1a, 0xd1a5ef90, 0xf9577960, 0xd6326a88, 0x396b3199, 0xc04ee4b8, 0x5a6690ae, 0xd09441bb,
  0xf2b82f6a, 0xd9e01006, 0x35eaa2c7, 0xc0b15502, 0x5a43b190, 0xcf89e3e8, 0xec2b51f8, 0xddc29958,
  0x3248d382, 0xc13ad060, 0x5a12e720, 0xce86ff2a, 0xe5b9f755, 0xe1d4a2c8, 0x2e88013a, 0xc1eb0209,
  0x59d438e5, 0xcd8bbb6d, 0xdf6d0fa1, 0xe61086bc, 0x2aaa7c7f, 0xc2c17d52, 0x5987b08a, 0xcc983f70,
  0xd94d586c, 0xea70658a, 0x26b2a794, 0xc3bdbdf6, 0x592d59da, 0xcbacb0bf, 0xd3635094, 0xeeee2d9d,
  0x22a2f4f8, 0xc4df2862, 0x58c542c5, 0xcac933ae, 0xcdb72c7e, 0xf383a3e2, 0x1e7de5df, 0xc6250a18,
  0x584f7b58, 0xc9edeb50, 0xc850cab4, 0xf82a6c6a, 0x1a4608ab, 0xc78e9a1d, 0x57cc15bc, 0xc91af976,
  0xc337a8f7, 0xfcdc1342, 0x15fdf758, 0xc91af976, 0x573b2635, 0xc8507ea7, 0xbe72d9df, 0x0192155f,
  0x11a855df, 0xcac933ae, 0x569cc31b, 0xc78e9a1d, 0xba08fb09, 0x0645e9af, 0x0d47d096, 0xcc983f70,
  0x55f104dc, 0xc6d569be, 0xb6002be9, 0x0af10a22, 0x08df1a8c, 0xce86ff2a, 0x553805f2, 0xc6250a18,
  0xb25e054b, 0x0f8cfcbe, 0x0470ebdc, 0xd09441bb, 0x5471e2e6, 0xc57d965d, 0xaf279193, 0x14135c94,
  0x00000000, 0xd2bec333, 0x539eba45, 0xc4df2862, 0xac6145bb, 0x187de2a7, 0xfb8f1424, 0xd5052d97,
  0x52beac9f, 0xc449d892, 0xaa0efb24, 0x1cc66e99, 0xf720e574, 0xd76619b6, 0x51d1dc80, 0xc3bdbdf6,
  0xa833ea44, 0x20e70f32, 0xf2b82f6a, 0xd9e01006, 0x50d86e6d, 0xc33aee27, 0xa6d2a626, 0x24da0a9a,
  0xee57aa21, 0xdc71898d, 0x4fd288dc, 0xc2c17d52, 0xa5ed18e0, 0x2899e64a, 0xea0208a8, 0xdf18f0ce,
  0x4ec05432, 0xc2517e31, 0xa58480e6, 0x2c216eaa, 0xe5b9f755, 0xe1d4a2c8, 0x4da1fab5, 0xc1eb0209,
  0xa5996f52, 0x2f6bbe45, 0xe1821a21, 0xe4a2eff6, 0x4c77a88e, 0xc18e18a7, 0xa62bc71b, 0x32744493,
  0xdd5d0b08, 0xe7821d59, 0x4b418bbe, 0xc13ad060, 0xa73abd3b, 0x3536cc52, 0xd94d586c, 0xea70658a,
  0x49ffd417, 0xc0f1360b, 0xa8c4d9cb, 0x37af8159, 0xd5558381, 0xed6bf9d1, 0x48b2b335, 0xc0b15502,
  0xaac7fa0e, 0x39daf5e8, 0xd177fec6, 0xf0730342, 0x475a5c77, 0xc07b371e, 0xad415361, 0x3bb6276e,
  0xcdb72c7e, 0xf383a3e2, 0x45f704f7, 0xc04ee4b8, 0xb02d7724, 0x3d3e82ae, 0xca155d39, 0xf69bf7c9,
  0x4488e37f, 0xc02c64a6, 0xb3885772, 0x3e71e759, 0xc694ce67, 0xf9ba1651, 0x43103085, 0xc013bc39,
  0xb74d4ccb, 0x3f4eaafe, 0xc337a8f7, 0xfcdc1342, 0x418d2621, 0xc004ef3f, 0xbb771c81, 0x3fd39b5a,
};
//}}}

/* bit reverse tables for FFT */
const int bitrevtabOffset[NUM_IMDCT_SIZES] = {0, 17};
//{{{
const unsigned char bitrevtab[17 + 129]  = {
/* nfft = 64 */
0x01, 0x08, 0x02, 0x04, 0x03, 0x0c, 0x05, 0x0a, 0x07, 0x0e, 0x0b, 0x0d, 0x00, 0x06, 0x09, 0x0f,
0x00,

/* nfft = 512 */
0x01, 0x40, 0x02, 0x20, 0x03, 0x60, 0x04, 0x10, 0x05, 0x50, 0x06, 0x30, 0x07, 0x70, 0x09, 0x48,
0x0a, 0x28, 0x0b, 0x68, 0x0c, 0x18, 0x0d, 0x58, 0x0e, 0x38, 0x0f, 0x78, 0x11, 0x44, 0x12, 0x24,
0x13, 0x64, 0x15, 0x54, 0x16, 0x34, 0x17, 0x74, 0x19, 0x4c, 0x1a, 0x2c, 0x1b, 0x6c, 0x1d, 0x5c,
0x1e, 0x3c, 0x1f, 0x7c, 0x21, 0x42, 0x23, 0x62, 0x25, 0x52, 0x26, 0x32, 0x27, 0x72, 0x29, 0x4a,
0x2b, 0x6a, 0x2d, 0x5a, 0x2e, 0x3a, 0x2f, 0x7a, 0x31, 0x46, 0x33, 0x66, 0x35, 0x56, 0x37, 0x76,
0x39, 0x4e, 0x3b, 0x6e, 0x3d, 0x5e, 0x3f, 0x7e, 0x43, 0x61, 0x45, 0x51, 0x47, 0x71, 0x4b, 0x69,
0x4d, 0x59, 0x4f, 0x79, 0x53, 0x65, 0x57, 0x75, 0x5b, 0x6d, 0x5f, 0x7d, 0x67, 0x73, 0x6f, 0x7b,
0x00, 0x08, 0x14, 0x1c, 0x22, 0x2a, 0x36, 0x3e, 0x41, 0x49, 0x55, 0x5d, 0x63, 0x6b, 0x77, 0x7f,
0x00,

};
//}}}
const unsigned char uniqueIDTab[8] = {0x5f, 0x4b, 0x43, 0x5f, 0x5f, 0x4a, 0x52, 0x5f};

//{{{
/**************************************************************************************
 * Function:    BitReverse
 *
 * Description: Ken's fast in-place bit reverse, using super-small table
 *
 * Inputs:      buffer of samples
 *              table index (for transform size)
 *
 * Outputs:     bit-reversed samples in same buffer
 *
 * Return:      none
 **************************************************************************************/
 static void BitReverse (int* inout, int tabidx)
{
  int *part0, *part1;
  int a,b, t,t1;
  const unsigned char* tab = bitrevtab + bitrevtabOffset[tabidx];
  int nbits = nfftlog2Tab[tabidx];

  part0 = inout;
  part1 = inout + (1 << nbits);

  while ((a = (*(const uint8_t*)(tab++))) != 0) {
    b = (*(const uint8_t*)(tab++));
    swapcplx(part0[4*a+0], part0[4*b+0]); /* 0xxx0 <-> 0yyy0 */
    swapcplx(part0[4*a+2], part1[4*b+0]); /* 0xxx1 <-> 1yyy0 */
    swapcplx(part1[4*a+0], part0[4*b+2]); /* 1xxx0 <-> 0yyy1 */
    swapcplx(part1[4*a+2], part1[4*b+2]); /* 1xxx1 <-> 1yyy1 */
    }

  do {
    swapcplx(part0[4*a+2], part1[4*a+0]); /* 0xxx1 <-> 1xxx0 */
    } while ((a =(*(const uint8_t*)(tab++))) != 0);
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R4FirstPass
 *
 * Description: radix-4 trivial pass for decimation-in-time FFT
 *
 * Inputs:      buffer of (bit-reversed) samples
 *              number of R4 butterflies per group (i.e. nfft / 4)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 2 guard bits, gains no integer bits,
 *                guard bits out = guard bits in - 2
 **************************************************************************************/
 static void R4FirstPass (int* x, int bg)
{
    int ar, ai, br, bi, cr, ci, dr, di;

  for (; bg != 0; bg--) {

    ar = x[0] + x[2];
    br = x[0] - x[2];
    ai = x[1] + x[3];
    bi = x[1] - x[3];
    cr = x[4] + x[6];
    dr = x[4] - x[6];
    ci = x[5] + x[7];
    di = x[5] - x[7];

    /* max per-sample gain = 4.0 (adding 4 inputs together) */
    x[0] = ar + cr;
    x[4] = ar - cr;
    x[1] = ai + ci;
    x[5] = ai - ci;
    x[2] = br + di;
    x[6] = br - di;
    x[3] = bi - dr;
    x[7] = bi + dr;

    x += 8;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R8FirstPass
 *
 * Description: radix-8 trivial pass for decimation-in-time FFT
 *
 * Inputs:      buffer of (bit-reversed) samples
 *              number of R8 butterflies per group (i.e. nfft / 8)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 3 guard bits, gains 1 integer bit
 *              guard bits out = guard bits in - 3 (if inputs are full scale)
 *                or guard bits in - 2 (if inputs bounded to +/- sqrt(2)/2)
 *              see scaling comments in code
 **************************************************************************************/
 static void R8FirstPass (int* x, int bg)
{
    int ar, ai, br, bi, cr, ci, dr, di;
  int sr, si, tr, ti, ur, ui, vr, vi;
  int wr, wi, xr, xi, yr, yi, zr, zi;

  for (; bg != 0; bg--) {

    ar = x[0] + x[2];
    br = x[0] - x[2];
    ai = x[1] + x[3];
    bi = x[1] - x[3];
    cr = x[4] + x[6];
    dr = x[4] - x[6];
    ci = x[5] + x[7];
    di = x[5] - x[7];

    sr = ar + cr;
    ur = ar - cr;
    si = ai + ci;
    ui = ai - ci;
    tr = br - di;
    vr = br + di;
    ti = bi + dr;
    vi = bi - dr;

    ar = x[ 8] + x[10];
    br = x[ 8] - x[10];
    ai = x[ 9] + x[11];
    bi = x[ 9] - x[11];
    cr = x[12] + x[14];
    dr = x[12] - x[14];
    ci = x[13] + x[15];
    di = x[13] - x[15];

    /* max gain of wr/wi/yr/yi vs input = 2
     *  (sum of 4 samples >> 1)
     */
    wr = (ar + cr) >> 1;
    yr = (ar - cr) >> 1;
    wi = (ai + ci) >> 1;
    yi = (ai - ci) >> 1;

    /* max gain of output vs input = 4
     *  (sum of 4 samples >> 1 + sum of 4 samples >> 1)
     */
    x[ 0] = (sr >> 1) + wr;
    x[ 8] = (sr >> 1) - wr;
    x[ 1] = (si >> 1) + wi;
    x[ 9] = (si >> 1) - wi;
    x[ 4] = (ur >> 1) + yi;
    x[12] = (ur >> 1) - yi;
    x[ 5] = (ui >> 1) - yr;
    x[13] = (ui >> 1) + yr;

    ar = br - di;
    cr = br + di;
    ai = bi + dr;
    ci = bi - dr;

    /* max gain of xr/xi/zr/zi vs input = 4*sqrt(2)/2 = 2*sqrt(2)
     *  (sum of 8 samples, multiply by sqrt(2)/2, implicit >> 1 from Q31)
     */
    xr = MULSHIFT32(SQRT1_2, ar - ai);
    xi = MULSHIFT32(SQRT1_2, ar + ai);
    zr = MULSHIFT32(SQRT1_2, cr - ci);
    zi = MULSHIFT32(SQRT1_2, cr + ci);

    /* max gain of output vs input = (2 + 2*sqrt(2) ~= 4.83)
     *  (sum of 4 samples >> 1, plus xr/xi/zr/zi with gain of 2*sqrt(2))
     * in absolute terms, we have max gain of appx 9.656 (4 + 0.707*8)
     *  but we also gain 1 int bit (from MULSHIFT32 or from explicit >> 1)
     */
    x[ 6] = (tr >> 1) - xr;
    x[14] = (tr >> 1) + xr;
    x[ 7] = (ti >> 1) - xi;
    x[15] = (ti >> 1) + xi;
    x[ 2] = (vr >> 1) + zi;
    x[10] = (vr >> 1) - zi;
    x[ 3] = (vi >> 1) - zr;
    x[11] = (vi >> 1) + zr;

    x += 16;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R4Core
 *
 * Description: radix-4 pass for decimation-in-time FFT
 *
 * Inputs:      buffer of samples
 *              number of R4 butterflies per group
 *              number of R4 groups per pass
 *              pointer to twiddle factors tables
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       gain 2 integer bits per pass (see scaling comments in code)
 *              min 1 GB in
 *              gbOut = gbIn - 1 (short block) or gbIn - 2 (long block)
 *              uses 3-mul, 3-add butterflies instead of 4-mul, 2-add
 **************************************************************************************/
 static void R4Core (int* x, int bg, int gp, int* wtab)
{
  int ar, ai, br, bi, cr, ci, dr, di, tr, ti;
  int wd, ws, wi;
  int i, j, step;
  int *xptr, *wptr;

  for (; bg != 0; gp <<= 2, bg >>= 2) {

    step = 2*gp;
    xptr = x;

    /* max per-sample gain, per group < 1 + 3*sqrt(2) ~= 5.25 if inputs x are full-scale
     * do 3 groups for long block, 2 groups for short block (gain 2 int bits per group)
     *
     * very conservative scaling:
     *   group 1: max gain = 5.25,           int bits gained = 2, gb used = 1 (2^3 = 8)
     *   group 2: max gain = 5.25^2 = 27.6,  int bits gained = 4, gb used = 1 (2^5 = 32)
     *   group 3: max gain = 5.25^3 = 144.7, int bits gained = 6, gb used = 2 (2^8 = 256)
     */
    for (i = bg; i != 0; i--) {

      wptr = wtab;

      for (j = gp; j != 0; j--) {

        ar = xptr[0];
        ai = xptr[1];
        xptr += step;

        /* gain 2 int bits for br/bi, cr/ci, dr/di (MULSHIFT32 by Q30)
         * gain 1 net GB
         */
        ws = wptr[0];
        wi = wptr[1];
        br = xptr[0];
        bi = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, br + bi);
        br = MULSHIFT32(wd, br) - tr; /* cos*br + sin*bi */
        bi = MULSHIFT32(ws, bi) + tr; /* cos*bi - sin*br */
        xptr += step;

        ws = wptr[2];
        wi = wptr[3];
        cr = xptr[0];
        ci = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, cr + ci);
        cr = MULSHIFT32(wd, cr) - tr;
        ci = MULSHIFT32(ws, ci) + tr;
        xptr += step;

        ws = wptr[4];
        wi = wptr[5];
        dr = xptr[0];
        di = xptr[1];
        wd = ws + 2*wi;
        tr = MULSHIFT32(wi, dr + di);
        dr = MULSHIFT32(wd, dr) - tr;
        di = MULSHIFT32(ws, di) + tr;
        wptr += 6;

        tr = ar;
        ti = ai;
        ar = (tr >> 2) - br;
        ai = (ti >> 2) - bi;
        br = (tr >> 2) + br;
        bi = (ti >> 2) + bi;

        tr = cr;
        ti = ci;
        cr = tr + dr;
        ci = di - ti;
        dr = tr - dr;
        di = di + ti;

        xptr[0] = ar + ci;
        xptr[1] = ai + dr;
        xptr -= step;
        xptr[0] = br - cr;
        xptr[1] = bi - di;
        xptr -= step;
        xptr[0] = ar - ci;
        xptr[1] = ai - dr;
        xptr -= step;
        xptr[0] = br + cr;
        xptr[1] = bi + di;
        xptr += 2;
      }
      xptr += 3*step;
    }
    wtab += 3*step;
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    R4FFT
 *
 * Description: Ken's very fast in-place radix-4 decimation-in-time FFT
 *
 * Inputs:      table index (for transform size)
 *              buffer of samples (non bit-reversed)
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       assumes 5 guard bits in for nfft <= 512
 *              gbOut = gbIn - 4 (assuming input is from PreMultiply)
 *              gains log2(nfft) - 2 int bits total
 *                so gain 7 int bits (LONG), 4 int bits (SHORT)
 **************************************************************************************/
void R4FFT (int tabidx, int* x)
{
  int order = nfftlog2Tab[tabidx];
  int nfft = nfftTab[tabidx];

  /* decimation in time */
  BitReverse(x, tabidx);

  if (order & 0x1) {
    /* long block: order = 9, nfft = 512 */
    R8FirstPass(x, nfft >> 3);            /* gain 1 int bit,  lose 2 GB */
    R4Core(x, nfft >> 5, 8, (int *)twidTabOdd);   /* gain 6 int bits, lose 2 GB */
  } else {
    /* short block: order = 6, nfft = 64 */
    R4FirstPass(x, nfft >> 2);            /* gain 0 int bits, lose 2 GB */
    R4Core(x, nfft >> 4, 4, (int *)twidTabEven);  /* gain 4 int bits, lose 1 GB */
  }
}
//}}}

// imdct
#define RND_VAL   (1 << (FBITS_OUT_IMDCT-1))
//{{{
/**************************************************************************************
 * Function:    IMDCT
 *
 * Description: inverse transform and convert to 16-bit PCM
 *
 * Inputs:      valid AACDecInfo struct
 *              index of current channel (0 for SCE/LFE, 0 or 1 for CPE)
 *              output channel (range = [0, nChans-1])
 *
 * Outputs:     complete frame of decoded PCM, after inverse transform
 *
 * Return:      0 if successful, -1 if error
 *
 * Notes:       If AAC_ENABLE_SBR is defined at compile time then window + overlap
 *                does NOT clip to 16-bit PCM and does NOT interleave channels
 *              If AAC_ENABLE_SBR is NOT defined at compile time, then window + overlap
 *                does clip to 16-bit PCM and interleaves channels
 *              If SBR is enabled at compile time, but we don't know whether it is
 *                actually used for this frame (e.g. the first frame of a stream),
 *                we need to produce both clipped 16-bit PCM in outbuf AND
 *                unclipped 32-bit PCM in the SBR input buffer. In this case we make
 *                a separate pass over the 32-bit PCM to produce 16-bit PCM output.
 *                This inflicts a slight performance hit when decoding non-SBR files.
 **************************************************************************************/
int IMDCT (AACDecInfo *aacDecInfo, int ch, int chOut, short *outbuf)
{
  int i;
  PSInfoBase *psi;
  ICSInfo *icsInfo;

  /* validate pointers */
  if (!aacDecInfo || !aacDecInfo->psInfoBase)
    return -1;
  psi = (PSInfoBase *)(aacDecInfo->psInfoBase);
  icsInfo = (ch == 1 && psi->commonWin == 1) ? &(psi->icsInfo[0]) : &(psi->icsInfo[ch]);
  outbuf += chOut;

  /* optimized type-IV DCT (operates inplace) */
  if (icsInfo->winSequence == 2) {
    /* 8 short blocks */
    for (i = 0; i < 8; i++)
      DCT4(0, psi->coef[ch] + i*128, psi->gbCurrent[ch]);
  } else {
    /* 1 long block */
    DCT4(1, psi->coef[ch], psi->gbCurrent[ch]);
  }

  /* window, overlap-add, don't clip to short (send to SBR decoder)
   * store the decoded 32-bit samples in top half (second AAC_MAX_NSAMPS samples) of coef buffer
   */
  if (icsInfo->winSequence == 0)
    DecWindowOverlapNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 1)
    DecWindowOverlapLongStartNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 2)
    DecWindowOverlapShortNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);
  else if (icsInfo->winSequence == 3)
    DecWindowOverlapLongStopNoClip(psi->coef[ch], psi->overlap[chOut], psi->sbrWorkBuf[ch], icsInfo->winShape, psi->prevWinShape[chOut]);

  if (!aacDecInfo->sbrEnabled) {
    for (i = 0; i < AAC_MAX_NSAMPS; i++) {
      *outbuf = CLIPTOSHORT((psi->sbrWorkBuf[ch][i] + RND_VAL) >> FBITS_OUT_IMDCT);
      outbuf += aacDecInfo->nChans;
    }
  }

  aacDecInfo->rawSampleBuf[ch] = psi->sbrWorkBuf[ch];
  aacDecInfo->rawSampleBytes = sizeof(int);
  aacDecInfo->rawSampleFBits = FBITS_OUT_IMDCT;

  psi->prevWinShape[chOut] = icsInfo->winShape;

  return 0;
}
//}}}

// dct4
static const int nmdctTab[NUM_IMDCT_SIZES] = {128, 1024};
static const int postSkip[NUM_IMDCT_SIZES] = {15, 1};

//{{{
/**************************************************************************************
 * Function:    PreMultiply
 *
 * Description: pre-twiddle stage of DCT4
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       minimum 1 GB in, 2 GB out, gains 5 (short) or 8 (long) frac bits
 *              i.e. gains 2-7= -5 int bits (short) or 2-10 = -8 int bits (long)
 *              normalization by -1/N is rolled into tables here (see trigtabs.c)
 *              uses 3-mul, 3-add butterflies instead of 4-mul, 2-add
 **************************************************************************************/
static void PreMultiply (int tabidx, int* zbuf1)
{
  int i, nmdct, ar1, ai1, ar2, ai2, z1, z2;
  int t, cms2, cps2a, sin2a, cps2b, sin2b;
  int *zbuf2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  zbuf2 = zbuf1 + nmdct - 1;
  csptr = cos4sin4tab + cos4sin4tabOffset[tabidx];

  /* whole thing should fit in registers - verify that compiler does this */
  for (i = nmdct >> 2; i != 0; i--) {
    /* cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin) */
    cps2a = *csptr++;
    sin2a = *csptr++;
    cps2b = *csptr++;
    sin2b = *csptr++;

    ar1 = *(zbuf1 + 0);
    ai2 = *(zbuf1 + 1);
    ai1 = *(zbuf2 + 0);
    ar2 = *(zbuf2 - 1);

    /* gain 2 ints bit from MULSHIFT32 by Q30, but drop 7 or 10 int bits from table scaling of 1/M
     * max per-sample gain (ignoring implicit scaling) = MAX(sin(angle)+cos(angle)) = 1.414
     * i.e. gain 1 GB since worst case is sin(angle) = cos(angle) = 0.707 (Q30), gain 2 from
     *   extra sign bits, and eat one in adding
     */
    t  = MULSHIFT32(sin2a, ar1 + ai1);
    z2 = MULSHIFT32(cps2a, ai1) - t;
    cms2 = cps2a - 2*sin2a;
    z1 = MULSHIFT32(cms2, ar1) + t;
    *zbuf1++ = z1;  /* cos*ar1 + sin*ai1 */
    *zbuf1++ = z2;  /* cos*ai1 - sin*ar1 */

    t  = MULSHIFT32(sin2b, ar2 + ai2);
    z2 = MULSHIFT32(cps2b, ai2) - t;
    cms2 = cps2b - 2*sin2b;
    z1 = MULSHIFT32(cms2, ar2) + t;
    *zbuf2-- = z2;  /* cos*ai2 - sin*ar2 */
    *zbuf2-- = z1;  /* cos*ar2 + sin*ai2 */
  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    PostMultiply
 *
 * Description: post-twiddle stage of DCT4
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       minimum 1 GB in, 2 GB out - gains 2 int bits
 *              uses 3-mul, 3-add butterflies instead of 4-mul, 2-add
 **************************************************************************************/
static void PostMultiply (int tabidx, int* fft1)
{
  int i, nmdct, ar1, ai1, ar2, ai2, skipFactor;
  int t, cms2, cps2, sin2;
  int *fft2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  csptr = cos1sin1tab;
  skipFactor = postSkip[tabidx];
  fft2 = fft1 + nmdct - 1;

  /* load coeffs for first pass
   * cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin)
   */
  cps2 = *csptr++;
  sin2 = *csptr;
  csptr += skipFactor;
  cms2 = cps2 - 2*sin2;

  for (i = nmdct >> 2; i != 0; i--) {
    ar1 = *(fft1 + 0);
    ai1 = *(fft1 + 1);
    ar2 = *(fft2 - 1);
    ai2 = *(fft2 + 0);

    /* gain 2 ints bit from MULSHIFT32 by Q30
     * max per-sample gain = MAX(sin(angle)+cos(angle)) = 1.414
     * i.e. gain 1 GB since worst case is sin(angle) = cos(angle) = 0.707 (Q30), gain 2 from
     *   extra sign bits, and eat one in adding
     */
    t = MULSHIFT32(sin2, ar1 + ai1);
    *fft2-- = t - MULSHIFT32(cps2, ai1);  /* sin*ar1 - cos*ai1 */
    *fft1++ = t + MULSHIFT32(cms2, ar1);  /* cos*ar1 + sin*ai1 */
    cps2 = *csptr++;
    sin2 = *csptr;
    csptr += skipFactor;

    ai2 = -ai2;
    t = MULSHIFT32(sin2, ar2 + ai2);
    *fft2-- = t - MULSHIFT32(cps2, ai2);  /* sin*ar1 - cos*ai1 */
    cms2 = cps2 - 2*sin2;
    *fft1++ = t + MULSHIFT32(cms2, ar2);  /* cos*ar1 + sin*ai1 */
  }
}
//}}}

//{{{
/**************************************************************************************
 * Function:    PreMultiplyRescale
 *
 * Description: pre-twiddle stage of DCT4, with rescaling for extra guard bits
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *              number of guard bits to add to input before processing
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       see notes on PreMultiply(), above
 **************************************************************************************/
 static void PreMultiplyRescale (int tabidx, int* zbuf1, int es)
{
  int i, nmdct, ar1, ai1, ar2, ai2, z1, z2;
  int t, cms2, cps2a, sin2a, cps2b, sin2b;
  int *zbuf2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  zbuf2 = zbuf1 + nmdct - 1;
  csptr = cos4sin4tab + cos4sin4tabOffset[tabidx];

  /* whole thing should fit in registers - verify that compiler does this */
  for (i = nmdct >> 2; i != 0; i--) {
    /* cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin) */
    cps2a = *csptr++;
    sin2a = *csptr++;
    cps2b = *csptr++;
    sin2b = *csptr++;

    ar1 = *(zbuf1 + 0) >> es;
    ai1 = *(zbuf2 + 0) >> es;
    ai2 = *(zbuf1 + 1) >> es;

    t  = MULSHIFT32(sin2a, ar1 + ai1);
    z2 = MULSHIFT32(cps2a, ai1) - t;
    cms2 = cps2a - 2*sin2a;
    z1 = MULSHIFT32(cms2, ar1) + t;
    *zbuf1++ = z1;
    *zbuf1++ = z2;

    ar2 = *(zbuf2 - 1) >> es; /* do here to free up register used for es */

    t  = MULSHIFT32(sin2b, ar2 + ai2);
    z2 = MULSHIFT32(cps2b, ai2) - t;
    cms2 = cps2b - 2*sin2b;
    z1 = MULSHIFT32(cms2, ar2) + t;
    *zbuf2-- = z2;
    *zbuf2-- = z1;

  }
}
//}}}
//{{{
/**************************************************************************************
 * Function:    PostMultiplyRescale
 *
 * Description: post-twiddle stage of DCT4, with rescaling for extra guard bits
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *              number of guard bits to remove from output
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       clips output to [-2^30, 2^30 - 1], guaranteeing at least 1 guard bit
 *              see notes on PostMultiply(), above
 **************************************************************************************/
 static void PostMultiplyRescale (int tabidx, int* fft1, int es)
{
  int i, nmdct, ar1, ai1, ar2, ai2, skipFactor, z;
  int t, cs2, sin2;
  int *fft2;
  const int *csptr;

  nmdct = nmdctTab[tabidx];
  csptr = cos1sin1tab;
  skipFactor = postSkip[tabidx];
  fft2 = fft1 + nmdct - 1;

  /* load coeffs for first pass
   * cps2 = (cos+sin), sin2 = sin, cms2 = (cos-sin)
   */
  cs2 = *csptr++;
  sin2 = *csptr;
  csptr += skipFactor;

  for (i = nmdct >> 2; i != 0; i--) {
    ar1 = *(fft1 + 0);
    ai1 = *(fft1 + 1);
    ai2 = *(fft2 + 0);

    t = MULSHIFT32(sin2, ar1 + ai1);
    z = t - MULSHIFT32(cs2, ai1);
    CLIP_2N_SHIFT(z, es);
    *fft2-- = z;
    cs2 -= 2*sin2;
    z = t + MULSHIFT32(cs2, ar1);
    CLIP_2N_SHIFT(z, es);
    *fft1++ = z;

    cs2 = *csptr++;
    sin2 = *csptr;
    csptr += skipFactor;

    ar2 = *fft2;
    ai2 = -ai2;
    t = MULSHIFT32(sin2, ar2 + ai2);
    z = t - MULSHIFT32(cs2, ai2);
    CLIP_2N_SHIFT(z, es);
    *fft2-- = z;
    cs2 -= 2*sin2;
    z = t + MULSHIFT32(cs2, ar2);
    CLIP_2N_SHIFT(z, es);
    *fft1++ = z;
    cs2 += 2*sin2;
  }
}
//}}}

//{{{
/**************************************************************************************
 * Function:    DCT4
 *
 * Description: type-IV DCT
 *
 * Inputs:      table index (for transform size)
 *              buffer of nmdct samples
 *              number of guard bits in the input buffer
 *
 * Outputs:     processed samples in same buffer
 *
 * Return:      none
 *
 * Notes:       operates in-place
 *              if number of guard bits in input is < GBITS_IN_DCT4, the input is
 *                scaled (>>) before the DCT4 and rescaled (<<, with clipping) after
 *                the DCT4 (rare)
 *              the output has FBITS_LOST_DCT4 fewer fraction bits than the input
 *              the output will always have at least 1 guard bit (GBITS_IN_DCT4 >= 4)
 *              int bits gained per stage (PreMul + FFT + PostMul)
 *                 short blocks = (-5 + 4 + 2) = 1 total
 *                 long blocks =  (-8 + 7 + 2) = 1 total
 **************************************************************************************/
void DCT4 (int tabidx, int *coef, int gb)
{
  int es;

  /* fast in-place DCT-IV - adds guard bits if necessary */
  if (gb < GBITS_IN_DCT4) {
    es = GBITS_IN_DCT4 - gb;
    PreMultiplyRescale(tabidx, coef, es);
    R4FFT(tabidx, coef);
    PostMultiplyRescale(tabidx, coef, es);
  } else {
    PreMultiply(tabidx, coef);
    R4FFT(tabidx, coef);
    PostMultiply(tabidx, coef);
  }
}
//}}}
