// sVgColour.h
#pragma once

struct sVgColour {
  sVgColour() {}
  //{{{
  sVgColour (float rValue, float gValue, float bValue, float aValue) {
    r = rValue;
    g = gValue;
    b = bValue;
    a = aValue;
    }
  //}}}
  //{{{
  sVgColour (float rValue, float gValue, float bValue) {
    r = rValue;
    g = gValue;
    b = bValue;
    a = 1.f;
    }
  //}}}
  //{{{
  sVgColour (uint32_t colour) {
    r = ((colour & 0xFF0000) >> 16) / 255.0f;
    g = ((colour & 0xFF00) >> 8) / 255.0f;
    b =  (colour & 0xFF) / 255.0f;
    a =  (colour >> 24) / 255.0f;
    }
  //}}}

  //{{{
  inline sVgColour getPremultipliedColour() {
    return sVgColour (r * a, g * a, b * a, a);
    }
  //}}}

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

const sVgColour kVgBlack =      { 0.f,    0.f,    0.f,    1.f };
const sVgColour kVgDarkerGrey = { 0.125f, 0.125f, 0.125f, 1.f };
const sVgColour kVgDarkGrey =   { 0.25f,  0.25f,  0.25f,  1.f };
const sVgColour kVgGrey =       { 0.5f,   0.5f,   0.5f,   1.f };
const sVgColour kVgLightGrey =  { 0.75f,  0.75f,  0.75f,  1.f };
const sVgColour kVgWhite =      { 1.f,    1.f,    1.f,    1.f };

const sVgColour kVgRed =        { 1.f, 0.f, 0.f, 1.f };
const sVgColour kVgGreen =      { 0.f, 1.f, 0.f, 1.f };
const sVgColour kVgBlue =       { 0.f, 0.f, 1.f, 1.f };
const sVgColour kVgYellow =     { 1.f, 1.f, 0.f, 1.f };
