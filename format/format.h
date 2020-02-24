/*
 Formatting library for C++

 Copyright (c) 2012 - present, Victor Zverovich

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 --- Optional exception to the license ---

 As an exception, if, as a result of your compiling your source code, portions
 of this Software are embedded into a machine-executable object form of such
 source code, you may redistribute such embedded portions in such object form
 without including the above copyright and permission notices.
 */

#ifndef FMT_FORMAT_H_
#define FMT_FORMAT_H_

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>

//{{{  include "core.h"
// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_CORE_H_
#define FMT_CORE_H_

#include <cstdio>  // std::FILE
#include <cstring>
#include <iterator>
#include <string>
#include <type_traits>

// The fmt library version in the form major * 10000 + minor * 100 + patch.
#define FMT_VERSION 60103

#ifdef __has_feature
#  define FMT_HAS_FEATURE(x) __has_feature(x)
#else
#  define FMT_HAS_FEATURE(x) 0
#endif

#if defined(__has_include) && !defined(__INTELLISENSE__) && \
    !(defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1600)
#  define FMT_HAS_INCLUDE(x) __has_include(x)
#else
#  define FMT_HAS_INCLUDE(x) 0
#endif

#ifdef __has_cpp_attribute
#  define FMT_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#  define FMT_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  define FMT_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#  define FMT_GCC_VERSION 0
#endif

#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#  define FMT_HAS_GXX_CXX11 FMT_GCC_VERSION
#else
#  define FMT_HAS_GXX_CXX11 0
#endif

#ifdef __NVCC__
#  define FMT_NVCC __NVCC__
#else
#  define FMT_NVCC 0
#endif

#ifdef _MSC_VER
#  define FMT_MSC_VER _MSC_VER
#else
#  define FMT_MSC_VER 0
#endif

// Check if relaxed C++14 constexpr is supported.
// GCC doesn't allow throw in constexpr until version 6 (bug 67371).
#ifndef FMT_USE_CONSTEXPR
#  define FMT_USE_CONSTEXPR                                           \
    (FMT_HAS_FEATURE(cxx_relaxed_constexpr) || FMT_MSC_VER >= 1910 || \
     (FMT_GCC_VERSION >= 600 && __cplusplus >= 201402L)) &&           \
        !FMT_NVCC
#endif
#if FMT_USE_CONSTEXPR
#  define FMT_CONSTEXPR constexpr
#  define FMT_CONSTEXPR_DECL constexpr
#else
#  define FMT_CONSTEXPR inline
#  define FMT_CONSTEXPR_DECL
#endif

#ifndef FMT_OVERRIDE
#  if FMT_HAS_FEATURE(cxx_override) || \
      (FMT_GCC_VERSION >= 408 && FMT_HAS_GXX_CXX11) || FMT_MSC_VER >= 1900
#    define FMT_OVERRIDE override
#  else
#    define FMT_OVERRIDE
#  endif
#endif

// Check if exceptions are disabled.
#ifndef FMT_EXCEPTIONS
#  if (defined(__GNUC__) && !defined(__EXCEPTIONS)) || \
      FMT_MSC_VER && !_HAS_EXCEPTIONS
#    define FMT_EXCEPTIONS 0
#  else
#    define FMT_EXCEPTIONS 1
#  endif
#endif

// Define FMT_USE_NOEXCEPT to make fmt use noexcept (C++11 feature).
#ifndef FMT_USE_NOEXCEPT
#  define FMT_USE_NOEXCEPT 0
#endif

#if FMT_USE_NOEXCEPT || FMT_HAS_FEATURE(cxx_noexcept) || \
    (FMT_GCC_VERSION >= 408 && FMT_HAS_GXX_CXX11) || FMT_MSC_VER >= 1900
#  define FMT_DETECTED_NOEXCEPT noexcept
#  define FMT_HAS_CXX11_NOEXCEPT 1
#else
#  define FMT_DETECTED_NOEXCEPT throw()
#  define FMT_HAS_CXX11_NOEXCEPT 0
#endif

#ifndef FMT_NOEXCEPT
#  if FMT_EXCEPTIONS || FMT_HAS_CXX11_NOEXCEPT
#    define FMT_NOEXCEPT FMT_DETECTED_NOEXCEPT
#  else
#    define FMT_NOEXCEPT
#  endif
#endif

// [[noreturn]] is disabled on MSVC and NVCC because of bogus unreachable code
// warnings.
#if FMT_EXCEPTIONS && FMT_HAS_CPP_ATTRIBUTE(noreturn) && !FMT_MSC_VER && \
    !FMT_NVCC
#  define FMT_NORETURN [[noreturn]]
#else
#  define FMT_NORETURN
#endif

#ifndef FMT_DEPRECATED
#  if (FMT_HAS_CPP_ATTRIBUTE(deprecated) && __cplusplus >= 201402L) || \
      FMT_MSC_VER >= 1900
#    define FMT_DEPRECATED [[deprecated]]
#  else
#    if defined(__GNUC__) || defined(__clang__)
#      define FMT_DEPRECATED __attribute__((deprecated))
#    elif FMT_MSC_VER
#      define FMT_DEPRECATED __declspec(deprecated)
#    else
#      define FMT_DEPRECATED /* deprecated */
#    endif
#  endif
#endif

// Workaround broken [[deprecated]] in the Intel compiler and NVCC.
#if defined(__INTEL_COMPILER) || FMT_NVCC
#  define FMT_DEPRECATED_ALIAS
#else
#  define FMT_DEPRECATED_ALIAS FMT_DEPRECATED
#endif

#ifndef FMT_BEGIN_NAMESPACE
#  if FMT_HAS_FEATURE(cxx_inline_namespaces) || FMT_GCC_VERSION >= 404 || \
      FMT_MSC_VER >= 1900
#    define FMT_INLINE_NAMESPACE inline namespace
#    define FMT_END_NAMESPACE \
      }                       \
      }
#  else
#    define FMT_INLINE_NAMESPACE namespace
#    define FMT_END_NAMESPACE \
      }                       \
      using namespace v6;     \
      }
#  endif
#  define FMT_BEGIN_NAMESPACE \
    namespace fmt {           \
    FMT_INLINE_NAMESPACE v6 {
#endif

#if !defined(FMT_HEADER_ONLY) && defined(_WIN32)
#  if FMT_MSC_VER
#    define FMT_NO_W4275 __pragma(warning(suppress : 4275))
#  else
#    define FMT_NO_W4275
#  endif
#  define FMT_CLASS_API FMT_NO_W4275
#  ifdef FMT_EXPORT
#    define FMT_API __declspec(dllexport)
#  elif defined(FMT_SHARED)
#    define FMT_API __declspec(dllimport)
#    define FMT_EXTERN_TEMPLATE_API FMT_API
#  endif
#endif
#ifndef FMT_CLASS_API
#  define FMT_CLASS_API
#endif
#ifndef FMT_API
#  if FMT_GCC_VERSION || FMT_CLANG_VERSION
#    define FMT_API __attribute__((visibility("default")))
#    define FMT_EXTERN_TEMPLATE_API FMT_API
#    define FMT_INSTANTIATION_DEF_API
#  else
#    define FMT_API
#  endif
#endif
#ifndef FMT_EXTERN_TEMPLATE_API
#  define FMT_EXTERN_TEMPLATE_API
#endif
#ifndef FMT_INSTANTIATION_DEF_API
#  define FMT_INSTANTIATION_DEF_API FMT_API
#endif

#ifndef FMT_HEADER_ONLY
#  define FMT_EXTERN extern
#else
#  define FMT_EXTERN
#endif

// libc++ supports string_view in pre-c++17.
#if (FMT_HAS_INCLUDE(<string_view>) &&                       \
     (__cplusplus > 201402L || defined(_LIBCPP_VERSION))) || \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L && _MSC_VER >= 1910)
#  include <string_view>
#  define FMT_USE_STRING_VIEW
#elif FMT_HAS_INCLUDE("experimental/string_view") && __cplusplus >= 201402L
#  include <experimental/string_view>
#  define FMT_USE_EXPERIMENTAL_STRING_VIEW
#endif

#ifndef FMT_UNICODE
#  define FMT_UNICODE 0
#endif
#if FMT_UNICODE
#  pragma execution_character_set("utf-8")
#endif

FMT_BEGIN_NAMESPACE

// Implementations of enable_if_t and other metafunctions for older systems.
template <bool B, class T = void>
using enable_if_t = typename std::enable_if<B, T>::type;
template <bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;
template <bool B> using bool_constant = std::integral_constant<bool, B>;
template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;
template <typename T>
using remove_const_t = typename std::remove_const<T>::type;
template <typename T>
using remove_cvref_t = typename std::remove_cv<remove_reference_t<T>>::type;
template <typename T> struct type_identity { using type = T; };
template <typename T> using type_identity_t = typename type_identity<T>::type;

struct monostate {};

// An enable_if helper to be used in template parameters which results in much
// shorter symbols: https://godbolt.org/z/sWw4vP. Extra parentheses are needed
// to workaround a bug in MSVC 2019 (see #1140 and #1186).
#define FMT_ENABLE_IF(...) enable_if_t<(__VA_ARGS__), int> = 0

namespace internal {

// A workaround for gcc 4.8 to make void_t work in a SFINAE context.
template <typename... Ts> struct void_t_impl { using type = void; };

FMT_API void assert_fail(const char* file, int line, const char* message);

#ifndef FMT_ASSERT
#  ifdef NDEBUG
#    define FMT_ASSERT(condition, message)
#  else
#    define FMT_ASSERT(condition, message) \
      ((condition)                         \
           ? void()                        \
           : ::fmt::internal::assert_fail(__FILE__, __LINE__, (message)))
#  endif
#endif

#if defined(FMT_USE_STRING_VIEW)
template <typename Char> using std_string_view = std::basic_string_view<Char>;
#elif defined(FMT_USE_EXPERIMENTAL_STRING_VIEW)
template <typename Char>
using std_string_view = std::experimental::basic_string_view<Char>;
#else
template <typename T> struct std_string_view {};
#endif

#ifdef FMT_USE_INT128
// Do nothing.
#elif defined(__SIZEOF_INT128__) && !FMT_NVCC
#  define FMT_USE_INT128 1
using int128_t = __int128_t;
using uint128_t = __uint128_t;
#else
#  define FMT_USE_INT128 0
#endif
#if !FMT_USE_INT128
struct int128_t {};
struct uint128_t {};
#endif

// Casts a nonnegative integer to unsigned.
template <typename Int>
FMT_CONSTEXPR typename std::make_unsigned<Int>::type to_unsigned(Int value) {
  FMT_ASSERT(value >= 0, "negative value");
  return static_cast<typename std::make_unsigned<Int>::type>(value);
}
}  // namespace internal

template <typename... Ts>
using void_t = typename internal::void_t_impl<Ts...>::type;

/**
  An implementation of ``std::basic_string_view`` for pre-C++17. It provides a
  subset of the API. ``fmt::basic_string_view`` is used for format strings even
  if ``std::string_view`` is available to prevent issues when a library is
  compiled with a different ``-std`` option than the client code (which is not
  recommended).
 */
template <typename Char> class basic_string_view {
 private:
  const Char* data_;
  size_t size_;

 public:
  using char_type FMT_DEPRECATED_ALIAS = Char;
  using value_type = Char;
  using iterator = const Char*;

  FMT_CONSTEXPR basic_string_view() FMT_NOEXCEPT : data_(nullptr), size_(0) {}

  /** Constructs a string reference object from a C string and a size. */
  FMT_CONSTEXPR basic_string_view(const Char* s, size_t count) FMT_NOEXCEPT
      : data_(s),
        size_(count) {}

  /**
    \rst
    Constructs a string reference object from a C string computing
    the size with ``std::char_traits<Char>::length``.
    \endrst
   */
  basic_string_view(const Char* s)
      : data_(s), size_(std::char_traits<Char>::length(s)) {}

  /** Constructs a string reference from a ``std::basic_string`` object. */
  template <typename Traits, typename Alloc>
  FMT_CONSTEXPR basic_string_view(
      const std::basic_string<Char, Traits, Alloc>& s) FMT_NOEXCEPT
      : data_(s.data()),
        size_(s.size()) {}

  template <
      typename S,
      FMT_ENABLE_IF(std::is_same<S, internal::std_string_view<Char>>::value)>
  FMT_CONSTEXPR basic_string_view(S s) FMT_NOEXCEPT : data_(s.data()),
                                                      size_(s.size()) {}

  /** Returns a pointer to the string data. */
  FMT_CONSTEXPR const Char* data() const { return data_; }

  /** Returns the string size. */
  FMT_CONSTEXPR size_t size() const { return size_; }

  FMT_CONSTEXPR iterator begin() const { return data_; }
  FMT_CONSTEXPR iterator end() const { return data_ + size_; }

  FMT_CONSTEXPR const Char& operator[](size_t pos) const { return data_[pos]; }

  FMT_CONSTEXPR void remove_prefix(size_t n) {
    data_ += n;
    size_ -= n;
  }

  // Lexicographically compare this string reference to other.
  int compare(basic_string_view other) const {
    size_t str_size = size_ < other.size_ ? size_ : other.size_;
    int result = std::char_traits<Char>::compare(data_, other.data_, str_size);
    if (result == 0)
      result = size_ == other.size_ ? 0 : (size_ < other.size_ ? -1 : 1);
    return result;
  }

  friend bool operator==(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) == 0;
  }
  friend bool operator!=(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) != 0;
  }
  friend bool operator<(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) < 0;
  }
  friend bool operator<=(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) <= 0;
  }
  friend bool operator>(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) > 0;
  }
  friend bool operator>=(basic_string_view lhs, basic_string_view rhs) {
    return lhs.compare(rhs) >= 0;
  }
};

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;

#ifndef __cpp_char8_t
// A UTF-8 code unit type.
enum char8_t : unsigned char {};
#endif

/** Specifies if ``T`` is a character type. Can be specialized by users. */
template <typename T> struct is_char : std::false_type {};
template <> struct is_char<char> : std::true_type {};
template <> struct is_char<wchar_t> : std::true_type {};
template <> struct is_char<char8_t> : std::true_type {};
template <> struct is_char<char16_t> : std::true_type {};
template <> struct is_char<char32_t> : std::true_type {};

/**
  \rst
  Returns a string view of `s`. In order to add custom string type support to
  {fmt} provide an overload of `to_string_view` for it in the same namespace as
  the type for the argument-dependent lookup to work.

  **Example**::

    namespace my_ns {
    inline string_view to_string_view(const my_string& s) {
      return {s.data(), s.length()};
    }
    }
    std::string message = fmt::format(my_string("The answer is {}"), 42);
  \endrst
 */
template <typename Char, FMT_ENABLE_IF(is_char<Char>::value)>
inline basic_string_view<Char> to_string_view(const Char* s) {
  return s;
}

template <typename Char, typename Traits, typename Alloc>
inline basic_string_view<Char> to_string_view(
    const std::basic_string<Char, Traits, Alloc>& s) {
  return s;
}

template <typename Char>
inline basic_string_view<Char> to_string_view(basic_string_view<Char> s) {
  return s;
}

template <typename Char,
          FMT_ENABLE_IF(!std::is_empty<internal::std_string_view<Char>>::value)>
inline basic_string_view<Char> to_string_view(
    internal::std_string_view<Char> s) {
  return s;
}

// A base class for compile-time strings. It is defined in the fmt namespace to
// make formatting functions visible via ADL, e.g. format(fmt("{}"), 42).
struct compile_string {};

template <typename S>
struct is_compile_string : std::is_base_of<compile_string, S> {};

template <typename S, FMT_ENABLE_IF(is_compile_string<S>::value)>
constexpr basic_string_view<typename S::char_type> to_string_view(const S& s) {
  return s;
}

namespace internal {
void to_string_view(...);
using fmt::v6::to_string_view;

// Specifies whether S is a string type convertible to fmt::basic_string_view.
// It should be a constexpr function but MSVC 2017 fails to compile it in
// enable_if and MSVC 2015 fails to compile it as an alias template.
template <typename S>
struct is_string : std::is_class<decltype(to_string_view(std::declval<S>()))> {
};

template <typename S, typename = void> struct char_t_impl {};
template <typename S> struct char_t_impl<S, enable_if_t<is_string<S>::value>> {
  using result = decltype(to_string_view(std::declval<S>()));
  using type = typename result::value_type;
};

struct error_handler {
  FMT_CONSTEXPR error_handler() = default;
  FMT_CONSTEXPR error_handler(const error_handler&) = default;

  // This function is intentionally not constexpr to give a compile-time error.
  FMT_NORETURN FMT_API void on_error(const char* message);
};
}  // namespace internal

/** String's character type. */
template <typename S> using char_t = typename internal::char_t_impl<S>::type;

/**
  \rst
  Parsing context consisting of a format string range being parsed and an
  argument counter for automatic indexing.

  You can use one of the following type aliases for common character types:

  +-----------------------+-------------------------------------+
  | Type                  | Definition                          |
  +=======================+=====================================+
  | format_parse_context  | basic_format_parse_context<char>    |
  +-----------------------+-------------------------------------+
  | wformat_parse_context | basic_format_parse_context<wchar_t> |
  +-----------------------+-------------------------------------+
  \endrst
 */
template <typename Char, typename ErrorHandler = internal::error_handler>
class basic_format_parse_context : private ErrorHandler {
 private:
  basic_string_view<Char> format_str_;
  int next_arg_id_;

 public:
  using char_type = Char;
  using iterator = typename basic_string_view<Char>::iterator;

  explicit FMT_CONSTEXPR basic_format_parse_context(
      basic_string_view<Char> format_str, ErrorHandler eh = ErrorHandler())
      : ErrorHandler(eh), format_str_(format_str), next_arg_id_(0) {}

  /**
    Returns an iterator to the beginning of the format string range being
    parsed.
   */
  FMT_CONSTEXPR iterator begin() const FMT_NOEXCEPT {
    return format_str_.begin();
  }

  /**
    Returns an iterator past the end of the format string range being parsed.
   */
  FMT_CONSTEXPR iterator end() const FMT_NOEXCEPT { return format_str_.end(); }

  /** Advances the begin iterator to ``it``. */
  FMT_CONSTEXPR void advance_to(iterator it) {
    format_str_.remove_prefix(internal::to_unsigned(it - begin()));
  }

  /**
    Reports an error if using the manual argument indexing; otherwise returns
    the next argument index and switches to the automatic indexing.
   */
  FMT_CONSTEXPR int next_arg_id() {
    if (next_arg_id_ >= 0) return next_arg_id_++;
    on_error("cannot switch from manual to automatic argument indexing");
    return 0;
  }

  /**
    Reports an error if using the automatic argument indexing; otherwise
    switches to the manual indexing.
   */
  FMT_CONSTEXPR void check_arg_id(int) {
    if (next_arg_id_ > 0)
      on_error("cannot switch from automatic to manual argument indexing");
    else
      next_arg_id_ = -1;
  }

  FMT_CONSTEXPR void check_arg_id(basic_string_view<Char>) {}

  FMT_CONSTEXPR void on_error(const char* message) {
    ErrorHandler::on_error(message);
  }

  FMT_CONSTEXPR ErrorHandler error_handler() const { return *this; }
};

using format_parse_context = basic_format_parse_context<char>;
using wformat_parse_context = basic_format_parse_context<wchar_t>;

template <typename Char, typename ErrorHandler = internal::error_handler>
using basic_parse_context FMT_DEPRECATED_ALIAS =
    basic_format_parse_context<Char, ErrorHandler>;
using parse_context FMT_DEPRECATED_ALIAS = basic_format_parse_context<char>;
using wparse_context FMT_DEPRECATED_ALIAS = basic_format_parse_context<wchar_t>;

template <typename Context> class basic_format_arg;
template <typename Context> class basic_format_args;

// A formatter for objects of type T.
template <typename T, typename Char = char, typename Enable = void>
struct formatter {
  // A deleted default constructor indicates a disabled formatter.
  formatter() = delete;
};

template <typename T, typename Char, typename Enable = void>
struct FMT_DEPRECATED convert_to_int
    : bool_constant<!std::is_arithmetic<T>::value &&
                    std::is_convertible<T, int>::value> {};

// Specifies if T has an enabled formatter specialization. A type can be
// formattable even if it doesn't have a formatter e.g. via a conversion.
template <typename T, typename Context>
using has_formatter =
    std::is_constructible<typename Context::template formatter_type<T>>;

namespace internal {

/** A contiguous memory buffer with an optional growing ability. */
template <typename T> class buffer {
 private:
  T* ptr_;
  std::size_t size_;
  std::size_t capacity_;

 protected:
  // Don't initialize ptr_ since it is not accessed to save a few cycles.
  buffer(std::size_t sz) FMT_NOEXCEPT : size_(sz), capacity_(sz) {}

  buffer(T* p = nullptr, std::size_t sz = 0, std::size_t cap = 0) FMT_NOEXCEPT
      : ptr_(p),
        size_(sz),
        capacity_(cap) {}

  /** Sets the buffer data and capacity. */
  void set(T* buf_data, std::size_t buf_capacity) FMT_NOEXCEPT {
    ptr_ = buf_data;
    capacity_ = buf_capacity;
  }

  /** Increases the buffer capacity to hold at least *capacity* elements. */
  virtual void grow(std::size_t capacity) = 0;

 public:
  using value_type = T;
  using const_reference = const T&;

  buffer(const buffer&) = delete;
  void operator=(const buffer&) = delete;
  virtual ~buffer() = default;

  T* begin() FMT_NOEXCEPT { return ptr_; }
  T* end() FMT_NOEXCEPT { return ptr_ + size_; }

  const T* begin() const FMT_NOEXCEPT { return ptr_; }
  const T* end() const FMT_NOEXCEPT { return ptr_ + size_; }

  /** Returns the size of this buffer. */
  std::size_t size() const FMT_NOEXCEPT { return size_; }

  /** Returns the capacity of this buffer. */
  std::size_t capacity() const FMT_NOEXCEPT { return capacity_; }

  /** Returns a pointer to the buffer data. */
  T* data() FMT_NOEXCEPT { return ptr_; }

  /** Returns a pointer to the buffer data. */
  const T* data() const FMT_NOEXCEPT { return ptr_; }

  /**
    Resizes the buffer. If T is a POD type new elements may not be initialized.
   */
  void resize(std::size_t new_size) {
    reserve(new_size);
    size_ = new_size;
  }

  /** Clears this buffer. */
  void clear() { size_ = 0; }

  /** Reserves space to store at least *capacity* elements. */
  void reserve(std::size_t new_capacity) {
    if (new_capacity > capacity_) grow(new_capacity);
  }

  void push_back(const T& value) {
    reserve(size_ + 1);
    ptr_[size_++] = value;
  }

  /** Appends data to the end of the buffer. */
  template <typename U> void append(const U* begin, const U* end);

  T& operator[](std::size_t index) { return ptr_[index]; }
  const T& operator[](std::size_t index) const { return ptr_[index]; }
};

// A container-backed buffer.
template <typename Container>
class container_buffer : public buffer<typename Container::value_type> {
 private:
  Container& container_;

 protected:
  void grow(std::size_t capacity) FMT_OVERRIDE {
    container_.resize(capacity);
    this->set(&container_[0], capacity);
  }

 public:
  explicit container_buffer(Container& c)
      : buffer<typename Container::value_type>(c.size()), container_(c) {}
};

// Extracts a reference to the container from back_insert_iterator.
template <typename Container>
inline Container& get_container(std::back_insert_iterator<Container> it) {
  using bi_iterator = std::back_insert_iterator<Container>;
  struct accessor : bi_iterator {
    accessor(bi_iterator iter) : bi_iterator(iter) {}
    using bi_iterator::container;
  };
  return *accessor(it).container;
}

template <typename T, typename Char = char, typename Enable = void>
struct fallback_formatter {
  fallback_formatter() = delete;
};

// Specifies if T has an enabled fallback_formatter specialization.
template <typename T, typename Context>
using has_fallback_formatter =
    std::is_constructible<fallback_formatter<T, typename Context::char_type>>;

template <typename Char> struct named_arg_base;
template <typename T, typename Char> struct named_arg;

enum class type {
  none_type,
  named_arg_type,
  // Integer types should go first,
  int_type,
  uint_type,
  long_long_type,
  ulong_long_type,
  int128_type,
  uint128_type,
  bool_type,
  char_type,
  last_integer_type = char_type,
  // followed by floating-point types.
  float_type,
  double_type,
  long_double_type,
  last_numeric_type = long_double_type,
  cstring_type,
  string_type,
  pointer_type,
  custom_type
};

// Maps core type T to the corresponding type enum constant.
template <typename T, typename Char>
struct type_constant : std::integral_constant<type, type::custom_type> {};

#define FMT_TYPE_CONSTANT(Type, constant) \
  template <typename Char>                \
  struct type_constant<Type, Char>        \
      : std::integral_constant<type, type::constant> {}

FMT_TYPE_CONSTANT(const named_arg_base<Char>&, named_arg_type);
FMT_TYPE_CONSTANT(int, int_type);
FMT_TYPE_CONSTANT(unsigned, uint_type);
FMT_TYPE_CONSTANT(long long, long_long_type);
FMT_TYPE_CONSTANT(unsigned long long, ulong_long_type);
FMT_TYPE_CONSTANT(int128_t, int128_type);
FMT_TYPE_CONSTANT(uint128_t, uint128_type);
FMT_TYPE_CONSTANT(bool, bool_type);
FMT_TYPE_CONSTANT(Char, char_type);
FMT_TYPE_CONSTANT(float, float_type);
FMT_TYPE_CONSTANT(double, double_type);
FMT_TYPE_CONSTANT(long double, long_double_type);
FMT_TYPE_CONSTANT(const Char*, cstring_type);
FMT_TYPE_CONSTANT(basic_string_view<Char>, string_type);
FMT_TYPE_CONSTANT(const void*, pointer_type);

FMT_CONSTEXPR bool is_integral_type(type t) {
  FMT_ASSERT(t != type::named_arg_type, "invalid argument type");
  return t > type::none_type && t <= type::last_integer_type;
}

FMT_CONSTEXPR bool is_arithmetic_type(type t) {
  FMT_ASSERT(t != type::named_arg_type, "invalid argument type");
  return t > type::none_type && t <= type::last_numeric_type;
}

template <typename Char> struct string_value {
  const Char* data;
  std::size_t size;
};

template <typename Context> struct custom_value {
  using parse_context = basic_format_parse_context<typename Context::char_type>;
  const void* value;
  void (*format)(const void* arg, parse_context& parse_ctx, Context& ctx);
};

// A formatting argument value.
template <typename Context> class value {
 public:
  using char_type = typename Context::char_type;

  union {
    int int_value;
    unsigned uint_value;
    long long long_long_value;
    unsigned long long ulong_long_value;
    int128_t int128_value;
    uint128_t uint128_value;
    bool bool_value;
    char_type char_value;
    float float_value;
    double double_value;
    long double long_double_value;
    const void* pointer;
    string_value<char_type> string;
    custom_value<Context> custom;
    const named_arg_base<char_type>* named_arg;
  };

  FMT_CONSTEXPR value(int val = 0) : int_value(val) {}
  FMT_CONSTEXPR value(unsigned val) : uint_value(val) {}
  value(long long val) : long_long_value(val) {}
  value(unsigned long long val) : ulong_long_value(val) {}
  value(int128_t val) : int128_value(val) {}
  value(uint128_t val) : uint128_value(val) {}
  value(float val) : float_value(val) {}
  value(double val) : double_value(val) {}
  value(long double val) : long_double_value(val) {}
  value(bool val) : bool_value(val) {}
  value(char_type val) : char_value(val) {}
  value(const char_type* val) { string.data = val; }
  value(basic_string_view<char_type> val) {
    string.data = val.data();
    string.size = val.size();
  }
  value(const void* val) : pointer(val) {}

  template <typename T> value(const T& val) {
    custom.value = &val;
    // Get the formatter type through the context to allow different contexts
    // have different extension points, e.g. `formatter<T>` for `format` and
    // `printf_formatter<T>` for `printf`.
    custom.format = format_custom_arg<
        T, conditional_t<has_formatter<T, Context>::value,
                         typename Context::template formatter_type<T>,
                         fallback_formatter<T, char_type>>>;
  }

  value(const named_arg_base<char_type>& val) { named_arg = &val; }

 private:
  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T, typename Formatter>
  static void format_custom_arg(
      const void* arg, basic_format_parse_context<char_type>& parse_ctx,
      Context& ctx) {
    Formatter f;
    parse_ctx.advance_to(f.parse(parse_ctx));
    ctx.advance_to(f.format(*static_cast<const T*>(arg), ctx));
  }
};

template <typename Context, typename T>
FMT_CONSTEXPR basic_format_arg<Context> make_arg(const T& value);

// To minimize the number of types we need to deal with, long is translated
// either to int or to long long depending on its size.
enum { long_short = sizeof(long) == sizeof(int) };
using long_type = conditional_t<long_short, int, long long>;
using ulong_type = conditional_t<long_short, unsigned, unsigned long long>;

// Maps formatting arguments to core types.
template <typename Context> struct arg_mapper {
  using char_type = typename Context::char_type;

  FMT_CONSTEXPR int map(signed char val) { return val; }
  FMT_CONSTEXPR unsigned map(unsigned char val) { return val; }
  FMT_CONSTEXPR int map(short val) { return val; }
  FMT_CONSTEXPR unsigned map(unsigned short val) { return val; }
  FMT_CONSTEXPR int map(int val) { return val; }
  FMT_CONSTEXPR unsigned map(unsigned val) { return val; }
  FMT_CONSTEXPR long_type map(long val) { return val; }
  FMT_CONSTEXPR ulong_type map(unsigned long val) { return val; }
  FMT_CONSTEXPR long long map(long long val) { return val; }
  FMT_CONSTEXPR unsigned long long map(unsigned long long val) { return val; }
  FMT_CONSTEXPR int128_t map(int128_t val) { return val; }
  FMT_CONSTEXPR uint128_t map(uint128_t val) { return val; }
  FMT_CONSTEXPR bool map(bool val) { return val; }

  template <typename T, FMT_ENABLE_IF(is_char<T>::value)>
  FMT_CONSTEXPR char_type map(T val) {
    static_assert(
        std::is_same<T, char>::value || std::is_same<T, char_type>::value,
        "mixing character types is disallowed");
    return val;
  }

  FMT_CONSTEXPR float map(float val) { return val; }
  FMT_CONSTEXPR double map(double val) { return val; }
  FMT_CONSTEXPR long double map(long double val) { return val; }

  FMT_CONSTEXPR const char_type* map(char_type* val) { return val; }
  FMT_CONSTEXPR const char_type* map(const char_type* val) { return val; }
  template <typename T, FMT_ENABLE_IF(is_string<T>::value)>
  FMT_CONSTEXPR basic_string_view<char_type> map(const T& val) {
    static_assert(std::is_same<char_type, char_t<T>>::value,
                  "mixing character types is disallowed");
    return to_string_view(val);
  }
  template <typename T,
            FMT_ENABLE_IF(
                std::is_constructible<basic_string_view<char_type>, T>::value &&
                !is_string<T>::value && !has_formatter<T, Context>::value &&
                !has_fallback_formatter<T, Context>::value)>
  FMT_CONSTEXPR basic_string_view<char_type> map(const T& val) {
    return basic_string_view<char_type>(val);
  }
  template <
      typename T,
      FMT_ENABLE_IF(
          std::is_constructible<std_string_view<char_type>, T>::value &&
          !std::is_constructible<basic_string_view<char_type>, T>::value &&
          !is_string<T>::value && !has_formatter<T, Context>::value &&
          !has_fallback_formatter<T, Context>::value)>
  FMT_CONSTEXPR basic_string_view<char_type> map(const T& val) {
    return std_string_view<char_type>(val);
  }
  FMT_CONSTEXPR const char* map(const signed char* val) {
    static_assert(std::is_same<char_type, char>::value, "invalid string type");
    return reinterpret_cast<const char*>(val);
  }
  FMT_CONSTEXPR const char* map(const unsigned char* val) {
    static_assert(std::is_same<char_type, char>::value, "invalid string type");
    return reinterpret_cast<const char*>(val);
  }

  FMT_CONSTEXPR const void* map(void* val) { return val; }
  FMT_CONSTEXPR const void* map(const void* val) { return val; }
  FMT_CONSTEXPR const void* map(std::nullptr_t val) { return val; }
  template <typename T> FMT_CONSTEXPR int map(const T*) {
    // Formatting of arbitrary pointers is disallowed. If you want to output
    // a pointer cast it to "void *" or "const void *". In particular, this
    // forbids formatting of "[const] volatile char *" which is printed as bool
    // by iostreams.
    static_assert(!sizeof(T), "formatting of non-void pointers is disallowed");
    return 0;
  }

  template <typename T,
            FMT_ENABLE_IF(std::is_enum<T>::value &&
                          !has_formatter<T, Context>::value &&
                          !has_fallback_formatter<T, Context>::value)>
  FMT_CONSTEXPR auto map(const T& val) -> decltype(
      map(static_cast<typename std::underlying_type<T>::type>(val))) {
    return map(static_cast<typename std::underlying_type<T>::type>(val));
  }
  template <
      typename T,
      FMT_ENABLE_IF(
          !is_string<T>::value && !is_char<T>::value &&
          (has_formatter<T, Context>::value ||
           has_fallback_formatter<T, Context>::value))>
  FMT_CONSTEXPR const T& map(const T& val) {
    return val;
  }

  template <typename T>
  FMT_CONSTEXPR const named_arg_base<char_type>& map(
      const named_arg<T, char_type>& val) {
    auto arg = make_arg<Context>(val.value);
    std::memcpy(val.data, &arg, sizeof(arg));
    return val;
  }

  int map(...) {
    constexpr bool formattable = sizeof(Context) == 0;
    static_assert(
        formattable,
        "Cannot format argument. To make type T formattable provide a "
        "formatter<T> specialization: "
        "https://fmt.dev/latest/api.html#formatting-user-defined-types");
    return 0;
  }
};

// A type constant after applying arg_mapper<Context>.
template <typename T, typename Context>
using mapped_type_constant =
    type_constant<decltype(arg_mapper<Context>().map(std::declval<const T&>())),
                  typename Context::char_type>;

enum { packed_arg_bits = 5 };
// Maximum number of arguments with packed types.
enum { max_packed_args = 63 / packed_arg_bits };
enum : unsigned long long { is_unpacked_bit = 1ULL << 63 };

template <typename Context> class arg_map;
}  // namespace internal

// A formatting argument. It is a trivially copyable/constructible type to
// allow storage in basic_memory_buffer.
template <typename Context> class basic_format_arg {
 private:
  internal::value<Context> value_;
  internal::type type_;

  template <typename ContextType, typename T>
  friend FMT_CONSTEXPR basic_format_arg<ContextType> internal::make_arg(
      const T& value);

  template <typename Visitor, typename Ctx>
  friend FMT_CONSTEXPR auto visit_format_arg(Visitor&& vis,
                                             const basic_format_arg<Ctx>& arg)
      -> decltype(vis(0));

  friend class basic_format_args<Context>;
  friend class internal::arg_map<Context>;

  using char_type = typename Context::char_type;

 public:
  class handle {
   public:
    explicit handle(internal::custom_value<Context> custom) : custom_(custom) {}

    void format(basic_format_parse_context<char_type>& parse_ctx,
                Context& ctx) const {
      custom_.format(custom_.value, parse_ctx, ctx);
    }

   private:
    internal::custom_value<Context> custom_;
  };

  FMT_CONSTEXPR basic_format_arg() : type_(internal::type::none_type) {}

  FMT_CONSTEXPR explicit operator bool() const FMT_NOEXCEPT {
    return type_ != internal::type::none_type;
  }

  internal::type type() const { return type_; }

  bool is_integral() const { return internal::is_integral_type(type_); }
  bool is_arithmetic() const { return internal::is_arithmetic_type(type_); }
};

/**
  \rst
  Visits an argument dispatching to the appropriate visit method based on
  the argument type. For example, if the argument type is ``double`` then
  ``vis(value)`` will be called with the value of type ``double``.
  \endrst
 */
template <typename Visitor, typename Context>
FMT_CONSTEXPR auto visit_format_arg(Visitor&& vis,
                                    const basic_format_arg<Context>& arg)
    -> decltype(vis(0)) {
  using char_type = typename Context::char_type;
  switch (arg.type_) {
  case internal::type::none_type:
    break;
  case internal::type::named_arg_type:
    FMT_ASSERT(false, "invalid argument type");
    break;
  case internal::type::int_type:
    return vis(arg.value_.int_value);
  case internal::type::uint_type:
    return vis(arg.value_.uint_value);
  case internal::type::long_long_type:
    return vis(arg.value_.long_long_value);
  case internal::type::ulong_long_type:
    return vis(arg.value_.ulong_long_value);
#if FMT_USE_INT128
  case internal::type::int128_type:
    return vis(arg.value_.int128_value);
  case internal::type::uint128_type:
    return vis(arg.value_.uint128_value);
#else
  case internal::type::int128_type:
  case internal::type::uint128_type:
    break;
#endif
  case internal::type::bool_type:
    return vis(arg.value_.bool_value);
  case internal::type::char_type:
    return vis(arg.value_.char_value);
  case internal::type::float_type:
    return vis(arg.value_.float_value);
  case internal::type::double_type:
    return vis(arg.value_.double_value);
  case internal::type::long_double_type:
    return vis(arg.value_.long_double_value);
  case internal::type::cstring_type:
    return vis(arg.value_.string.data);
  case internal::type::string_type:
    return vis(basic_string_view<char_type>(arg.value_.string.data,
                                            arg.value_.string.size));
  case internal::type::pointer_type:
    return vis(arg.value_.pointer);
  case internal::type::custom_type:
    return vis(typename basic_format_arg<Context>::handle(arg.value_.custom));
  }
  return vis(monostate());
}

namespace internal {
// A map from argument names to their values for named arguments.
template <typename Context> class arg_map {
 private:
  using char_type = typename Context::char_type;

  struct entry {
    basic_string_view<char_type> name;
    basic_format_arg<Context> arg;
  };

  entry* map_;
  unsigned size_;

  void push_back(value<Context> val) {
    const auto& named = *val.named_arg;
    map_[size_] = {named.name, named.template deserialize<Context>()};
    ++size_;
  }

 public:
  arg_map(const arg_map&) = delete;
  void operator=(const arg_map&) = delete;
  arg_map() : map_(nullptr), size_(0) {}
  void init(const basic_format_args<Context>& args);
  ~arg_map() { delete[] map_; }

  basic_format_arg<Context> find(basic_string_view<char_type> name) const {
    // The list is unsorted, so just return the first matching name.
    for (entry *it = map_, *end = map_ + size_; it != end; ++it) {
      if (it->name == name) return it->arg;
    }
    return {};
  }
};

// A type-erased reference to an std::locale to avoid heavy <locale> include.
class locale_ref {
 private:
  const void* locale_;  // A type-erased pointer to std::locale.

 public:
  locale_ref() : locale_(nullptr) {}
  template <typename Locale> explicit locale_ref(const Locale& loc);

  explicit operator bool() const FMT_NOEXCEPT { return locale_ != nullptr; }

  template <typename Locale> Locale get() const;
};

template <typename> constexpr unsigned long long encode_types() { return 0; }

template <typename Context, typename Arg, typename... Args>
constexpr unsigned long long encode_types() {
  return static_cast<unsigned>(mapped_type_constant<Arg, Context>::value) |
         (encode_types<Context, Args...>() << packed_arg_bits);
}

template <typename Context, typename T>
FMT_CONSTEXPR basic_format_arg<Context> make_arg(const T& value) {
  basic_format_arg<Context> arg;
  arg.type_ = mapped_type_constant<T, Context>::value;
  arg.value_ = arg_mapper<Context>().map(value);
  return arg;
}

template <bool IS_PACKED, typename Context, typename T,
          FMT_ENABLE_IF(IS_PACKED)>
inline value<Context> make_arg(const T& val) {
  return arg_mapper<Context>().map(val);
}

template <bool IS_PACKED, typename Context, typename T,
          FMT_ENABLE_IF(!IS_PACKED)>
inline basic_format_arg<Context> make_arg(const T& value) {
  return make_arg<Context>(value);
}
}  // namespace internal

// Formatting context.
template <typename OutputIt, typename Char> class basic_format_context {
 public:
  /** The character type for the output. */
  using char_type = Char;

 private:
  OutputIt out_;
  basic_format_args<basic_format_context> args_;
  internal::arg_map<basic_format_context> map_;
  internal::locale_ref loc_;

 public:
  using iterator = OutputIt;
  using format_arg = basic_format_arg<basic_format_context>;
  template <typename T> using formatter_type = formatter<T, char_type>;

  basic_format_context(const basic_format_context&) = delete;
  void operator=(const basic_format_context&) = delete;
  /**
   Constructs a ``basic_format_context`` object. References to the arguments are
   stored in the object so make sure they have appropriate lifetimes.
   */
  basic_format_context(OutputIt out,
                       basic_format_args<basic_format_context> ctx_args,
                       internal::locale_ref loc = internal::locale_ref())
      : out_(out), args_(ctx_args), loc_(loc) {}

  format_arg arg(int id) const { return args_.get(id); }

  // Checks if manual indexing is used and returns the argument with the
  // specified name.
  format_arg arg(basic_string_view<char_type> name);

  internal::error_handler error_handler() { return {}; }
  void on_error(const char* message) { error_handler().on_error(message); }

  // Returns an iterator to the beginning of the output range.
  iterator out() { return out_; }

  // Advances the begin iterator to ``it``.
  void advance_to(iterator it) { out_ = it; }

  internal::locale_ref locale() { return loc_; }
};

template <typename Char>
using buffer_context =
    basic_format_context<std::back_insert_iterator<internal::buffer<Char>>,
                         Char>;
using format_context = buffer_context<char>;
using wformat_context = buffer_context<wchar_t>;

/**
  \rst
  An array of references to arguments. It can be implicitly converted into
  `~fmt::basic_format_args` for passing into type-erased formatting functions
  such as `~fmt::vformat`.
  \endrst
 */
template <typename Context, typename... Args>
class format_arg_store
#if FMT_GCC_VERSION < 409
    // Workaround a GCC template argument substitution bug.
    : public basic_format_args<Context>
#endif
{
 private:
  static const size_t num_args = sizeof...(Args);
  static const bool is_packed = num_args < internal::max_packed_args;

  using value_type = conditional_t<is_packed, internal::value<Context>,
                                   basic_format_arg<Context>>;

  // If the arguments are not packed, add one more element to mark the end.
  value_type data_[num_args + (num_args == 0 ? 1 : 0)];

  friend class basic_format_args<Context>;

 public:
  static constexpr unsigned long long types =
      is_packed ? internal::encode_types<Context, Args...>()
                : internal::is_unpacked_bit | num_args;

  format_arg_store(const Args&... args)
      :
#if FMT_GCC_VERSION < 409
        basic_format_args<Context>(*this),
#endif
        data_{internal::make_arg<is_packed, Context>(args)...} {
  }
};

/**
  \rst
  Constructs an `~fmt::format_arg_store` object that contains references to
  arguments and can be implicitly converted to `~fmt::format_args`. `Context`
  can be omitted in which case it defaults to `~fmt::context`.
  See `~fmt::arg` for lifetime considerations.
  \endrst
 */
template <typename Context = format_context, typename... Args>
inline format_arg_store<Context, Args...> make_format_args(
    const Args&... args) {
  return {args...};
}

/**
  \rst
  A view of a collection of formatting arguments. To avoid lifetime issues it
  should only be used as a parameter type in type-erased functions such as
  ``vformat``::

    void vlog(string_view format_str, format_args args);  // OK
    format_args args = make_format_args(42);  // Error: dangling reference
  \endrst
 */
template <typename Context> class basic_format_args {
 public:
  using size_type = int;
  using format_arg = basic_format_arg<Context>;

 private:
  // To reduce compiled code size per formatting function call, types of first
  // max_packed_args arguments are passed in the types_ field.
  unsigned long long types_;
  union {
    // If the number of arguments is less than max_packed_args, the argument
    // values are stored in values_, otherwise they are stored in args_.
    // This is done to reduce compiled code size as storing larger objects
    // may require more code (at least on x86-64) even if the same amount of
    // data is actually copied to stack. It saves ~10% on the bloat test.
    const internal::value<Context>* values_;
    const format_arg* args_;
  };

  bool is_packed() const { return (types_ & internal::is_unpacked_bit) == 0; }

  internal::type type(int index) const {
    int shift = index * internal::packed_arg_bits;
    unsigned int mask = (1 << internal::packed_arg_bits) - 1;
    return static_cast<internal::type>((types_ >> shift) & mask);
  }

  friend class internal::arg_map<Context>;

  void set_data(const internal::value<Context>* values) { values_ = values; }
  void set_data(const format_arg* args) { args_ = args; }

  format_arg do_get(int index) const {
    format_arg arg;
    if (!is_packed()) {
      auto num_args = max_size();
      if (index < num_args) arg = args_[index];
      return arg;
    }
    if (index > internal::max_packed_args) return arg;
    arg.type_ = type(index);
    if (arg.type_ == internal::type::none_type) return arg;
    internal::value<Context>& val = arg.value_;
    val = values_[index];
    return arg;
  }

 public:
  basic_format_args() : types_(0) {}

  /**
   \rst
   Constructs a `basic_format_args` object from `~fmt::format_arg_store`.
   \endrst
   */
  template <typename... Args>
  basic_format_args(const format_arg_store<Context, Args...>& store)
      : types_(store.types) {
    set_data(store.data_);
  }

  /**
   \rst
   Constructs a `basic_format_args` object from a dynamic set of arguments.
   \endrst
   */
  basic_format_args(const format_arg* args, int count)
      : types_(internal::is_unpacked_bit | internal::to_unsigned(count)) {
    set_data(args);
  }

  /** Returns the argument at specified index. */
  format_arg get(int index) const {
    format_arg arg = do_get(index);
    if (arg.type_ == internal::type::named_arg_type)
      arg = arg.value_.named_arg->template deserialize<Context>();
    return arg;
  }

  int max_size() const {
    unsigned long long max_packed = internal::max_packed_args;
    return static_cast<int>(is_packed() ? max_packed
                                        : types_ & ~internal::is_unpacked_bit);
  }
};

/** An alias to ``basic_format_args<context>``. */
// It is a separate type rather than an alias to make symbols readable.
struct format_args : basic_format_args<format_context> {
  template <typename... Args>
  format_args(Args&&... args)
      : basic_format_args<format_context>(static_cast<Args&&>(args)...) {}
};
struct wformat_args : basic_format_args<wformat_context> {
  template <typename... Args>
  wformat_args(Args&&... args)
      : basic_format_args<wformat_context>(static_cast<Args&&>(args)...) {}
};

template <typename Container> struct is_contiguous : std::false_type {};

template <typename Char>
struct is_contiguous<std::basic_string<Char>> : std::true_type {};

template <typename Char>
struct is_contiguous<internal::buffer<Char>> : std::true_type {};

namespace internal {

template <typename OutputIt>
struct is_contiguous_back_insert_iterator : std::false_type {};
template <typename Container>
struct is_contiguous_back_insert_iterator<std::back_insert_iterator<Container>>
    : is_contiguous<Container> {};

template <typename Char> struct named_arg_base {
  basic_string_view<Char> name;

  // Serialized value<context>.
  mutable char data[sizeof(basic_format_arg<buffer_context<Char>>)];

  named_arg_base(basic_string_view<Char> nm) : name(nm) {}

  template <typename Context> basic_format_arg<Context> deserialize() const {
    basic_format_arg<Context> arg;
    std::memcpy(&arg, data, sizeof(basic_format_arg<Context>));
    return arg;
  }
};

struct view {};

template <typename T, typename Char>
struct named_arg : view, named_arg_base<Char> {
  const T& value;

  named_arg(basic_string_view<Char> name, const T& val)
      : named_arg_base<Char>(name), value(val) {}
};

template <typename..., typename S, FMT_ENABLE_IF(!is_compile_string<S>::value)>
inline void check_format_string(const S&) {
#if defined(FMT_ENFORCE_COMPILE_STRING)
  static_assert(is_compile_string<S>::value,
                "FMT_ENFORCE_COMPILE_STRING requires all format strings to "
                "utilize FMT_STRING() or fmt().");
#endif
}
template <typename..., typename S, FMT_ENABLE_IF(is_compile_string<S>::value)>
void check_format_string(S);

template <bool...> struct bool_pack;
template <bool... Args>
using all_true =
    std::is_same<bool_pack<Args..., true>, bool_pack<true, Args...>>;

template <typename... Args, typename S, typename Char = char_t<S>>
inline format_arg_store<buffer_context<Char>, remove_reference_t<Args>...>
make_args_checked(const S& format_str,
                  const remove_reference_t<Args>&... args) {
  static_assert(
      all_true<(!std::is_base_of<view, remove_reference_t<Args>>::value ||
                !std::is_reference<Args>::value)...>::value,
      "passing views as lvalues is disallowed");
  check_format_string<remove_const_t<remove_reference_t<Args>>...>(format_str);
  return {args...};
}

template <typename Char>
std::basic_string<Char> vformat(
    basic_string_view<Char> format_str,
    basic_format_args<buffer_context<type_identity_t<Char>>> args);

template <typename Char>
typename buffer_context<Char>::iterator vformat_to(
    buffer<Char>& buf, basic_string_view<Char> format_str,
    basic_format_args<buffer_context<type_identity_t<Char>>> args);

FMT_API void vprint_mojibake(std::FILE*, string_view, format_args);
}  // namespace internal

/**
  \rst
  Returns a named argument to be used in a formatting function. It should only
  be used in a call to a formatting function.

  **Example**::

    fmt::print("Elapsed time: {s:.2f} seconds", fmt::arg("s", 1.23));
  \endrst
 */
template <typename S, typename T, typename Char = char_t<S>>
inline internal::named_arg<T, Char> arg(const S& name, const T& arg) {
  static_assert(internal::is_string<S>::value, "");
  return {name, arg};
}

// Disable nested named arguments, e.g. ``arg("a", arg("b", 42))``.
template <typename S, typename T, typename Char>
void arg(S, internal::named_arg<T, Char>) = delete;

/** Formats a string and writes the output to ``out``. */
// GCC 8 and earlier cannot handle std::back_insert_iterator<Container> with
// vformat_to<ArgFormatter>(...) overload, so SFINAE on iterator type instead.
template <typename OutputIt, typename S, typename Char = char_t<S>,
          FMT_ENABLE_IF(
              internal::is_contiguous_back_insert_iterator<OutputIt>::value)>
OutputIt vformat_to(
    OutputIt out, const S& format_str,
    basic_format_args<buffer_context<type_identity_t<Char>>> args) {
  using container = remove_reference_t<decltype(internal::get_container(out))>;
  internal::container_buffer<container> buf((internal::get_container(out)));
  internal::vformat_to(buf, to_string_view(format_str), args);
  return out;
}

template <typename Container, typename S, typename... Args,
          FMT_ENABLE_IF(
              is_contiguous<Container>::value&& internal::is_string<S>::value)>
inline std::back_insert_iterator<Container> format_to(
    std::back_insert_iterator<Container> out, const S& format_str,
    Args&&... args) {
  return vformat_to(out, to_string_view(format_str),
                    internal::make_args_checked<Args...>(format_str, args...));
}

template <typename S, typename Char = char_t<S>>
inline std::basic_string<Char> vformat(
    const S& format_str,
    basic_format_args<buffer_context<type_identity_t<Char>>> args) {
  return internal::vformat(to_string_view(format_str), args);
}

/**
  \rst
  Formats arguments and returns the result as a string.

  **Example**::

    #include <fmt/core.h>
    std::string message = fmt::format("The answer is {}", 42);
  \endrst
*/
// Pass char_t as a default template parameter instead of using
// std::basic_string<char_t<S>> to reduce the symbol size.
template <typename S, typename... Args, typename Char = char_t<S>>
inline std::basic_string<Char> format(const S& format_str, Args&&... args) {
  return internal::vformat(
      to_string_view(format_str),
      internal::make_args_checked<Args...>(format_str, args...));
}

FMT_API void vprint(string_view, format_args);
FMT_API void vprint(std::FILE*, string_view, format_args);

/**
  \rst
  Formats ``args`` according to specifications in ``format_str`` and writes the
  output to the file ``f``. Strings are assumed to be Unicode-encoded unless the
  ``FMT_UNICODE`` macro is set to 0.

  **Example**::

    fmt::print(stderr, "Don't {}!", "panic");
  \endrst
 */
template <typename S, typename... Args,
          FMT_ENABLE_IF(internal::is_string<S>::value)>
inline void print(std::FILE* f, const S& format_str, Args&&... args) {
#if !defined(_WIN32) || FMT_UNICODE
  vprint(f, to_string_view(format_str),
         internal::make_args_checked<Args...>(format_str, args...));
#else
  internal::vprint_mojibake(
      f, to_string_view(format_str),
      internal::make_args_checked<Args...>(format_str, args...));
#endif
}

/**
  \rst
  Formats ``args`` according to specifications in ``format_str`` and writes
  the output to ``stdout``. Strings are assumed to be Unicode-encoded unless
  the ``FMT_UNICODE`` macro is set to 0.

  **Example**::

    fmt::print("Elapsed time: {0:.2f} seconds", 1.23);
  \endrst
 */
template <typename S, typename... Args,
          FMT_ENABLE_IF(internal::is_string<S>::value)>
inline void print(const S& format_str, Args&&... args) {
#if !defined(_WIN32) || FMT_UNICODE
  vprint(to_string_view(format_str),
         internal::make_args_checked<Args...>(format_str, args...));
#else
  internal::vprint_mojibake(
      stdout, to_string_view(format_str),
      internal::make_args_checked<Args...>(format_str, args...));
#endif
}
FMT_END_NAMESPACE

#endif  // FMT_CORE_H_
//}}}

#ifdef __clang__
#  define FMT_CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#else
#  define FMT_CLANG_VERSION 0
#endif

#ifdef __INTEL_COMPILER
#  define FMT_ICC_VERSION __INTEL_COMPILER
#elif defined(__ICL)
#  define FMT_ICC_VERSION __ICL
#else
#  define FMT_ICC_VERSION 0
#endif

#ifdef __NVCC__
#  define FMT_CUDA_VERSION (__CUDACC_VER_MAJOR__ * 100 + __CUDACC_VER_MINOR__)
#else
#  define FMT_CUDA_VERSION 0
#endif

#ifdef __has_builtin
#  define FMT_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define FMT_HAS_BUILTIN(x) 0
#endif

#if __cplusplus == 201103L || __cplusplus == 201402L
#  if defined(__clang__)
#    define FMT_FALLTHROUGH [[clang::fallthrough]]
#  elif FMT_GCC_VERSION >= 700
#    define FMT_FALLTHROUGH [[gnu::fallthrough]]
#  else
#    define FMT_FALLTHROUGH
#  endif
#elif (FMT_HAS_CPP_ATTRIBUTE(fallthrough) && (__cplusplus >= 201703)) || \
    (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#  define FMT_FALLTHROUGH [[fallthrough]]
#else
#  define FMT_FALLTHROUGH
#endif

#ifndef FMT_THROW
#  if FMT_EXCEPTIONS
#    if FMT_MSC_VER || FMT_NVCC
FMT_BEGIN_NAMESPACE
namespace internal {
template <typename Exception> inline void do_throw(const Exception& x) {
  // Silence unreachable code warnings in MSVC and NVCC because these
  // are nearly impossible to fix in a generic code.
  volatile bool b = true;
  if (b) throw x;
}
}  // namespace internal
FMT_END_NAMESPACE
#      define FMT_THROW(x) internal::do_throw(x)
#    else
#      define FMT_THROW(x) throw x
#    endif
#  else
#    define FMT_THROW(x)              \
      do {                            \
        static_cast<void>(sizeof(x)); \
        FMT_ASSERT(false, "");        \
      } while (false)
#  endif
#endif

#if FMT_EXCEPTIONS
#  define FMT_TRY try
#  define FMT_CATCH(x) catch (x)
#else
#  define FMT_TRY if (true)
#  define FMT_CATCH(x) if (false)
#endif

#ifndef FMT_USE_USER_DEFINED_LITERALS
// For Intel and NVIDIA compilers both they and the system gcc/msc support UDLs.
#  if (FMT_HAS_FEATURE(cxx_user_literals) || FMT_GCC_VERSION >= 407 ||      \
       FMT_MSC_VER >= 1900) &&                                              \
      (!(FMT_ICC_VERSION || FMT_CUDA_VERSION) || FMT_ICC_VERSION >= 1500 || \
       FMT_CUDA_VERSION >= 700)
#    define FMT_USE_USER_DEFINED_LITERALS 1
#  else
#    define FMT_USE_USER_DEFINED_LITERALS 0
#  endif
#endif

#ifndef FMT_USE_UDL_TEMPLATE
// EDG front end based compilers (icc, nvcc) and GCC < 6.4 do not propertly
// support UDL templates and GCC >= 9 warns about them.
#  if FMT_USE_USER_DEFINED_LITERALS && FMT_ICC_VERSION == 0 && \
      FMT_CUDA_VERSION == 0 &&                                 \
      ((FMT_GCC_VERSION >= 604 && FMT_GCC_VERSION <= 900 &&    \
        __cplusplus >= 201402L) ||                             \
       FMT_CLANG_VERSION >= 304)
#    define FMT_USE_UDL_TEMPLATE 1
#  else
#    define FMT_USE_UDL_TEMPLATE 0
#  endif
#endif

// __builtin_clz is broken in clang with Microsoft CodeGen:
// https://github.com/fmtlib/fmt/issues/519
#if (FMT_GCC_VERSION || FMT_HAS_BUILTIN(__builtin_clz)) && !FMT_MSC_VER
#  define FMT_BUILTIN_CLZ(n) __builtin_clz(n)
#endif
#if (FMT_GCC_VERSION || FMT_HAS_BUILTIN(__builtin_clzll)) && !FMT_MSC_VER
#  define FMT_BUILTIN_CLZLL(n) __builtin_clzll(n)
#endif

// Some compilers masquerade as both MSVC and GCC-likes or otherwise support
// __builtin_clz and __builtin_clzll, so only define FMT_BUILTIN_CLZ using the
// MSVC intrinsics if the clz and clzll builtins are not available.
#if FMT_MSC_VER && !defined(FMT_BUILTIN_CLZLL) && !defined(_MANAGED)
#  include <intrin.h>  // _BitScanReverse, _BitScanReverse64

FMT_BEGIN_NAMESPACE
namespace internal {
// Avoid Clang with Microsoft CodeGen's -Wunknown-pragmas warning.
#  ifndef __clang__
#    pragma intrinsic(_BitScanReverse)
#  endif
inline uint32_t clz(uint32_t x) {
  unsigned long r = 0;
  _BitScanReverse(&r, x);

  FMT_ASSERT(x != 0, "");
  // Static analysis complains about using uninitialized data
  // "r", but the only way that can happen is if "x" is 0,
  // which the callers guarantee to not happen.
#  pragma warning(suppress : 6102)
  return 31 - r;
}
#  define FMT_BUILTIN_CLZ(n) internal::clz(n)

#  if defined(_WIN64) && !defined(__clang__)
#    pragma intrinsic(_BitScanReverse64)
#  endif

inline uint32_t clzll(uint64_t x) {
  unsigned long r = 0;
#  ifdef _WIN64
  _BitScanReverse64(&r, x);
#  else
  // Scan the high 32 bits.
  if (_BitScanReverse(&r, static_cast<uint32_t>(x >> 32))) return 63 - (r + 32);

  // Scan the low 32 bits.
  _BitScanReverse(&r, static_cast<uint32_t>(x));
#  endif

  FMT_ASSERT(x != 0, "");
  // Static analysis complains about using uninitialized data
  // "r", but the only way that can happen is if "x" is 0,
  // which the callers guarantee to not happen.
#  pragma warning(suppress : 6102)
  return 63 - r;
}
#  define FMT_BUILTIN_CLZLL(n) internal::clzll(n)
}  // namespace internal
FMT_END_NAMESPACE
#endif

// Enable the deprecated numeric alignment.
#ifndef FMT_NUMERIC_ALIGN
#  define FMT_NUMERIC_ALIGN 1
#endif

// Enable the deprecated percent specifier.
#ifndef FMT_DEPRECATED_PERCENT
#  define FMT_DEPRECATED_PERCENT 0
#endif

FMT_BEGIN_NAMESPACE
namespace internal {

// A helper function to suppress bogus "conditional expression is constant"
// warnings.
template <typename T> FMT_CONSTEXPR T const_check(T value) { return value; }

// An equivalent of `*reinterpret_cast<Dest*>(&source)` that doesn't have
// undefined behavior (e.g. due to type aliasing).
// Example: uint64_t d = bit_cast<uint64_t>(2.718);
template <typename Dest, typename Source>
inline Dest bit_cast(const Source& source) {
  static_assert(sizeof(Dest) == sizeof(Source), "size mismatch");
  Dest dest;
  std::memcpy(&dest, &source, sizeof(dest));
  return dest;
}

inline bool is_big_endian() {
  const auto u = 1u;
  struct bytes {
    char data[sizeof(u)];
  };
  return bit_cast<bytes>(u).data[0] == 0;
}

// A fallback implementation of uintptr_t for systems that lack it.
struct fallback_uintptr {
  unsigned char value[sizeof(void*)];

  fallback_uintptr() = default;
  explicit fallback_uintptr(const void* p) {
    *this = bit_cast<fallback_uintptr>(p);
    if (is_big_endian()) {
      for (size_t i = 0, j = sizeof(void*) - 1; i < j; ++i, --j)
        std::swap(value[i], value[j]);
    }
  }
};
#ifdef UINTPTR_MAX
using uintptr_t = ::uintptr_t;
inline uintptr_t to_uintptr(const void* p) { return bit_cast<uintptr_t>(p); }
#else
using uintptr_t = fallback_uintptr;
inline fallback_uintptr to_uintptr(const void* p) {
  return fallback_uintptr(p);
}
#endif

// Returns the largest possible value for type T. Same as
// std::numeric_limits<T>::max() but shorter and not affected by the max macro.
template <typename T> constexpr T max_value() {
  return (std::numeric_limits<T>::max)();
}
template <typename T> constexpr int num_bits() {
  return std::numeric_limits<T>::digits;
}
template <> constexpr int num_bits<fallback_uintptr>() {
  return static_cast<int>(sizeof(void*) *
                          std::numeric_limits<unsigned char>::digits);
}

// An approximation of iterator_t for pre-C++20 systems.
template <typename T>
using iterator_t = decltype(std::begin(std::declval<T&>()));

// Detect the iterator category of *any* given type in a SFINAE-friendly way.
// Unfortunately, older implementations of std::iterator_traits are not safe
// for use in a SFINAE-context.
template <typename It, typename Enable = void>
struct iterator_category : std::false_type {};

template <typename T> struct iterator_category<T*> {
  using type = std::random_access_iterator_tag;
};

template <typename It>
struct iterator_category<It, void_t<typename It::iterator_category>> {
  using type = typename It::iterator_category;
};

// Detect if *any* given type models the OutputIterator concept.
template <typename It> class is_output_iterator {
  // Check for mutability because all iterator categories derived from
  // std::input_iterator_tag *may* also meet the requirements of an
  // OutputIterator, thereby falling into the category of 'mutable iterators'
  // [iterator.requirements.general] clause 4. The compiler reveals this
  // property only at the point of *actually dereferencing* the iterator!
  template <typename U>
  static decltype(*(std::declval<U>())) test(std::input_iterator_tag);
  template <typename U> static char& test(std::output_iterator_tag);
  template <typename U> static const char& test(...);

  using type = decltype(test<It>(typename iterator_category<It>::type{}));

 public:
  enum { value = !std::is_const<remove_reference_t<type>>::value };
};

// A workaround for std::string not having mutable data() until C++17.
template <typename Char> inline Char* get_data(std::basic_string<Char>& s) {
  return &s[0];
}
template <typename Container>
inline typename Container::value_type* get_data(Container& c) {
  return c.data();
}

#if defined(_SECURE_SCL) && _SECURE_SCL
// Make a checked iterator to avoid MSVC warnings.
template <typename T> using checked_ptr = stdext::checked_array_iterator<T*>;
template <typename T> checked_ptr<T> make_checked(T* p, std::size_t size) {
  return {p, size};
}
#else
template <typename T> using checked_ptr = T*;
template <typename T> inline T* make_checked(T* p, std::size_t) { return p; }
#endif

template <typename Container, FMT_ENABLE_IF(is_contiguous<Container>::value)>
inline checked_ptr<typename Container::value_type> reserve(
    std::back_insert_iterator<Container>& it, std::size_t n) {
  Container& c = get_container(it);
  std::size_t size = c.size();
  c.resize(size + n);
  return make_checked(get_data(c) + size, n);
}

template <typename Iterator>
inline Iterator& reserve(Iterator& it, std::size_t) {
  return it;
}

// An output iterator that counts the number of objects written to it and
// discards them.
class counting_iterator {
 private:
  std::size_t count_;

 public:
  using iterator_category = std::output_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;
  using _Unchecked_type = counting_iterator;  // Mark iterator as checked.

  struct value_type {
    template <typename T> void operator=(const T&) {}
  };

  counting_iterator() : count_(0) {}

  std::size_t count() const { return count_; }

  counting_iterator& operator++() {
    ++count_;
    return *this;
  }

  counting_iterator operator++(int) {
    auto it = *this;
    ++*this;
    return it;
  }

  value_type operator*() const { return {}; }
};

template <typename OutputIt> class truncating_iterator_base {
 protected:
  OutputIt out_;
  std::size_t limit_;
  std::size_t count_;

  truncating_iterator_base(OutputIt out, std::size_t limit)
      : out_(out), limit_(limit), count_(0) {}

 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = typename std::iterator_traits<OutputIt>::value_type;
  using difference_type = void;
  using pointer = void;
  using reference = void;
  using _Unchecked_type =
      truncating_iterator_base;  // Mark iterator as checked.

  OutputIt base() const { return out_; }
  std::size_t count() const { return count_; }
};

// An output iterator that truncates the output and counts the number of objects
// written to it.
template <typename OutputIt,
          typename Enable = typename std::is_void<
              typename std::iterator_traits<OutputIt>::value_type>::type>
class truncating_iterator;

template <typename OutputIt>
class truncating_iterator<OutputIt, std::false_type>
    : public truncating_iterator_base<OutputIt> {
  mutable typename truncating_iterator_base<OutputIt>::value_type blackhole_;

 public:
  using value_type = typename truncating_iterator_base<OutputIt>::value_type;

  truncating_iterator(OutputIt out, std::size_t limit)
      : truncating_iterator_base<OutputIt>(out, limit) {}

  truncating_iterator& operator++() {
    if (this->count_++ < this->limit_) ++this->out_;
    return *this;
  }

  truncating_iterator operator++(int) {
    auto it = *this;
    ++*this;
    return it;
  }

  value_type& operator*() const {
    return this->count_ < this->limit_ ? *this->out_ : blackhole_;
  }
};

template <typename OutputIt>
class truncating_iterator<OutputIt, std::true_type>
    : public truncating_iterator_base<OutputIt> {
 public:
  truncating_iterator(OutputIt out, std::size_t limit)
      : truncating_iterator_base<OutputIt>(out, limit) {}

  template <typename T> truncating_iterator& operator=(T val) {
    if (this->count_++ < this->limit_) *this->out_++ = val;
    return *this;
  }

  truncating_iterator& operator++() { return *this; }
  truncating_iterator& operator++(int) { return *this; }
  truncating_iterator& operator*() { return *this; }
};

// A range with the specified output iterator and value type.
template <typename OutputIt, typename T = typename OutputIt::value_type>
class output_range {
 private:
  OutputIt it_;

 public:
  using value_type = T;
  using iterator = OutputIt;
  struct sentinel {};

  explicit output_range(OutputIt it) : it_(it) {}
  OutputIt begin() const { return it_; }
  sentinel end() const { return {}; }  // Sentinel is not used yet.
};

template <typename Char>
inline size_t count_code_points(basic_string_view<Char> s) {
  return s.size();
}

// Counts the number of code points in a UTF-8 string.
inline size_t count_code_points(basic_string_view<char8_t> s) {
  const char8_t* data = s.data();
  size_t num_code_points = 0;
  for (size_t i = 0, size = s.size(); i != size; ++i) {
    if ((data[i] & 0xc0) != 0x80) ++num_code_points;
  }
  return num_code_points;
}

template <typename Char>
inline size_t code_point_index(basic_string_view<Char> s, size_t n) {
  size_t size = s.size();
  return n < size ? n : size;
}

// Calculates the index of the nth code point in a UTF-8 string.
inline size_t code_point_index(basic_string_view<char8_t> s, size_t n) {
  const char8_t* data = s.data();
  size_t num_code_points = 0;
  for (size_t i = 0, size = s.size(); i != size; ++i) {
    if ((data[i] & 0xc0) != 0x80 && ++num_code_points > n) {
      return i;
    }
  }
  return s.size();
}

inline char8_t to_char8_t(char c) { return static_cast<char8_t>(c); }

template <typename InputIt, typename OutChar>
using needs_conversion = bool_constant<
    std::is_same<typename std::iterator_traits<InputIt>::value_type,
                 char>::value &&
    std::is_same<OutChar, char8_t>::value>;

template <typename OutChar, typename InputIt, typename OutputIt,
          FMT_ENABLE_IF(!needs_conversion<InputIt, OutChar>::value)>
OutputIt copy_str(InputIt begin, InputIt end, OutputIt it) {
  return std::copy(begin, end, it);
}

template <typename OutChar, typename InputIt, typename OutputIt,
          FMT_ENABLE_IF(needs_conversion<InputIt, OutChar>::value)>
OutputIt copy_str(InputIt begin, InputIt end, OutputIt it) {
  return std::transform(begin, end, it, to_char8_t);
}

#ifndef FMT_USE_GRISU
#  define FMT_USE_GRISU 1
#endif

template <typename T> constexpr bool use_grisu() {
  return FMT_USE_GRISU && std::numeric_limits<double>::is_iec559 &&
         sizeof(T) <= sizeof(double);
}

template <typename T>
template <typename U>
void buffer<T>::append(const U* begin, const U* end) {
  std::size_t new_size = size_ + to_unsigned(end - begin);
  reserve(new_size);
  std::uninitialized_copy(begin, end, make_checked(ptr_, capacity_) + size_);
  size_ = new_size;
}
}  // namespace internal

// A range with an iterator appending to a buffer.
template <typename T>
class buffer_range : public internal::output_range<
                         std::back_insert_iterator<internal::buffer<T>>, T> {
 public:
  using iterator = std::back_insert_iterator<internal::buffer<T>>;
  using internal::output_range<iterator, T>::output_range;
  buffer_range(internal::buffer<T>& buf)
      : internal::output_range<iterator, T>(std::back_inserter(buf)) {}
};

class FMT_DEPRECATED u8string_view : public basic_string_view<char8_t> {
 public:
  u8string_view(const char* s)
      : basic_string_view<char8_t>(reinterpret_cast<const char8_t*>(s)) {}
  u8string_view(const char* s, size_t count) FMT_NOEXCEPT
      : basic_string_view<char8_t>(reinterpret_cast<const char8_t*>(s), count) {
  }
};

#if FMT_USE_USER_DEFINED_LITERALS
inline namespace literals {
inline basic_string_view<char8_t> operator"" _u(const char* s, std::size_t n) {
  return {reinterpret_cast<const char8_t*>(s), n};
}
}  // namespace literals
#endif

// The number of characters to store in the basic_memory_buffer object itself
// to avoid dynamic memory allocation.
enum { inline_buffer_size = 500 };

/**
  \rst
  A dynamically growing memory buffer for trivially copyable/constructible types
  with the first ``SIZE`` elements stored in the object itself.

  You can use one of the following type aliases for common character types:

  +----------------+------------------------------+
  | Type           | Definition                   |
  +================+==============================+
  | memory_buffer  | basic_memory_buffer<char>    |
  +----------------+------------------------------+
  | wmemory_buffer | basic_memory_buffer<wchar_t> |
  +----------------+------------------------------+

  **Example**::

     fmt::memory_buffer out;
     format_to(out, "The answer is {}.", 42);

  This will append the following output to the ``out`` object:

  .. code-block:: none

     The answer is 42.

  The output can be converted to an ``std::string`` with ``to_string(out)``.
  \endrst
 */
template <typename T, std::size_t SIZE = inline_buffer_size,
          typename Allocator = std::allocator<T>>
class basic_memory_buffer : private Allocator, public internal::buffer<T> {
 private:
  T store_[SIZE];

  // Deallocate memory allocated by the buffer.
  void deallocate() {
    T* data = this->data();
    if (data != store_) Allocator::deallocate(data, this->capacity());
  }

 protected:
  void grow(std::size_t size) FMT_OVERRIDE;

 public:
  using value_type = T;
  using const_reference = const T&;

  explicit basic_memory_buffer(const Allocator& alloc = Allocator())
      : Allocator(alloc) {
    this->set(store_, SIZE);
  }
  ~basic_memory_buffer() FMT_OVERRIDE { deallocate(); }

 private:
  // Move data from other to this buffer.
  void move(basic_memory_buffer& other) {
    Allocator &this_alloc = *this, &other_alloc = other;
    this_alloc = std::move(other_alloc);
    T* data = other.data();
    std::size_t size = other.size(), capacity = other.capacity();
    if (data == other.store_) {
      this->set(store_, capacity);
      std::uninitialized_copy(other.store_, other.store_ + size,
                              internal::make_checked(store_, capacity));
    } else {
      this->set(data, capacity);
      // Set pointer to the inline array so that delete is not called
      // when deallocating.
      other.set(other.store_, 0);
    }
    this->resize(size);
  }

 public:
  /**
    \rst
    Constructs a :class:`fmt::basic_memory_buffer` object moving the content
    of the other object to it.
    \endrst
   */
  basic_memory_buffer(basic_memory_buffer&& other) FMT_NOEXCEPT { move(other); }

  /**
    \rst
    Moves the content of the other ``basic_memory_buffer`` object to this one.
    \endrst
   */
  basic_memory_buffer& operator=(basic_memory_buffer&& other) FMT_NOEXCEPT {
    FMT_ASSERT(this != &other, "");
    deallocate();
    move(other);
    return *this;
  }

  // Returns a copy of the allocator associated with this buffer.
  Allocator get_allocator() const { return *this; }
};

template <typename T, std::size_t SIZE, typename Allocator>
void basic_memory_buffer<T, SIZE, Allocator>::grow(std::size_t size) {
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  if (size > 1000) throw std::runtime_error("fuzz mode - won't grow that much");
#endif
  std::size_t old_capacity = this->capacity();
  std::size_t new_capacity = old_capacity + old_capacity / 2;
  if (size > new_capacity) new_capacity = size;
  T* old_data = this->data();
  T* new_data = std::allocator_traits<Allocator>::allocate(*this, new_capacity);
  // The following code doesn't throw, so the raw pointer above doesn't leak.
  std::uninitialized_copy(old_data, old_data + this->size(),
                          internal::make_checked(new_data, new_capacity));
  this->set(new_data, new_capacity);
  // deallocate must not throw according to the standard, but even if it does,
  // the buffer already uses the new storage and will deallocate it in
  // destructor.
  if (old_data != store_) Allocator::deallocate(old_data, old_capacity);
}

using memory_buffer = basic_memory_buffer<char>;
using wmemory_buffer = basic_memory_buffer<wchar_t>;

/** A formatting error such as invalid format string. */
FMT_CLASS_API
class FMT_API format_error : public std::runtime_error {
 public:
  explicit format_error(const char* message) : std::runtime_error(message) {}
  explicit format_error(const std::string& message)
      : std::runtime_error(message) {}
  format_error(const format_error&) = default;
  format_error& operator=(const format_error&) = default;
  format_error(format_error&&) = default;
  format_error& operator=(format_error&&) = default;
  ~format_error() FMT_NOEXCEPT FMT_OVERRIDE;
};

namespace internal {

// Returns true if value is negative, false otherwise.
// Same as `value < 0` but doesn't produce warnings if T is an unsigned type.
template <typename T, FMT_ENABLE_IF(std::numeric_limits<T>::is_signed)>
FMT_CONSTEXPR bool is_negative(T value) {
  return value < 0;
}
template <typename T, FMT_ENABLE_IF(!std::numeric_limits<T>::is_signed)>
FMT_CONSTEXPR bool is_negative(T) {
  return false;
}

// Smallest of uint32_t, uint64_t, uint128_t that is large enough to
// represent all values of T.
template <typename T>
using uint32_or_64_or_128_t = conditional_t<
    std::numeric_limits<T>::digits <= 32, uint32_t,
    conditional_t<std::numeric_limits<T>::digits <= 64, uint64_t, uint128_t>>;

// Static data is placed in this class template for the header-only config.
template <typename T = void> struct FMT_EXTERN_TEMPLATE_API basic_data {
  static const uint64_t powers_of_10_64[];
  static const uint32_t zero_or_powers_of_10_32[];
  static const uint64_t zero_or_powers_of_10_64[];
  static const uint64_t pow10_significands[];
  static const int16_t pow10_exponents[];
  static const char digits[];
  static const char hex_digits[];
  static const char foreground_color[];
  static const char background_color[];
  static const char reset_color[5];
  static const wchar_t wreset_color[5];
  static const char signs[];
};

FMT_EXTERN template struct basic_data<void>;

// This is a struct rather than an alias to avoid shadowing warnings in gcc.
struct data : basic_data<> {};

#ifdef FMT_BUILTIN_CLZLL
// Returns the number of decimal digits in n. Leading zeros are not counted
// except for n == 0 in which case count_digits returns 1.
inline int count_digits(uint64_t n) {
  // Based on http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
  // and the benchmark https://github.com/localvoid/cxx-benchmark-count-digits.
  int t = (64 - FMT_BUILTIN_CLZLL(n | 1)) * 1233 >> 12;
  return t - (n < data::zero_or_powers_of_10_64[t]) + 1;
}
#else
// Fallback version of count_digits used when __builtin_clz is not available.
inline int count_digits(uint64_t n) {
  int count = 1;
  for (;;) {
    // Integer division is slow so do it for a group of four digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    if (n < 10) return count;
    if (n < 100) return count + 1;
    if (n < 1000) return count + 2;
    if (n < 10000) return count + 3;
    n /= 10000u;
    count += 4;
  }
}
#endif

#if FMT_USE_INT128
inline int count_digits(uint128_t n) {
  int count = 1;
  for (;;) {
    // Integer division is slow so do it for a group of four digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    if (n < 10) return count;
    if (n < 100) return count + 1;
    if (n < 1000) return count + 2;
    if (n < 10000) return count + 3;
    n /= 10000U;
    count += 4;
  }
}
#endif

// Counts the number of digits in n. BITS = log2(radix).
template <unsigned BITS, typename UInt> inline int count_digits(UInt n) {
  int num_digits = 0;
  do {
    ++num_digits;
  } while ((n >>= BITS) != 0);
  return num_digits;
}

template <> int count_digits<4>(internal::fallback_uintptr n);

#if FMT_GCC_VERSION || FMT_CLANG_VERSION
#  define FMT_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#  define FMT_ALWAYS_INLINE
#endif

#ifdef FMT_BUILTIN_CLZ
// Optional version of count_digits for better performance on 32-bit platforms.
inline int count_digits(uint32_t n) {
  int t = (32 - FMT_BUILTIN_CLZ(n | 1)) * 1233 >> 12;
  return t - (n < data::zero_or_powers_of_10_32[t]) + 1;
}
#endif

template <typename Char> FMT_API std::string grouping_impl(locale_ref loc);
template <typename Char> inline std::string grouping(locale_ref loc) {
  return grouping_impl<char>(loc);
}
template <> inline std::string grouping<wchar_t>(locale_ref loc) {
  return grouping_impl<wchar_t>(loc);
}

template <typename Char> FMT_API Char thousands_sep_impl(locale_ref loc);
template <typename Char> inline Char thousands_sep(locale_ref loc) {
  return Char(thousands_sep_impl<char>(loc));
}
template <> inline wchar_t thousands_sep(locale_ref loc) {
  return thousands_sep_impl<wchar_t>(loc);
}

template <typename Char> FMT_API Char decimal_point_impl(locale_ref loc);
template <typename Char> inline Char decimal_point(locale_ref loc) {
  return Char(decimal_point_impl<char>(loc));
}
template <> inline wchar_t decimal_point(locale_ref loc) {
  return decimal_point_impl<wchar_t>(loc);
}

// Formats a decimal unsigned integer value writing into buffer.
// add_thousands_sep is called after writing each char to add a thousands
// separator if necessary.
template <typename UInt, typename Char, typename F>
inline Char* format_decimal(Char* buffer, UInt value, int num_digits,
                            F add_thousands_sep) {
  FMT_ASSERT(num_digits >= 0, "invalid digit count");
  buffer += num_digits;
  Char* end = buffer;
  while (value >= 100) {
    // Integer division is slow so do it for a group of two digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    auto index = static_cast<unsigned>((value % 100) * 2);
    value /= 100;
    *--buffer = static_cast<Char>(data::digits[index + 1]);
    add_thousands_sep(buffer);
    *--buffer = static_cast<Char>(data::digits[index]);
    add_thousands_sep(buffer);
  }
  if (value < 10) {
    *--buffer = static_cast<Char>('0' + value);
    return end;
  }
  auto index = static_cast<unsigned>(value * 2);
  *--buffer = static_cast<Char>(data::digits[index + 1]);
  add_thousands_sep(buffer);
  *--buffer = static_cast<Char>(data::digits[index]);
  return end;
}

template <typename Int> constexpr int digits10() FMT_NOEXCEPT {
  return std::numeric_limits<Int>::digits10;
}
template <> constexpr int digits10<int128_t>() FMT_NOEXCEPT { return 38; }
template <> constexpr int digits10<uint128_t>() FMT_NOEXCEPT { return 38; }

template <typename Char, typename UInt, typename Iterator, typename F>
inline Iterator format_decimal(Iterator out, UInt value, int num_digits,
                               F add_thousands_sep) {
  FMT_ASSERT(num_digits >= 0, "invalid digit count");
  // Buffer should be large enough to hold all digits (<= digits10 + 1).
  enum { max_size = digits10<UInt>() + 1 };
  Char buffer[2 * max_size];
  auto end = format_decimal(buffer, value, num_digits, add_thousands_sep);
  return internal::copy_str<Char>(buffer, end, out);
}

template <typename Char, typename It, typename UInt>
inline It format_decimal(It out, UInt value, int num_digits) {
  return format_decimal<Char>(out, value, num_digits, [](Char*) {});
}

template <unsigned BASE_BITS, typename Char, typename UInt>
inline Char* format_uint(Char* buffer, UInt value, int num_digits,
                         bool upper = false) {
  buffer += num_digits;
  Char* end = buffer;
  do {
    const char* digits = upper ? "0123456789ABCDEF" : data::hex_digits;
    unsigned digit = (value & ((1 << BASE_BITS) - 1));
    *--buffer = static_cast<Char>(BASE_BITS < 4 ? static_cast<char>('0' + digit)
                                                : digits[digit]);
  } while ((value >>= BASE_BITS) != 0);
  return end;
}

template <unsigned BASE_BITS, typename Char>
Char* format_uint(Char* buffer, internal::fallback_uintptr n, int num_digits,
                  bool = false) {
  auto char_digits = std::numeric_limits<unsigned char>::digits / 4;
  int start = (num_digits + char_digits - 1) / char_digits - 1;
  if (int start_digits = num_digits % char_digits) {
    unsigned value = n.value[start--];
    buffer = format_uint<BASE_BITS>(buffer, value, start_digits);
  }
  for (; start >= 0; --start) {
    unsigned value = n.value[start];
    buffer += char_digits;
    auto p = buffer;
    for (int i = 0; i < char_digits; ++i) {
      unsigned digit = (value & ((1 << BASE_BITS) - 1));
      *--p = static_cast<Char>(data::hex_digits[digit]);
      value >>= BASE_BITS;
    }
  }
  return buffer;
}

template <unsigned BASE_BITS, typename Char, typename It, typename UInt>
inline It format_uint(It out, UInt value, int num_digits, bool upper = false) {
  // Buffer should be large enough to hold all digits (digits / BASE_BITS + 1).
  char buffer[num_bits<UInt>() / BASE_BITS + 1];
  format_uint<BASE_BITS>(buffer, value, num_digits, upper);
  return internal::copy_str<Char>(buffer, buffer + num_digits, out);
}

// A converter from UTF-8 to UTF-16.
class utf8_to_utf16 {
 private:
  wmemory_buffer buffer_;

 public:
  FMT_API explicit utf8_to_utf16(string_view s);
  operator wstring_view() const { return {&buffer_[0], size()}; }
  size_t size() const { return buffer_.size() - 1; }
  const wchar_t* c_str() const { return &buffer_[0]; }
  std::wstring str() const { return {&buffer_[0], size()}; }
};

template <typename T = void> struct null {};

// Workaround an array initialization issue in gcc 4.8.
template <typename Char> struct fill_t {
 private:
  enum { max_size = 4 };
  Char data_[max_size];
  unsigned char size_;

 public:
  FMT_CONSTEXPR void operator=(basic_string_view<Char> s) {
    auto size = s.size();
    if (size > max_size) {
      FMT_THROW(format_error("invalid fill"));
      return;
    }
    for (size_t i = 0; i < size; ++i) data_[i] = s[i];
    size_ = static_cast<unsigned char>(size);
  }

  size_t size() const { return size_; }
  const Char* data() const { return data_; }

  FMT_CONSTEXPR Char& operator[](size_t index) { return data_[index]; }
  FMT_CONSTEXPR const Char& operator[](size_t index) const {
    return data_[index];
  }

  static FMT_CONSTEXPR fill_t<Char> make() {
    auto fill = fill_t<Char>();
    fill[0] = Char(' ');
    fill.size_ = 1;
    return fill;
  }
};
}  // namespace internal

// We cannot use enum classes as bit fields because of a gcc bug
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61414.
namespace align {
enum type { none, left, right, center, numeric };
}
using align_t = align::type;

namespace sign {
enum type { none, minus, plus, space };
}
using sign_t = sign::type;

// Format specifiers for built-in and string types.
template <typename Char> struct basic_format_specs {
  int width;
  int precision;
  char type;
  align_t align : 4;
  sign_t sign : 3;
  bool alt : 1;  // Alternate form ('#').
  internal::fill_t<Char> fill;

  constexpr basic_format_specs()
      : width(0),
        precision(-1),
        type(0),
        align(align::none),
        sign(sign::none),
        alt(false),
        fill(internal::fill_t<Char>::make()) {}
};

using format_specs = basic_format_specs<char>;

namespace internal {

// A floating-point presentation format.
enum class float_format : unsigned char {
  general,  // General: exponent notation or fixed point based on magnitude.
  exp,      // Exponent notation with the default precision of 6, e.g. 1.2e-3.
  fixed,    // Fixed point with the default precision of 6, e.g. 0.0012.
  hex
};

struct float_specs {
  int precision;
  float_format format : 8;
  sign_t sign : 8;
  bool upper : 1;
  bool locale : 1;
  bool percent : 1;
  bool binary32 : 1;
  bool use_grisu : 1;
  bool showpoint : 1;
};

// Writes the exponent exp in the form "[+-]d{2,3}" to buffer.
template <typename Char, typename It> It write_exponent(int exp, It it) {
  FMT_ASSERT(-10000 < exp && exp < 10000, "exponent out of range");
  if (exp < 0) {
    *it++ = static_cast<Char>('-');
    exp = -exp;
  } else {
    *it++ = static_cast<Char>('+');
  }
  if (exp >= 100) {
    const char* top = data::digits + (exp / 100) * 2;
    if (exp >= 1000) *it++ = static_cast<Char>(top[0]);
    *it++ = static_cast<Char>(top[1]);
    exp %= 100;
  }
  const char* d = data::digits + exp * 2;
  *it++ = static_cast<Char>(d[0]);
  *it++ = static_cast<Char>(d[1]);
  return it;
}

template <typename Char> class float_writer {
 private:
  // The number is given as v = digits_ * pow(10, exp_).
  const char* digits_;
  int num_digits_;
  int exp_;
  size_t size_;
  float_specs specs_;
  Char decimal_point_;

  template <typename It> It prettify(It it) const {
    // pow(10, full_exp - 1) <= v <= pow(10, full_exp).
    int full_exp = num_digits_ + exp_;
    if (specs_.format == float_format::exp) {
      // Insert a decimal point after the first digit and add an exponent.
      *it++ = static_cast<Char>(*digits_);
      int num_zeros = specs_.precision - num_digits_;
      if (num_digits_ > 1 || specs_.showpoint) *it++ = decimal_point_;
      it = copy_str<Char>(digits_ + 1, digits_ + num_digits_, it);
      if (num_zeros > 0 && specs_.showpoint)
        it = std::fill_n(it, num_zeros, static_cast<Char>('0'));
      *it++ = static_cast<Char>(specs_.upper ? 'E' : 'e');
      return write_exponent<Char>(full_exp - 1, it);
    }
    if (num_digits_ <= full_exp) {
      // 1234e7 -> 12340000000[.0+]
      it = copy_str<Char>(digits_, digits_ + num_digits_, it);
      it = std::fill_n(it, full_exp - num_digits_, static_cast<Char>('0'));
      if (specs_.showpoint || specs_.precision < 0) {
        *it++ = decimal_point_;
        int num_zeros = specs_.precision - full_exp;
        if (num_zeros <= 0) {
          if (specs_.format != float_format::fixed)
            *it++ = static_cast<Char>('0');
          return it;
        }
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        if (num_zeros > 1000)
          throw std::runtime_error("fuzz mode - avoiding excessive cpu use");
#endif
        it = std::fill_n(it, num_zeros, static_cast<Char>('0'));
      }
    } else if (full_exp > 0) {
      // 1234e-2 -> 12.34[0+]
      it = copy_str<Char>(digits_, digits_ + full_exp, it);
      if (!specs_.showpoint) {
        // Remove trailing zeros.
        int num_digits = num_digits_;
        while (num_digits > full_exp && digits_[num_digits - 1] == '0')
          --num_digits;
        if (num_digits != full_exp) *it++ = decimal_point_;
        return copy_str<Char>(digits_ + full_exp, digits_ + num_digits, it);
      }
      *it++ = decimal_point_;
      it = copy_str<Char>(digits_ + full_exp, digits_ + num_digits_, it);
      if (specs_.precision > num_digits_) {
        // Add trailing zeros.
        int num_zeros = specs_.precision - num_digits_;
        it = std::fill_n(it, num_zeros, static_cast<Char>('0'));
      }
    } else {
      // 1234e-6 -> 0.001234
      *it++ = static_cast<Char>('0');
      int num_zeros = -full_exp;
      if (specs_.precision >= 0 && specs_.precision < num_zeros)
        num_zeros = specs_.precision;
      int num_digits = num_digits_;
      // Remove trailing zeros.
      if (!specs_.showpoint)
        while (num_digits > 0 && digits_[num_digits - 1] == '0') --num_digits;
      if (num_zeros != 0 || num_digits != 0 || specs_.showpoint) {
        *it++ = decimal_point_;
        it = std::fill_n(it, num_zeros, static_cast<Char>('0'));
        it = copy_str<Char>(digits_, digits_ + num_digits, it);
      }
    }
    return it;
  }

 public:
  float_writer(const char* digits, int num_digits, int exp, float_specs specs,
               Char decimal_point)
      : digits_(digits),
        num_digits_(num_digits),
        exp_(exp),
        specs_(specs),
        decimal_point_(decimal_point) {
    int full_exp = num_digits + exp - 1;
    int precision = specs.precision > 0 ? specs.precision : 16;
    if (specs_.format == float_format::general &&
        !(full_exp >= -4 && full_exp < precision)) {
      specs_.format = float_format::exp;
    }
    size_ = prettify(counting_iterator()).count();
    size_ += specs.sign ? 1 : 0;
  }

  size_t size() const { return size_; }
  size_t width() const { return size(); }

  template <typename It> void operator()(It&& it) {
    if (specs_.sign) *it++ = static_cast<Char>(data::signs[specs_.sign]);
    it = prettify(it);
  }
};

template <typename T>
int format_float(T value, int precision, float_specs specs, buffer<char>& buf);

// Formats a floating-point number with snprintf.
template <typename T>
int snprintf_float(T value, int precision, float_specs specs,
                   buffer<char>& buf);

template <typename T> T promote_float(T value) { return value; }
inline double promote_float(float value) { return static_cast<double>(value); }

template <typename Handler>
FMT_CONSTEXPR void handle_int_type_spec(char spec, Handler&& handler) {
  switch (spec) {
  case 0:
  case 'd':
    handler.on_dec();
    break;
  case 'x':
  case 'X':
    handler.on_hex();
    break;
  case 'b':
  case 'B':
    handler.on_bin();
    break;
  case 'o':
    handler.on_oct();
    break;
  case 'n':
    handler.on_num();
    break;
  default:
    handler.on_error();
  }
}

template <typename ErrorHandler = error_handler, typename Char>
FMT_CONSTEXPR float_specs parse_float_type_spec(
    const basic_format_specs<Char>& specs, ErrorHandler&& eh = {}) {
  auto result = float_specs();
  result.showpoint = specs.alt;
  switch (specs.type) {
  case 0:
    result.format = float_format::general;
    result.showpoint |= specs.precision > 0;
    break;
  case 'G':
    result.upper = true;
    FMT_FALLTHROUGH;
  case 'g':
    result.format = float_format::general;
    break;
  case 'E':
    result.upper = true;
    FMT_FALLTHROUGH;
  case 'e':
    result.format = float_format::exp;
    result.showpoint |= specs.precision != 0;
    break;
  case 'F':
    result.upper = true;
    FMT_FALLTHROUGH;
  case 'f':
    result.format = float_format::fixed;
    result.showpoint |= specs.precision != 0;
    break;
#if FMT_DEPRECATED_PERCENT
  case '%':
    result.format = float_format::fixed;
    result.percent = true;
    break;
#endif
  case 'A':
    result.upper = true;
    FMT_FALLTHROUGH;
  case 'a':
    result.format = float_format::hex;
    break;
  case 'n':
    result.locale = true;
    break;
  default:
    eh.on_error("invalid type specifier");
    break;
  }
  return result;
}

template <typename Char, typename Handler>
FMT_CONSTEXPR void handle_char_specs(const basic_format_specs<Char>* specs,
                                     Handler&& handler) {
  if (!specs) return handler.on_char();
  if (specs->type && specs->type != 'c') return handler.on_int();
  if (specs->align == align::numeric || specs->sign != sign::none || specs->alt)
    handler.on_error("invalid format specifier for char");
  handler.on_char();
}

template <typename Char, typename Handler>
FMT_CONSTEXPR void handle_cstring_type_spec(Char spec, Handler&& handler) {
  if (spec == 0 || spec == 's')
    handler.on_string();
  else if (spec == 'p')
    handler.on_pointer();
  else
    handler.on_error("invalid type specifier");
}

template <typename Char, typename ErrorHandler>
FMT_CONSTEXPR void check_string_type_spec(Char spec, ErrorHandler&& eh) {
  if (spec != 0 && spec != 's') eh.on_error("invalid type specifier");
}

template <typename Char, typename ErrorHandler>
FMT_CONSTEXPR void check_pointer_type_spec(Char spec, ErrorHandler&& eh) {
  if (spec != 0 && spec != 'p') eh.on_error("invalid type specifier");
}

template <typename ErrorHandler> class int_type_checker : private ErrorHandler {
 public:
  FMT_CONSTEXPR explicit int_type_checker(ErrorHandler eh) : ErrorHandler(eh) {}

  FMT_CONSTEXPR void on_dec() {}
  FMT_CONSTEXPR void on_hex() {}
  FMT_CONSTEXPR void on_bin() {}
  FMT_CONSTEXPR void on_oct() {}
  FMT_CONSTEXPR void on_num() {}

  FMT_CONSTEXPR void on_error() {
    ErrorHandler::on_error("invalid type specifier");
  }
};

template <typename ErrorHandler>
class char_specs_checker : public ErrorHandler {
 private:
  char type_;

 public:
  FMT_CONSTEXPR char_specs_checker(char type, ErrorHandler eh)
      : ErrorHandler(eh), type_(type) {}

  FMT_CONSTEXPR void on_int() {
    handle_int_type_spec(type_, int_type_checker<ErrorHandler>(*this));
  }
  FMT_CONSTEXPR void on_char() {}
};

template <typename ErrorHandler>
class cstring_type_checker : public ErrorHandler {
 public:
  FMT_CONSTEXPR explicit cstring_type_checker(ErrorHandler eh)
      : ErrorHandler(eh) {}

  FMT_CONSTEXPR void on_string() {}
  FMT_CONSTEXPR void on_pointer() {}
};

template <typename Context>
void arg_map<Context>::init(const basic_format_args<Context>& args) {
  if (map_) return;
  map_ = new entry[internal::to_unsigned(args.max_size())];
  if (args.is_packed()) {
    for (int i = 0;; ++i) {
      internal::type arg_type = args.type(i);
      if (arg_type == internal::type::none_type) return;
      if (arg_type == internal::type::named_arg_type)
        push_back(args.values_[i]);
    }
  }
  for (int i = 0, n = args.max_size(); i < n; ++i) {
    auto type = args.args_[i].type_;
    if (type == internal::type::named_arg_type) push_back(args.args_[i].value_);
  }
}

template <typename Char> struct nonfinite_writer {
  sign_t sign;
  const char* str;
  static constexpr size_t str_size = 3;

  size_t size() const { return str_size + (sign ? 1 : 0); }
  size_t width() const { return size(); }

  template <typename It> void operator()(It&& it) const {
    if (sign) *it++ = static_cast<Char>(data::signs[sign]);
    it = copy_str<Char>(str, str + str_size, it);
  }
};

template <typename OutputIt, typename Char>
OutputIt fill(OutputIt it, size_t n, const fill_t<Char>& fill) {
  auto fill_size = fill.size();
  if (fill_size == 1) return std::fill_n(it, n, fill[0]);
  for (size_t i = 0; i < n; ++i) it = std::copy_n(fill.data(), fill_size, it);
  return it;
}

// This template provides operations for formatting and writing data into a
// character range.
template <typename Range> class basic_writer {
 public:
  using char_type = typename Range::value_type;
  using iterator = typename Range::iterator;
  using format_specs = basic_format_specs<char_type>;

 private:
  iterator out_;  // Output iterator.
  locale_ref locale_;

  // Attempts to reserve space for n extra characters in the output range.
  // Returns a pointer to the reserved range or a reference to out_.
  auto reserve(std::size_t n) -> decltype(internal::reserve(out_, n)) {
    return internal::reserve(out_, n);
  }

  template <typename F> struct padded_int_writer {
    size_t size_;
    string_view prefix;
    char_type fill;
    std::size_t padding;
    F f;

    size_t size() const { return size_; }
    size_t width() const { return size_; }

    template <typename It> void operator()(It&& it) const {
      if (prefix.size() != 0)
        it = copy_str<char_type>(prefix.begin(), prefix.end(), it);
      it = std::fill_n(it, padding, fill);
      f(it);
    }
  };

  // Writes an integer in the format
  //   <left-padding><prefix><numeric-padding><digits><right-padding>
  // where <digits> are written by f(it).
  template <typename F>
  void write_int(int num_digits, string_view prefix, format_specs specs, F f) {
    std::size_t size = prefix.size() + to_unsigned(num_digits);
    char_type fill = specs.fill[0];
    std::size_t padding = 0;
    if (specs.align == align::numeric) {
      auto unsiged_width = to_unsigned(specs.width);
      if (unsiged_width > size) {
        padding = unsiged_width - size;
        size = unsiged_width;
      }
    } else if (specs.precision > num_digits) {
      size = prefix.size() + to_unsigned(specs.precision);
      padding = to_unsigned(specs.precision - num_digits);
      fill = static_cast<char_type>('0');
    }
    if (specs.align == align::none) specs.align = align::right;
    write_padded(specs, padded_int_writer<F>{size, prefix, fill, padding, f});
  }

  // Writes a decimal integer.
  template <typename Int> void write_decimal(Int value) {
    auto abs_value = static_cast<uint32_or_64_or_128_t<Int>>(value);
    bool negative = is_negative(value);
    // Don't do -abs_value since it trips unsigned-integer-overflow sanitizer.
    if (negative) abs_value = ~abs_value + 1;
    int num_digits = count_digits(abs_value);
    auto&& it = reserve((negative ? 1 : 0) + static_cast<size_t>(num_digits));
    if (negative) *it++ = static_cast<char_type>('-');
    it = format_decimal<char_type>(it, abs_value, num_digits);
  }

  // The handle_int_type_spec handler that writes an integer.
  template <typename Int, typename Specs> struct int_writer {
    using unsigned_type = uint32_or_64_or_128_t<Int>;

    basic_writer<Range>& writer;
    const Specs& specs;
    unsigned_type abs_value;
    char prefix[4];
    unsigned prefix_size;

    string_view get_prefix() const { return string_view(prefix, prefix_size); }

    int_writer(basic_writer<Range>& w, Int value, const Specs& s)
        : writer(w),
          specs(s),
          abs_value(static_cast<unsigned_type>(value)),
          prefix_size(0) {
      if (is_negative(value)) {
        prefix[0] = '-';
        ++prefix_size;
        abs_value = 0 - abs_value;
      } else if (specs.sign != sign::none && specs.sign != sign::minus) {
        prefix[0] = specs.sign == sign::plus ? '+' : ' ';
        ++prefix_size;
      }
    }

    struct dec_writer {
      unsigned_type abs_value;
      int num_digits;

      template <typename It> void operator()(It&& it) const {
        it = internal::format_decimal<char_type>(it, abs_value, num_digits);
      }
    };

    void on_dec() {
      int num_digits = count_digits(abs_value);
      writer.write_int(num_digits, get_prefix(), specs,
                       dec_writer{abs_value, num_digits});
    }

    struct hex_writer {
      int_writer& self;
      int num_digits;

      template <typename It> void operator()(It&& it) const {
        it = format_uint<4, char_type>(it, self.abs_value, num_digits,
                                       self.specs.type != 'x');
      }
    };

    void on_hex() {
      if (specs.alt) {
        prefix[prefix_size++] = '0';
        prefix[prefix_size++] = specs.type;
      }
      int num_digits = count_digits<4>(abs_value);
      writer.write_int(num_digits, get_prefix(), specs,
                       hex_writer{*this, num_digits});
    }

    template <int BITS> struct bin_writer {
      unsigned_type abs_value;
      int num_digits;

      template <typename It> void operator()(It&& it) const {
        it = format_uint<BITS, char_type>(it, abs_value, num_digits);
      }
    };

    void on_bin() {
      if (specs.alt) {
        prefix[prefix_size++] = '0';
        prefix[prefix_size++] = static_cast<char>(specs.type);
      }
      int num_digits = count_digits<1>(abs_value);
      writer.write_int(num_digits, get_prefix(), specs,
                       bin_writer<1>{abs_value, num_digits});
    }

    void on_oct() {
      int num_digits = count_digits<3>(abs_value);
      if (specs.alt && specs.precision <= num_digits && abs_value != 0) {
        // Octal prefix '0' is counted as a digit, so only add it if precision
        // is not greater than the number of digits.
        prefix[prefix_size++] = '0';
      }
      writer.write_int(num_digits, get_prefix(), specs,
                       bin_writer<3>{abs_value, num_digits});
    }

    enum { sep_size = 1 };

    struct num_writer {
      unsigned_type abs_value;
      int size;
      const std::string& groups;
      char_type sep;

      template <typename It> void operator()(It&& it) const {
        basic_string_view<char_type> s(&sep, sep_size);
        // Index of a decimal digit with the least significant digit having
        // index 0.
        int digit_index = 0;
        std::string::const_iterator group = groups.cbegin();
        it = format_decimal<char_type>(
            it, abs_value, size,
            [this, s, &group, &digit_index](char_type*& buffer) {
              if (*group <= 0 || ++digit_index % *group != 0 ||
                  *group == max_value<char>())
                return;
              if (group + 1 != groups.cend()) {
                digit_index = 0;
                ++group;
              }
              buffer -= s.size();
              std::uninitialized_copy(s.data(), s.data() + s.size(),
                                      make_checked(buffer, s.size()));
            });
      }
    };

    void on_num() {
      std::string groups = grouping<char_type>(writer.locale_);
      if (groups.empty()) return on_dec();
      auto sep = thousands_sep<char_type>(writer.locale_);
      if (!sep) return on_dec();
      int num_digits = count_digits(abs_value);
      int size = num_digits;
      std::string::const_iterator group = groups.cbegin();
      while (group != groups.cend() && num_digits > *group && *group > 0 &&
             *group != max_value<char>()) {
        size += sep_size;
        num_digits -= *group;
        ++group;
      }
      if (group == groups.cend())
        size += sep_size * ((num_digits - 1) / groups.back());
      writer.write_int(size, get_prefix(), specs,
                       num_writer{abs_value, size, groups, sep});
    }

    FMT_NORETURN void on_error() {
      FMT_THROW(format_error("invalid type specifier"));
    }
  };

  template <typename Char> struct str_writer {
    const Char* s;
    size_t size_;

    size_t size() const { return size_; }
    size_t width() const {
      return count_code_points(basic_string_view<Char>(s, size_));
    }

    template <typename It> void operator()(It&& it) const {
      it = copy_str<char_type>(s, s + size_, it);
    }
  };

  template <typename UIntPtr> struct pointer_writer {
    UIntPtr value;
    int num_digits;

    size_t size() const { return to_unsigned(num_digits) + 2; }
    size_t width() const { return size(); }

    template <typename It> void operator()(It&& it) const {
      *it++ = static_cast<char_type>('0');
      *it++ = static_cast<char_type>('x');
      it = format_uint<4, char_type>(it, value, num_digits);
    }
  };

 public:
  explicit basic_writer(Range out, locale_ref loc = locale_ref())
      : out_(out.begin()), locale_(loc) {}

  iterator out() const { return out_; }

  // Writes a value in the format
  //   <left-padding><value><right-padding>
  // where <value> is written by f(it).
  template <typename F> void write_padded(const format_specs& specs, F&& f) {
    // User-perceived width (in code points).
    unsigned width = to_unsigned(specs.width);
    size_t size = f.size();  // The number of code units.
    size_t num_code_points = width != 0 ? f.width() : size;
    if (width <= num_code_points) return f(reserve(size));
    size_t padding = width - num_code_points;
    size_t fill_size = specs.fill.size();
    auto&& it = reserve(size + padding * fill_size);
    if (specs.align == align::right) {
      it = fill(it, padding, specs.fill);
      f(it);
    } else if (specs.align == align::center) {
      std::size_t left_padding = padding / 2;
      it = fill(it, left_padding, specs.fill);
      f(it);
      it = fill(it, padding - left_padding, specs.fill);
    } else {
      f(it);
      it = fill(it, padding, specs.fill);
    }
  }

  void write(int value) { write_decimal(value); }
  void write(long value) { write_decimal(value); }
  void write(long long value) { write_decimal(value); }

  void write(unsigned value) { write_decimal(value); }
  void write(unsigned long value) { write_decimal(value); }
  void write(unsigned long long value) { write_decimal(value); }

#if FMT_USE_INT128
  void write(int128_t value) { write_decimal(value); }
  void write(uint128_t value) { write_decimal(value); }
#endif

  template <typename T, typename Spec>
  void write_int(T value, const Spec& spec) {
    handle_int_type_spec(spec.type, int_writer<T, Spec>(*this, value, spec));
  }

  template <typename T, FMT_ENABLE_IF(std::is_floating_point<T>::value)>
  void write(T value, format_specs specs = {}) {
    float_specs fspecs = parse_float_type_spec(specs);
    fspecs.sign = specs.sign;
    if (std::signbit(value)) {  // value < 0 is false for NaN so use signbit.
      fspecs.sign = sign::minus;
      value = -value;
    } else if (fspecs.sign == sign::minus) {
      fspecs.sign = sign::none;
    }

    if (!std::isfinite(value)) {
      auto str = std::isinf(value) ? (fspecs.upper ? "INF" : "inf")
                                   : (fspecs.upper ? "NAN" : "nan");
      return write_padded(specs, nonfinite_writer<char_type>{fspecs.sign, str});
    }

    if (specs.align == align::none) {
      specs.align = align::right;
    } else if (specs.align == align::numeric) {
      if (fspecs.sign) {
        auto&& it = reserve(1);
        *it++ = static_cast<char_type>(data::signs[fspecs.sign]);
        fspecs.sign = sign::none;
        if (specs.width != 0) --specs.width;
      }
      specs.align = align::right;
    }

    memory_buffer buffer;
    if (fspecs.format == float_format::hex) {
      if (fspecs.sign) buffer.push_back(data::signs[fspecs.sign]);
      snprintf_float(promote_float(value), specs.precision, fspecs, buffer);
      write_padded(specs, str_writer<char>{buffer.data(), buffer.size()});
      return;
    }
    int precision = specs.precision >= 0 || !specs.type ? specs.precision : 6;
    if (fspecs.format == float_format::exp) {
      if (precision == max_value<int>())
        FMT_THROW(format_error("number is too big"));
      else
        ++precision;
    }
    if (const_check(std::is_same<T, float>())) fspecs.binary32 = true;
    fspecs.use_grisu = use_grisu<T>();
    if (const_check(FMT_DEPRECATED_PERCENT) && fspecs.percent) value *= 100;
    int exp = format_float(promote_float(value), precision, fspecs, buffer);
    if (const_check(FMT_DEPRECATED_PERCENT) && fspecs.percent) {
      buffer.push_back('%');
      --exp;  // Adjust decimal place position.
    }
    fspecs.precision = precision;
    char_type point = fspecs.locale ? decimal_point<char_type>(locale_)
                                    : static_cast<char_type>('.');
    write_padded(specs, float_writer<char_type>(buffer.data(),
                                                static_cast<int>(buffer.size()),
                                                exp, fspecs, point));
  }

  void write(char value) {
    auto&& it = reserve(1);
    *it++ = value;
  }

  template <typename Char, FMT_ENABLE_IF(std::is_same<Char, char_type>::value)>
  void write(Char value) {
    auto&& it = reserve(1);
    *it++ = value;
  }

  void write(string_view value) {
    auto&& it = reserve(value.size());
    it = copy_str<char_type>(value.begin(), value.end(), it);
  }
  void write(wstring_view value) {
    static_assert(std::is_same<char_type, wchar_t>::value, "");
    auto&& it = reserve(value.size());
    it = std::copy(value.begin(), value.end(), it);
  }

  template <typename Char>
  void write(const Char* s, std::size_t size, const format_specs& specs) {
    write_padded(specs, str_writer<Char>{s, size});
  }

  template <typename Char>
  void write(basic_string_view<Char> s, const format_specs& specs = {}) {
    const Char* data = s.data();
    std::size_t size = s.size();
    if (specs.precision >= 0 && to_unsigned(specs.precision) < size)
      size = code_point_index(s, to_unsigned(specs.precision));
    write(data, size, specs);
  }

  template <typename UIntPtr>
  void write_pointer(UIntPtr value, const format_specs* specs) {
    int num_digits = count_digits<4>(value);
    auto pw = pointer_writer<UIntPtr>{value, num_digits};
    if (!specs) return pw(reserve(to_unsigned(num_digits) + 2));
    format_specs specs_copy = *specs;
    if (specs_copy.align == align::none) specs_copy.align = align::right;
    write_padded(specs_copy, pw);
  }
};

using writer = basic_writer<buffer_range<char>>;

template <typename T> struct is_integral : std::is_integral<T> {};
template <> struct is_integral<int128_t> : std::true_type {};
template <> struct is_integral<uint128_t> : std::true_type {};

template <typename Range, typename ErrorHandler = internal::error_handler>
class arg_formatter_base {
 public:
  using char_type = typename Range::value_type;
  using iterator = typename Range::iterator;
  using format_specs = basic_format_specs<char_type>;

 private:
  using writer_type = basic_writer<Range>;
  writer_type writer_;
  format_specs* specs_;

  struct char_writer {
    char_type value;

    size_t size() const { return 1; }
    size_t width() const { return 1; }

    template <typename It> void operator()(It&& it) const { *it++ = value; }
  };

  void write_char(char_type value) {
    if (specs_)
      writer_.write_padded(*specs_, char_writer{value});
    else
      writer_.write(value);
  }

  void write_pointer(const void* p) {
    writer_.write_pointer(internal::to_uintptr(p), specs_);
  }

 protected:
  writer_type& writer() { return writer_; }
  FMT_DEPRECATED format_specs* spec() { return specs_; }
  format_specs* specs() { return specs_; }
  iterator out() { return writer_.out(); }

  void write(bool value) {
    string_view sv(value ? "true" : "false");
    specs_ ? writer_.write(sv, *specs_) : writer_.write(sv);
  }

  void write(const char_type* value) {
    if (!value) {
      FMT_THROW(format_error("string pointer is null"));
    } else {
      auto length = std::char_traits<char_type>::length(value);
      basic_string_view<char_type> sv(value, length);
      specs_ ? writer_.write(sv, *specs_) : writer_.write(sv);
    }
  }

 public:
  arg_formatter_base(Range r, format_specs* s, locale_ref loc)
      : writer_(r, loc), specs_(s) {}

  iterator operator()(monostate) {
    FMT_ASSERT(false, "invalid argument type");
    return out();
  }

  template <typename T, FMT_ENABLE_IF(is_integral<T>::value)>
  iterator operator()(T value) {
    if (specs_)
      writer_.write_int(value, *specs_);
    else
      writer_.write(value);
    return out();
  }

  iterator operator()(char_type value) {
    internal::handle_char_specs(
        specs_, char_spec_handler(*this, static_cast<char_type>(value)));
    return out();
  }

  iterator operator()(bool value) {
    if (specs_ && specs_->type) return (*this)(value ? 1 : 0);
    write(value != 0);
    return out();
  }

  template <typename T, FMT_ENABLE_IF(std::is_floating_point<T>::value)>
  iterator operator()(T value) {
    writer_.write(value, specs_ ? *specs_ : format_specs());
    return out();
  }

  struct char_spec_handler : ErrorHandler {
    arg_formatter_base& formatter;
    char_type value;

    char_spec_handler(arg_formatter_base& f, char_type val)
        : formatter(f), value(val) {}

    void on_int() {
      if (formatter.specs_)
        formatter.writer_.write_int(value, *formatter.specs_);
      else
        formatter.writer_.write(value);
    }
    void on_char() { formatter.write_char(value); }
  };

  struct cstring_spec_handler : internal::error_handler {
    arg_formatter_base& formatter;
    const char_type* value;

    cstring_spec_handler(arg_formatter_base& f, const char_type* val)
        : formatter(f), value(val) {}

    void on_string() { formatter.write(value); }
    void on_pointer() { formatter.write_pointer(value); }
  };

  iterator operator()(const char_type* value) {
    if (!specs_) return write(value), out();
    internal::handle_cstring_type_spec(specs_->type,
                                       cstring_spec_handler(*this, value));
    return out();
  }

  iterator operator()(basic_string_view<char_type> value) {
    if (specs_) {
      internal::check_string_type_spec(specs_->type, internal::error_handler());
      writer_.write(value, *specs_);
    } else {
      writer_.write(value);
    }
    return out();
  }

  iterator operator()(const void* value) {
    if (specs_)
      check_pointer_type_spec(specs_->type, internal::error_handler());
    write_pointer(value);
    return out();
  }
};

template <typename Char> FMT_CONSTEXPR bool is_name_start(Char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || '_' == c;
}

// Parses the range [begin, end) as an unsigned integer. This function assumes
// that the range is non-empty and the first character is a digit.
template <typename Char, typename ErrorHandler>
FMT_CONSTEXPR int parse_nonnegative_int(const Char*& begin, const Char* end,
                                        ErrorHandler&& eh) {
  FMT_ASSERT(begin != end && '0' <= *begin && *begin <= '9', "");
  if (*begin == '0') {
    ++begin;
    return 0;
  }
  unsigned value = 0;
  // Convert to unsigned to prevent a warning.
  constexpr unsigned max_int = max_value<int>();
  unsigned big = max_int / 10;
  do {
    // Check for overflow.
    if (value > big) {
      value = max_int + 1;
      break;
    }
    value = value * 10 + unsigned(*begin - '0');
    ++begin;
  } while (begin != end && '0' <= *begin && *begin <= '9');
  if (value > max_int) eh.on_error("number is too big");
  return static_cast<int>(value);
}

template <typename Context> class custom_formatter {
 private:
  using char_type = typename Context::char_type;

  basic_format_parse_context<char_type>& parse_ctx_;
  Context& ctx_;

 public:
  explicit custom_formatter(basic_format_parse_context<char_type>& parse_ctx,
                            Context& ctx)
      : parse_ctx_(parse_ctx), ctx_(ctx) {}

  bool operator()(typename basic_format_arg<Context>::handle h) const {
    h.format(parse_ctx_, ctx_);
    return true;
  }

  template <typename T> bool operator()(T) const { return false; }
};

template <typename T>
using is_integer =
    bool_constant<is_integral<T>::value && !std::is_same<T, bool>::value &&
                  !std::is_same<T, char>::value &&
                  !std::is_same<T, wchar_t>::value>;

template <typename ErrorHandler> class width_checker {
 public:
  explicit FMT_CONSTEXPR width_checker(ErrorHandler& eh) : handler_(eh) {}

  template <typename T, FMT_ENABLE_IF(is_integer<T>::value)>
  FMT_CONSTEXPR unsigned long long operator()(T value) {
    if (is_negative(value)) handler_.on_error("negative width");
    return static_cast<unsigned long long>(value);
  }

  template <typename T, FMT_ENABLE_IF(!is_integer<T>::value)>
  FMT_CONSTEXPR unsigned long long operator()(T) {
    handler_.on_error("width is not integer");
    return 0;
  }

 private:
  ErrorHandler& handler_;
};

template <typename ErrorHandler> class precision_checker {
 public:
  explicit FMT_CONSTEXPR precision_checker(ErrorHandler& eh) : handler_(eh) {}

  template <typename T, FMT_ENABLE_IF(is_integer<T>::value)>
  FMT_CONSTEXPR unsigned long long operator()(T value) {
    if (is_negative(value)) handler_.on_error("negative precision");
    return static_cast<unsigned long long>(value);
  }

  template <typename T, FMT_ENABLE_IF(!is_integer<T>::value)>
  FMT_CONSTEXPR unsigned long long operator()(T) {
    handler_.on_error("precision is not integer");
    return 0;
  }

 private:
  ErrorHandler& handler_;
};

// A format specifier handler that sets fields in basic_format_specs.
template <typename Char> class specs_setter {
 public:
  explicit FMT_CONSTEXPR specs_setter(basic_format_specs<Char>& specs)
      : specs_(specs) {}

  FMT_CONSTEXPR specs_setter(const specs_setter& other)
      : specs_(other.specs_) {}

  FMT_CONSTEXPR void on_align(align_t align) { specs_.align = align; }
  FMT_CONSTEXPR void on_fill(basic_string_view<Char> fill) {
    specs_.fill = fill;
  }
  FMT_CONSTEXPR void on_plus() { specs_.sign = sign::plus; }
  FMT_CONSTEXPR void on_minus() { specs_.sign = sign::minus; }
  FMT_CONSTEXPR void on_space() { specs_.sign = sign::space; }
  FMT_CONSTEXPR void on_hash() { specs_.alt = true; }

  FMT_CONSTEXPR void on_zero() {
    specs_.align = align::numeric;
    specs_.fill[0] = Char('0');
  }

  FMT_CONSTEXPR void on_width(int width) { specs_.width = width; }
  FMT_CONSTEXPR void on_precision(int precision) {
    specs_.precision = precision;
  }
  FMT_CONSTEXPR void end_precision() {}

  FMT_CONSTEXPR void on_type(Char type) {
    specs_.type = static_cast<char>(type);
  }

 protected:
  basic_format_specs<Char>& specs_;
};

template <typename ErrorHandler> class numeric_specs_checker {
 public:
  FMT_CONSTEXPR numeric_specs_checker(ErrorHandler& eh, internal::type arg_type)
      : error_handler_(eh), arg_type_(arg_type) {}

  FMT_CONSTEXPR void require_numeric_argument() {
    if (!is_arithmetic_type(arg_type_))
      error_handler_.on_error("format specifier requires numeric argument");
  }

  FMT_CONSTEXPR void check_sign() {
    require_numeric_argument();
    if (is_integral_type(arg_type_) && arg_type_ != type::int_type &&
        arg_type_ != type::long_long_type && arg_type_ != type::char_type) {
      error_handler_.on_error("format specifier requires signed argument");
    }
  }

  FMT_CONSTEXPR void check_precision() {
    if (is_integral_type(arg_type_) || arg_type_ == type::pointer_type)
      error_handler_.on_error("precision not allowed for this argument type");
  }

 private:
  ErrorHandler& error_handler_;
  internal::type arg_type_;
};

// A format specifier handler that checks if specifiers are consistent with the
// argument type.
template <typename Handler> class specs_checker : public Handler {
 public:
  FMT_CONSTEXPR specs_checker(const Handler& handler, internal::type arg_type)
      : Handler(handler), checker_(*this, arg_type) {}

  FMT_CONSTEXPR specs_checker(const specs_checker& other)
      : Handler(other), checker_(*this, other.arg_type_) {}

  FMT_CONSTEXPR void on_align(align_t align) {
    if (align == align::numeric) checker_.require_numeric_argument();
    Handler::on_align(align);
  }

  FMT_CONSTEXPR void on_plus() {
    checker_.check_sign();
    Handler::on_plus();
  }

  FMT_CONSTEXPR void on_minus() {
    checker_.check_sign();
    Handler::on_minus();
  }

  FMT_CONSTEXPR void on_space() {
    checker_.check_sign();
    Handler::on_space();
  }

  FMT_CONSTEXPR void on_hash() {
    checker_.require_numeric_argument();
    Handler::on_hash();
  }

  FMT_CONSTEXPR void on_zero() {
    checker_.require_numeric_argument();
    Handler::on_zero();
  }

  FMT_CONSTEXPR void end_precision() { checker_.check_precision(); }

 private:
  numeric_specs_checker<Handler> checker_;
};

template <template <typename> class Handler, typename FormatArg,
          typename ErrorHandler>
FMT_CONSTEXPR int get_dynamic_spec(FormatArg arg, ErrorHandler eh) {
  unsigned long long value = visit_format_arg(Handler<ErrorHandler>(eh), arg);
  if (value > to_unsigned(max_value<int>())) eh.on_error("number is too big");
  return static_cast<int>(value);
}

struct auto_id {};

template <typename Context>
FMT_CONSTEXPR typename Context::format_arg get_arg(Context& ctx, int id) {
  auto arg = ctx.arg(id);
  if (!arg) ctx.on_error("argument index out of range");
  return arg;
}

// The standard format specifier handler with checking.
template <typename ParseContext, typename Context>
class specs_handler : public specs_setter<typename Context::char_type> {
 public:
  using char_type = typename Context::char_type;

  FMT_CONSTEXPR specs_handler(basic_format_specs<char_type>& specs,
                              ParseContext& parse_ctx, Context& ctx)
      : specs_setter<char_type>(specs),
        parse_context_(parse_ctx),
        context_(ctx) {}

  template <typename Id> FMT_CONSTEXPR void on_dynamic_width(Id arg_id) {
    this->specs_.width = get_dynamic_spec<width_checker>(
        get_arg(arg_id), context_.error_handler());
  }

  template <typename Id> FMT_CONSTEXPR void on_dynamic_precision(Id arg_id) {
    this->specs_.precision = get_dynamic_spec<precision_checker>(
        get_arg(arg_id), context_.error_handler());
  }

  void on_error(const char* message) { context_.on_error(message); }

 private:
  // This is only needed for compatibility with gcc 4.4.
  using format_arg = typename Context::format_arg;

  FMT_CONSTEXPR format_arg get_arg(auto_id) {
    return internal::get_arg(context_, parse_context_.next_arg_id());
  }

  FMT_CONSTEXPR format_arg get_arg(int arg_id) {
    parse_context_.check_arg_id(arg_id);
    return internal::get_arg(context_, arg_id);
  }

  FMT_CONSTEXPR format_arg get_arg(basic_string_view<char_type> arg_id) {
    parse_context_.check_arg_id(arg_id);
    return context_.arg(arg_id);
  }

  ParseContext& parse_context_;
  Context& context_;
};

enum class arg_id_kind { none, index, name };

// An argument reference.
template <typename Char> struct arg_ref {
  FMT_CONSTEXPR arg_ref() : kind(arg_id_kind::none), val() {}
  FMT_CONSTEXPR explicit arg_ref(int index)
      : kind(arg_id_kind::index), val(index) {}
  FMT_CONSTEXPR explicit arg_ref(basic_string_view<Char> name)
      : kind(arg_id_kind::name), val(name) {}

  FMT_CONSTEXPR arg_ref& operator=(int idx) {
    kind = arg_id_kind::index;
    val.index = idx;
    return *this;
  }

  arg_id_kind kind;
  union value {
    FMT_CONSTEXPR value(int id = 0) : index{id} {}
    FMT_CONSTEXPR value(basic_string_view<Char> n) : name(n) {}

    int index;
    basic_string_view<Char> name;
  } val;
};

// Format specifiers with width and precision resolved at formatting rather
// than parsing time to allow re-using the same parsed specifiers with
// different sets of arguments (precompilation of format strings).
template <typename Char>
struct dynamic_format_specs : basic_format_specs<Char> {
  arg_ref<Char> width_ref;
  arg_ref<Char> precision_ref;
};

// Format spec handler that saves references to arguments representing dynamic
// width and precision to be resolved at formatting time.
template <typename ParseContext>
class dynamic_specs_handler
    : public specs_setter<typename ParseContext::char_type> {
 public:
  using char_type = typename ParseContext::char_type;

  FMT_CONSTEXPR dynamic_specs_handler(dynamic_format_specs<char_type>& specs,
                                      ParseContext& ctx)
      : specs_setter<char_type>(specs), specs_(specs), context_(ctx) {}

  FMT_CONSTEXPR dynamic_specs_handler(const dynamic_specs_handler& other)
      : specs_setter<char_type>(other),
        specs_(other.specs_),
        context_(other.context_) {}

  template <typename Id> FMT_CONSTEXPR void on_dynamic_width(Id arg_id) {
    specs_.width_ref = make_arg_ref(arg_id);
  }

  template <typename Id> FMT_CONSTEXPR void on_dynamic_precision(Id arg_id) {
    specs_.precision_ref = make_arg_ref(arg_id);
  }

  FMT_CONSTEXPR void on_error(const char* message) {
    context_.on_error(message);
  }

 private:
  using arg_ref_type = arg_ref<char_type>;

  FMT_CONSTEXPR arg_ref_type make_arg_ref(int arg_id) {
    context_.check_arg_id(arg_id);
    return arg_ref_type(arg_id);
  }

  FMT_CONSTEXPR arg_ref_type make_arg_ref(auto_id) {
    return arg_ref_type(context_.next_arg_id());
  }

  FMT_CONSTEXPR arg_ref_type make_arg_ref(basic_string_view<char_type> arg_id) {
    context_.check_arg_id(arg_id);
    basic_string_view<char_type> format_str(
        context_.begin(), to_unsigned(context_.end() - context_.begin()));
    return arg_ref_type(arg_id);
  }

  dynamic_format_specs<char_type>& specs_;
  ParseContext& context_;
};

template <typename Char, typename IDHandler>
FMT_CONSTEXPR const Char* parse_arg_id(const Char* begin, const Char* end,
                                       IDHandler&& handler) {
  FMT_ASSERT(begin != end, "");
  Char c = *begin;
  if (c == '}' || c == ':') {
    handler();
    return begin;
  }
  if (c >= '0' && c <= '9') {
    int index = parse_nonnegative_int(begin, end, handler);
    if (begin == end || (*begin != '}' && *begin != ':'))
      handler.on_error("invalid format string");
    else
      handler(index);
    return begin;
  }
  if (!is_name_start(c)) {
    handler.on_error("invalid format string");
    return begin;
  }
  auto it = begin;
  do {
    ++it;
  } while (it != end && (is_name_start(c = *it) || ('0' <= c && c <= '9')));
  handler(basic_string_view<Char>(begin, to_unsigned(it - begin)));
  return it;
}

// Adapts SpecHandler to IDHandler API for dynamic width.
template <typename SpecHandler, typename Char> struct width_adapter {
  explicit FMT_CONSTEXPR width_adapter(SpecHandler& h) : handler(h) {}

  FMT_CONSTEXPR void operator()() { handler.on_dynamic_width(auto_id()); }
  FMT_CONSTEXPR void operator()(int id) { handler.on_dynamic_width(id); }
  FMT_CONSTEXPR void operator()(basic_string_view<Char> id) {
    handler.on_dynamic_width(id);
  }

  FMT_CONSTEXPR void on_error(const char* message) {
    handler.on_error(message);
  }

  SpecHandler& handler;
};

// Adapts SpecHandler to IDHandler API for dynamic precision.
template <typename SpecHandler, typename Char> struct precision_adapter {
  explicit FMT_CONSTEXPR precision_adapter(SpecHandler& h) : handler(h) {}

  FMT_CONSTEXPR void operator()() { handler.on_dynamic_precision(auto_id()); }
  FMT_CONSTEXPR void operator()(int id) { handler.on_dynamic_precision(id); }
  FMT_CONSTEXPR void operator()(basic_string_view<Char> id) {
    handler.on_dynamic_precision(id);
  }

  FMT_CONSTEXPR void on_error(const char* message) {
    handler.on_error(message);
  }

  SpecHandler& handler;
};

template <typename Char>
FMT_CONSTEXPR const Char* next_code_point(const Char* begin, const Char* end) {
  if (const_check(sizeof(Char) != 1) || (*begin & 0x80) == 0) return begin + 1;
  do {
    ++begin;
  } while (begin != end && (*begin & 0xc0) == 0x80);
  return begin;
}

// Parses fill and alignment.
template <typename Char, typename Handler>
FMT_CONSTEXPR const Char* parse_align(const Char* begin, const Char* end,
                                      Handler&& handler) {
  FMT_ASSERT(begin != end, "");
  auto align = align::none;
  auto p = next_code_point(begin, end);
  if (p == end) p = begin;
  for (;;) {
    switch (static_cast<char>(*p)) {
    case '<':
      align = align::left;
      break;
    case '>':
      align = align::right;
      break;
#if FMT_NUMERIC_ALIGN
    case '=':
      align = align::numeric;
      break;
#endif
    case '^':
      align = align::center;
      break;
    }
    if (align != align::none) {
      if (p != begin) {
        auto c = *begin;
        if (c == '{')
          return handler.on_error("invalid fill character '{'"), begin;
        handler.on_fill(basic_string_view<Char>(begin, p - begin));
        begin = p + 1;
      } else
        ++begin;
      handler.on_align(align);
      break;
    } else if (p == begin) {
      break;
    }
    p = begin;
  }
  return begin;
}

template <typename Char, typename Handler>
FMT_CONSTEXPR const Char* parse_width(const Char* begin, const Char* end,
                                      Handler&& handler) {
  FMT_ASSERT(begin != end, "");
  if ('0' <= *begin && *begin <= '9') {
    handler.on_width(parse_nonnegative_int(begin, end, handler));
  } else if (*begin == '{') {
    ++begin;
    if (begin != end)
      begin = parse_arg_id(begin, end, width_adapter<Handler, Char>(handler));
    if (begin == end || *begin != '}')
      return handler.on_error("invalid format string"), begin;
    ++begin;
  }
  return begin;
}

template <typename Char, typename Handler>
FMT_CONSTEXPR const Char* parse_precision(const Char* begin, const Char* end,
                                          Handler&& handler) {
  ++begin;
  auto c = begin != end ? *begin : Char();
  if ('0' <= c && c <= '9') {
    handler.on_precision(parse_nonnegative_int(begin, end, handler));
  } else if (c == '{') {
    ++begin;
    if (begin != end) {
      begin =
          parse_arg_id(begin, end, precision_adapter<Handler, Char>(handler));
    }
    if (begin == end || *begin++ != '}')
      return handler.on_error("invalid format string"), begin;
  } else {
    return handler.on_error("missing precision specifier"), begin;
  }
  handler.end_precision();
  return begin;
}

// Parses standard format specifiers and sends notifications about parsed
// components to handler.
template <typename Char, typename SpecHandler>
FMT_CONSTEXPR const Char* parse_format_specs(const Char* begin, const Char* end,
                                             SpecHandler&& handler) {
  if (begin == end || *begin == '}') return begin;

  begin = parse_align(begin, end, handler);
  if (begin == end) return begin;

  // Parse sign.
  switch (static_cast<char>(*begin)) {
  case '+':
    handler.on_plus();
    ++begin;
    break;
  case '-':
    handler.on_minus();
    ++begin;
    break;
  case ' ':
    handler.on_space();
    ++begin;
    break;
  }
  if (begin == end) return begin;

  if (*begin == '#') {
    handler.on_hash();
    if (++begin == end) return begin;
  }

  // Parse zero flag.
  if (*begin == '0') {
    handler.on_zero();
    if (++begin == end) return begin;
  }

  begin = parse_width(begin, end, handler);
  if (begin == end) return begin;

  // Parse precision.
  if (*begin == '.') {
    begin = parse_precision(begin, end, handler);
  }

  // Parse type.
  if (begin != end && *begin != '}') handler.on_type(*begin++);
  return begin;
}

// Return the result via the out param to workaround gcc bug 77539.
template <bool IS_CONSTEXPR, typename T, typename Ptr = const T*>
FMT_CONSTEXPR bool find(Ptr first, Ptr last, T value, Ptr& out) {
  for (out = first; out != last; ++out) {
    if (*out == value) return true;
  }
  return false;
}

template <>
inline bool find<false, char>(const char* first, const char* last, char value,
                              const char*& out) {
  out = static_cast<const char*>(
      std::memchr(first, value, internal::to_unsigned(last - first)));
  return out != nullptr;
}

template <typename Handler, typename Char> struct id_adapter {
  FMT_CONSTEXPR void operator()() { handler.on_arg_id(); }
  FMT_CONSTEXPR void operator()(int id) { handler.on_arg_id(id); }
  FMT_CONSTEXPR void operator()(basic_string_view<Char> id) {
    handler.on_arg_id(id);
  }
  FMT_CONSTEXPR void on_error(const char* message) {
    handler.on_error(message);
  }
  Handler& handler;
};

template <bool IS_CONSTEXPR, typename Char, typename Handler>
FMT_CONSTEXPR void parse_format_string(basic_string_view<Char> format_str,
                                       Handler&& handler) {
  struct pfs_writer {
    FMT_CONSTEXPR void operator()(const Char* begin, const Char* end) {
      if (begin == end) return;
      for (;;) {
        const Char* p = nullptr;
        if (!find<IS_CONSTEXPR>(begin, end, '}', p))
          return handler_.on_text(begin, end);
        ++p;
        if (p == end || *p != '}')
          return handler_.on_error("unmatched '}' in format string");
        handler_.on_text(begin, p);
        begin = p + 1;
      }
    }
    Handler& handler_;
  } write{handler};
  auto begin = format_str.data();
  auto end = begin + format_str.size();
  while (begin != end) {
    // Doing two passes with memchr (one for '{' and another for '}') is up to
    // 2.5x faster than the naive one-pass implementation on big format strings.
    const Char* p = begin;
    if (*begin != '{' && !find<IS_CONSTEXPR>(begin + 1, end, '{', p))
      return write(begin, end);
    write(begin, p);
    ++p;
    if (p == end) return handler.on_error("invalid format string");
    if (static_cast<char>(*p) == '}') {
      handler.on_arg_id();
      handler.on_replacement_field(p);
    } else if (*p == '{') {
      handler.on_text(p, p + 1);
    } else {
      p = parse_arg_id(p, end, id_adapter<Handler, Char>{handler});
      Char c = p != end ? *p : Char();
      if (c == '}') {
        handler.on_replacement_field(p);
      } else if (c == ':') {
        p = handler.on_format_specs(p + 1, end);
        if (p == end || *p != '}')
          return handler.on_error("unknown format specifier");
      } else {
        return handler.on_error("missing '}' in format string");
      }
    }
    begin = p + 1;
  }
}

template <typename T, typename ParseContext>
FMT_CONSTEXPR const typename ParseContext::char_type* parse_format_specs(
    ParseContext& ctx) {
  using char_type = typename ParseContext::char_type;
  using context = buffer_context<char_type>;
  using mapped_type =
      conditional_t<internal::mapped_type_constant<T, context>::value !=
                        type::custom_type,
                    decltype(arg_mapper<context>().map(std::declval<T>())), T>;
  auto f = conditional_t<has_formatter<mapped_type, context>::value,
                         formatter<mapped_type, char_type>,
                         internal::fallback_formatter<T, char_type>>();
  return f.parse(ctx);
}

template <typename Char, typename ErrorHandler, typename... Args>
class format_string_checker {
 public:
  explicit FMT_CONSTEXPR format_string_checker(
      basic_string_view<Char> format_str, ErrorHandler eh)
      : arg_id_(-1),
        context_(format_str, eh),
        parse_funcs_{&parse_format_specs<Args, parse_context_type>...} {}

  FMT_CONSTEXPR void on_text(const Char*, const Char*) {}

  FMT_CONSTEXPR void on_arg_id() {
    arg_id_ = context_.next_arg_id();
    check_arg_id();
  }
  FMT_CONSTEXPR void on_arg_id(int id) {
    arg_id_ = id;
    context_.check_arg_id(id);
    check_arg_id();
  }
  FMT_CONSTEXPR void on_arg_id(basic_string_view<Char>) {
    on_error("compile-time checks don't support named arguments");
  }

  FMT_CONSTEXPR void on_replacement_field(const Char*) {}

  FMT_CONSTEXPR const Char* on_format_specs(const Char* begin, const Char*) {
    advance_to(context_, begin);
    return arg_id_ < num_args ? parse_funcs_[arg_id_](context_) : begin;
  }

  FMT_CONSTEXPR void on_error(const char* message) {
    context_.on_error(message);
  }

 private:
  using parse_context_type = basic_format_parse_context<Char, ErrorHandler>;
  enum { num_args = sizeof...(Args) };

  FMT_CONSTEXPR void check_arg_id() {
    if (arg_id_ >= num_args) context_.on_error("argument index out of range");
  }

  // Format specifier parsing function.
  using parse_func = const Char* (*)(parse_context_type&);

  int arg_id_;
  parse_context_type context_;
  parse_func parse_funcs_[num_args > 0 ? num_args : 1];
};

template <typename Char, typename ErrorHandler, typename... Args>
FMT_CONSTEXPR bool do_check_format_string(basic_string_view<Char> s,
                                          ErrorHandler eh = ErrorHandler()) {
  format_string_checker<Char, ErrorHandler, Args...> checker(s, eh);
  parse_format_string<true>(s, checker);
  return true;
}

template <typename... Args, typename S,
          enable_if_t<(is_compile_string<S>::value), int>>
void check_format_string(S format_str) {
  FMT_CONSTEXPR_DECL bool invalid_format =
      internal::do_check_format_string<typename S::char_type,
                                       internal::error_handler, Args...>(
          to_string_view(format_str));
  (void)invalid_format;
}

template <template <typename> class Handler, typename Context>
void handle_dynamic_spec(int& value, arg_ref<typename Context::char_type> ref,
                         Context& ctx) {
  switch (ref.kind) {
  case arg_id_kind::none:
    break;
  case arg_id_kind::index:
    value = internal::get_dynamic_spec<Handler>(ctx.arg(ref.val.index),
                                                ctx.error_handler());
    break;
  case arg_id_kind::name:
    value = internal::get_dynamic_spec<Handler>(ctx.arg(ref.val.name),
                                                ctx.error_handler());
    break;
  }
}

using format_func = void (*)(internal::buffer<char>&, int, string_view);

FMT_API void format_error_code(buffer<char>& out, int error_code,
                               string_view message) FMT_NOEXCEPT;

FMT_API void report_error(format_func func, int error_code,
                          string_view message) FMT_NOEXCEPT;
}  // namespace internal

template <typename Range>
using basic_writer FMT_DEPRECATED_ALIAS = internal::basic_writer<Range>;
using writer FMT_DEPRECATED_ALIAS = internal::writer;
using wwriter FMT_DEPRECATED_ALIAS =
    internal::basic_writer<buffer_range<wchar_t>>;

/** The default argument formatter. */
template <typename Range>
class arg_formatter : public internal::arg_formatter_base<Range> {
 private:
  using char_type = typename Range::value_type;
  using base = internal::arg_formatter_base<Range>;
  using context_type = basic_format_context<typename base::iterator, char_type>;

  context_type& ctx_;
  basic_format_parse_context<char_type>* parse_ctx_;

 public:
  using range = Range;
  using iterator = typename base::iterator;
  using format_specs = typename base::format_specs;

  /**
    \rst
    Constructs an argument formatter object.
    *ctx* is a reference to the formatting context,
    *specs* contains format specifier information for standard argument types.
    \endrst
   */
  explicit arg_formatter(
      context_type& ctx,
      basic_format_parse_context<char_type>* parse_ctx = nullptr,
      format_specs* specs = nullptr)
      : base(Range(ctx.out()), specs, ctx.locale()),
        ctx_(ctx),
        parse_ctx_(parse_ctx) {}

  using base::operator();

  /** Formats an argument of a user-defined type. */
  iterator operator()(typename basic_format_arg<context_type>::handle handle) {
    handle.format(*parse_ctx_, ctx_);
    return ctx_.out();
  }
};

/**
 An error returned by an operating system or a language runtime,
 for example a file opening error.
*/
FMT_CLASS_API
class FMT_API system_error : public std::runtime_error {
 private:
  void init(int err_code, string_view format_str, format_args args);

 protected:
  int error_code_;

  system_error() : std::runtime_error(""), error_code_(0) {}

 public:
  /**
   \rst
   Constructs a :class:`fmt::system_error` object with a description
   formatted with `fmt::format_system_error`. *message* and additional
   arguments passed into the constructor are formatted similarly to
   `fmt::format`.

   **Example**::

     // This throws a system_error with the description
     //   cannot open file 'madeup': No such file or directory
     // or similar (system message may vary).
     const char *filename = "madeup";
     std::FILE *file = std::fopen(filename, "r");
     if (!file)
       throw fmt::system_error(errno, "cannot open file '{}'", filename);
   \endrst
  */
  template <typename... Args>
  system_error(int error_code, string_view message, const Args&... args)
      : std::runtime_error("") {
    init(error_code, message, make_format_args(args...));
  }
  system_error(const system_error&) = default;
  system_error& operator=(const system_error&) = default;
  system_error(system_error&&) = default;
  system_error& operator=(system_error&&) = default;
  ~system_error() FMT_NOEXCEPT FMT_OVERRIDE;

  int error_code() const { return error_code_; }
};

/**
  \rst
  Formats an error returned by an operating system or a language runtime,
  for example a file opening error, and writes it to *out* in the following
  form:

  .. parsed-literal::
     *<message>*: *<system-message>*

  where *<message>* is the passed message and *<system-message>* is
  the system message corresponding to the error code.
  *error_code* is a system error code as given by ``errno``.
  If *error_code* is not a valid error code such as -1, the system message
  may look like "Unknown error -1" and is platform-dependent.
  \endrst
 */
FMT_API void format_system_error(internal::buffer<char>& out, int error_code,
                                 string_view message) FMT_NOEXCEPT;

// Reports a system error without throwing an exception.
// Can be used to report errors from destructors.
FMT_API void report_system_error(int error_code,
                                 string_view message) FMT_NOEXCEPT;

/** Fast integer formatter. */
class format_int {
 private:
  // Buffer should be large enough to hold all digits (digits10 + 1),
  // a sign and a null character.
  enum { buffer_size = std::numeric_limits<unsigned long long>::digits10 + 3 };
  mutable char buffer_[buffer_size];
  char* str_;

  // Formats value in reverse and returns a pointer to the beginning.
  char* format_decimal(unsigned long long value) {
    char* ptr = buffer_ + (buffer_size - 1);  // Parens to workaround MSVC bug.
    while (value >= 100) {
      // Integer division is slow so do it for a group of two digits instead
      // of for every digit. The idea comes from the talk by Alexandrescu
      // "Three Optimization Tips for C++". See speed-test for a comparison.
      auto index = static_cast<unsigned>((value % 100) * 2);
      value /= 100;
      *--ptr = internal::data::digits[index + 1];
      *--ptr = internal::data::digits[index];
    }
    if (value < 10) {
      *--ptr = static_cast<char>('0' + value);
      return ptr;
    }
    auto index = static_cast<unsigned>(value * 2);
    *--ptr = internal::data::digits[index + 1];
    *--ptr = internal::data::digits[index];
    return ptr;
  }

  void format_signed(long long value) {
    auto abs_value = static_cast<unsigned long long>(value);
    bool negative = value < 0;
    if (negative) abs_value = 0 - abs_value;
    str_ = format_decimal(abs_value);
    if (negative) *--str_ = '-';
  }

 public:
  explicit format_int(int value) { format_signed(value); }
  explicit format_int(long value) { format_signed(value); }
  explicit format_int(long long value) { format_signed(value); }
  explicit format_int(unsigned value) : str_(format_decimal(value)) {}
  explicit format_int(unsigned long value) : str_(format_decimal(value)) {}
  explicit format_int(unsigned long long value) : str_(format_decimal(value)) {}

  /** Returns the number of characters written to the output buffer. */
  std::size_t size() const {
    return internal::to_unsigned(buffer_ - str_ + buffer_size - 1);
  }

  /**
    Returns a pointer to the output buffer content. No terminating null
    character is appended.
   */
  const char* data() const { return str_; }

  /**
    Returns a pointer to the output buffer content with terminating null
    character appended.
   */
  const char* c_str() const {
    buffer_[buffer_size - 1] = '\0';
    return str_;
  }

  /**
    \rst
    Returns the content of the output buffer as an ``std::string``.
    \endrst
   */
  std::string str() const { return std::string(str_, size()); }
};

// A formatter specialization for the core types corresponding to internal::type
// constants.
template <typename T, typename Char>
struct formatter<T, Char,
                 enable_if_t<internal::type_constant<T, Char>::value !=
                             internal::type::custom_type>> {
  FMT_CONSTEXPR formatter() = default;

  // Parses format specifiers stopping either at the end of the range or at the
  // terminating '}'.
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    using handler_type = internal::dynamic_specs_handler<ParseContext>;
    auto type = internal::type_constant<T, Char>::value;
    internal::specs_checker<handler_type> handler(handler_type(specs_, ctx),
                                                  type);
    auto it = parse_format_specs(ctx.begin(), ctx.end(), handler);
    auto eh = ctx.error_handler();
    switch (type) {
    case internal::type::none_type:
    case internal::type::named_arg_type:
      FMT_ASSERT(false, "invalid argument type");
      break;
    case internal::type::int_type:
    case internal::type::uint_type:
    case internal::type::long_long_type:
    case internal::type::ulong_long_type:
    case internal::type::int128_type:
    case internal::type::uint128_type:
    case internal::type::bool_type:
      handle_int_type_spec(specs_.type,
                           internal::int_type_checker<decltype(eh)>(eh));
      break;
    case internal::type::char_type:
      handle_char_specs(
          &specs_, internal::char_specs_checker<decltype(eh)>(specs_.type, eh));
      break;
    case internal::type::float_type:
    case internal::type::double_type:
    case internal::type::long_double_type:
      internal::parse_float_type_spec(specs_, eh);
      break;
    case internal::type::cstring_type:
      internal::handle_cstring_type_spec(
          specs_.type, internal::cstring_type_checker<decltype(eh)>(eh));
      break;
    case internal::type::string_type:
      internal::check_string_type_spec(specs_.type, eh);
      break;
    case internal::type::pointer_type:
      internal::check_pointer_type_spec(specs_.type, eh);
      break;
    case internal::type::custom_type:
      // Custom format specifiers should be checked in parse functions of
      // formatter specializations.
      break;
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const T& val, FormatContext& ctx) -> decltype(ctx.out()) {
    internal::handle_dynamic_spec<internal::width_checker>(
        specs_.width, specs_.width_ref, ctx);
    internal::handle_dynamic_spec<internal::precision_checker>(
        specs_.precision, specs_.precision_ref, ctx);
    using range_type =
        internal::output_range<typename FormatContext::iterator,
                               typename FormatContext::char_type>;
    return visit_format_arg(arg_formatter<range_type>(ctx, nullptr, &specs_),
                            internal::make_arg<FormatContext>(val));
  }

 private:
  internal::dynamic_format_specs<Char> specs_;
};

#define FMT_FORMAT_AS(Type, Base)                                             \
  template <typename Char>                                                    \
  struct formatter<Type, Char> : formatter<Base, Char> {                      \
    template <typename FormatContext>                                         \
    auto format(const Type& val, FormatContext& ctx) -> decltype(ctx.out()) { \
      return formatter<Base, Char>::format(val, ctx);                         \
    }                                                                         \
  }

FMT_FORMAT_AS(signed char, int);
FMT_FORMAT_AS(unsigned char, unsigned);
FMT_FORMAT_AS(short, int);
FMT_FORMAT_AS(unsigned short, unsigned);
FMT_FORMAT_AS(long, long long);
FMT_FORMAT_AS(unsigned long, unsigned long long);
FMT_FORMAT_AS(Char*, const Char*);
FMT_FORMAT_AS(std::basic_string<Char>, basic_string_view<Char>);
FMT_FORMAT_AS(std::nullptr_t, const void*);
FMT_FORMAT_AS(internal::std_string_view<Char>, basic_string_view<Char>);

template <typename Char>
struct formatter<void*, Char> : formatter<const void*, Char> {
  template <typename FormatContext>
  auto format(void* val, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<const void*, Char>::format(val, ctx);
  }
};

template <typename Char, size_t N>
struct formatter<Char[N], Char> : formatter<basic_string_view<Char>, Char> {
  template <typename FormatContext>
  auto format(const Char* val, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<basic_string_view<Char>, Char>::format(val, ctx);
  }
};

// A formatter for types known only at run time such as variant alternatives.
//
// Usage:
//   using variant = std::variant<int, std::string>;
//   template <>
//   struct formatter<variant>: dynamic_formatter<> {
//     void format(buffer &buf, const variant &v, context &ctx) {
//       visit([&](const auto &val) { format(buf, val, ctx); }, v);
//     }
//   };
template <typename Char = char> class dynamic_formatter {
 private:
  struct null_handler : internal::error_handler {
    void on_align(align_t) {}
    void on_plus() {}
    void on_minus() {}
    void on_space() {}
    void on_hash() {}
  };

 public:
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    format_str_ = ctx.begin();
    // Checks are deferred to formatting time when the argument type is known.
    internal::dynamic_specs_handler<ParseContext> handler(specs_, ctx);
    return parse_format_specs(ctx.begin(), ctx.end(), handler);
  }

  template <typename T, typename FormatContext>
  auto format(const T& val, FormatContext& ctx) -> decltype(ctx.out()) {
    handle_specs(ctx);
    internal::specs_checker<null_handler> checker(
        null_handler(),
        internal::mapped_type_constant<T, FormatContext>::value);
    checker.on_align(specs_.align);
    switch (specs_.sign) {
    case sign::none:
      break;
    case sign::plus:
      checker.on_plus();
      break;
    case sign::minus:
      checker.on_minus();
      break;
    case sign::space:
      checker.on_space();
      break;
    }
    if (specs_.alt) checker.on_hash();
    if (specs_.precision >= 0) checker.end_precision();
    using range = internal::output_range<typename FormatContext::iterator,
                                         typename FormatContext::char_type>;
    visit_format_arg(arg_formatter<range>(ctx, nullptr, &specs_),
                     internal::make_arg<FormatContext>(val));
    return ctx.out();
  }

 private:
  template <typename Context> void handle_specs(Context& ctx) {
    internal::handle_dynamic_spec<internal::width_checker>(
        specs_.width, specs_.width_ref, ctx);
    internal::handle_dynamic_spec<internal::precision_checker>(
        specs_.precision, specs_.precision_ref, ctx);
  }

  internal::dynamic_format_specs<Char> specs_;
  const Char* format_str_;
};

template <typename Range, typename Char>
typename basic_format_context<Range, Char>::format_arg
basic_format_context<Range, Char>::arg(basic_string_view<char_type> name) {
  map_.init(args_);
  format_arg arg = map_.find(name);
  if (arg.type() == internal::type::none_type)
    this->on_error("argument not found");
  return arg;
}

template <typename Char, typename ErrorHandler>
FMT_CONSTEXPR void advance_to(
    basic_format_parse_context<Char, ErrorHandler>& ctx, const Char* p) {
  ctx.advance_to(ctx.begin() + (p - &*ctx.begin()));
}

template <typename ArgFormatter, typename Char, typename Context>
struct format_handler : internal::error_handler {
  using range = typename ArgFormatter::range;

  format_handler(range r, basic_string_view<Char> str,
                 basic_format_args<Context> format_args,
                 internal::locale_ref loc)
      : parse_context(str), context(r.begin(), format_args, loc) {}

  void on_text(const Char* begin, const Char* end) {
    auto size = internal::to_unsigned(end - begin);
    auto out = context.out();
    auto&& it = internal::reserve(out, size);
    it = std::copy_n(begin, size, it);
    context.advance_to(out);
  }

  void get_arg(int id) { arg = internal::get_arg(context, id); }

  void on_arg_id() { get_arg(parse_context.next_arg_id()); }
  void on_arg_id(int id) {
    parse_context.check_arg_id(id);
    get_arg(id);
  }
  void on_arg_id(basic_string_view<Char> id) { arg = context.arg(id); }

  void on_replacement_field(const Char* p) {
    advance_to(parse_context, p);
    context.advance_to(
        visit_format_arg(ArgFormatter(context, &parse_context), arg));
  }

  const Char* on_format_specs(const Char* begin, const Char* end) {
    advance_to(parse_context, begin);
    internal::custom_formatter<Context> f(parse_context, context);
    if (visit_format_arg(f, arg)) return parse_context.begin();
    basic_format_specs<Char> specs;
    using internal::specs_handler;
    using parse_context_t = basic_format_parse_context<Char>;
    internal::specs_checker<specs_handler<parse_context_t, Context>> handler(
        specs_handler<parse_context_t, Context>(specs, parse_context, context),
        arg.type());
    begin = parse_format_specs(begin, end, handler);
    if (begin == end || *begin != '}') on_error("missing '}' in format string");
    advance_to(parse_context, begin);
    context.advance_to(
        visit_format_arg(ArgFormatter(context, &parse_context, &specs), arg));
    return begin;
  }

  basic_format_parse_context<Char> parse_context;
  Context context;
  basic_format_arg<Context> arg;
};

/** Formats arguments and writes the output to the range. */
template <typename ArgFormatter, typename Char, typename Context>
typename Context::iterator vformat_to(
    typename ArgFormatter::range out, basic_string_view<Char> format_str,
    basic_format_args<Context> args,
    internal::locale_ref loc = internal::locale_ref()) {
  format_handler<ArgFormatter, Char, Context> h(out, format_str, args, loc);
  internal::parse_format_string<false>(format_str, h);
  return h.context.out();
}

// Casts ``p`` to ``const void*`` for pointer formatting.
// Example:
//   auto s = format("{}", ptr(p));
template <typename T> inline const void* ptr(const T* p) { return p; }
template <typename T> inline const void* ptr(const std::unique_ptr<T>& p) {
  return p.get();
}
template <typename T> inline const void* ptr(const std::shared_ptr<T>& p) {
  return p.get();
}

class bytes {
 private:
  string_view data_;
  friend struct formatter<bytes>;

 public:
  explicit bytes(string_view data) : data_(data) {}
};

template <> struct formatter<bytes> : formatter<string_view> {
  template <typename FormatContext>
  auto format(bytes b, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<string_view>::format(b.data_, ctx);
  }
};

template <typename It, typename Char> struct arg_join : internal::view {
  It begin;
  It end;
  basic_string_view<Char> sep;

  arg_join(It b, It e, basic_string_view<Char> s) : begin(b), end(e), sep(s) {}
};

template <typename It, typename Char>
struct formatter<arg_join<It, Char>, Char>
    : formatter<typename std::iterator_traits<It>::value_type, Char> {
  template <typename FormatContext>
  auto format(const arg_join<It, Char>& value, FormatContext& ctx)
      -> decltype(ctx.out()) {
    using base = formatter<typename std::iterator_traits<It>::value_type, Char>;
    auto it = value.begin;
    auto out = ctx.out();
    if (it != value.end) {
      out = base::format(*it++, ctx);
      while (it != value.end) {
        out = std::copy(value.sep.begin(), value.sep.end(), out);
        ctx.advance_to(out);
        out = base::format(*it++, ctx);
      }
    }
    return out;
  }
};

/**
  Returns an object that formats the iterator range `[begin, end)` with elements
  separated by `sep`.
 */
template <typename It>
arg_join<It, char> join(It begin, It end, string_view sep) {
  return {begin, end, sep};
}

template <typename It>
arg_join<It, wchar_t> join(It begin, It end, wstring_view sep) {
  return {begin, end, sep};
}

/**
  \rst
  Returns an object that formats `range` with elements separated by `sep`.

  **Example**::

    std::vector<int> v = {1, 2, 3};
    fmt::print("{}", fmt::join(v, ", "));
    // Output: "1, 2, 3"

  ``fmt::join`` applies passed format specifiers to the range elements::

    fmt::print("{:02}", fmt::join(v, ", "));
    // Output: "01, 02, 03"
  \endrst
 */
template <typename Range>
arg_join<internal::iterator_t<const Range>, char> join(const Range& range,
                                                       string_view sep) {
  return join(std::begin(range), std::end(range), sep);
}

template <typename Range>
arg_join<internal::iterator_t<const Range>, wchar_t> join(const Range& range,
                                                          wstring_view sep) {
  return join(std::begin(range), std::end(range), sep);
}

/**
  \rst
  Converts *value* to ``std::string`` using the default format for type *T*.

  **Example**::

    #include <fmt/format.h>

    std::string answer = fmt::to_string(42);
  \endrst
 */
template <typename T> inline std::string to_string(const T& value) {
  return format("{}", value);
}

/**
  Converts *value* to ``std::wstring`` using the default format for type *T*.
 */
template <typename T> inline std::wstring to_wstring(const T& value) {
  return format(L"{}", value);
}

template <typename Char, std::size_t SIZE>
std::basic_string<Char> to_string(const basic_memory_buffer<Char, SIZE>& buf) {
  return std::basic_string<Char>(buf.data(), buf.size());
}

template <typename Char>
typename buffer_context<Char>::iterator internal::vformat_to(
    internal::buffer<Char>& buf, basic_string_view<Char> format_str,
    basic_format_args<buffer_context<type_identity_t<Char>>> args) {
  using range = buffer_range<Char>;
  return vformat_to<arg_formatter<range>>(buf, to_string_view(format_str),
                                          args);
}

template <typename S, typename Char = char_t<S>,
          FMT_ENABLE_IF(internal::is_string<S>::value)>
inline typename buffer_context<Char>::iterator vformat_to(
    internal::buffer<Char>& buf, const S& format_str,
    basic_format_args<buffer_context<type_identity_t<Char>>> args) {
  return internal::vformat_to(buf, to_string_view(format_str), args);
}

template <typename S, typename... Args, std::size_t SIZE = inline_buffer_size,
          typename Char = enable_if_t<internal::is_string<S>::value, char_t<S>>>
inline typename buffer_context<Char>::iterator format_to(
    basic_memory_buffer<Char, SIZE>& buf, const S& format_str, Args&&... args) {
  internal::check_format_string<Args...>(format_str);
  using context = buffer_context<Char>;
  return internal::vformat_to(buf, to_string_view(format_str),
                              make_format_args<context>(args...));
}

template <typename OutputIt, typename Char = char>
using format_context_t = basic_format_context<OutputIt, Char>;

template <typename OutputIt, typename Char = char>
using format_args_t = basic_format_args<format_context_t<OutputIt, Char>>;

template <typename S, typename OutputIt, typename... Args,
          FMT_ENABLE_IF(
              internal::is_output_iterator<OutputIt>::value &&
              !internal::is_contiguous_back_insert_iterator<OutputIt>::value)>
inline OutputIt vformat_to(
    OutputIt out, const S& format_str,
    format_args_t<type_identity_t<OutputIt>, char_t<S>> args) {
  using range = internal::output_range<OutputIt, char_t<S>>;
  return vformat_to<arg_formatter<range>>(range(out),
                                          to_string_view(format_str), args);
}

/**
 \rst
 Formats arguments, writes the result to the output iterator ``out`` and returns
 the iterator past the end of the output range.

 **Example**::

   std::vector<char> out;
   fmt::format_to(std::back_inserter(out), "{}", 42);
 \endrst
 */
template <typename OutputIt, typename S, typename... Args,
          FMT_ENABLE_IF(
              internal::is_output_iterator<OutputIt>::value &&
              !internal::is_contiguous_back_insert_iterator<OutputIt>::value &&
              internal::is_string<S>::value)>
inline OutputIt format_to(OutputIt out, const S& format_str, Args&&... args) {
  internal::check_format_string<Args...>(format_str);
  using context = format_context_t<OutputIt, char_t<S>>;
  return vformat_to(out, to_string_view(format_str),
                    make_format_args<context>(args...));
}

template <typename OutputIt> struct format_to_n_result {
  /** Iterator past the end of the output range. */
  OutputIt out;
  /** Total (not truncated) output size. */
  std::size_t size;
};

template <typename OutputIt, typename Char = typename OutputIt::value_type>
using format_to_n_context =
    format_context_t<internal::truncating_iterator<OutputIt>, Char>;

template <typename OutputIt, typename Char = typename OutputIt::value_type>
using format_to_n_args = basic_format_args<format_to_n_context<OutputIt, Char>>;

template <typename OutputIt, typename Char, typename... Args>
inline format_arg_store<format_to_n_context<OutputIt, Char>, Args...>
make_format_to_n_args(const Args&... args) {
  return format_arg_store<format_to_n_context<OutputIt, Char>, Args...>(
      args...);
}

template <typename OutputIt, typename Char, typename... Args,
          FMT_ENABLE_IF(internal::is_output_iterator<OutputIt>::value)>
inline format_to_n_result<OutputIt> vformat_to_n(
    OutputIt out, std::size_t n, basic_string_view<Char> format_str,
    format_to_n_args<type_identity_t<OutputIt>, type_identity_t<Char>> args) {
  auto it = vformat_to(internal::truncating_iterator<OutputIt>(out, n),
                       format_str, args);
  return {it.base(), it.count()};
}

/**
 \rst
 Formats arguments, writes up to ``n`` characters of the result to the output
 iterator ``out`` and returns the total output size and the iterator past the
 end of the output range.
 \endrst
 */
template <typename OutputIt, typename S, typename... Args,
          FMT_ENABLE_IF(internal::is_string<S>::value&&
                            internal::is_output_iterator<OutputIt>::value)>
inline format_to_n_result<OutputIt> format_to_n(OutputIt out, std::size_t n,
                                                const S& format_str,
                                                const Args&... args) {
  internal::check_format_string<Args...>(format_str);
  using context = format_to_n_context<OutputIt, char_t<S>>;
  return vformat_to_n(out, n, to_string_view(format_str),
                      make_format_args<context>(args...));
}

template <typename Char>
inline std::basic_string<Char> internal::vformat(
    basic_string_view<Char> format_str,
    basic_format_args<buffer_context<type_identity_t<Char>>> args) {
  basic_memory_buffer<Char> buffer;
  internal::vformat_to(buffer, format_str, args);
  return to_string(buffer);
}

/**
  Returns the number of characters in the output of
  ``format(format_str, args...)``.
 */
template <typename... Args>
inline std::size_t formatted_size(string_view format_str, const Args&... args) {
  return format_to(internal::counting_iterator(), format_str, args...).count();
}

template <typename Char, FMT_ENABLE_IF(std::is_same<Char, wchar_t>::value)>
void vprint(std::FILE* f, basic_string_view<Char> format_str,
            wformat_args args) {
  wmemory_buffer buffer;
  internal::vformat_to(buffer, format_str, args);
  buffer.push_back(L'\0');
  if (std::fputws(buffer.data(), f) == -1)
    FMT_THROW(system_error(errno, "cannot write to file"));
}

template <typename Char, FMT_ENABLE_IF(std::is_same<Char, wchar_t>::value)>
void vprint(basic_string_view<Char> format_str, wformat_args args) {
  vprint(stdout, format_str, args);
}

#if FMT_USE_USER_DEFINED_LITERALS
namespace internal {

#  if FMT_USE_UDL_TEMPLATE
template <typename Char, Char... CHARS> class udl_formatter {
 public:
  template <typename... Args>
  std::basic_string<Char> operator()(Args&&... args) const {
    FMT_CONSTEXPR_DECL Char s[] = {CHARS..., '\0'};
    FMT_CONSTEXPR_DECL bool invalid_format =
        do_check_format_string<Char, error_handler, remove_cvref_t<Args>...>(
            basic_string_view<Char>(s, sizeof...(CHARS)));
    (void)invalid_format;
    return format(s, std::forward<Args>(args)...);
  }
};
#  else
template <typename Char> struct udl_formatter {
  basic_string_view<Char> str;

  template <typename... Args>
  std::basic_string<Char> operator()(Args&&... args) const {
    return format(str, std::forward<Args>(args)...);
  }
};
#  endif  // FMT_USE_UDL_TEMPLATE

template <typename Char> struct udl_arg {
  basic_string_view<Char> str;

  template <typename T> named_arg<T, Char> operator=(T&& value) const {
    return {str, std::forward<T>(value)};
  }
};

template <typename Char, size_t N>
FMT_CONSTEXPR basic_string_view<Char> literal_to_view(const Char (&s)[N]) {
  return {s, N - 1};
}
}  // namespace internal

inline namespace literals {
#  if FMT_USE_UDL_TEMPLATE
#    pragma GCC diagnostic push
#    if FMT_CLANG_VERSION
#      pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"
#    endif
template <typename Char, Char... CHARS>
FMT_CONSTEXPR internal::udl_formatter<Char, CHARS...> operator""_format() {
  return {};
}
#    pragma GCC diagnostic pop
#  else
/**
  \rst
  User-defined literal equivalent of :func:`fmt::format`.

  **Example**::

    using namespace fmt::literals;
    std::string message = "The answer is {}"_format(42);
  \endrst
 */
FMT_CONSTEXPR internal::udl_formatter<char> operator"" _format(const char* s,
                                                               std::size_t n) {
  return {{s, n}};
}
FMT_CONSTEXPR internal::udl_formatter<wchar_t> operator"" _format(
    const wchar_t* s, std::size_t n) {
  return {{s, n}};
}
#  endif  // FMT_USE_UDL_TEMPLATE

/**
  \rst
  User-defined literal equivalent of :func:`fmt::arg`.

  **Example**::

    using namespace fmt::literals;
    fmt::print("Elapsed time: {s:.2f} seconds", "s"_a=1.23);
  \endrst
 */
FMT_CONSTEXPR internal::udl_arg<char> operator"" _a(const char* s,
                                                    std::size_t n) {
  return {{s, n}};
}
FMT_CONSTEXPR internal::udl_arg<wchar_t> operator"" _a(const wchar_t* s,
                                                       std::size_t n) {
  return {{s, n}};
}
}  // namespace literals
#endif  // FMT_USE_USER_DEFINED_LITERALS
FMT_END_NAMESPACE

#define FMT_STRING_IMPL(s, ...)                              \
  [] {                                                       \
    /* Use a macro-like name to avoid shadowing warnings. */ \
    struct FMT_STRING : fmt::compile_string {                \
      using char_type = fmt::remove_cvref_t<decltype(*s)>;   \
      __VA_ARGS__ FMT_CONSTEXPR                              \
      operator fmt::basic_string_view<char_type>() const {   \
        /* FMT_STRING only accepts string literals. */       \
        return fmt::internal::literal_to_view(s);            \
      }                                                      \
    };                                                       \
    return FMT_STRING();                                     \
  }()

/**
  \rst
  Constructs a compile-time format string from a string literal *s*.

  **Example**::

    // A compile-time error because 'd' is an invalid specifier for strings.
    std::string s = format(FMT_STRING("{:d}"), "foo");
  \endrst
 */
#define FMT_STRING(s) FMT_STRING_IMPL(s, )

#if defined(FMT_STRING_ALIAS) && FMT_STRING_ALIAS
#  define fmt(s) FMT_STRING_IMPL(s, [[deprecated]])
#endif

#ifdef FMT_HEADER_ONLY
#  define FMT_FUNC inline
#  include "format-inl.h"
#else
#  define FMT_FUNC
#endif

#endif  // FMT_FORMAT_H_
