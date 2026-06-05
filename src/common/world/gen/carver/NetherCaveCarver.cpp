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

#include "NetherCaveCarver.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {

namespace {
// TODO: NETHER_HEIGHT 常量在 NetherChunkGenerator.cpp 中也有定义，应该统一提取到公共头文件中
constexpr i32 NETHER_HEIGHT = 128;
} // namespace

// ============================================================================
// 下界可雕刻方块集合 - 使用延迟初始化避免静态初始化顺序问题
// ============================================================================

static const std::unordered_set<BlockId>& getNetherCarvableBlocks()
{
    static std::unordered_set<BlockId> blocks = {VanillaBlocks::STONE->blockId(),
        VanillaBlocks::GRANITE->blockId(),
        VanillaBlocks::DIORITE->blockId(),
        VanillaBlocks::ANDESITE->blockId(),
        VanillaBlocks::DIRT->blockId(),
        VanillaBlocks::COARSE_DIRT->blockId(),
        VanillaBlocks::PODZOL->blockId(),
        VanillaBlocks::GRASS_BLOCK->blockId(),
        VanillaBlocks::NETHERRACK->blockId(),
        VanillaBlocks::SOUL_SAND->blockId(),
        VanillaBlocks::SOUL_SOIL->blockId(),
        VanillaBlocks::CRIMSON_NYLIUM->blockId(),
        VanillaBlocks::WARPED_NYLIUM->blockId(),
        VanillaBlocks::NETHER_WART_BLOCK->blockId(),
        VanillaBlocks::WARPED_WART_BLOCK->blockId(),
        VanillaBlocks::BASALT->blockId(),
        VanillaBlocks::BLACKSTONE->blockId()};
    return blocks;
}

// ============================================================================
// NetherCaveCarver 实现
// ============================================================================

NetherCaveCarver::NetherCaveCarver()
    : CaveCarver(NETHER_HEIGHT)
{}

f32 NetherCaveCarver::getCaveRadius(math::IRandom& rng) const
{
    // 下界洞穴半径更大：(nextFloat() * 2.0F + nextFloat()) * 2.0F
    return (rng.nextFloat() * 2.0f + rng.nextFloat()) * 2.0f;
}

i32 NetherCaveCarver::getCaveStartY(math::IRandom& rng) const
{
    // 下界使用完整高度范围
    return rng.nextInt(m_maxHeight);
}

bool NetherCaveCarver::_isNetherCarvable(const BlockState& state)
{
    const auto& blocks = getNetherCarvableBlocks();
    return blocks.find(state.blockId()) != blocks.end();
}

bool NetherCaveCarver::canCarveBlock(const BlockState* state, const BlockState* aboveState) const
{
    if (!state) {
        return false;
    }

    // 检查是否在下界可雕刻方块列表中
    if (_isNetherCarvable(*state)) {
        return true;
    }

    // 沙子和沙砾可以在特定条件下雕刻
    bool isSandOrGravel = state->is(VanillaBlocks::SAND) || state->is(VanillaBlocks::GRAVEL);
    if (isSandOrGravel && aboveState) {
        return !aboveState->isLiquid();
    }

    return false;
}

} // namespace mc
