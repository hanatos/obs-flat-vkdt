/*
 * Copyright 2023 MotionCam
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef Decoder_hpp
#define Decoder_hpp

#include <motioncam/Container.hpp>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <map>

namespace motioncam {
    typedef int64_t Timestamp;
    typedef std::pair<Timestamp, std::vector<int16_t>> AudioChunk;

    class MotionCamException : public std::runtime_error {
    public:
        MotionCamException(const std::string& error) : runtime_error(error) {}
    };
    
    class IOException : public MotionCamException {
    public:
        IOException(const std::string& error) : MotionCamException(error) {}
    };

    class Decoder {
    public:
        Decoder(const std::string& path);
        Decoder(FILE* file);
        
        ~Decoder();
        
        // Get container metadata
        const nlohmann::json& getContainerMetadata() const;
        
        // Get all frame timestamps in container
        const std::vector<Timestamp>& getFrames() const;

        // get encoded pointers to data for external decoding
        int getEncoded(
            const Timestamp timestamp,
            uint16_t *out_bits, int out_bits_len,
            uint16_t *out_refs, int out_refs_len,
            uint8_t  *out_data, size_t out_data_max_len);
        
        // Load a single frame and its metadata.
        void loadFrame(const Timestamp timestamp, std::vector<uint16_t>& outData, nlohmann::json& outMetadata);
        
        // Audio sample rate
        int audioSampleRateHz() const;
        
        // Number of channels in audio
        int numAudioChannels() const;
        
        // Load all audio chunks.
        void loadAudio(std::vector<AudioChunk>& outAudioChunks);
        
    private:
        void init();
        void read(void* data, size_t size, size_t items=1) const;
        void readIndex();
        void reindexOffsets();
        void readExtra();
        void uncompress(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst);
        
    private:
        FILE* mFile;
        std::vector<BufferOffset> mOffsets;
        std::vector<BufferOffset> mAudioOffsets;
        std::map<Timestamp, BufferOffset> mFrameOffsetMap;
        std::vector<Timestamp> mFrameList;
        nlohmann::json mMetadata;
        std::vector<uint8_t> mTmpBuffer;
        Timestamp cache_timestamp;
    };
} // namespace motioncam

#endif /* Decoder_hpp */
