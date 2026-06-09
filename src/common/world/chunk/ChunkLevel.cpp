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

#include "ChunkLevel.hpp"
#include "ChunkPyramid.hpp"
#include "ChunkStatus.hpp"

namespace mc::world::chunk {

i32 ChunkLevel::radiusAroundFullChunk()
{
    static const i32 s_radius = mc::ChunkPyramid::generationPyramid().getStepTo(mc::ChunkStatuses::FULL).accumulatedDependencies().getRadius();
    return s_radius;
}

i32 ChunkLevel::maxLevel()
{
    static const i32 s_maxLevel = FULL_CHUNK_LEVEL + radiusAroundFullChunk();
    return s_maxLevel;
}

const mc::ChunkStatus* ChunkLevel::generationStatus(i32 level)
{
    if (level <= FULL_CHUNK_LEVEL) {
        return &mc::ChunkStatuses::FULL;
    }
    const i32 distance = level - FULL_CHUNK_LEVEL;
    return getStatusAroundFullChunk(distance);
}

i32 ChunkLevel::byStatus(const mc::ChunkStatus& status)
{
    if (status == mc::ChunkStatuses::FULL) {
        return FULL_CHUNK_LEVEL;
    }
    return FULL_CHUNK_LEVEL + mc::ChunkPyramid::generationPyramid().getStepTo(mc::ChunkStatuses::FULL).getAccumulatedRadiusOf(status);
}

const mc::ChunkStatus* ChunkLevel::getStatusAroundFullChunk(i32 distance)
{
    if (distance <= 0) {
        return &mc::ChunkStatuses::FULL;
    }
    if (distance > radiusAroundFullChunk()) {
        return nullptr;
    }
    return mc::ChunkPyramid::generationPyramid().getStepTo(mc::ChunkStatuses::FULL).accumulatedDependencies().get(distance);
}

} // namespace mc::world::chunk
