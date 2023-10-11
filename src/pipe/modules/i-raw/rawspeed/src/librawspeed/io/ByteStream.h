/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2009-2014 Klaus Post
    Copyright (C) 2017 Axel Waggershauser

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

#include "AddressSanitizer.h" // for ASan
#include "adt/Invariant.h"    // for invariant
#include "common/Common.h"    // for roundUp
#include "io/Buffer.h"        // for Buffer::size_type, DataBuffer, Buffer
#include "io/IOException.h"   // for ThrowException, ThrowIOE
#include <cassert>            // for invariant
#include <cstdint>            // for uint8_t, uint16_t, int32_t, uint32_t
#include <cstring>            // for memchr, memcmp, memcpy
#include <limits>             // for numeric_limits

namespace rawspeed {

class ByteStream final : public DataBuffer {
  size_type pos =
      0; // position of stream in bytes (this is next byte to deliver)

public:
  ByteStream() = default;

  explicit ByteStream(DataBuffer buffer) : DataBuffer(buffer) {}

  // return ByteStream that starts at given offset
  // i.e. this->data + offset == getSubStream(offset).data
  [[nodiscard]] ByteStream getSubStream(size_type offset,
                                        size_type size_) const {
    return ByteStream(DataBuffer(getSubView(offset, size_), getByteOrder()));
  }

  [[nodiscard]] ByteStream getSubStream(size_type offset) const {
    return ByteStream(DataBuffer(getSubView(offset), getByteOrder()));
  }

  [[nodiscard]] inline size_type check(size_type bytes) const {
    if (static_cast<uint64_t>(pos) + bytes > getSize())
      ThrowIOE("Out of bounds access in ByteStream");
    assert(!ASan::RegionIsPoisoned(data + pos, bytes));
    return bytes;
  }

  [[nodiscard]] inline size_type check(size_type nmemb, size_type size_) const {
    if (size_ && nmemb > std::numeric_limits<size_type>::max() / size_)
      ThrowIOE("Integer overflow when calculating stream length");
    return check(nmemb * size_);
  }

  [[nodiscard]] inline size_type getPosition() const {
    invariant(getSize() >= pos);
    (void)check(0);
    return pos;
  }
  inline void setPosition(size_type newPos) {
    pos = newPos;
    (void)check(0);
  }
  [[nodiscard]] inline size_type RAWSPEED_READONLY getRemainSize() const {
    invariant(getSize() >= pos);
    (void)check(0);
    return getSize() - pos;
  }
  [[nodiscard]] inline const uint8_t* peekData(size_type count) const {
    return Buffer::getData(pos, count);
  }
  inline const uint8_t* getData(size_type count) {
    const uint8_t* ret = Buffer::getData(pos, count);
    pos += count;
    return ret;
  }
  [[nodiscard]] inline Buffer peekBuffer(size_type size_) const {
    return getSubView(pos, size_);
  }
  inline Buffer getBuffer(size_type size_) {
    Buffer ret = peekBuffer(size_);
    pos += size_;
    return ret;
  }
  [[nodiscard]] inline ByteStream peekStream(size_type size_) const {
    return getSubStream(pos, size_);
  }
  [[nodiscard]] inline ByteStream peekStream(size_type nmemb,
                                             size_type size_) const {
    if (size_ && nmemb > std::numeric_limits<size_type>::max() / size_)
      ThrowIOE("Integer overflow when calculating stream length");
    return peekStream(nmemb * size_);
  }
  inline ByteStream getStream(size_type size_) {
    ByteStream ret = peekStream(size_);
    pos += size_;
    return ret;
  }
  inline ByteStream getStream(size_type nmemb, size_type size_) {
    if (size_ && nmemb > std::numeric_limits<size_type>::max() / size_)
      ThrowIOE("Integer overflow when calculating stream length");
    return getStream(nmemb * size_);
  }

  [[nodiscard]] inline uint8_t peekByte(size_type i = 0) const {
    invariant(data);
    (void)check(i + 1);
    return data[pos + i];
  }

  inline void skipBytes(size_type nbytes) { pos += check(nbytes); }
  inline void skipBytes(size_type nmemb, size_type size_) {
    pos += check(nmemb, size_);
  }

  inline bool hasPatternAt(const char* pattern, size_type size_,
                           size_type relPos) const {
    invariant(data);
    if (!isValid(pos + relPos, size_))
      return false;
    return memcmp(&data[pos + relPos], pattern, size_) == 0;
  }

  inline bool hasPrefix(const char* prefix, size_type size_) const {
    return hasPatternAt(prefix, size_, 0);
  }

  inline bool skipPrefix(const char* prefix, size_type size_) {
    bool has_prefix = hasPrefix(prefix, size_);
    if (has_prefix)
      pos += size_;
    return has_prefix;
  }

  inline uint8_t getByte() {
    invariant(data);
    (void)check(1);
    return data[pos++];
  }

  template <typename T> [[nodiscard]] inline T peek(size_type i = 0) const {
    return DataBuffer::get<T>(pos, i);
  }

  [[nodiscard]] inline uint16_t peekU16() const { return peek<uint16_t>(); }

  template <typename T> inline T get() {
    auto ret = peek<T>();
    pos += sizeof(T);
    return ret;
  }

  inline uint16_t getI16() { return get<int16_t>(); }
  inline uint16_t getU16() { return get<uint16_t>(); }
  inline int32_t getI32() { return get<int32_t>(); }
  inline uint32_t getU32() { return get<uint32_t>(); }
  inline float getFloat() { return get<float>(); }

  [[nodiscard]] std::string_view peekString() const {
    Buffer tmp = peekBuffer(getRemainSize());
    const auto* termIter = std::find(tmp.begin(), tmp.end(), '\0');
    if (termIter == tmp.end())
      ThrowIOE("String is not null-terminated");
    std::string_view::size_type strlen = std::distance(tmp.begin(), termIter);
    return {reinterpret_cast<const char*>(tmp.begin()), strlen};
  }

  // Increments the stream to after the next zero byte and returns the bytes in
  // between (not a copy). If the first byte is zero, stream is incremented one.
  [[nodiscard]] std::string_view getString() {
    std::string_view str = peekString();
    skipBytes(1 + str.size());
    return str;
  }
};

} // namespace rawspeed
