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

#include "AnvilBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 铁砧伤害参数（参考 MC 原版 AnvilBlock） ==========

/// 铁砧每格下落伤害系数
static constexpr f32 ANVIL_FALL_DAMAGE_PER_DISTANCE = 2.0f;

/// 铁砧最大伤害值
static constexpr i32 ANVIL_FALL_DAMAGE_MAX = 40;

// ========== 铁砧形状常量（像素坐标，0-16范围） ==========
//
// 铁砧形状由四个长方体组成，与 MC 原版 AnvilBlock.SHAPES 完全一致。
// MC 原版使用 Block.column(xDiameter, zDiameter, minY, maxY) 构造形状：
//   column(xD, zD, yMin, yMax) = box(8-xD/2, yMin, 8-zD/2, 8+xD/2, yMax, 8+zD/2)
//
// Z轴形状（North/South朝向，铁砧长轴沿Z方向）：
//   底座：column(12, 12, 0, 4)    → box(2, 0, 2, 14, 4, 14)     (12×4×12，对称)
//   中段：column(8, 10, 4, 5)     → box(4, 4, 3, 12, 5, 13)     (8×1×10)
//   窄颈：column(4, 8, 5, 10)     → box(6, 5, 4, 10, 10, 12)    (4×5×8)
//   顶面：column(10, 16, 10, 16)  → box(3, 10, 0, 13, 16, 16)   (10×6×16，Z方向满宽)
//
// X轴形状（East/West朝向）：由Z轴形状绕Y轴旋转90度得到，X/Z坐标互换。
//

/// 像素单位常量
static constexpr f32 P = 1.0f / 16.0f;

AnvilBlock::AnvilBlock(const BlockProperties& properties)
    : FallingBlock(properties)
{
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认朝向为北
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));

    // ========== 构建铁砧形状 ==========

    // Z轴形状（North/South朝向）：铁砧长轴沿Z方向
    // 底座：X=2~14, Z=2~14, Y=0~4
    CollisionShape zBase = CollisionShape::box(2 * P, 0 * P, 2 * P, 14 * P, 4 * P, 14 * P);
    // 中段：X=4~12, Z=3~13, Y=4~5
    CollisionShape zMid = CollisionShape::box(4 * P, 4 * P, 3 * P, 12 * P, 5 * P, 13 * P);
    // 窄颈：X=6~10, Z=4~12, Y=5~10
    CollisionShape zNeck = CollisionShape::box(6 * P, 5 * P, 4 * P, 10 * P, 10 * P, 12 * P);
    // 顶面：X=3~13, Z=0~16, Y=10~16（Z方向满宽）
    CollisionShape zTop = CollisionShape::box(3 * P, 10 * P, 0 * P, 13 * P, 16 * P, 16 * P);

    m_shapesByAxis[0] =
        CollisionShape::combine(CollisionShape::combine(CollisionShape::combine(zBase, zMid), zNeck), zTop);

    // X轴形状（East/West朝向）：铁砧长轴沿X方向，由Z轴形状X/Z互换得到
    // 底座：X=2~14, Z=2~14, Y=0~4（对称，与Z轴相同）
    CollisionShape xBase = CollisionShape::box(2 * P, 0 * P, 2 * P, 14 * P, 4 * P, 14 * P);
    // 中段：X=3~13, Z=4~12, Y=4~5
    CollisionShape xMid = CollisionShape::box(3 * P, 4 * P, 4 * P, 13 * P, 5 * P, 12 * P);
    // 窄颈：X=4~12, Z=6~10, Y=5~10
    CollisionShape xNeck = CollisionShape::box(4 * P, 5 * P, 6 * P, 12 * P, 10 * P, 10 * P);
    // 顶面：X=0~16, Z=3~13, Y=10~16（X方向满宽）
    CollisionShape xTop = CollisionShape::box(0 * P, 10 * P, 3 * P, 16 * P, 16 * P, 13 * P);

    m_shapesByAxis[1] =
        CollisionShape::combine(CollisionShape::combine(CollisionShape::combine(xBase, xMid), xNeck), xTop);
}

void AnvilBlock::onStartFalling(IWorld& /*world*/, const BlockPos& /*pos*/, entity::FallingBlockEntity& entity)
{
    // 铁砧下落时设置伤害参数
    entity.setHurtEntities(true);
    entity.setFallDamagePerDistance(ANVIL_FALL_DAMAGE_PER_DISTANCE);
    entity.setFallDamageMax(ANVIL_FALL_DAMAGE_MAX);
}

void AnvilBlock::onEndFalling(IWorld& world,
    const BlockPos& pos,
    const BlockState& /*fallingState*/,
    const BlockState& /*hitState*/,
    entity::FallingBlockEntity& entity)
{
    MC_UNUSED(entity);
    // 播放铁砧落地音效
    world.playEvent(world::WorldEvents::ANVIL_LAND_SOUND, pos, 0);
}

void AnvilBlock::onBroken(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity)
{
    MC_UNUSED(entity);
    // 播放铁砧破碎音效
    world.playEvent(world::WorldEvents::ANVIL_DESTROYED_SOUND, pos, 0);
}

BlockState AnvilBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // MC 原版：朝向 = 玩家水平朝向的顺时针旋转90度
    Direction facing = Directions::rotateY(context.horizontalDirection());
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

const BlockState& AnvilBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = currentFacing;

    switch (rotation) {
        case Rotation::Clockwise90:
            newFacing = Directions::rotateY(currentFacing);
            break;
        case Rotation::Clockwise180:
            newFacing = Directions::opposite(currentFacing);
            break;
        case Rotation::CounterClockwise90:
            newFacing = Directions::rotateYCCW(currentFacing);
            break;
        case Rotation::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const BlockState& AnvilBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction newFacing = currentFacing;

    switch (mirror) {
        case Mirror::LeftRight:
            // 南北镜像：东西互换
            if (currentFacing == Direction::East) {
                newFacing = Direction::West;
            } else if (currentFacing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            // 前后镜像：南北互换
            if (currentFacing == Direction::North) {
                newFacing = Direction::South;
            } else if (currentFacing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(BlockStateProperties::HORIZONTAL_FACING(), newFacing);
}

const CollisionShape& AnvilBlock::getShape(const BlockState& state) const
{
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return m_shapesByAxis[_getAxisIndex(facing)];
}

const CollisionShape& AnvilBlock::getCollisionShape(const BlockState& state) const
{
    // 铁砧的碰撞箱与视觉形状一致
    return getShape(state);
}

BlockActionResult AnvilBlock::onBlockActivated(const BlockState& /*state*/,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand /*hand*/,
    const BlockRaycastResult& /*hit*/)
{
    // 客户端直接返回成功，由服务端处理容器打开
    if (world.asServerWorld() == nullptr) {
        return ActionResultType::Success;
    }

    // 触发与铁砧交互统计
    player.awardCustomStat(ResourceLocation(stats::INTERACT_WITH_ANVIL), 1);

    // 打开铁砧修复容器
    if (world.openContainer(ContainerType::Anvil, pos, player)) {
        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

const BlockState* AnvilBlock::damageAnvil(const BlockState& state)
{
    const Block& block = state.getBlock();
    const ResourceLocation& loc = block.blockLocation();

    // 获取当前铁砧的朝向，用于设置损坏后的状态
    Direction facing = Direction::North;
    if (state.hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    }

    // anvil → chipped_anvil
    if (loc == ResourceLocation("minecraft", "anvil")) {
        const Block* nextBlock = block_registry::BuildingBlocks::CHIPPED_ANVIL;
        if (nextBlock != nullptr) {
            return &nextBlock->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        }
        return nullptr;
    }

    // chipped_anvil → damaged_anvil
    if (loc == ResourceLocation("minecraft", "chipped_anvil")) {
        const Block* nextBlock = block_registry::BuildingBlocks::DAMAGED_ANVIL;
        if (nextBlock != nullptr) {
            return &nextBlock->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        }
        return nullptr;
    }

    // damaged_anvil → 完全损坏（返回 nullptr）
    if (loc == ResourceLocation("minecraft", "damaged_anvil")) {
        return nullptr;
    }

    // 非铁砧方块，不进行损坏
    return nullptr;
}

size_t AnvilBlock::_getAxisIndex(Direction facing)
{
    // North/South → Z轴 (index 0)，East/West → X轴 (index 1)
    switch (facing) {
        case Direction::North:
        case Direction::South:
            return 0;
        case Direction::East:
        case Direction::West:
            return 1;
        default:
            return 0;
    }
}

} // namespace blocks
} // namespace mc
