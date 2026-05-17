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
#include "../../../../entity/core/Entity.hpp"
#include "../../../../entity/core/EntityType.hpp"
#include "../../../../entity/core/LivingEntity.hpp"
#include "../../../../entity/damage/DamageSource.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../physics/PhysicsConstants.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockTags.hpp"
#include <cmath>

namespace mc {
namespace blocks {

SweetBerryBushBlock::SweetBerryBushBlock(const BlockProperties& properties)
    : BushBlock(properties)
{

    // 创建状态容器，添加 AGE 属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(AGE()).create(
        [](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(AGE(), 0));

    // 初始化形状
    initShapes();
}

int SweetBerryBushBlock::getAge(const BlockState& state) const
{
    return static_cast<int>(state.get(AGE()));
}

const BlockState& SweetBerryBushBlock::withAge(const BlockState& state, int age) const
{
    return state.with(AGE(), std::clamp(age, 0, getMaxAge()));
}

bool SweetBerryBushBlock::isMaxAge(const BlockState& state) const
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
    int age = getAge(state);
    if (age >= getMaxAge()) {
        return;
    }

    // 光照检查：需要光照 >= 9
    const BlockPos abovePos = pos.up();
    const i32 blockLight = static_cast<i32>(world.getBlockLight(abovePos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(abovePos));
    if (std::max(blockLight, skyLight) < 9) {
        return;
    }

    // 1/5 概率生长
    // 参考: net.minecraft.block.SweetBerryBushBlock#randomTick
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

    int age = getAge(state);
    if (age < getMaxAge()) {
        const BlockState& newState = withAge(state, age + 1);
        world.setBlockState(pos, &newState, 2);
    }
}

const CollisionShape& SweetBerryBushBlock::getShape(const BlockState& state) const
{
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_shapesByAge[age];
}

const CollisionShape& SweetBerryBushBlock::getCollisionShape(const BlockState& state) const
{
    int age = getAge(state);
    MC_ASSERT(age >= 0 && age <= 3);
    return m_collisionShapesByAge[age];
}

void SweetBerryBushBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(pos);

    // 参考: net.minecraft.block.SweetBerryBushBlock#onEntityCollision
    // 只对 LivingEntity 生效，且狐狸和蜜蜂免疫

    // 检查是否为 LivingEntity
    auto* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity == nullptr) {
        return;
    }

    // 检查实体类型（狐狸和蜜蜂免疫伤害和减速）
    // MC 1.16.5: if (entityIn instanceof LivingEntity && entityIn.getType() != EntityType.FOX && entityIn.getType() !=
    // EntityType.BEE)
    const std::string& typeId = entity.getTypeId();
    if (typeId == "minecraft:fox" || typeId == "minecraft:bee") {
        return;
    }

    // 应用减速效果
    // MC 1.16.5: entityIn.setMotionMultiplier(state, new Vector3d(0.8D, 0.75D, 0.8D));
    entity.setMotionMultiplier(Vector3(physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ,
        physics::SWEET_BERRY_BUSH_SLOWDOWN_Y,
        physics::SWEET_BERRY_BUSH_SLOWDOWN_XZ));

    int age = getAge(state);

    // 伤害逻辑（只在服务端执行，且 AGE > 0 时）
    // 参考 MC 1.16.5: if (!worldIn.isRemote && state.get(AGE) > 0 && ...)
    if (age > 0 && !world.isClientSide()) {
        // 检查实体是否移动
        // MC 1.16.5: (entityIn.lastTickPosX != entityIn.getPosX() || entityIn.lastTickPosZ != entityIn.getPosZ())
        f32 prevX = entity.prevX();
        f32 prevZ = entity.prevZ();
        f32 currX = entity.x();
        f32 currZ = entity.z();

        if (prevX != currX || prevZ != currZ) {
            // 检查移动距离 >= 0.003
            // MC 1.16.5: double d0 = Math.abs(entityIn.getPosX() - entityIn.lastTickPosX);
            //           double d1 = Math.abs(entityIn.getPosZ() - entityIn.lastTickPosZ);
            //           if (d0 >= (double)0.003F || d1 >= (double)0.003F)
            f32 dx = std::abs(currX - prevX);
            f32 dz = std::abs(currZ - prevZ);

            if (dx >= physics::MOTION_THRESHOLD || dz >= physics::MOTION_THRESHOLD) {
                // 造成伤害
                // MC 1.16.5: entityIn.attackEntityFrom(DamageSource.SWEET_BERRY_BUSH, 1.0F);
                auto damageSource = DamageSources::sweetBerryBush();
                livingEntity->hurt(damageSource, 1.0f);
            }
        }
    }
}

ActionResultType SweetBerryBushBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    int age = getAge(state);
    bool fullyGrown = (age == 3);

    // AGE > 1 时可以采摘
    if (age > 1) {
        // 计算掉落数量
        // AGE 2: 1-2 个浆果
        // AGE 3: 2-3 个浆果
        // 参考 MC 1.16.5: int j = 1 + worldIn.rand.nextInt(2);
        // spawnAsEntity(..., new ItemStack(Items.SWEET_BERRIES, j + (flag ? 1 : 0)));
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

    // 参考 MC 1.16.5 SweetBerryBushBlock#isValidGround
    // 甜浆果丛可以种在草地、泥土、砂土、灰化土、耕地上
    // return state.isIn(BlockTags.VALID_SWEET_BERRY_BUSH_GROUND);
    return BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(groundState);
}

void SweetBerryBushBlock::initShapes()
{
    // 参考 MC 1.16.5 SweetBerryBushBlock 的形状定义
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
