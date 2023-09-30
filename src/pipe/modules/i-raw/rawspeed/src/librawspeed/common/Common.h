/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2009-2014 Klaus Post

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once

#include "rawspeedconfig.h" // for RAWSPEED_READNONE
#include "adt/Invariant.h"  // for invariant
#include <algorithm>        // for max, clamp
#include <array>            // for array
#include <cassert>          // for invariant
#include <climits>          // for CHAR_BIT
#include <cstdint>          // for uintptr_t, uint8_t, uint16_t
#include <cstring>          // for size_t, memcpy
#include <initializer_list> // for initializer_list
#include <string>           // for string, basic_string, allocator
#include <string_view>      // for string_view
#include <type_traits>      // for enable_if_t, enable_if, make_signed_t
#include <vector>           // for vector

extern "C" int rawspeed_get_number_of_processor_cores();

namespace rawspeed {

enum class DEBUG_PRIO {
  ERROR = 0x10,
  WARNING = 0x100,
  INFO = 0x1000,
  EXTRA = 0x10000
};

void writeLog(DEBUG_PRIO priority, const char* format, ...)
    __attribute__((format(printf, 2, 3)));

inline void copyPixels(uint8_t* dest, int dstPitch, const uint8_t* src,
                       int srcPitch, int rowSize, int height) {
  if (height == 1 || (dstPitch == srcPitch && srcPitch == rowSize))
    memcpy(dest, src, static_cast<size_t>(rowSize) * height);
  else {
    for (int y = height; y > 0; --y) {
      memcpy(dest, src, rowSize);
      dest += dstPitch;
      src += srcPitch;
    }
  }
}

template <typename T_TO, typename T_FROM,
          typename = std::enable_if_t<sizeof(T_TO) == sizeof(T_FROM)>,
          typename = std::enable_if_t<std::is_trivially_constructible_v<T_TO>>,
          typename = std::enable_if_t<std::is_trivially_copyable_v<T_TO>>,
          typename = std::enable_if_t<std::is_trivially_copyable_v<T_FROM>>>
inline T_TO bit_cast(const T_FROM& from) noexcept {
  T_TO to;
  memcpy(&to, &from, sizeof(T_TO));
  return to;
}

// only works for positive values and zero
template <typename T> constexpr bool RAWSPEED_READNONE isPowerOfTwo(T val) {
  return (val & (~val + 1)) == val;
}

template <class T>
constexpr unsigned RAWSPEED_READNONE bitwidth([[maybe_unused]] T unused = {}) {
  return CHAR_BIT * sizeof(T);
}

template <typename T>
constexpr size_t RAWSPEED_READNONE getMisalignmentOffset(
    T value, size_t multiple,
    typename std::enable_if<std::is_pointer_v<T>>::type* /*unused*/ = nullptr) {
  if (multiple == 0)
    return 0;
  static_assert(bitwidth<uintptr_t>() >= bitwidth<T>(),
                "uintptr_t can not represent all pointer values?");
  return reinterpret_cast<uintptr_t>(value) % multiple;
}

template <typename T>
constexpr size_t RAWSPEED_READNONE getMisalignmentOffset(
    T value, size_t multiple,
    typename std::enable_if<std::is_integral_v<T>>::type* /*unused*/ =
        nullptr) {
  if (multiple == 0)
    return 0;
  return value % multiple;
}

template <typename T>
constexpr T RAWSPEED_READNONE roundToMultiple(T value, size_t multiple,
                                              bool roundDown) {
  size_t offset = getMisalignmentOffset(value, multiple);
  if (offset == 0)
    return value;
  // Drop remainder.
  T roundedDown = value - offset;
  if (roundDown) // If we were rounding down, then that's it.
    return roundedDown;
  // Else, just add one multiple.
  return roundedDown + multiple;
}

constexpr size_t RAWSPEED_READNONE roundDown(size_t value, size_t multiple) {
  return roundToMultiple(value, multiple, /*roundDown=*/true);
}

constexpr size_t RAWSPEED_READNONE roundUp(size_t value, size_t multiple) {
  return roundToMultiple(value, multiple, /*roundDown=*/false);
}

constexpr size_t RAWSPEED_READNONE roundUpDivision(size_t value, size_t div) {
  return (value != 0) ? (1 + ((value - 1) / div)) : 0;
}

template <class T>
constexpr RAWSPEED_READNONE bool isAligned(T value, size_t multiple) {
  return (multiple == 0) || (getMisalignmentOffset(value, multiple) == 0);
}

template <typename T, typename T2>
bool RAWSPEED_READONLY isIn(const T value,
                            const std::initializer_list<T2>& list) {
  return std::any_of(list.begin(), list.end(),
                     [value](const T2& t) { return t == value; });
}

// Clamps the given value to the range 0 .. 2^n-1, with n <= 16
template <typename T>
constexpr uint16_t RAWSPEED_READNONE clampBits(
    T value, unsigned int nBits,
    typename std::enable_if_t<std::is_arithmetic_v<T>>* /*unused*/ = nullptr) {
  // We expect to produce uint16_t.
  invariant(nBits <= 16);
  // Check that the clamp is not a no-op. Not of uint16_t to 16 bits e.g.
  // (Well, not really, if we are called from clampBits<signed>, it's ok..).
  invariant(bitwidth<T>() > nBits); // If nBits >= bitwidth, then shift is UB.
  const T maxVal = (T(1) << nBits) - T(1);
  return std::clamp(value, T(0), maxVal);
}

template <typename T>
constexpr bool RAWSPEED_READNONE isIntN(
    T value, unsigned int nBits,
    typename std::enable_if_t<std::is_arithmetic_v<T>>* /*unused*/ = nullptr) {
  invariant(nBits < bitwidth<T>() && "Check must not be tautological.");
  using UnsignedT = std::make_unsigned_t<T>;
  const auto highBits = static_cast<UnsignedT>(value) >> nBits;
  return highBits == 0;
}

template <class T, typename std::enable_if_t<std::is_unsigned_v<T>>* = nullptr>
constexpr int countl_zero(T x) noexcept {
  if (x == T(0))
    return bitwidth<T>();
  return __builtin_clz(x);
}

template <class T>
constexpr RAWSPEED_READNONE T extractHighBits(
    T value, unsigned nBits, unsigned effectiveBitwidth = bitwidth<T>(),
    typename std::enable_if_t<std::is_unsigned_v<T>>* /*unused*/ = nullptr) {
  invariant(effectiveBitwidth <= bitwidth<T>());
  invariant(nBits <= effectiveBitwidth);
  auto numLowBitsToSkip = effectiveBitwidth - nBits;
  invariant(numLowBitsToSkip < bitwidth<T>());
  return value >> numLowBitsToSkip;
}

template <typename T>
constexpr typename std::make_signed_t<T> RAWSPEED_READNONE signExtend(
    T value, unsigned int nBits,
    typename std::enable_if_t<std::is_unsigned_v<T>>* /*unused*/ = nullptr) {
  invariant(nBits != 0 && "Only valid for non-zero bit count.");
  const T SpareSignBits = bitwidth<T>() - nBits;
  using SignedT = std::make_signed_t<T>;
  return static_cast<SignedT>(value << SpareSignBits) >> SpareSignBits;
}

// Trim both leading and trailing spaces from the string
inline std::string trimSpaces(std::string_view str) {
  // Find the first character position after excluding leading blank spaces
  size_t startpos = str.find_first_not_of(" \t");

  // Find the first character position from reverse af
  size_t endpos = str.find_last_not_of(" \t");

  // if all spaces or empty return an empty string
  if ((startpos == std::string::npos) || (endpos == std::string::npos))
    return "";

  str = str.substr(startpos, endpos - startpos + 1);
  return {str.begin(), str.end()};
}

inline std::vector<std::string> splitString(const std::string& input,
                                            char c = ' ') {
  std::vector<std::string> result;
  const char* str = input.c_str();

  while (true) {
    const char* begin = str;

    while (*str != c && *str != '\0')
      str++;

    if (begin != str)
      result.emplace_back(begin, str);

    const bool isNullTerminator = (*str == '\0');
    str++;

    if (isNullTerminator)
      break;
  }

  return result;
}

template <int N, typename T>
inline std::array<T, N> to_array(const std::vector<T>& v) {
  std::array<T, N> a;
  assert(v.size() == N && "Size mismatch");
  std::move(v.begin(), v.end(), a.begin());
  return a;
}

enum class BitOrder {
  LSB,   /* Memory order */
  MSB,   /* Input is added to stack byte by byte, and output is lifted
                     from top */
  MSB16, /* Same as above, but 16 bits at the time */
  MSB32, /* Same as above, but 32 bits at the time */
};

} // namespace rawspeed
