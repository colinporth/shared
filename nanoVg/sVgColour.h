// sVgColour.h
#pragma once

struct sVgColour {
  union {
    float rgba[4];
    struct {
      float r;
      float g;
      float b;
      float a;
      };
    };
  };

//{{{
inline sVgColour getRgbA32 (uint32_t colour) {

  sVgColour vgColour;
  vgColour.r = ((colour & 0xFF0000) >> 16) / 255.0f;
  vgColour.g = ((colour & 0xFF00) >> 8) / 255.0f;
  vgColour.b = (colour & 0xFF) / 255.0f;
  vgColour.a = (colour >> 24) / 255.0f;

  return vgColour;
  }
//}}}
//{{{
inline sVgColour getRgbA (uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

  sVgColour vgColour;
  vgColour.r = r / 255.0f;
  vgColour.g = g / 255.0f;
  vgColour.b = b / 255.0f;
  vgColour.a = a / 255.0f;
  return vgColour;
  }
//}}}
//{{{
inline sVgColour getRgb (uint8_t r, uint8_t g, uint8_t b) {
  return getRgbA (r,g,b,255);
  }
//}}}
//{{{
inline sVgColour getRgbAf (float r, float g, float b, float a) {
  sVgColour vgColour;
  vgColour.r = r;
  vgColour.g = g;
  vgColour.b = b;
  vgColour.a = a;
  return vgColour;
  }
//}}}
//{{{
inline sVgColour getRgbf (float r, float g, float b) {
  return getRgbAf (r,g,b,1.0f);
  }
//}}}
//{{{
inline sVgColour nvgPremulColour (sVgColour colour) {
  colour.r *= colour.a;
  colour.g *= colour.a;
  colour.b *= colour.a;
  return colour;
  }
//}}}

const sVgColour kVgWhite =      { 1.f,    1.f,    1.f,    1.f };
const sVgColour kVgLightGrey =  { 0.75f,  0.75f,  0.75f,  1.f };
const sVgColour kVgGrey =       { 0.5f,   0.5f,   0.5f,   1.f };
const sVgColour kVgDarkGrey =   { 0.25f,  0.25f,  0.25f,  1.f };
const sVgColour kVgDarkerGrey = { 0.125f, 0.125f, 0.125f, 1.f };
const sVgColour kVgBlack =      { 0.f,    0.f,    0.f,    1.f };

const sVgColour kVgBlue =       { 0.f, 0.f, 1.f, 1.f };
const sVgColour kVgGreen =      { 0.f, 1.f, 0.f, 1.f };
const sVgColour kVgRed =        { 1.f, 0.f, 0.f, 1.f };
const sVgColour kVgYellow =     { 1.f, 1.f, 0.f, 1.f };
