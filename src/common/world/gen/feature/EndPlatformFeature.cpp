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

#include "EndPlatformFeature.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc::world::gen::feature {

bool ConfiguredEndPlatformFeature::place(WorldGenRegion& region,
    ChunkPrimer& /*chunk*/,
    IChunkGenerator& /*generator*/,
    math::Random& /*random*/,
    const BlockPos& pos) const
{
    // MC: createEndPlatform(level, origin, false)
    // 5x5 水平范围（i,j ∈ [-2,2]），y 偏移 k ∈ [-1,2]：
    //   k==-1 → OBSIDIAN，其余 → AIR。
    // 仅当当前方块非目标方块时才 setBlock（force=false，不 destroyBlock）。
    Block* obsidianBlock = VanillaBlocks::OBSIDIAN;
    Block* airBlock = VanillaBlocks::AIR;

    for (i32 i = -2; i <= 2; ++i) {
        for (i32 j = -2; j <= 2; ++j) {
            for (i32 k = -1; k < 3; ++k) {
                const BlockPos target(pos.x + j, pos.y + k, pos.z + i);
                Block* desiredBlock = (k == -1) ? obsidianBlock : airBlock;
                const BlockState* current = region.getBlockState(target);
                if (current == nullptr || !current->is(desiredBlock)) {
                    const BlockState* desired = VanillaBlocks::getState(desiredBlock);
                    region.setBlockState(target, desired);
                }
            }
        }
    }
    return true;
}

} // namespace mc::world::gen::feature
