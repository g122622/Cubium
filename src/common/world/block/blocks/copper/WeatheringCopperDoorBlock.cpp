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

#include "WeatheringCopperDoorBlock.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/BlockPos.hpp"
#include "IOxidizableBlock.hpp"

namespace mc {
namespace blocks {

WeatheringCopperDoorBlock::WeatheringCopperDoorBlock(
    const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : DoorBlock(properties, true) // 铜门只能红石控制，类似铁门
    , m_oxidationLevel(oxidationLevel)
{
    // 只有未达到最高氧化等级的方块需要随机刻
    if (m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized) {
        m_ticksRandomly = true;
    }
}

void WeatheringCopperDoorBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 只有下半部分方块才触发氧化，避免上下半部分双重氧化
    if (state.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) != BlockStateProperties::DoubleBlockHalf::Lower) {
        return;
    }

    // 已是最高氧化等级则不处理
    if (m_oxidationLevel == BlockStateProperties::OxidationLevel::Oxidized) {
        return;
    }

    // 获取下一氧化等级的方块
    if (m_nextOxidationBlock == nullptr) {
        return;
    }

    // 外层门限概率：约 5.69% (MC原版 0.05688889)
    constexpr float OXIDATION_GATE_CHANCE = 0.05688889f;
    if (random.nextFloat() >= OXIDATION_GATE_CHANCE) {
        return;
    }

    // 扫描4格曼哈顿距离内的可氧化铜方块，计算氧化概率
    const i32 myOrdinal = static_cast<i32>(m_oxidationLevel);
    i32 sameAgeCount = 0;             // 同等级邻居数 (j)
    i32 higherAgeCount = 0;           // 更高等级邻居数 (k)
    bool hasLowerAgeNeighbor = false; // 是否存在更低等级邻居

    // 遍历曼哈顿距离4以内的所有方块位置
    for (i32 dy = -4; dy <= 4; ++dy) {
        for (i32 dz = -4; dz <= 4; ++dz) {
            for (i32 dx = -4; dx <= 4; ++dx) {
                // 跳过自身
                if (dx == 0 && dy == 0 && dz == 0) {
                    continue;
                }

                // 曼哈顿距离检查
                i32 dist = std::abs(dx) + std::abs(dy) + std::abs(dz);
                if (dist > 4) {
                    continue;
                }

                BlockPos neighborPos(pos.x + dx, pos.y + dy, pos.z + dz);

                // 获取邻居方块
                const BlockState* neighborState = world.getBlockState(neighborPos);
                if (neighborState == nullptr) {
                    continue;
                }

                // 检查是否为可氧化方块
                const Block& neighborBlock = neighborState->getBlock();
                const auto* oxidizable = dynamic_cast<const IOxidizableBlock*>(&neighborBlock);
                if (oxidizable == nullptr) {
                    continue;
                }

                // 比较氧化等级
                i32 neighborOrdinal = static_cast<i32>(oxidizable->getOxidationLevel());

                if (neighborOrdinal < myOrdinal) {
                    // 存在更低氧化等级的邻居 → 立即取消氧化
                    hasLowerAgeNeighbor = true;
                    break;
                }

                if (neighborOrdinal > myOrdinal) {
                    higherAgeCount++;
                } else {
                    sameAgeCount++;
                }
            }
            if (hasLowerAgeNeighbor) {
                break;
            }
        }
        if (hasLowerAgeNeighbor) {
            break;
        }
    }

    // 若存在更低等级邻居，不氧化
    if (hasLowerAgeNeighbor) {
        return;
    }

    // 计算最终氧化概率
    // f = (k+1) / (k+j+1)，然后 f1 = f^2 * chanceModifier
    float f = static_cast<float>(higherAgeCount + 1) / static_cast<float>(higherAgeCount + sameAgeCount + 1);
    float f1 = f * f * getOxidationChanceModifier();

    if (random.nextFloat() < f1) {
        const BlockState& nextState = m_nextOxidationBlock->defaultState();
        world.setBlockState(pos, &nextState, 3);

        // 上半部分也需要一起替换
        BlockPos upperPos = pos.up();
        const BlockState* upperState = world.getBlockState(upperPos);
        if (upperState != nullptr && upperState->is(this)) {
            BlockState upperNext =
                m_nextOxidationBlock->defaultState()
                    .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper)
                    .with(BlockStateProperties::FACING(), upperState->get(BlockStateProperties::FACING()))
                    .with(BlockStateProperties::HINGE(), upperState->get(BlockStateProperties::HINGE()))
                    .with(BlockStateProperties::OPEN(), upperState->get(BlockStateProperties::OPEN()))
                    .with(BlockStateProperties::POWERED(), upperState->get(BlockStateProperties::POWERED()));
            world.setBlockState(upperPos, &upperNext, 3);
        }
    }
}

} // namespace blocks
} // namespace mc
