#include "client/sound/backend/AudioBuffer.hpp"

#include "common/resource/ResourcePackList.hpp"

#include <algorithm>
#include <fmt/format.h>

namespace mc::client::sound {

// ============================================================================
// AudioData 实现
// ============================================================================

AudioData::AudioData(AudioFormat format, std::vector<u8> data)
    : format(format)
    , samples(std::move(data))
{
    duration = calculateDuration();
}

size_t AudioData::sampleCount() const noexcept {
    if (format.channels == 0 || format.bitsPerSample == 0) {
        return 0;
    }

    size_t bytesPerSample = format.bitsPerSample / 8;
    return samples.size() / (format.channels * bytesPerSample);
}

f32 AudioData::calculateDuration() const noexcept {
    if (format.sampleRate == 0 || format.channels == 0) {
        return 0.0f;
    }

    size_t bytesPerSample = format.bitsPerSample / 8;
    if (bytesPerSample == 0) {
        return 0.0f;
    }

    size_t totalSamples = samples.size() / (format.channels * bytesPerSample);
    return static_cast<f32>(totalSamples) / static_cast<f32>(format.sampleRate);
}

} // namespace mc::client::sound
