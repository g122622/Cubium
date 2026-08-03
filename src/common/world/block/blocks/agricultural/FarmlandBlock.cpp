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

#include "FarmlandBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

FarmlandBlock::FarmlandBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 15.0f / 16.0f, 1.0f))
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::MOISTURE_0_7())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::MOISTURE_0_7(), 0));
}

// ========== 放置和更新 ==========

BlockState FarmlandBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 检查是否可以放置
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 上方不能有固体方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
        // 不能放置，返回泥土状态
        if (VanillaBlocks::DIRT != nullptr) {
            return VanillaBlocks::DIRT->defaultState();
        }
        return defaultState();
    }

    return defaultState();
}

bool FarmlandBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 上方不能有固体方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
        return false;
    }

    return true;
}

BlockState FarmlandBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 上方方块更新时检查有效性
    if (facing == Direction::Up) {
        BlockPos abovePos(currentPos.x, currentPos.y + 1, currentPos.z);
        const BlockState* aboveState = world.getBlockState(abovePos);
        if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
            // 安排下一 tick 转变为泥土
            world.tickManager().scheduleBlockTick(currentPos, *this, 1);
        }
    }

    return state;
}

// ========== Tick ==========

void FarmlandBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState != nullptr && aboveState->hasOpaqueCollisionShape()) {
        turnToDirt(nullptr, world, pos, state);
    }
}

void FarmlandBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    i32 moisture = state.get(BlockStateProperties::MOISTURE_0_7());

    // 检查附近是否有水
    bool nearWater = hasWater(world, pos);
    const bool raining = world.isRaining() && world.canRainAt(pos.up());

    if (!nearWater && !raining) {
        // 没有水且不下雨，湿润度降低
        if (moisture > 0) {
            world.setBlockState(pos, &state.with(BlockStateProperties::MOISTURE_0_7(), moisture - 1), 2);
        } else if (!hasCrops(world, pos)) {
            // 没有作物且干燥，转变为泥土
            turnToDirt(nullptr, world, pos, state);
        }
    } else if (moisture < 7) {
        // 有水或下雨，增加湿润度
        world.setBlockState(pos, &state.with(BlockStateProperties::MOISTURE_0_7(), 7), 2);
    }
}

// ========== 形状 ==========

const CollisionShape& FarmlandBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& FarmlandBlock::getCollisionShape(const BlockState& state) const
{
    // 碰撞箱是完整方块
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== 其他重写 ==========

bool FarmlandBlock::allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 耕地不允许路径寻找（实体不会穿越耕地）
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return false;
}

void FarmlandBlock::onFallenUpon(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{
    // 踩踏耕地的条件（对齐 MC 1.21 FarmBlock.fallOn）：
    // 1. 必须在服务端
    // 2. 随机概率：random.nextFloat() < fallDistance - 0.5f（落下距离越大，踩踏概率越高）
    // 3. 实体必须是 LivingEntity（物品、箭矢等非生物不会踩踏耕地）
    // 4. 实体是玩家，或者 mobGriefing 游戏规则为 true
    // 5. 实体的体积（width * width * height）必须大于 0.512（排除蝙蝠等小型实体）
    if (!world.isClientSide() && world.getRandom().nextFloat() < fallDistance - 0.5f) {
        auto* living = dynamic_cast<LivingEntity*>(&entity);
        if (living != nullptr) {
            auto* player = dynamic_cast<Player*>(living);
            bool canTrample =
                (player != nullptr) || world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
            if (canTrample && entity.width() * entity.width() * entity.height() > 0.512f) {
                turnToDirt(&entity, world, pos, state);
            }
        }
    }

    // 调用父类方法处理实体着地（坠落伤害等）
    Block::onFallenUpon(world, pos, state, entity, fallDistance);
}

// ========== 工具方法 ==========

void FarmlandBlock::turnToDirt(Entity* entity, IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(entity);
    if (VanillaBlocks::DIRT != nullptr) {
        const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
        // 使用 Block::pushEntitiesUp 将嵌入方块的实体向上推出
        // 耕地高度为 15/16 格，泥土为完整方块（1 格），碰撞形状增大时实体需要被推出
        Block::pushEntitiesUp(state, *dirtState, world, pos);
        world.setBlockState(pos, dirtState, 3);
    }
}

bool FarmlandBlock::hasWater(IWorld& world, const BlockPos& pos)
{
    // 检查 4 格范围内的水，高度范围 0-1
    // 使用流体标签检测，而不是材质
    for (i32 dx = -4; dx <= 4; ++dx) {
        for (i32 dz = -4; dz <= 4; ++dz) {
            for (i32 dy = 0; dy <= 1; ++dy) {
                BlockPos checkPos(pos.x + dx, pos.y + dy, pos.z + dz);
                const fluid::FluidState* fluidState = world.getFluidState(checkPos);
                if (fluidState != nullptr && !fluidState->isEmpty()) {
                    // 使用流体标签检测水
                    if (fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool FarmlandBlock::hasCrops(IWorld& world, const BlockPos& pos)
{
    // 检查上方是否有作物
    // 对齐 MC 1.21.11 FarmBlock.shouldMaintainFarmland()：
    // 使用 MAINTAINS_FARMLAND 方块标签检测，而非 IPlantable 动态类型检查
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr) {
        return false;
    }

    return BlockTags::MAINTAINS_FARMLAND().contains(*aboveState);
}

bool FarmlandBlock::canSustainPlant(
    const BlockState& state, IBlockReader& world, const BlockPos& pos, Direction facing, const IPlantable& plant) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 耕地只支撑朝上方向的植物
    if (facing != Direction::Up) {
        return false;
    }

    // 耕地支持 Crop 类型（农作物）和 Plains 类型（花、树苗等可种植在耕地上的植物）
    PlantType plantType = plant.getPlantType(const_cast<IBlockReader&>(world), pos);
    return plantType == PlantType::Crop || plantType == PlantType::Plains;
}

} // namespace blocks
} // namespace mc
