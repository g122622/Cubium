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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "AnvilBlock.hpp"

#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"

namespace mc {
namespace blocks {

// ========== 铁砧伤害参数（参考 MC 原版 AnvilBlock） ==========

/// 铁砧每格下落伤害系数
static constexpr f32 ANVIL_FALL_DAMAGE_PER_DISTANCE = 2.0f;

/// 铁砧最大伤害值
static constexpr i32 ANVIL_FALL_DAMAGE_MAX = 40;

// ========== 铁砧形状常量（像素坐标，0-16范围） ==========
//
// MC 原版铁砧形状由四个柱体组成（围绕Y轴中心对称，所有朝向共用）：
//   底座：宽12像素（偏移2~14），高0~4
//   中段：宽10像素（偏移3~13），高4~5
//   窄颈：宽8像素（偏移4~12），高5~10
//   顶面：宽10像素（偏移3~13），高10~16
//
// 由于 CollisionShape 不支持锥形（每段只能用矩形包围盒），
// 对 MC 原版中的锥形部分取最大包围矩形。

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
    // 底座：Y=0~4，宽12像素，居中偏移2
    CollisionShape base = CollisionShape::box(2 * P, 0 * P, 2 * P, 14 * P, 4 * P, 14 * P);
    // 中段收缩：Y=4~5，宽10像素，居中偏移3
    CollisionShape mid = CollisionShape::box(3 * P, 4 * P, 3 * P, 13 * P, 5 * P, 13 * P);
    // 窄颈：Y=5~10，宽8像素，居中偏移4
    CollisionShape neck = CollisionShape::box(4 * P, 5 * P, 4 * P, 12 * P, 10 * P, 12 * P);
    // 顶面：Y=10~16，宽10像素，居中偏移3
    CollisionShape top = CollisionShape::box(3 * P, 10 * P, 3 * P, 13 * P, 16 * P, 13 * P);

    m_shape = CollisionShape::combine(CollisionShape::combine(CollisionShape::combine(base, mid), neck), top);
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

const CollisionShape& AnvilBlock::getShape(const BlockState& /*state*/) const
{
    // 铁砧形状围绕Y轴中心对称，所有朝向共用同一形状
    return m_shape;
}

const CollisionShape& AnvilBlock::getCollisionShape(const BlockState& state) const
{
    // 铁砧的碰撞箱与视觉形状一致
    return getShape(state);
}

ActionResultType AnvilBlock::onBlockActivated(const BlockState& /*state*/,
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

} // namespace blocks
} // namespace mc
