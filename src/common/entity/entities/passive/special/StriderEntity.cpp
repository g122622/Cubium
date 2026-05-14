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

#include "StriderEntity.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/fluid/Fluid.hpp"
#include "../../../../world/fluid/FluidTags.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../player/Player.hpp"
#include <cmath>

namespace mc {

namespace {
// MC 1.16.5 StriderEntity 常量
constexpr f32 STRIDER_SPEED = 0.175f;       // 基础移动速度
constexpr f32 STRIDER_FOLLOW_RANGE = 16.0f; // 跟随范围
constexpr f32 MOUNTED_SPEED_NORMAL = 0.55f; // 正常骑乘速度
constexpr f32 MOUNTED_SPEED_COLD = 0.23f;   // 寒冷时骑乘速度
constexpr f32 STRIDE_SPEED_NORMAL = 1.0f;   // 正常行走速度乘数
constexpr f32 STRIDE_SPEED_COLD = 0.66f;    // 寒冷时行走速度乘数
constexpr i32 COLD_TIMER_MAX = 100;         // 寒冷持续时间 (5秒)
constexpr i32 BOOST_DURATION_MIN = 140;     // 最小加速时间
constexpr i32 BOOST_DURATION_MAX = 700;     // 最大加速时间
constexpr f32 LAVA_BUOYANCY = 0.05f;        // 熔岩浮力
} // namespace

StriderEntity::StriderEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // MC 1.16.5: preventEntitySpawning = true
    // 设置 AI 导航优先级
    // this.setPathPriority(PathNodeType.WATER, -1.0F);
    // this.setPathPriority(PathNodeType.LAVA, 0.0F);
    // this.setPathPriority(PathNodeType.DANGER_FIRE, 0.0F);
    // this.setPathPriority(PathNodeType.DAMAGE_FIRE, 0.0F);
    registerGoals();
}

std::unique_ptr<Entity> StriderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<StriderEntity>(LegacyEntityType::Strider, EntityId(0));
}

// ========== 熔岩状态 ==========

bool StriderEntity::isInLava() const
{
    // MC 1.16.5: 重写 isInLava 检查
    // 炽足兽可以站在熔岩表面
    return Entity::isInLava() || m_onLavaSurface;
}

// ========== 骑乘系统 (IRideable) ==========

void StriderEntity::setSaddle(bool saddle)
{
    m_saddled = saddle;
    // MC 1.16.5: 播放鞍音效
    // if (saddle && world != null && !world.isRemote) {
    //     world.playMovingSound(null, this, SoundEvents.ENTITY_STRIDER_SADDLE, SoundCategory.NEUTRAL, 0.5F, 1.0F);
    // }
}

f32 StriderEntity::getSteeringSpeed() const
{
    // MC 1.16.5: StriderEntity.getMountedSpeed()
    // return (float)this.getAttributeValue(Attributes.MOVEMENT_SPEED) * (this.func_234315_eI_() ? 0.23F : 0.55F);
    f32 baseSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    return baseSpeed * (isCold() ? MOUNTED_SPEED_COLD : MOUNTED_SPEED_NORMAL);
}

bool StriderEntity::boost()
{
    // MC 1.16.5: 使用 BoostHelper 进行加速
    math::Random rng = getRandom();
    return m_boostHelper.boost(rng);
}

void StriderEntity::travelTowards(const Vector3& travelVec)
{
    // MC 1.16.5: StriderEntity.travelTowards() -> super.travel(travelVec)
    AnimalEntity::travel(travelVec);
}

void StriderEntity::travel(const Vector3& travelVec)
{
    // MC 1.16.5: StriderEntity.travel()
    // this.setAIMoveSpeed(this.func_234316_eJ_());
    // this.ride(this, this.field_234313_bz_, travelVec);

    // 设置 AI 移动速度（考虑寒冷状态）
    const f32 moveSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED)) *
        (isCold() ? STRIDE_SPEED_COLD : STRIDE_SPEED_NORMAL);
    MC_UNUSED(moveSpeed);
    // setAIMoveSpeed(moveSpeed);  // TODO: 当 MobEntity 实现 setAIMoveSpeed 后添加

    // 调用 IRideable::ride() 处理骑乘移动
    ride(*this, m_boostHelper, travelVec);
}

// ========== 繁殖 ==========

bool StriderEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: 炽足兽使用诡异菌繁殖
    // field_234308_bu_ = Ingredient.fromItems(Items.WARPED_FUNGUS)
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    // TODO: 检查 Items::WARPED_FUNGUS
    // return item == Items::WARPED_FUNGUS;
    MC_UNUSED(item);
    return false; // 暂时返回 false，等待 Items::WARPED_FUNGUS 实现
}

std::unique_ptr<AnimalEntity> StriderEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // MC 1.16.5: 创建小炽足兽
    auto baby = std::make_unique<StriderEntity>(LegacyEntityType::Strider, EntityId(0));
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}

// ========== 生命周期 ==========

void StriderEntity::tick()
{
    // MC 1.16.5 StriderEntity.tick()

    // 检查是否在恐慌或诱惑状态（用于音效）
    // bool isTempted = m_temptGoal != nullptr && m_temptGoal->isRunning();
    // bool isPanicked = m_panicGoal != nullptr && m_panicGoal->isRunning();

    // 播放随机音效
    // if (isTempted && m_random.nextInt(140) == 0) {
    //     playSound(SoundEvents::ENTITY_STRIDER_HAPPY, 1.0f, getSoundPitch());
    // } else if (isPanicked && m_random.nextInt(60) == 0) {
    //     playSound(SoundEvents::ENTITY_STRIDER_RETREAT, 1.0f, getSoundPitch());
    // }

    // 更新寒冷状态
    updateColdStatus();

    // 调用父类 tick
    AnimalEntity::tick();

    // 处理熔岩行走物理
    updateLavaWalking();

    // 执行方块碰撞
    // doBlockCollisions();

    // 更新加速计时（MC 1.16.5: 使用 BoostHelper）
    m_boostHelper.tick();

    // 更新寒冷计时器
    if (m_coldTimer > 0) {
        m_coldTimer--;
    }
}

void StriderEntity::updateColdStatus()
{
    // MC 1.16.5: 检查是否处于寒冷状态
    // func_234319_t_(!flag) 其中 flag = blockstate.isIn(BlockTags.STRIDER_WARM_BLOCKS) ||
    // blockstate1.isIn(BlockTags.STRIDER_WARM_BLOCKS) || this.func_233571_b_(FluidTags.LAVA) > 0.0D
    if (m_world == nullptr) {
        return;
    }

    // 检查当前位置和下方方块
    BlockPos currentPos(static_cast<BlockCoord>(std::floor(m_position.x)),
        static_cast<BlockCoord>(std::floor(m_position.y)),
        static_cast<BlockCoord>(std::floor(m_position.z)));
    BlockPos belowPos = currentPos.down();

    // 检查是否接触熔岩
    bool inLavaOrWarm = false;

    // 检查是否在熔岩中
    f32 lavaHeight = Entity::lavaHeight();
    if (lavaHeight > 0.0f) {
        inLavaOrWarm = true;
    }

    // TODO: 检查 BlockTags::STRIDER_WARM_BLOCKS
    // 当前方块或下方方块是否为温暖方块（熔岩、营火等）
    // const BlockState* currentState = m_world->getBlockState(currentPos);
    // const BlockState* belowState = m_world->getBlockState(belowPos);
    // if (currentState != nullptr && currentState->isIn(BlockTags::STRIDER_WARM_BLOCKS())) {
    //     inLavaOrWarm = true;
    // }
    // if (belowState != nullptr && belowState->isIn(BlockTags::STRIDER_WARM_BLOCKS())) {
    //     inLavaOrWarm = true;
    // }

    // 更新寒冷状态
    if (!inLavaOrWarm) {
        // 不在熔岩中，开始寒冷计时
        if (m_coldTimer < COLD_TIMER_MAX) {
            m_coldTimer = COLD_TIMER_MAX;
        }
    } else {
        // 在熔岩中，重置寒冷计时
        m_coldTimer = 0;
    }
}

void StriderEntity::updateLavaWalking()
{
    // MC 1.16.5: func_234318_eL_()
    // 处理炽足兽在熔岩上的行走物理
    if (!isInLava()) {
        m_onLavaSurface = false;
        return;
    }

    // 检查是否站在熔岩表面
    // ISelectionContext iselectioncontext = ISelectionContext.forEntity(this);
    // if (iselectioncontext.func_216378_a(FlowingFluidBlock.field_235510_c_, this.getPosition(), true)
    //     && !this.world.getFluidState(this.getPosition().up()).isTagged(FluidTags.LAVA)) {
    //     this.onGround = true;
    // } else {
    //     this.setMotion(this.getMotion().scale(0.5D).add(0.0D, 0.05D, 0.0D));
    // }

    if (m_world == nullptr) {
        return;
    }

    // 获取当前位置的流体状态
    BlockPos pos(static_cast<BlockCoord>(std::floor(m_position.x)),
        static_cast<BlockCoord>(std::floor(m_position.y)),
        static_cast<BlockCoord>(std::floor(m_position.z)));

    const fluid::FluidState* fluid = m_world->getFluidState(pos);
    if (fluid != nullptr && !fluid->isEmpty()) {
        // 检查是否是熔岩
        if (fluid->getFluid().isIn(fluid::FluidTags::LAVA())) {
            f32 fluidHeight = fluid->getActualHeight(*m_world, pos);
            f32 fluidSurfaceY = static_cast<f32>(pos.y) + fluidHeight;

            // 检查上方是否也有熔岩
            BlockPos abovePos = pos.up();
            const fluid::FluidState* aboveFluid = m_world->getFluidState(abovePos);
            bool hasLavaAbove = (aboveFluid != nullptr && !aboveFluid->isEmpty() &&
                aboveFluid->getFluid().isIn(fluid::FluidTags::LAVA()));

            // 如果站在熔岩表面且上方没有熔岩，则设置 onGround
            if (m_position.y >= fluidSurfaceY - 0.1f && !hasLavaAbove) {
                m_onGround = true;
                m_onLavaSurface = true;
            } else {
                // 在熔岩中，应用浮力和减速
                Vector3 vel = velocity();
                vel.x *= 0.5;
                vel.y += LAVA_BUOYANCY;
                vel.z *= 0.5;
                setVelocity(vel);
                m_onLavaSurface = false;
            }
        }
    } else {
        m_onLavaSurface = false;
    }
}

bool StriderEntity::canBeSteered() const
{
    // MC 1.16.5: 炽足兽需要玩家手持诡异菌钓竿才能控制
    // Entity entity = this.getControllingPassenger();
    // if (!(entity instanceof PlayerEntity)) {
    //     return false;
    // } else {
    //     PlayerEntity playerentity = (PlayerEntity)entity;
    //     return playerentity.getHeldItemMainhand().getItem() == Items.WARPED_FUNGUS_ON_A_STICK
    //         || playerentity.getHeldItemOffhand().getItem() == Items.WARPED_FUNGUS_ON_A_STICK;
    // }

    if (!hasSaddle()) {
        return false;
    }

    // 获取控制乘客
    const auto& passengers = getPassengers();
    if (passengers.empty()) {
        return false;
    }

    if (m_world == nullptr) {
        return false;
    }

    Entity* passenger = m_world->getEntity(passengers[0]);
    if (passenger == nullptr) {
        return false;
    }

    // 检查是否是玩家
    Player* player = dynamic_cast<Player*>(passenger);
    if (player == nullptr) {
        return false;
    }

    // TODO: 检查玩家手持物品是否为 WARPED_FUNGUS_ON_A_STICK
    // const ItemStack& mainHand = player->getHeldItemMainhand();
    // const ItemStack& offHand = player->getHeldItemOffhand();
    // return mainHand.getItem() == Items::WARPED_FUNGUS_ON_A_STICK
    //     || offHand.getItem() == Items::WARPED_FUNGUS_ON_A_STICK;

    // 暂时返回 true（有鞍时）
    return true;
}

// ========== AI 目标注册 ==========

void StriderEntity::registerGoals()
{
    // 调用父类方法
    AnimalEntity::registerGoals();

    // MC 1.16.5 StriderEntity.registerGoals()
    // 优先级 0: 游泳 - 但炽足兽不需要游泳 AI，它们在熔岩上行走
    // m_goalSelector.addGoal(0, new SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.65));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（诡异菌、诡异菌钓竿）
    // MC 1.16.5: field_234309_bv_ = Ingredient.fromItems(Items.WARPED_FUNGUS, Items.WARPED_FUNGUS_ON_A_STICK)
    m_goalSelector.addGoal(3,
        std::make_unique<::mc::entity::ai::goal::TemptGoal>(
            this,
            1.4,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                if (item == nullptr) return false;
                // TODO: 检查 Items::WARPED_FUNGUS 和 Items::WARPED_FUNGUS_ON_A_STICK
                MC_UNUSED(item);
                return false;
            },
            false));

    // 优先级 4: 寻找熔岩目标
    // MC 1.16.5: new StriderEntity.MoveToLavaGoal(this, 1.5D)
    // TODO: 实现 MoveToLavaGoal
    // m_goalSelector.addGoal(4, new MoveToLavaGoal(this, 1.5));

    // 优先级 5: 跟随父母
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 7: 随机漫步
    // MC 1.16.5: 使用 RandomWalkingGoal，间隔 60 tick
    m_goalSelector.addGoal(7, new entity::ai::goal::RandomWalkingGoal(this, 1.0, 60));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 优先级 9: 看向其他炽足兽
    // MC 1.16.5: new LookAtGoal(this, StriderEntity.class, 8.0F)
    // TODO: 实现 LookAtGoal 对特定实体类型的支持
}

// ========== 属性注册 ==========

void StriderEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MC 1.16.5 StriderEntity.func_234317_eK_()
    // return MobEntity.func_233666_p_()
    //     .createMutableAttribute(Attributes.MOVEMENT_SPEED, 0.175D)
    //     .createMutableAttribute(Attributes.FOLLOW_RANGE, 16.0D);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, STRIDER_SPEED);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, STRIDER_FOLLOW_RANGE);
}

// ========== IRideable 接口额外方法 ==========

bool StriderEntity::canBeRiddenInWater() const
{
    // MC 1.16.5: 炽足兽可以在熔岩中被骑乘
    // 但不能在水中被骑乘
    return false; // 水中不能骑乘
}

} // namespace mc
