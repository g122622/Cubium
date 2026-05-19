/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

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

size_t AudioData::sampleCount() const noexcept
{
    if (format.channels == 0 || format.bitsPerSample == 0) {
        return 0;
    }

    size_t bytesPerSample = format.bitsPerSample / 8;
    return samples.size() / (format.channels * bytesPerSample);
}

f32 AudioData::calculateDuration() const noexcept
{
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
