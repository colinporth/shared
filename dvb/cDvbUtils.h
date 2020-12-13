// cDvbUtils.h - dvb get utils
#pragma once

class cDvbUtils {
public:
  static int getSectionLength (uint8_t* buf);

  static uint32_t getCrc32 (uint8_t* buf, uint32_t len);

  static uint32_t getEpochTime (uint8_t* buf);
  static uint32_t getBcdTime (uint8_t* buf);
  static int64_t getPts (const uint8_t* buf);

  static bool isHuff (uint8_t* buf);
  static std::string getString (uint8_t* buf);
  };
