// cDvbString.h - possible huffman decode
#pragma once

class cDvbString {
public:
  static bool isHuff (uint8_t* buf);
  static std::string getString (uint8_t* buf);
  };
