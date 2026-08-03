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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ChorusFlowerBlock.hpp"
#include "ChorusPlantBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

ChorusFlowerBlock::ChorusFlowerBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 初始化状态容器，添加AGE_0_5属性（生长阶段0-5）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_5())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：年龄为0
    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_5(), 0));

    // 初始化各年龄阶段的碰撞形状（目前都是完整方块）
    for (i32 i = 0; i < 6; ++i) {
        m_shapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
}

i32 ChorusFlowerBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_5());
}

BlockState ChorusFlowerBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_5(), std::min(age, 5));
}

BlockState ChorusFlowerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState();
}

bool ChorusFlowerBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 检查下方方块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    // 如果下方是空气，检查是否有紫颂植物支撑
    if (belowState == nullptr || belowState->isAir()) {
        bool foundChorusPlant = false;

        // 检查四个水平方向是否有紫颂植物
        for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
            BlockPos adjPos = pos.offset(dir);
            const BlockState* adjState = world.getBlockState(adjPos);

            if (adjState == nullptr) {
                continue;
            }

            if (adjState->is(VanillaBlocks::CHORUS_PLANT)) {
                // 只能有一个紫颂植物支撑
                if (foundChorusPlant) {
                    return false;
                }
                foundChorusPlant = true;
            } else if (!adjState->isAir()) {
                // 其他非空气方块阻挡
                return false;
            }
        }

        return foundChorusPlant;
    }

    const Block& belowBlock = belowState->getBlock();

    // 下方是紫颂植物，可以放置
    if (belowState->is(VanillaBlocks::CHORUS_PLANT)) {
        return true;
    }

    // 下方是末地石，可以放置
    if (belowState->is(VanillaBlocks::END_STONE)) {
        return true;
    }

    // 下方是紫颂花，可以放置
    if (&belowBlock == VanillaBlocks::CHORUS_FLOWER) {
        return true;
    }

    return false;
}

void ChorusFlowerBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    i32 age = getAge(state);

    // 如果还未达到最大年龄，有概率生长
    if (age < getMaxAge()) {
        if (random.nextInt(5) == 0) {
            BlockState newState = withAge(age + 1);
            world.setBlockState(pos, &newState, 2);
        }
    }
}

const CollisionShape& ChorusFlowerBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    return m_shapesByAge[std::min(age, 5)];
}

// ========== 世界生成 ==========

void ChorusFlowerBlock::generatePlant(
    WorldGenRegion& world, const BlockPos& pos, math::Random& random, i32 maxHorizontalDistance)
{
    // 在起始位置放置一个带连接的紫颂植物茎干，然后递归生长
    const BlockState* chorusPlantState = &VanillaBlocks::CHORUS_PLANT->defaultState();
    if (chorusPlantState == nullptr) {
        return;
    }

    BlockState connectedState = ChorusPlantBlock::getStateWithConnections(world, pos, *chorusPlantState);
    world.setBlockState(pos, &connectedState, 2);

    growTreeRecursive(world, pos, random, pos, maxHorizontalDistance, 0);
}

void ChorusFlowerBlock::growTreeRecursive(WorldGenRegion& world,
    const BlockPos& pos,
    math::Random& random,
    const BlockPos& origin,
    i32 maxHorizontalDistance,
    i32 depth)
{
    // 确定向上生长的茎干高度
    const BlockState* chorusPlantState = &VanillaBlocks::CHORUS_PLANT->defaultState();
    const BlockState* chorusFlowerState = &VanillaBlocks::CHORUS_FLOWER->defaultState();
    if (chorusPlantState == nullptr || chorusFlowerState == nullptr) {
        return;
    }

    // 确定向上生长的茎干高度
    i32 stemHeight = random.nextInt(4) + 1;
    if (depth == 0) {
        stemHeight++; // 根部茎干多一格
    }

    // 向上放置茎干
    for (i32 j = 0; j < stemHeight; ++j) {
        BlockPos abovePos(pos.x, pos.y + j + 1, pos.z);
        if (!allNeighborsEmpty(world, abovePos, std::nullopt)) {
            return;
        }
        BlockState aboveState = ChorusPlantBlock::getStateWithConnections(world, abovePos, *chorusPlantState);
        world.setBlockState(abovePos, &aboveState, 2);
        BlockPos belowPos(pos.x, pos.y + j, pos.z);
        BlockState belowState = ChorusPlantBlock::getStateWithConnections(world, belowPos, *chorusPlantState);
        world.setBlockState(belowPos, &belowState, 2);
    }

    // 尝试水平分枝
    bool branched = false;
    if (depth < 4) {
        i32 branchCount = random.nextInt(4);
        if (depth == 0) {
            branchCount++; // 根部多一个分枝机会
        }

        for (i32 k = 0; k < branchCount; ++k) {
            Direction direction = Directions::horizontal()[random.nextInt(0, 4)];
            BlockPos branchPos(
                pos.x + Directions::xOffset(direction), pos.y + stemHeight, pos.z + Directions::zOffset(direction));

            // 检查是否在最大水平距离范围内
            if (std::abs(branchPos.x - origin.x) >= maxHorizontalDistance ||
                std::abs(branchPos.z - origin.z) >= maxHorizontalDistance) {
                continue;
            }

            // 检查分枝位置和其下方是否都为空气
            const BlockState* branchState = world.getBlockState(branchPos);
            const BlockState* belowBranchState = world.getBlockState(branchPos.x, branchPos.y - 1, branchPos.z);
            if ((branchState == nullptr || branchState->isAir()) &&
                (belowBranchState == nullptr || belowBranchState->isAir()) &&
                allNeighborsEmpty(world, branchPos, Directions::opposite(direction))) {
                branched = true;
                BlockState branchBlockState =
                    ChorusPlantBlock::getStateWithConnections(world, branchPos, *chorusPlantState);
                world.setBlockState(branchPos, &branchBlockState, 2);
                Direction oppositeDir = Directions::opposite(direction);
                BlockPos oppositePos(branchPos.x + Directions::xOffset(oppositeDir),
                    branchPos.y,
                    branchPos.z + Directions::zOffset(oppositeDir));
                BlockState oppositeBlockState =
                    ChorusPlantBlock::getStateWithConnections(world, oppositePos, *chorusPlantState);
                world.setBlockState(oppositePos, &oppositeBlockState, 2);
                growTreeRecursive(world, branchPos, random, origin, maxHorizontalDistance, depth + 1);
            }
        }
    }

    // 如果没有分枝，在顶部放置死亡的花（age=5）
    if (!branched) {
        BlockPos topPos(pos.x, pos.y + stemHeight, pos.z);
        const ChorusFlowerBlock& flowerBlock = static_cast<const ChorusFlowerBlock&>(chorusFlowerState->getBlock());
        BlockState deadFlowerState = flowerBlock.withAge(5);
        world.setBlockState(topPos, &deadFlowerState, 2);
    }
}

bool ChorusFlowerBlock::allNeighborsEmpty(
    WorldGenRegion& world, const BlockPos& pos, const std::optional<Direction>& excludeDir)
{
    // 检查四个水平方向的邻居是否为空气（排除指定方向）
    static const Direction horizontalDirs[] = {Direction::North, Direction::South, Direction::East, Direction::West};

    for (Direction dir : horizontalDirs) {
        if (excludeDir.has_value() && dir == excludeDir.value()) {
            continue;
        }
        BlockPos adjPos = pos.offset(dir);
        const BlockState* adjState = world.getBlockState(adjPos);
        if (adjState != nullptr && !adjState->isAir()) {
            return false;
        }
    }
    return true;
}

} // namespace blocks
} // namespace mc
