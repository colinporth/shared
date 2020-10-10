// cPoint.h
#pragma once

struct cPoint {
public:
  //{{{
  cPoint()  {
    x = 0;
    y = 0;
    }
  //}}}
  //{{{
  cPoint (const cPoint& p) {
    this->x = p.x;
    this->y = p.y;
    }
  //}}}
  //{{{
  cPoint (float x, float y) {
    this->x = x;
    this->y = y;
    }
  //}}}

  //{{{
  cPoint operator - (const cPoint& point) const {
    return cPoint (x - point.x, y - point.y);
    }
  //}}}
  //{{{
  cPoint operator + (const cPoint& point) const {
    return cPoint (x + point.x, y + point.y);
    }
  //}}}
  //{{{
  cPoint operator * (const float s) const {
    return cPoint (x * s, y * s);
    }
  //}}}
  //{{{
  cPoint operator / (const float s) const {
    return cPoint (x / s, y / s);
    }
  //}}}
  //{{{
  const cPoint& operator += (const cPoint& point)  {
    x += point.x;
    y += point.y;
    return *this;
    }
  //}}}
  //{{{
  const cPoint& operator -= (const cPoint& point)  {
    x -= point.x;
    y -= point.y;
    return *this;
    }
  //}}}
  //{{{
  const cPoint& operator *= (const float s)  {
    x *= s;
    y *= s;
    return *this;
    }
  //}}}
  //{{{
  const cPoint& operator /= (const float s)  {
    x /= s;
    y /= s;
    return *this;
    }
  //}}}

  //{{{
  bool equalWithinTolerance (cPoint p, float tolerance) {

    float dx = p.x - x;
    float dy = p.y - y;
    return dx*dx + dy*dy < tolerance*tolerance;
    }
  //}}}

  //{{{
  bool inside (const cPoint& pos) const {
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
  cPoint perp() {
    float mag = magnitude();
    return cPoint (-y / mag, x / mag);
    }
  //}}}

  float x;
  float y;
  };
