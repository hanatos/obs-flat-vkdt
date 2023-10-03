/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2023 Roman Lebedev

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

#include "rawspeedconfig.h" // for RAWSPEED_READONLY
#include "adt/Array1DRef.h" // for Array1DRef
#include "adt/Invariant.h"  // for invariant
#include <type_traits>      // for negation, is_const, remove_const_t, is_same
#include <vector>           // for vector

namespace rawspeed {

template <class T> class CroppedArray1DRef {
  const Array1DRef<T> base;
  int offset = 0;
  int numElts = 0;

  friend CroppedArray1DRef<const T>; // We need to be able to convert to const
                                     // version.

  // Only allow construction from `Array1DRef<T>::getCrop()`.
  friend CroppedArray1DRef<T> Array1DRef<T>::getCrop(int offset,
                                                     int numElts) const;
  CroppedArray1DRef(Array1DRef<T> base, int offset, int numElts);

public:
  using value_type = T;
  using cvless_value_type = std::remove_cv_t<value_type>;

  CroppedArray1DRef() = default;

  // Can not cast away constness.
  template <
      typename T2,
      std::enable_if_t<std::conjunction_v<std::is_const<T2>,
                                          std::negation<std::is_const<T>>>,
                       bool> = true>
  CroppedArray1DRef(CroppedArray1DRef<T2> RHS) = delete;

  // Can not change type.
  template <typename T2,
            std::enable_if_t<
                std::conjunction_v<
                    std::negation<std::conjunction<
                        std::is_const<T2>, std::negation<std::is_const<T>>>>,
                    std::negation<std::is_same<std::remove_const_t<T>,
                                               std::remove_const_t<T2>>>>,
                bool> = true>
  CroppedArray1DRef(CroppedArray1DRef<T2> RHS) = delete;

  // Conversion from CroppedArray1DRef<T> to CroppedArray1DRef<const T>.
  template <
      typename T2,
      std::enable_if_t<
          std::conjunction_v<
              std::conjunction<std::negation<std::is_const<T2>>,
                               std::is_const<T>>,
              std::is_same<std::remove_const_t<T>, std::remove_const_t<T2>>>,
          bool> = true>
  CroppedArray1DRef( // NOLINT google-explicit-constructor
      CroppedArray1DRef<T2> RHS)
      : base(RHS.base), numElts(RHS.numElts) {}

  [[nodiscard]] const T* begin() const;

  [[nodiscard]] int RAWSPEED_READONLY size() const;

  [[nodiscard]] T& operator()(int eltIdx) const;
};

// CTAD deduction guide
template <typename T>
CroppedArray1DRef(Array1DRef<T> base, int offset, int numElts)
    -> CroppedArray1DRef<T>;

template <class T>
CroppedArray1DRef<T>::CroppedArray1DRef(Array1DRef<T> base_, const int offset_,
                                        const int numElts_)
    : base(base_), offset(offset_), numElts(numElts_) {
  invariant(offset >= 0);
  invariant(numElts >= 0);
  invariant(offset + numElts <= base.size());
}

template <class T> inline const T* CroppedArray1DRef<T>::begin() const {
  return &operator()(/*eltIdx=*/0);
}

template <class T> inline int CroppedArray1DRef<T>::size() const {
  return numElts;
}

template <class T>
inline T& CroppedArray1DRef<T>::operator()(const int eltIdx) const {
  invariant(eltIdx >= 0);
  invariant(eltIdx < numElts);
  return base(offset + eltIdx);
}

} // namespace rawspeed
