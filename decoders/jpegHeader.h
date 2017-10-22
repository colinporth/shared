// jpegHeader.h - portable jpeg header
#pragma once

//{{{
static int jfifApp0Marker (BYTE* ptr) {

  *ptr++ = 0xFF; // APP0 marker
  *ptr++ = 0xE0;

  int length = 16;
  *ptr++ = length >> 8;// length field
  *ptr++ = length & 0xFF;

  *ptr++ = 0x4A; // JFIF identifier
  *ptr++ = 0x46;
  *ptr++ = 0x49;
  *ptr++ = 0x46;
  *ptr++ = 0x00;

  *ptr++ = 0x01; // version
  *ptr++ = 0x02;

  *ptr++ = 0x00; // units
  *ptr++ = 0x00; // X density
  *ptr++ = 0x01;
  *ptr++ = 0x00; // Y density
  *ptr++ = 0x01;

  *ptr++ = 0x00; // X thumbnail
  *ptr++ = 0x00; // Y thumbnail

  return length+2;
  }
//}}}
//{{{
static int sofMarker (BYTE* ptr, int width, int height) {

  *ptr++ = 0xFF; // start of frame: baseline DCT
  *ptr++ = 0xC0;

  int length = 17;
  *ptr++ = length >> 8;// length field
  *ptr++ = length & 0xFF;

  *ptr++ = 0x08; // sample precision

  *ptr++ = height >> 8; // number of lines
  *ptr++ = height & 0xFF;

  *ptr++ = width >> 8; // number of samples per line
  *ptr++ = width & 0xFF;

  *ptr++ = 0x03; // number of image components in frame

  *ptr++ = 0x00; // component identifier: Y
  *ptr++ = 0x21; // horizontal | vertical sampling factor: Y
  *ptr++ = 0x00; // quantization table selector: Y

  *ptr++ = 0x01; // component identifier: Cb
  *ptr++ = 0x11; // horizontal | vertical sampling factor: Cb
  *ptr++ = 0x01; // quantization table selector: Cb

  *ptr++ = 0x02; // component identifier: Cr
  *ptr++ = 0x11; // horizontal | vertical sampling factor: Cr
  *ptr++ = 0x01; // quantization table selector: Cr

  return length+2;
  }
//}}}

//{{{
static uint8_t JPEG_StdQuantTblY_ZZ[64] = {
  16, 11, 12, 14, 12, 10, 16, 14,
  13, 14, 18, 17, 16, 19, 24, 40,
  26, 24, 22, 22, 24, 49, 35, 37,
  29, 40, 58, 51, 61, 60, 57, 51,
  56, 55, 64, 72, 92, 78, 64, 68,
  87, 69, 55, 56, 80, 109, 81, 87,
  95, 98, 103, 104, 103, 62, 77, 113,
  121, 112, 100, 120, 92, 101, 103, 99
  };
//}}}
//{{{
static uint8_t JPEG_StdQuantTblC_ZZ[64] = {
  17, 18, 18, 24, 21, 24, 47, 26,
  26, 47, 99, 66, 56, 66, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99
  };
//}}}
//{{{
static int quantizationTableMarker (BYTE* ptr, int qscale) {

  *ptr++ = 0xFF;// define quantization table marker
  *ptr++ = 0xDB;

  int length = 132;
  *ptr++ = length >> 8;// length field
  *ptr++ = length & 0xFF;

  *ptr++ = 0;// quantization table precision | identifier
  for (int i = 0; i < 64; i++) {
    int q = (JPEG_StdQuantTblY_ZZ[i] * qscale + 16) >> 5;
    *ptr++ = q;// quantization table element
    }

  *ptr++ = 1;// quantization table precision | identifier
  for (int i = 0; i < 64; i++) {
    int q = (JPEG_StdQuantTblC_ZZ[i] * qscale + 16) >> 5;
    *ptr++ = q;// quantization table element
    }

  return length+2;
  }
//}}}

//{{{
static unsigned int JPEG_StdHuffmanTbl[384] = {
  0x100, 0x101, 0x204, 0x30b, 0x41a, 0x678, 0x7f8, 0x9f6,
  0xf82, 0xf83, 0x30c, 0x41b, 0x679, 0x8f6, 0xaf6, 0xf84,
  0xf85, 0xf86, 0xf87, 0xf88, 0x41c, 0x7f9, 0x9f7, 0xbf4,
  0xf89, 0xf8a, 0xf8b, 0xf8c, 0xf8d, 0xf8e, 0x53a, 0x8f7,
  0xbf5, 0xf8f, 0xf90, 0xf91, 0xf92, 0xf93, 0xf94, 0xf95,
  0x53b, 0x9f8, 0xf96, 0xf97, 0xf98, 0xf99, 0xf9a, 0xf9b,
  0xf9c, 0xf9d, 0x67a, 0xaf7, 0xf9e, 0xf9f, 0xfa0, 0xfa1,
  0xfa2, 0xfa3, 0xfa4, 0xfa5, 0x67b, 0xbf6, 0xfa6, 0xfa7,
  0xfa8, 0xfa9, 0xfaa, 0xfab, 0xfac, 0xfad, 0x7fa, 0xbf7,
  0xfae, 0xfaf, 0xfb0, 0xfb1, 0xfb2, 0xfb3, 0xfb4, 0xfb5,
  0x8f8, 0xec0, 0xfb6, 0xfb7, 0xfb8, 0xfb9, 0xfba, 0xfbb,
  0xfbc, 0xfbd, 0x8f9, 0xfbe, 0xfbf, 0xfc0, 0xfc1, 0xfc2,
  0xfc3, 0xfc4, 0xfc5, 0xfc6, 0x8fa, 0xfc7, 0xfc8, 0xfc9,
  0xfca, 0xfcb, 0xfcc, 0xfcd, 0xfce, 0xfcf, 0x9f9, 0xfd0,
  0xfd1, 0xfd2, 0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7, 0xfd8,
  0x9fa, 0xfd9, 0xfda, 0xfdb, 0xfdc, 0xfdd, 0xfde, 0xfdf,
  0xfe0, 0xfe1, 0xaf8, 0xfe2, 0xfe3, 0xfe4, 0xfe5, 0xfe6,
  0xfe7, 0xfe8, 0xfe9, 0xfea, 0xfeb, 0xfec, 0xfed, 0xfee,
  0xfef, 0xff0, 0xff1, 0xff2, 0xff3, 0xff4, 0xff5, 0xff6,
  0xff7, 0xff8, 0xff9, 0xffa, 0xffb, 0xffc, 0xffd, 0xffe,
  0x30a, 0xaf9, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
  0xfd0, 0xfd1, 0xfd2, 0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7,
  0x101, 0x204, 0x30a, 0x418, 0x419, 0x538, 0x678, 0x8f4,
  0x9f6, 0xbf4, 0x30b, 0x539, 0x7f6, 0x8f5, 0xaf6, 0xbf5,
  0xf88, 0xf89, 0xf8a, 0xf8b, 0x41a, 0x7f7, 0x9f7, 0xbf6,
  0xec2, 0xf8c, 0xf8d, 0xf8e, 0xf8f, 0xf90, 0x41b, 0x7f8,
  0x9f8, 0xbf7, 0xf91, 0xf92, 0xf93, 0xf94, 0xf95, 0xf96,
  0x53a, 0x8f6, 0xf97, 0xf98, 0xf99, 0xf9a, 0xf9b, 0xf9c,
  0xf9d, 0xf9e, 0x53b, 0x9f9, 0xf9f, 0xfa0, 0xfa1, 0xfa2,
  0xfa3, 0xfa4, 0xfa5, 0xfa6, 0x679, 0xaf7, 0xfa7, 0xfa8,
  0xfa9, 0xfaa, 0xfab, 0xfac, 0xfad, 0xfae, 0x67a, 0xaf8,
  0xfaf, 0xfb0, 0xfb1, 0xfb2, 0xfb3, 0xfb4, 0xfb5, 0xfb6,
  0x7f9, 0xfb7, 0xfb8, 0xfb9, 0xfba, 0xfbb, 0xfbc, 0xfbd,
  0xfbe, 0xfbf, 0x8f7, 0xfc0, 0xfc1, 0xfc2, 0xfc3, 0xfc4,
  0xfc5, 0xfc6, 0xfc7, 0xfc8, 0x8f8, 0xfc9, 0xfca, 0xfcb,
  0xfcc, 0xfcd, 0xfce, 0xfcf, 0xfd0, 0xfd1, 0x8f9, 0xfd2,
  0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7, 0xfd8, 0xfd9, 0xfda,
  0x8fa, 0xfdb, 0xfdc, 0xfdd, 0xfde, 0xfdf, 0xfe0, 0xfe1,
  0xfe2, 0xfe3, 0xaf9, 0xfe4, 0xfe5, 0xfe6, 0xfe7, 0xfe8,
  0xfe9, 0xfea, 0xfeb, 0xfec, 0xde0, 0xfed, 0xfee, 0xfef,
  0xff0, 0xff1, 0xff2, 0xff3, 0xff4, 0xff5, 0xec3, 0xff6,
  0xff7, 0xff8, 0xff9, 0xffa, 0xffb, 0xffc, 0xffd, 0xffe,
  0x100, 0x9fa, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
  0xfd0, 0xfd1, 0xfd2, 0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7,
  0x100, 0x202, 0x203, 0x204, 0x205, 0x206, 0x30e, 0x41e,
  0x53e, 0x67e, 0x7fe, 0x8fe, 0xfff, 0xfff, 0xfff, 0xfff,
  0x100, 0x101, 0x102, 0x206, 0x30e, 0x41e, 0x53e, 0x67e,
  0x7fe, 0x8fe, 0x9fe, 0xafe, 0xfff, 0xfff, 0xfff, 0xfff
  };
//}}}
//{{{
static int huffmanTableMarkerDC (BYTE* ptr, unsigned int* htable, int class_id) {

  *ptr++ = 0xFF; // define huffman table marker
  *ptr++ = 0xC4;

  int length = 19;
  BYTE* plength = ptr; // place holder for length field
  *ptr++;
  *ptr++;

  *ptr++ = class_id; // huffman table class | identifier
  for (int l = 0; l < 16; l++) {
    int count = 0;
    for (int i = 0; i < 12; i++) {
      if ((htable[i] >> 8) == l)
        count++;
      }
    *ptr++ = count; // number of huffman codes of length l+1
    }

  for (int l = 0; l < 16; l++) {
    for (int i = 0; i < 12; i++) {
      if ((htable[i] >> 8) == l) {
        *ptr++ = i; // HUFFVAL with huffman codes of length l+1
        length++;
        }
      }
    }

  *plength++ = length >> 8;// length field
  *plength = length & 0xFF;

  return length + 2;
  }
//}}}
//{{{
static int huffmanTableMarkerAC (BYTE* ptr, unsigned int* htable, int class_id) {

  *ptr++ = 0xFF; // define huffman table marker
  *ptr++ = 0xC4;

  int length = 19;
  BYTE* plength = ptr; // place holder for length field
  *ptr++;
  *ptr++;

  *ptr++ = class_id;// huffman table class | identifier
  for (int l = 0; l < 16; l++) {
    int count = 0;
    for (int i = 0; i < 162; i++) {
      if ((htable[i] >> 8) == l)
        count++;
      }
    *ptr++ = count; // number of huffman codes of length l+1
    }

  for (int l = 0; l < 16; l++) {
    // check EOB: 0|0
    if ((htable[160] >> 8) == l) {
      *ptr++ = 0; // HUFFVAL with huffman codes of length l+1
      length++;
      }

    // check HUFFVAL: 0|1 to E|A
    for (int i = 0; i < 150; i++) {
      if ((htable[i] >> 8) == l) {
        int a = i/10;
        int b = i%10;
        *ptr++ = (a<<4)|(b+1); // HUFFVAL with huffman codes of length l+1
        length++;
        }
      }

    // check ZRL: F|0
    if ((htable[161] >> 8) == l) {
      *ptr++ = 0xF0; // HUFFVAL with huffman codes of length l+1
      length++;
      }

    // check HUFFVAL: F|1 to F|A
    for (int i = 150; i < 160; i++) {
      if ((htable[i] >> 8) == l) {
        int a = i/10;
        int b = i%10;
        *ptr++ = (a<<4)|(b+1); // HUFFVAL with huffman codes of length l+1
        length++;
        }
      }
    }

  *plength++ = length >> 8; // length field
  *plength = length & 0xFF;

  return length + 2;
  }
//}}}

//{{{
static int driMarker (BYTE* ptr, int restartInterval) {

  *ptr++ = 0xFF;// define restart interval marker
  *ptr++ = 0xDD;

  int length = 4;
  *ptr++ = length >> 8;// length field
  *ptr++ = length & 0xFF;

  *ptr++ = restartInterval >> 8;// restart interval
  *ptr++ = restartInterval & 0xFF;

  return length + 2;
  }
//}}}
//{{{
static int sosMarker (BYTE* ptr) {

  *ptr++ = 0xFF;// start of scan
  *ptr++ = 0xDA;

  int length = 12;
  *ptr++ = length >> 8;// length field
  *ptr++ = length & 0xFF;

  *ptr++ = 0x03;// number of image components in scan
  *ptr++ = 0x00;// scan component selector: Y
  *ptr++ = 0x00;// DC | AC huffman table selector: Y
  *ptr++ = 0x01;// scan component selector: Cb
  *ptr++ = 0x11;// DC | AC huffman table selector: Cb
  *ptr++ = 0x02;// scan component selector: Cr
  *ptr++ = 0x11;// DC | AC huffman table selector: Cr

  *ptr++ = 0x00;// Ss: start of predictor selector
  *ptr++ = 0x3F;// Se: end of spectral selector
  *ptr++ = 0x00;// Ah | Al: successive approximation bit position

  return length+2;
  }
//}}}

//{{{
int setJpegHeader (BYTE* header, int width, int height, int restartInterval, int qscale) {

  // SOI marker
  BYTE* ptr = header;
  *ptr++ = 0xFF;
  *ptr++ = 0xD8;
  int length = 2;

  // JFIF APP0 marker
  length += jfifApp0Marker (ptr);
  ptr = header + length;

  // quantization tables
  length += quantizationTableMarker (ptr, qscale);
  ptr = header + length;

  // SOF start of frame marker
  length += sofMarker (ptr, width, height);
  ptr = header + length;

  // huffman Table DC0 for luma marker
  length += huffmanTableMarkerDC (ptr, &JPEG_StdHuffmanTbl[352], 0x00);
  ptr = header + length;

  // huffman Table AC0 for luma marker
  length += huffmanTableMarkerAC (ptr, &JPEG_StdHuffmanTbl[0], 0x10);
  ptr = header + length;

  // huffman Table DC1 for chroma marker
  length += huffmanTableMarkerDC (ptr, &JPEG_StdHuffmanTbl[368], 0x01);
  ptr = header + length;

  // huffman Table AC1 for chroma marker
  length += huffmanTableMarkerAC (ptr, &JPEG_StdHuffmanTbl[176], 0x11);
    ptr = header + length;

  // DRI restart interval marker
  if (restartInterval > 0) {
    length += driMarker (ptr, restartInterval);
    ptr = header + length;
    }

  // SOS start of scan marker
  return length + sosMarker (ptr);
  }
//}}}
