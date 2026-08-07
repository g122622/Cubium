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

#include "RedstonePower.hpp"
#include "../../util/Direction.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "../block/BlockPos.hpp"
#include "../block/BlockState.hpp"
#include "../block/blocks/redstone/RedstoneWireBlock.hpp"
#include "RedstoneHelper.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace redstone {

// ========== 强信号 ==========

i32 RedstonePower::getStrongPower(IWorld& world, const BlockPos& pos, Direction side)
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        return MIN_POWER;
    }

    // 获取方块在指定方向的强信号输出
    return state->getBlock().getStrongPower(*state, world, pos, side);
}

i32 RedstonePower::getStrongPower(IWorld& world, const BlockPos& pos)
{
    i32 maxPower = MIN_POWER;

    // 遍历六个方向
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);
        i32 power = getStrongPower(world, neighborPos, oppositeDir);
        maxPower = std::max(maxPower, power);

        if (maxPower >= MAX_POWER) {
            break; // 已经是最大值，无需继续
        }
    }

    return maxPower;
}

// ========== 弱信号 ==========

i32 RedstonePower::getWeakPower(IWorld& world, const BlockPos& pos, Direction side)
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || state->isAir()) {
        return MIN_POWER;
    }

    // 获取方块在指定方向的弱信号输出
    return state->getBlock().getWeakPower(*state, world, pos, side);
}

i32 RedstonePower::getWeakPower(IWorld& world, const BlockPos& pos)
{
    i32 maxPower = MIN_POWER;

    // 遍历六个方向
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);
        i32 power = getWeakPower(world, neighborPos, oppositeDir);
        maxPower = std::max(maxPower, power);

        if (maxPower >= MAX_POWER) {
            break; // 已经是最大值，无需继续
        }
    }

    return maxPower;
}

// ========== 充能检测 ==========

bool RedstonePower::isPowered(IWorld& world, const BlockPos& pos)
{
    // 检查是否被间接充能（相邻方块有强信号输出）
    return isIndirectlyPowered(world, pos);
}

bool RedstonePower::isIndirectlyPowered(IWorld& world, const BlockPos& pos)
{
    // 遍历六个方向检查是否有强信号或弱信号输出。
    // 必须同时检查强信号(getStrongPower)与弱信号(getWeakPower)：
    //   - 强信号：红石火把的 Down 方向、中继器输出端等，直接充能相邻实体方块。
    //   - 弱信号：朝上立红石火把对其水平相邻方块输出 15、红石块全方向 15、被充能的实体方块等。
    // 此前仅查 getStrongPower，导致"朝上立火把水平相邻充能铁轨"这一基岩 vanilla 可观察行为无法复现
    // （torch 的 getStrongPower 仅 Down 返回 15，水平方向返回 0；但其 getWeakPower 水平方向返回 15），
    // 进而 PoweredRailBlock::neighborChanged 重算 shouldBePowered 恒为 false，把结构预置的 powered=true
    // 覆盖成 false，矿车读 powered=false 永不启动（GameTest minibiomes 根因）。
    // 普通方块 getWeakPower/getStrongPower 默认返回 0（见 Block.hpp 基类默认实现），故遍历无副作用。
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();
        if (neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir) > 0) {
            return true;
        }
        if (neighborBlock.getWeakPower(*neighborState, world, neighborPos, oppositeDir) > 0) {
            return true;
        }
    }

    return false;
}

bool RedstonePower::isSidePowered(IWorld& world, const BlockPos& pos, Direction side)
{
    BlockPos neighborPos = pos.offset(side);

    const BlockState* neighborState = world.getBlockState(neighborPos);
    if (!neighborState || neighborState->isAir()) {
        return false;
    }

    const Block& neighborBlock = neighborState->getBlock();

    // 如果相邻方块是红石线，直接获取其信号强度
    if (neighborState->is(VanillaBlocks::REDSTONE_WIRE)) {
        return blocks::RedstoneWireBlock::getPower(*neighborState) > 0;
    }

    // 其他方块：检查强信号
    Direction oppositeDir = Directions::opposite(side);
    return neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir) > 0;
}

// ========== 特殊信号计算 ==========

i32 RedstonePower::getWireInputPower(IWorld& world, const BlockPos& pos)
{
    i32 maxPower = MIN_POWER;

    // 1. 从相邻信号源获取强信号
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();

        // 检查强信号
        if (neighborBlock.canProvidePower(*neighborState)) {
            i32 strongPower = neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir);
            if (strongPower > maxPower) {
                maxPower = strongPower;
            }
        }
    }

    // 2. 从相邻红石线获取信号（每格衰减1）
    if (maxPower < MAX_POWER) {
        for (Direction dir : Directions::horizontal()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);

            if (!neighborState) {
                continue;
            }

            // 检查水平相邻的红石线
            if (neighborState->is(VanillaBlocks::REDSTONE_WIRE)) {
                i32 wirePower = blocks::RedstoneWireBlock::getPower(*neighborState) - 1;
                if (wirePower > maxPower) {
                    maxPower = wirePower;
                }
            }

            // 检查向上/向下连接
            // 如果相邻是实体方块，检查其上方是否有红石线
            if (RedstoneHelper::isNormalCube(*neighborState)) {
                BlockPos upPos = neighborPos.up();
                const BlockState* upState = world.getBlockState(upPos);
                if (upState && upState->is(VanillaBlocks::REDSTONE_WIRE)) {
                    i32 wirePower = blocks::RedstoneWireBlock::getPower(*upState) - 1;
                    if (wirePower > maxPower) {
                        maxPower = wirePower;
                    }
                }
            } else {
                // 相邻不是实体方块，检查其下方是否有红石线
                BlockPos downPos = neighborPos.down();
                const BlockState* downState = world.getBlockState(downPos);
                if (downState && downState->is(VanillaBlocks::REDSTONE_WIRE)) {
                    i32 wirePower = blocks::RedstoneWireBlock::getPower(*downState) - 1;
                    if (wirePower > maxPower) {
                        maxPower = wirePower;
                    }
                }
            }
        }
    }

    return maxPower;
}

i32 RedstonePower::getComparatorInput(IWorld& world, const BlockPos& pos, Direction facing)
{
    // 输入端在比较器的背面（朝向的反方向）
    BlockPos inputPos = pos.offset(Directions::opposite(facing));

    const BlockState* inputState = world.getBlockState(inputPos);
    if (!inputState || inputState->isAir()) {
        return MIN_POWER;
    }

    const Block& inputBlock = inputState->getBlock();

    // 1. 检查是否有比较器输入覆盖（容器等）
    if (inputBlock.hasComparatorInputOverride(*inputState)) {
        return inputBlock.getComparatorInputOverride(*inputState, world, inputPos);
    }

    // 2. 检查红石线信号
    // 如果输入端是红石线，获取其信号强度
    if (inputState->is(VanillaBlocks::REDSTONE_WIRE)) {
        return blocks::RedstoneWireBlock::getPower(*inputState);
    }

    // 3. 获取普通的红石信号
    i32 power = MIN_POWER;

    // 从输入端方向获取信号
    Direction inputDir = facing; // 信号从输入端传来
    i32 weakPower = inputBlock.getWeakPower(*inputState, world, inputPos, inputDir);
    i32 strongPower = inputBlock.getStrongPower(*inputState, world, inputPos, inputDir);

    power = std::max(weakPower, strongPower);

    return power;
}

i32 RedstonePower::getRedstonePowerFromNeighbors(IWorld& world, const BlockPos& pos)
{
    i32 maxPower = MIN_POWER;

    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        Direction oppositeDir = Directions::opposite(dir);

        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();

        // 只检测能输出红石信号的方块
        if (!neighborBlock.canProvidePower(*neighborState)) {
            continue;
        }

        // 获取强信号
        i32 strongPower = neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir);
        if (strongPower > maxPower) {
            maxPower = strongPower;
        }

        // 获取弱信号
        i32 weakPower = neighborBlock.getWeakPower(*neighborState, world, neighborPos, oppositeDir);
        if (weakPower > maxPower) {
            maxPower = weakPower;
        }

        if (maxPower >= MAX_POWER) {
            break;
        }
    }

    return maxPower;
}

// ========== 私有方法 ==========

bool RedstonePower::_canConnectRedstone(const BlockState& state)
{
    // 检查方块是否可以连接红石
    // 可以输出红石信号或可以被红石充能的方块
    return state.getBlock().canProvidePower(state);
}

} // namespace redstone
} // namespace world
} // namespace mc
