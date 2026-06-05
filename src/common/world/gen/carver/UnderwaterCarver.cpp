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

#include "UnderwaterCarver.hpp"
#include "../../../core/Constants.hpp"
#include "../../../util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <unordered_set>

namespace mc {

// ============================================================================
// 水下可雕刻方块集合
// ============================================================================

static const std::unordered_set<u32>& getUnderwaterCarvableBlocks()
{
    static std::unordered_set<u32> blocks = {// 标准可雕刻方块
        VanillaBlocks::STONE->blockId(),
        VanillaBlocks::GRANITE->blockId(),
        VanillaBlocks::DIORITE->blockId(),
        VanillaBlocks::ANDESITE->blockId(),
        VanillaBlocks::DIRT->blockId(),
        VanillaBlocks::COARSE_DIRT->blockId(),
        VanillaBlocks::PODZOL->blockId(),
        VanillaBlocks::GRASS_BLOCK->blockId(),
        // 陶瓦（包括染色陶瓦）
        VanillaBlocks::TERRACOTTA->blockId(),
        VanillaBlocks::WHITE_TERRACOTTA->blockId(),
        VanillaBlocks::ORANGE_TERRACOTTA->blockId(),
        VanillaBlocks::MAGENTA_TERRACOTTA->blockId(),
        VanillaBlocks::LIGHT_BLUE_TERRACOTTA->blockId(),
        VanillaBlocks::YELLOW_TERRACOTTA->blockId(),
        VanillaBlocks::LIME_TERRACOTTA->blockId(),
        VanillaBlocks::PINK_TERRACOTTA->blockId(),
        VanillaBlocks::GRAY_TERRACOTTA->blockId(),
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA->blockId(),
        VanillaBlocks::CYAN_TERRACOTTA->blockId(),
        VanillaBlocks::PURPLE_TERRACOTTA->blockId(),
        VanillaBlocks::BLUE_TERRACOTTA->blockId(),
        VanillaBlocks::BROWN_TERRACOTTA->blockId(),
        VanillaBlocks::GREEN_TERRACOTTA->blockId(),
        VanillaBlocks::RED_TERRACOTTA->blockId(),
        VanillaBlocks::BLACK_TERRACOTTA->blockId(),
        // 沙子和砂岩
        VanillaBlocks::SANDSTONE->blockId(),
        VanillaBlocks::RED_SANDSTONE->blockId(),
        VanillaBlocks::MYCELIUM->blockId(),
        VanillaBlocks::SNOW->blockId(),
        // 水下特有的可雕刻方块
        VanillaBlocks::SAND->blockId(),
        VanillaBlocks::GRAVEL->blockId(),
        VanillaBlocks::WATER->blockId(),
        VanillaBlocks::LAVA->blockId(),
        VanillaBlocks::OBSIDIAN->blockId(),
        VanillaBlocks::PACKED_ICE->blockId()};
    return blocks;
}

// ============================================================================
// UnderwaterCaveCarver 实现
// ============================================================================

UnderwaterCaveCarver::UnderwaterCaveCarver()
    : CaveCarver(world::MAX_BUILD_HEIGHT)
{}

bool UnderwaterCaveCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 /*y*/) const noexcept
{
    // 水下洞穴使用与普通洞穴相同的椭球检测
    // MC: shouldSkip(dy <= floorLevel ? true : dx*dx + dy*dy + dz*dz >= 1.0)
    return dy <= -0.7f || dx * dx + dy * dy + dz * dz >= 1.0f;
}

bool UnderwaterCaveCarver::canCarveBlock(const BlockState* state, const BlockState* /*aboveState*/) const
{
    if (!state) {
        return false;
    }

    // 检查是否为空气
    if (state->isAir()) {
        return true;
    }

    // 检查是否在水下可雕刻方块列表中
    const auto& blocks = getUnderwaterCarvableBlocks();
    return blocks.find(state->blockId()) != blocks.end();
}

// ============================================================================
// UnderwaterCanyonCarver 实现
// ============================================================================

UnderwaterCanyonCarver::UnderwaterCanyonCarver()
    : CanyonCarver(world::MAX_BUILD_HEIGHT)
{}

bool UnderwaterCanyonCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const noexcept
{
    // 水下峡谷使用与普通峡谷相同的厚度检测
    return CanyonCarver::shouldSkipEllipsoidPosition(dx, dy, dz, y);
}

bool UnderwaterCanyonCarver::canCarveBlock(const BlockState* state, const BlockState* /*aboveState*/) const
{
    if (!state) {
        return false;
    }

    // 检查是否为空气
    if (state->isAir()) {
        return true;
    }

    // 检查是否在水下可雕刻方块列表中
    const auto& blocks = getUnderwaterCarvableBlocks();
    return blocks.find(state->blockId()) != blocks.end();
}

} // namespace mc
