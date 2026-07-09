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
 */

#include "MatchingBlockPredicate.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc::world::gen::feature::predicate {

bool MatchingBlockPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos + m_offset);
    // ChunkData 对未初始化 section 返回 nullptr 表示空气（空气不持久化到 section），
    // 故 nullptr 等价于 AIR 方块状态：当且仅当本谓词匹配的正是空气方块时为真。
    if (state == nullptr) {
        return m_block != nullptr && m_block == BlockRegistry::instance().getBlock(ResourceLocation("minecraft:air"));
    }
    return &state->getBlock() == m_block;
}

} // namespace mc::world::gen::feature::predicate
