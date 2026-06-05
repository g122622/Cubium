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

#include "SnowBlock.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc::blocks {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 雪融化的最小光照等级阈值
constexpr i32 MELT_LIGHT_LEVEL = 11;

} // namespace

// ============================================================================
// SnowBlock 实现
// ============================================================================

SnowBlock::SnowBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 注册 LAYERS 属性（1-8层）
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(LAYERS()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 默认状态：1层
    setDefaultState(getDefaultState().with(LAYERS(), 1));
}

void SnowBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    (void)random; // 雪融化不需要随机数

    // 如果方块光照 > 11，融化
    u8 blockLight = world.getBlockLight(pos);
    u8 skyLight = world.getSkyLight(pos);

    // 计算综合光照（不考虑天气衰减的简化版本）
    i32 lightLevel = static_cast<i32>(std::max(blockLight, skyLight));

    if (lightLevel > MELT_LIGHT_LEVEL) {
        // 融化：掉落雪球并移除方块
        // 雪层掉落雪球数量等于层数
        i32 layers = state.get(LAYERS());
        if (layers > 0) {
            ItemStack dropStack(*Items::SNOWBALL, layers);
            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world,
                dropStack,
                static_cast<f64>(pos.x) + 0.5,
                static_cast<f64>(pos.y) + 0.5,
                static_cast<f64>(pos.z) + 0.5,
                rng);
        }
        const BlockState* airState = &VanillaBlocks::AIR->defaultState();
        world.setBlockState(pos, airState);
    }
}

} // namespace mc::blocks
