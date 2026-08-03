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

#include "SweetBerryBushBlock.hpp"
#include "common/core/Constants.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

SweetBerryBushBlock::SweetBerryBushBlock(const BlockProperties& properties)
    : BushBlock(properties)
{

    // 创建状态容器，添加 AGE 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(AGE()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(AGE(), 0));

    // 初始化形状
    initShapes();
}

i32 SweetBerryBushBlock::getAge(const BlockState& state) const noexcept
{
    return static_cast<i32>(state.get(AGE()));
}

const BlockState& SweetBerryBushBlock::withAge(const BlockState& state, i32 age) const
{
    return state.with(AGE(), std::clamp(age, 0, getMaxAge()));
}

bool SweetBerryBushBlock::isMaxAge(const BlockState& state) const noexcept
{
    return getAge(state) >= getMaxAge();
}

BlockState SweetBerryBushBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState();
}

void SweetBerryBushBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    i32 age = getAge(state);
    if (age >= getMaxAge()) {
        return;
    }

    // 光照检查：需要光照 >= CROP_GROWTH_LIGHT_THRESHOLD
    const BlockPos abovePos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(abovePos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(abovePos));
    if (std::max(blockLight, skyLight) < game::CROP_GROWTH_LIGHT_THRESHOLD) {
        return;
    }

    // 1/5 概率生长
    if (random.nextInt(5) == 0) {
        const BlockState& newState = withAge(state, age + 1);
        world.setBlockState(pos, &newState, 2);
    }
}

bool SweetBerryBushBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);

    return !isMaxAge(state);
}

bool SweetBerryBushBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{

    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);

    return true;
}

void SweetBerryBushBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);

    i32 age = getAge(state);
    if (age < getMaxAge()) {
        const BlockState& newState = withAge(state, age + 1);
        world.setBlockState(pos, &newState, 2);
    }
}

const CollisionShape& SweetBerryBushBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_shapesByAge[age];
}

const CollisionShape& SweetBerryBushBlock::getCollisionShape(const BlockState& state) const
{
    i32 age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_collisionShapesByAge[age];
}

void SweetBerryBushBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(pos);

    // 只对 LivingEntity 生效，且狐狸和蜜蜂免疫伤害和减速
    auto* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity == nullptr) {
        return;
    }

    // 狐狸和蜜蜂免疫伤害和减速
    const std::string& typeId = entity.getTypeId();
    if (typeId == "minecraft:fox" || typeId == "minecraft:bee") {
        return;
    }

    // 应用减速效果
    entity.setMotionMultiplier(Vector3(physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ,
        physics::SWEET_BERRY_BUSH_SLOWDOWN_Y,
        physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ));

    i32 age = getAge(state);

    // 伤害逻辑（只在服务端执行，且 AGE > 0 时）
    if (age > 0 && !world.isClientSide()) {
        // 检查实体是否移动
        f32 prevX = entity.prevX();
        f32 prevZ = entity.prevZ();
        f32 currX = entity.x();
        f32 currZ = entity.z();

        if (prevX != currX || prevZ != currZ) {
            // 检查移动距离 >= 0.003
            f32 dx = std::abs(currX - prevX);
            f32 dz = std::abs(currZ - prevZ);

            if (dx >= physics::MOTION_THRESHOLD || dz >= physics::MOTION_THRESHOLD) {
                // 造成伤害
                auto damageSource = DamageSources::sweetBerryBush();
                livingEntity->hurt(damageSource, 1.0f);
            }
        }
    }
}

BlockActionResult SweetBerryBushBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    i32 age = getAge(state);
    bool fullyGrown = (age == 3);

    // AGE > 1 时可以采摘
    if (age > 1) {
        // 计算掉落数量：AGE 2 -> 1-2 个浆果，AGE 3 -> 2-3 个浆果
        math::Random rng;
        i32 berryCount = 1 + rng.nextInt(2) + (fullyGrown ? 1 : 0);

        // 生成浆果掉落
        if (Items::SWEET_BERRIES != nullptr && berryCount > 0) {
            ItemStack dropStack(*Items::SWEET_BERRIES, berryCount);
            ItemDropHelper::spawnItemEntity(&world,
                dropStack,
                static_cast<f64>(pos.x) + 0.5,
                static_cast<f64>(pos.y) + 0.5,
                static_cast<f64>(pos.z) + 0.5,
                rng);
        }

        // AGE 重置为 1
        const BlockState& newState = withAge(state, 1);
        world.setBlockState(pos, &newState, 2);

        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

bool SweetBerryBushBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 甜浆果丛可以种在草地、泥土、砂土、灰化土、耕地上
    return BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(groundState);
}

void SweetBerryBushBlock::initShapes()
{
    constexpr f32 P = 1.0f / 16.0f;

    // AGE 0: 幼苗形状 (3, 0, 3) -> (13, 8, 13)
    m_shapesByAge[0] = CollisionShape::box(3.0f * P, 0.0f, 3.0f * P, 13.0f * P, 8.0f * P, 13.0f * P);

    // AGE 1-3: 完整灌木形状 (1, 0, 1) -> (15, 16, 15)
    CollisionShape fullShape = CollisionShape::box(1.0f * P, 0.0f, 1.0f * P, 15.0f * P, 16.0f * P, 15.0f * P);

    m_shapesByAge[1] = fullShape;
    m_shapesByAge[2] = fullShape;
    m_shapesByAge[3] = fullShape;

    // 碰撞形状
    // AGE 0: 无碰撞
    m_collisionShapesByAge[0] = CollisionShape::empty();

    // AGE 1-3: 有碰撞
    m_collisionShapesByAge[1] = fullShape;
    m_collisionShapesByAge[2] = fullShape;
    m_collisionShapesByAge[3] = fullShape;
}

} // namespace blocks
} // namespace mc
