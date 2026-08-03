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

#include "JukeboxSong.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cmath>
#include <string>
#include <utility>

namespace mc {

JukeboxSong::JukeboxSong(
    ResourceLocation soundEventId, std::string descriptionKey, f32 lengthInSeconds, i32 comparatorOutput)
    : m_soundEventId(std::move(soundEventId))
    , m_descriptionKey(std::move(descriptionKey))
    , m_lengthInSeconds(lengthInSeconds)
    , m_comparatorOutput(comparatorOutput)
{}

i32 JukeboxSong::lengthInTicks() const
{
    return static_cast<i32>(std::ceil(m_lengthInSeconds * 20.0f));
}

bool JukeboxSong::hasFinished(i64 ticksSinceSongStarted) const
{
    return ticksSinceSongStarted >= static_cast<i64>(lengthInTicks()) + SONG_END_PADDING_TICKS;
}

} // namespace mc
