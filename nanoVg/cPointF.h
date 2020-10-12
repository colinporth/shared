// cPointF.h
#pragma once
#include <math.h>

struct cPointF {
public:
  //{{{
  cPointF()  {
    x = 0;
    y = 0;
    }
  //}}}
  //{{{
  cPointF (const cPointF& p) {
    this->x = p.x;
    this->y = p.y;
    }
  //}}}
  //{{{
  cPointF (float x, float y) {
    this->x = x;
    this->y = y;
    }
  //}}}

  //{{{
  cPointF operator - (const cPointF& point) const {
    return cPointF (x - point.x, y - point.y);
    }
  //}}}
  //{{{
  cPointF operator + (const cPointF& point) const {
    return cPointF (x + point.x, y + point.y);
    }
  //}}}
  //{{{
  cPointF operator * (const float s) const {
    return cPointF (x * s, y * s);
    }
  //}}}
  //{{{
  cPointF operator * (const cPointF& scale) const {
    return cPointF (x * scale.x, y * scale.y);
    }
  //}}}
  //{{{
  cPointF operator / (const float s) const {
    return cPointF (x / s, y / s);
    }
  //}}}
  //{{{
  const cPointF& operator += (const cPointF& point)  {
    x += point.x;
    y += point.y;
    return *this;
    }
  //}}}
  //{{{
  const cPointF& operator -= (const cPointF& point)  {
    x -= point.x;
    y -= point.y;
    return *this;
    }
  //}}}
  //{{{
  const cPointF& operator *= (const float s)  {
    x *= s;
    y *= s;
    return *this;
    }
  //}}}
  //{{{
  const cPointF& operator *= (const cPointF& scale)  {
    x *= scale.x;
    y *= scale.y;
    return *this;
    }
  //}}}
  //{{{
  const cPointF& operator /= (const float s)  {
    x /= s;
    y /= s;
    return *this;
    }
  //}}}

  //{{{
  bool equalWithinTolerance (cPointF p, float tolerance) {

    float dx = p.x - x;
    float dy = p.y - y;
    return dx*dx + dy*dy < tolerance*tolerance;
    }
  //}}}

  //{{{
  bool inside (const cPointF& pos) const {
  // return pos inside rect formed by us as size
    return pos.x >= 0 && pos.x < x && pos.y >= 0 && pos.y < y;
    }
  //}}}
  //{{{
  float magnitude() const {
  // return magnitude of point as vector
    return sqrtf ((x*x) + (y*y));
    }
  //}}}
  //{{{
  cPointF perp() {
    float mag = magnitude();
    return cPointF (-y / mag, x / mag);
    }
  //}}}

  float x;
  float y;
  };
