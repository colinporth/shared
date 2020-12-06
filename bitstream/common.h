// linux bitstream library, header copyright removed only to see code more clearly
#pragma once
//{{{  includes
#include <stdint.h>   /* uint8_t, uint16_t, etc... */
#include <stdbool.h>  /* bool */
#include <string.h>   /* memset, memcpy */
//}}}

#define BITSTREAM_VERSION_MAJOR 1
#define BITSTREAM_VERSION_MINOR 0
#define BITSTREAM_VERSION_REVISION 0

//{{{
#ifdef __cplusplus
extern "C"
{
#endif
//}}}

typedef enum print_type_t {
  PRINT_TEXT,
  PRINT_XML
  } print_type_t;

typedef void (*f_print)(void *, const char *, ...) __attribute__ ((format(printf, 2, 3)));
typedef char * (*f_iconv)(void *, const char *, char *, size_t);

//{{{
#ifdef __cplusplus
}
#endif
//}}}
