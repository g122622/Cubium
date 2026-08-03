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

#include "StructureBoundingBox.hpp"
#include "../../../util/Direction.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"

namespace mc::world::gen::structure {

StructureBoundingBox StructureBoundingBox::fromChunk(i32 chunkX, i32 chunkZ) noexcept
{
    const i32 chunkMinX = chunkX << world::CHUNK_SHIFT;
    const i32 chunkMinZ = chunkZ << world::CHUNK_SHIFT;
    const i32 chunkMaxX = chunkMinX + world::CHUNK_WIDTH - 1;
    const i32 chunkMaxZ = chunkMinZ + world::CHUNK_WIDTH - 1;

    return StructureBoundingBox(
        chunkMinX, world::MIN_BUILD_HEIGHT, chunkMinZ, chunkMaxX, world::MAX_BUILD_HEIGHT - 1, chunkMaxZ);
}

StructureBoundingBox StructureBoundingBox::createBox(i32 x,
    i32 y,
    i32 z,
    i32 offsetX,
    i32 offsetY,
    i32 offsetZ,
    i32 sizeX,
    i32 sizeY,
    i32 sizeZ,
    Direction direction) noexcept
{

    switch (direction) {
        case Direction::North:
            return StructureBoundingBox(x + offsetX,
                y + offsetY,
                z + offsetZ,
                x + offsetX + sizeX - 1,
                y + offsetY + sizeY - 1,
                z + offsetZ + sizeZ - 1);

        case Direction::South:
            return StructureBoundingBox(x + offsetX,
                y + offsetY,
                z - sizeZ - offsetZ + 1,
                x + offsetX + sizeX - 1,
                y + offsetY + sizeY - 1,
                z - offsetZ);

        case Direction::West:
            return StructureBoundingBox(x - sizeZ - offsetZ + 1,
                y + offsetY,
                z + offsetX,
                x - offsetZ,
                y + offsetY + sizeY - 1,
                z + offsetX + sizeX - 1);

        case Direction::East:
            return StructureBoundingBox(x + offsetZ,
                y + offsetY,
                z + offsetX,
                x + offsetZ + sizeZ - 1,
                y + offsetY + sizeY - 1,
                z + offsetX + sizeX - 1);

        default:
            // 默认朝北
            return StructureBoundingBox(x + offsetX,
                y + offsetY,
                z + offsetZ,
                x + offsetX + sizeX - 1,
                y + offsetY + sizeY - 1,
                z + offsetZ + sizeZ - 1);
    }
}

} // namespace mc::world::gen::structure
