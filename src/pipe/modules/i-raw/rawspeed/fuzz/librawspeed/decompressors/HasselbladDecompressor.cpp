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

#include "decompressors/HasselbladDecompressor.h"
#include "MemorySanitizer.h"                // for MSan
#include "codes/PrefixCodeDecoder.h"        // for PrefixCodeDecoder
#include "codes/PrefixCodeDecoder/Common.h" // for createPrefixCodeDecoder
#include "common/RawImage.h"                // for RawImage, RawImageData
#include "common/RawspeedException.h"       // for RawspeedException
#include "fuzz/Common.h"                    // for CreateRawImage
#include "io/Buffer.h"                      // for Buffer, DataBuffer
#include "io/ByteStream.h"                  // for ByteStream
#include "io/Endianness.h" // for Endianness, Endianness::little
#include <algorithm>       // for fill, copy, fill_n, generate_n
#include <cassert>         // for assert
#include <cstddef>         // for size_t
#include <cstdint>         // for uint16_t, uint8_t
#include <vector>          // for vector

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  assert(Data);

  try {
    const rawspeed::Buffer b(Data, Size);
    const rawspeed::DataBuffer db(b, rawspeed::Endianness::little);
    rawspeed::ByteStream bs(db);

    rawspeed::RawImage mRaw(CreateRawImage(bs));

    const auto ht = createPrefixCodeDecoder<rawspeed::PrefixCodeDecoder<>>(bs);
    const auto initPred = bs.get<uint16_t>();

    rawspeed::HasselbladDecompressor::PerComponentRecipe rec = {ht, initPred};

    rawspeed::HasselbladDecompressor d(mRaw, rec,
                                       bs.getSubStream(/*offset=*/0));
    mRaw->createData();
    (void)d.decompress();

    rawspeed::MSan::CheckMemIsInitialized(
        mRaw->getByteDataAsUncroppedArray2DRef());
  } catch (const rawspeed::RawspeedException&) {
    // Exceptions are good, crashes are bad.
  }

  return 0;
}
