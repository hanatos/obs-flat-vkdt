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
#include "io/Endianness.h"    // for Endianness, getHostEndianness, Endiannes...
#include "io/IOException.h"   // for ThrowException, ThrowIOE
#include <cassert>            // for invariant
#include <cstdint>            // for uint8_t, uint64_t, uint32_t
#include <memory>             // for unique_ptr
#include <utility>            // for move, operator<, pair, swap

namespace rawspeed {

/*************************************************************************
 * This is the buffer abstraction.
 *
 * It allows access to some piece of memory, typically a whole or part
 * of a raw file. The underlying memory is never owned by the buffer.
 * It intentionally supports only read/const access to the underlying memory.
 *
 *************************************************************************/
class Buffer {
public:
  using size_type = uint32_t;

protected:
  const uint8_t* data = nullptr;

private:
  size_type size = 0;

public:
  Buffer() = default;

  // Data already allocated
  explicit Buffer(const uint8_t* data_, size_type size_)
      : data(data_), size(size_) {
    assert(!ASan::RegionIsPoisoned(data, size));
  }

  [[nodiscard]] Buffer getSubView(size_type offset, size_type size_) const {
    if (!isValid(0, offset))
      ThrowIOE("Buffer overflow: image file may be truncated");

    return Buffer(getData(offset, size_), size_);
  }

  [[nodiscard]] Buffer getSubView(size_type offset) const {
    if (!isValid(0, offset))
      ThrowIOE("Buffer overflow: image file may be truncated");

    size_type newSize = size - offset;
    return getSubView(offset, newSize);
  }

  // get pointer to memory at 'offset', make sure at least 'count' bytes are
  // accessible
  [[nodiscard]] const uint8_t* getData(size_type offset,
                                       size_type count) const {
    if (!isValid(offset, count))
      ThrowIOE("Buffer overflow: image file may be truncated");

    invariant(data);

    return data + offset;
  }

  // convenience getter for single bytes
  uint8_t operator[](size_type offset) const { return *getData(offset, 1); }

  // std begin/end iterators to allow for range loop
  [[nodiscard]] const uint8_t* begin() const {
    invariant(data);
    return data;
  }
  [[nodiscard]] const uint8_t* end() const {
    invariant(data);
    return data + size;
  }

  // get memory of type T from byte offset 'offset + sizeof(T)*index' and swap
  // byte order if required
  template <typename T>
  [[nodiscard]] inline T get(bool inNativeByteOrder, size_type offset,
                             size_type index = 0) const {
    return getByteSwapped<T>(
        getData(offset + index * static_cast<size_type>(sizeof(T)),
                static_cast<size_type>(sizeof(T))),
        !inNativeByteOrder);
  }

  [[nodiscard]] inline size_type RAWSPEED_READONLY getSize() const {
    return size;
  }

  [[nodiscard]] inline bool isValid(size_type offset,
                                    size_type count = 1) const {
    return static_cast<uint64_t>(offset) + count <= static_cast<uint64_t>(size);
  }
};

// WARNING: both buffers must belong to the same allocation, else this is UB!
inline bool operator<(Buffer lhs, Buffer rhs) {
  return std::pair(lhs.begin(), lhs.end()) < std::pair(rhs.begin(), rhs.end());
}

/*
 * DataBuffer is a simple extension to Buffer. It knows about the byte order
 * of its contents and can therefore provide save access to larger than
 * byte sized data, like int, float, etc.
 */
class DataBuffer : public Buffer {
  // FIXME: default should be Endianness::unknown !

  Endianness endianness = Endianness::little;

public:
  DataBuffer() = default;

  explicit DataBuffer(Buffer data_, Endianness endianness_)
      : Buffer(data_), endianness(endianness_) {}

  // get memory of type T from byte offset 'offset + sizeof(T)*index' and swap
  // byte order if required
  template <typename T>
  [[nodiscard]] inline T get(size_type offset, size_type index = 0) const {
    assert(Endianness::unknown != endianness);
    assert(Endianness::little == endianness || Endianness::big == endianness);

    return Buffer::get<T>(getHostEndianness() == endianness, offset, index);
  }

  [[nodiscard]] inline Endianness getByteOrder() const { return endianness; }

  inline Endianness setByteOrder(Endianness endianness_) {
    std::swap(endianness, endianness_);
    return endianness_;
  }
};

} // namespace rawspeed
