// yuv2rgbsse2.h
#ifdef __cplusplus
  extern "C" {
#endif

#ifdef _M_X64
  void yuv420_rgba32_sse2 (void* fromYPtr, void* fromUPtr, void* fromVPtr, void* toPtr, _int64 width);
#else
  void yuv420_rgba32_sse2 (void* fromYPtr, void* fromUPtr, void* fromVPtr, void* toPtr, int width);
#endif

#ifdef __cplusplus
  }
#endif
