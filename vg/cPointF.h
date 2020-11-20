// cPointF.h
#pragma once
#include <math.h>
#include "../fmt/format.h"

class cPointF {
public:
  //{{{
  cPointF()  {
    x = 0.f;
    y = 0.f;
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
    return pos.x >= 0.f && pos.x < x && pos.y >= 0.f && pos.y < y;
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

  //{{{
  cPointF min (cPointF point) {
    return cPointF (std::min (x, point.x), std::max (y, point.y));
    }
  //}}}
  //{{{
  cPointF max (cPointF point) {
    return cPointF (std::max (x, point.x), std::max (y, point.y));
    }
  //}}}
  //{{{
  cPointF max (cPointF point1, cPointF point2) {
    return cPointF (std::max (point1.x, point2.x), std::max (point1.y, point2.y));
    }
  //}}}

  float x;
  float y;
  };

//{{{
template <> struct fmt::formatter<cPointF> {

  char presentation = 'f';

  // Parses format specifications of the form ['f' | 'e'].
  constexpr auto parse (format_parse_context& context) {
    // [context.begin(), context.end()) is a character range that contains a part of
    // the format string starting from the format specifications to be parsed,
    // e.g. in
    //   fmt::format("{:f} - point of interest", point{1, 2});
    // the range will contain "f} - point of interest".
    // The formatter should parse specifiers until '}' or the end of the range.
    // In this example the formatter should parse the 'f' specifier and return an iterator pointing to '}'.

    // Parse the presentation format and store it in the formatter:
    auto it = context.begin(), end = context.end();
    if (it != end && (*it == 'f' || *it == 'F'))
      presentation = *it++;

    // Check if reached the end of the range:
    if (it != end && *it != '}')
      throw format_error ("invalid format");

    // Return an iterator past the end of the parsed range:
    return it;
    }

  // Formats point using parsed format specification(presentation) stored in this formatter.
  template <typename tFormatContext> auto format (const cPointF& point, tFormatContext& context) {

    // ctx.out() is an output iterator to write to.
    return format_to (context.out(), presentation == 'f' ? "{:.0f},{:.0f}" : "{:.1f},{:.1f}", point.x, point.y);
    }
  };
//}}}
