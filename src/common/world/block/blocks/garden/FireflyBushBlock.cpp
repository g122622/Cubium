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

#include "FireflyBushBlock.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"

#include <array>
#include <optional>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

FireflyBushBlock::FireflyBushBlock(const BlockProperties& properties)
    : BushBlock(properties)
{}

// ========== IGrowable 接口实现 ==========

bool FireflyBushBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);

    // 等价于 Java BonemealableBlock.hasSpreadableNeighbourPos：以正序遍历水平 4 方向邻居，
    // 只要存在一个「空气 + 其下方可支撑萤火虫灌木」的位置即返回 true（存在可种植蔓延目标）。
    // canGrow 仅判定存在性，不消费随机数（方向打乱只在 grow 选址时使用）。
    return getSpreadableNeighbourPos(world, pos, Directions::horizontal()).has_value();
}

bool FireflyBushBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // 萤火虫灌木骨粉 100% 成功（wiki 记录为确定行为），始终返回 true。
    return true;
}

void FireflyBushBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 等价于 Java BonemealableBlock.findSpreadableNeighbourPos：以随机顺序遍历水平 4 方向邻居，
    // 找首个「空气 + 其下方可支撑萤火虫灌木」的位置放置默认灌木状态。曼哈顿距离恰为 1（仅水平）。
    // 复制方向数组后用 IRandom::shuffle（Fisher-Yates）打乱，等价 Java Direction.Plane.HORIZONTAL
    // .shuffledCopy(random) 的随机序遍历。
    std::array<Direction, 4> dirs = Directions::horizontal();
    std::vector<Direction> shuffledDirs(dirs.begin(), dirs.end());
    random.shuffle(shuffledDirs);
    for (i32 i = 0; i < 4; ++i) {
        dirs[static_cast<size_t>(i)] = shuffledDirs[static_cast<size_t>(i)];
    }

    const std::optional<BlockPos> target = getSpreadableNeighbourPos(static_cast<IBlockReader&>(world), pos, dirs);
    if (target.has_value()) {
        // 放置默认灌木状态。flags=3 触发邻居更新 + 同步（与 BigDripleafStemBlock 等生成新方块一致），
        // 确保新灌木立即接入支撑/光照判定链路。
        world.setBlockState(target.value(), &defaultState(), 3);
    }
}

// ========== 私有方法 ==========

std::optional<BlockPos> FireflyBushBlock::getSpreadableNeighbourPos(
    IBlockReader& world, const BlockPos& pos, const std::array<Direction, 4>& directions) const
{
    for (const Direction dir : directions) {
        // 邻居位置 = pos + 方向偏移（曼哈顿距离 1，仅水平方向）。
        const BlockPos neighbourPos(
            pos.x + Directions::xOffset(dir), pos.y + Directions::yOffset(dir), pos.z + Directions::zOffset(dir));

        // 邻居必须为空气（可被新灌木替换）。
        const BlockState* neighbourState = world.getBlockState(neighbourPos);
        if (neighbourState == nullptr || !neighbourState->isAir()) {
            continue;
        }

        // 邻居位置须可支撑萤火虫灌木（其下方为 #dirt 标签或耕地，走 BushBlock::isValidPosition）。
        // 等价 Java state.canSurvive(world, neighbourPos) → VegetationBlock.canSurvive → mayPlaceOn。
        if (isValidPosition(defaultState(), world, neighbourPos)) {
            return neighbourPos;
        }
    }
    return std::nullopt;
}

} // namespace blocks
} // namespace mc
