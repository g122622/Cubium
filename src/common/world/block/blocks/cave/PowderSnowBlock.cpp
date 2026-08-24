/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, WHETHER
 * ARISING FROM, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PowderSnowBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <algorithm>

namespace mc {
namespace blocks {

// 对齐 vanilla PowderSnowBlock.NUM_BLOCKS_TO_FALL_INTO_BLOCK = 2.5F（PowderSnowBlock.java:42）。
// 实体下落距离超过此值时，细雪提供半穿透碰撞箱（FALLING_COLLISION_SHAPE）减缓下落，而非完全阻挡或穿透。
constexpr f32 NUM_BLOCKS_TO_FALL_INTO_BLOCK = 2.5f;

// 对齐 vanilla PowderSnowBlock.FALLING_COLLISION_SHAPE = Shapes.box(0,0,0,1,0.9,1)
// （PowderSnowBlock.java:43）。下落实体半穿透碰撞箱，高度 0.9，使实体缓慢陷入细雪而非瞬移到顶。
CollisionShape g_powderSnowFallingShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.9f, 1.0f);

PowderSnowBlock::PowderSnowBlock(const BlockProperties& properties)
    : Block(properties)
{}

const CollisionShape& PowderSnowBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 无实体上下文时（AI 寻路、方块放置预检、渲染等），细雪默认无碰撞——实体可穿过。
    // 带实体上下文的判定（可行走实体得完整碰撞箱、下落实体得半穿透形状）见 getCollisionShapeForEntity。
    // 对齐 vanilla getCollisionShape 在 isPlacement()（无实体上下文）时返回 Shapes.empty()。
    return VoxelShapes::empty();
}

const CollisionShape& PowderSnowBlock::getCollisionShapeForEntity(
    const BlockState& state, const EntityCollisionContext& ctx, i32 blockY) const
{
    MC_UNUSED(state);
    // 对齐 vanilla PowderSnowBlock.getCollisionShape（PowderSnowBlock.java:116-132）。
    // 无实体上下文（如纯方块放置预检）时返回空形状（下沉）。
    if (ctx.entity == nullptr) {
        return VoxelShapes::empty();
    }

    const Entity& entity = *ctx.entity;

    // 分支 1：实体下落距离 > 2.5 → 半穿透碰撞箱（减缓下落，对齐 :120-122）。
    if (entity.fallDistance() > NUM_BLOCKS_TO_FALL_INTO_BLOCK) {
        return g_powderSnowFallingShape;
    }

    // 分支 2：可行走实体且在方块上方且非潜行下降中 → 完整方块碰撞箱（可行走不下沉，对齐 :124-127）。
    // canEntityWalkOnPowderSnow 内查 POWDER_SNOW_WALKABLE_MOBS 标签（rabbit/endermite/silverfish/fox）
    // 或 LivingEntity 穿皮革靴子。ctx.isAbove(blockY) 判定实体脚部在方块顶面（blockY+1）或更高，
    // 对齐 vanilla CollisionContext.isAbove(Shapes.block(), blockPos, false)；!descending 排除
    // 潜行下降（descending=Entity::isSneaking()，穿皮革靴玩家潜行时主动陷入细雪，见 wiki 第71行）。
    if (canEntityWalkOnPowderSnow(entity) && ctx.isAbove(blockY) && !ctx.descending) {
        return VoxelShapes::fullCube();
    }

    // 分支 3：否则 → 空形状（实体下沉陷入细雪，对齐 :131）。
    return VoxelShapes::empty();
}

bool PowderSnowBlock::canEntityWalkOnPowderSnow(const Entity& entity)
{
    // 对齐 vanilla PowderSnowBlock.canEntityWalkOnPowderSnow（PowderSnowBlock.java:139-145）。
    // 分支 1：实体类型属于 POWDER_SNOW_WALKABLE_MOBS 标签（rabbit/endermite/silverfish/fox）。
    // 安全检查：标签系统未初始化时跳过标签查询（与 Entity::canFreeze 范式一致）。
    if (EntityTypeTags::isInitialized() && EntityTypeTags::POWDER_SNOW_WALKABLE_MOBS().contains(entity.getTypeId())) {
        return true;
    }

    // 分支 2：LivingEntity 穿皮革靴子可行走。
    if (const auto* living = dynamic_cast<const LivingEntity*>(&entity)) {
        const ItemStack& boots = living->getEquipment(EquipmentSlot::Feet);
        return !boots.isEmpty() && boots.getItem() == Items::LEATHER_BOOTS;
    }

    return false;
}

fluid::Fluid* PowderSnowBlock::pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 细雪不是流体，使用 pickupItem 代替
    return nullptr;
}

const Item* PowderSnowBlock::pickupItem(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 将细雪方块替换为空气
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    if (airState != nullptr) {
        world.setBlockState(pos, airState, 3);
    }

    // 返回细雪桶物品
    return Items::POWDER_SNOW_BUCKET;
}

const ResourceLocation* PowderSnowBlock::getPickupSound(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    return &SoundEvents::ITEM_BUCKET_FILL_POWDER_SNOW;
}

void PowderSnowBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 对应 MC Java 的 InsideBlockEffectType.FREEZE 效果

    // 1. 设置实体处于细雪中的标志
    // MC Java: entity.setIsInPowderSnow(true)
    entity.setIsInPowderSnow(true);

    // 2. 如果实体可以冰冻，增加冰冻计时器
    // MC Java: if (entity.canFreeze()) { entity.setTicksFrozen(Math.min(entity.getTicksRequiredToFreeze(),
    // entity.getTicksFrozen() + 1)); }
    if (entity.canFreeze()) {
        const i32 current = entity.getTicksFrozen();
        const i32 max = entity.getTicksRequiredToFreeze();
        entity.setTicksFrozen(std::min(max, current + 1));
    }

    // 3. 设置运动减速乘数，使实体在细雪中缓慢移动
    // 细雪的减速效果：XZ 轴 0.9，Y 轴 0.9
    // 参考 MC Java 的 PowderSnowBlock.canEntityWalkOnTop() 中的运动乘数
    entity.setMotionMultiplier(Vector3(0.9, 0.9, 0.9));
}

} // namespace blocks
} // namespace mc
