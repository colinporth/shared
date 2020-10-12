// sVgColour.h
#pragma once

struct sColourF {
  sColourF() {}
  //{{{
  sColourF (float rValue, float gValue, float bValue, float aValue) {
    r = rValue;
    g = gValue;
    b = bValue;
    a = aValue;
    }
  //}}}
  //{{{
  sColourF (float rValue, float gValue, float bValue) {
    r = rValue;
    g = gValue;
    b = bValue;
    a = 1.f;
    }
  //}}}
  //{{{
  sColourF (uint32_t colour) {
    r = ((colour & 0xFF0000) >> 16) / 255.0f;
    g = ((colour & 0xFF00) >> 8) / 255.0f;
    b =  (colour & 0xFF) / 255.0f;
    a =  (colour >> 24) / 255.0f;
    }
  //}}}

  //{{{
  inline sColourF getPremultiplied() {
    return sColourF(r * a, g * a, b * a, a);
    }
  //}}}

  float r;
  float g;
  float b;
  float a;
  };

// greys
const sColourF kBlackF =      { 0.f,    0.f,    0.f,    1.f };
const sColourF kDarkerGreyF = { 0.125f, 0.125f, 0.125f, 1.f };
const sColourF kDarkGreyF =   { 0.25f,  0.25f,  0.25f,  1.f };
const sColourF kGreyF =       { 0.5f,   0.5f,   0.5f,   1.f };
const sColourF kLightGreyF =  { 0.75f,  0.75f,  0.75f,  1.f };
const sColourF kWhiteF =      { 1.f,    1.f,    1.f,    1.f };

// colours
const sColourF kRedF =        { 1.f,  0.f,   0.f,  1.f };
const sColourF kDarkRedF =    { 0.5f, 0.f,   0.f,  1.f };
const sColourF kLightRedF =   { 1.f,  0.5f,  0.5f, 1.f };
const sColourF kGreenF =      { 0.f,  1.f,   0.f,  1.f };
const sColourF kDarkGreenF =  { 0.f,  0.75f, 0.f,  1.f };
const sColourF kBlueF =       { 0.f,  0.f,   1.f,  1.f };
const sColourF kYellowF =     { 1.f,  1.f,   0.f,  1.f };
const sColourF kDarkOrangeF = { 0.6f, 0.6f,  0.f,  1.f };

// transparent colours
const sColourF kOpaqueBlackF =      { 0.f,  0.f,  0.f,  0.f };
const sColourF kSemiOpaqueBlackF =  { 0.f,  0.f,  0.f,  0.5f };
const sColourF kSemiOpaqueWhiteF =  { 1.f,  1.f,  1.f,  0.5f };
