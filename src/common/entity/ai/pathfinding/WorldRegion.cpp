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

#include "WorldRegion.hpp"
#include "common/core/Types.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc::entity::ai::pathfinding {

namespace {
// 区块坐标转换：方块坐标 → 区块坐标（地板除法对负数也正确，对齐 Minecraft 语义）。
// Minecraft 的 ChunkPos.x = floor(blockX / 16)，C++ 整数除法对负数已是向零截断，
// 故需显式 floor。但项目 ChunkCoord 转换历史上一致用 >> 4（算术右移即 floor），
// 此处沿用 getChunkSync 等同款位移语义避免不一致。
ChunkCoord blockToChunk(i32 blockCoord) noexcept
{
    return blockCoord >> 4;
}
} // namespace

u32 WorldRegion::getBlockStateId(i32 x, i32 y, i32 z) const
{
    if (m_world == nullptr) {
        return 0;
    }
    const BlockState* state = m_world->getBlockState(x, y, z);
    if (state == nullptr) {
        return 0;
    }
    return state->stateId();
}

bool WorldRegion::isLoaded(i32 x, i32 z) const
{
    if (m_world == nullptr) {
        return false;
    }
    return m_world->hasChunk(blockToChunk(x), blockToChunk(z));
}

i32 WorldRegion::getHeight(i32 x, i32 z) const
{
    if (m_world == nullptr) {
        return 0;
    }
    return m_world->getHeight(x, z);
}

bool WorldRegion::isWalkable(i32 x, i32 y, i32 z) const
{
    if (m_world == nullptr) {
        return false;
    }
    const BlockState* state = m_world->getBlockState(x, y, z);
    if (state == nullptr || state->isAir()) {
        return false;
    }
    // 实心且不可替换 = 可站立支撑（脚部下方地面）。
    // 对齐 WalkNodeProcessor 语义：isWalkable(x,y,z) 表示该位置方块是实心可站立支撑，
    // 空气返回 false、石头返回 true。详见 memory: walknodeprocessor-getnodetype-walkable-bug。
    return state->isSolid() && !state->canBeReplaced();
}

bool WorldRegion::isWater(i32 x, i32 y, i32 z) const
{
    if (m_world == nullptr) {
        return false;
    }
    return m_world->isWaterAt(x, y, z);
}

bool WorldRegion::isLava(i32 x, i32 y, i32 z) const
{
    if (m_world == nullptr) {
        return false;
    }
    return m_world->isLavaAt(x, y, z);
}

bool WorldRegion::canSeeSky(i32 x, i32 y, i32 z) const
{
    if (m_world == nullptr) {
        return false;
    }
    return m_world->canSeeSky(BlockPos(x, y, z));
}

} // namespace mc::entity::ai::pathfinding
