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

#include "common/world/block/blocks/container/WeatheringCopperChestBlock.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "util/math/random/IRandom.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ============================================================================
// WeatheringCopperChestBlock 实现
// ============================================================================

WeatheringCopperChestBlock::WeatheringCopperChestBlock(const BlockProperties& properties,
    BlockStateProperties::OxidationLevel oxidationLevel,
    const ResourceLocation& openSound,
    const ResourceLocation& closeSound)
    : CopperChestBlock(properties, oxidationLevel, openSound, closeSound)
{
    // 可氧化铜箱子同样不添加 OXIDATION 方块状态属性（氧化等级通过方块 ID 区分）
    // ticksRandomly 由 ticksRandomly() 虚方法动态返回，不修改 m_ticksRandomly 标志
    // 开合音效由父类 CopperChestBlock 存储，按氧化等级映射
}

void WeatheringCopperChestBlock::randomTick(
    IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 对应 MC Java: WeatheringCopperChestBlock.randomTick
    // 满足以下条件之一时不氧化：
    // 1. 当前箱子是双箱的 RIGHT 部分（避免双箱两侧同时氧化导致不同步）
    // 2. 箱子正在被玩家打开（ChestEntity::getOpenCount() > 0）
    if (state.get(BlockStateProperties::CHEST_TYPE()) == BlockStateProperties::ChestType::Right) {
        return;
    }

    BlockEntity* be = world.getBlockEntity(pos);
    if (be != nullptr) {
        auto* chestEntity = dynamic_cast<blockentity::ChestEntity*>(be);
        if (chestEntity != nullptr && chestEntity->getOpenCount() > 0) {
            return;
        }
    }

    // 调用 IOxidizableBlock::tryOxidize 尝试氧化到下一等级
    // tryOxidize 内部使用 withPropertiesOf() 保留 HORIZONTAL_FACING/CHEST_TYPE/WATERLOGGED 属性
    (void)tryOxidize(world, pos, state, random);
}

} // namespace blocks
} // namespace mc
