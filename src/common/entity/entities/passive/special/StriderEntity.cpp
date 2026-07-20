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
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathConstants.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/BlockTags.hpp"
#include "../../../../world/fluid/Fluid.hpp"
#include "../../../../world/fluid/FluidTags.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/special/MoveToLavaGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../utils/ItemDropHelper.hpp"
#include "../../player/Player.hpp"
#include <cmath>
#include <limits>

namespace mc {

namespace {

// 炽足兽常量
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

StriderEntity::StriderEntity(EntityInstanceId id)
    : AnimalEntity(id)
{
    // 设置 AI 导航优先级
    // this.setPathPriority(PathNodeType.WATER, -1.0F);
    // this.setPathPriority(PathNodeType.LAVA, 0.0F);
    // this.setPathPriority(PathNodeType.DANGER_FIRE, 0.0F);
    // this.setPathPriority(PathNodeType.DAMAGE_FIRE, 0.0F);
    registerGoals();
}

std::unique_ptr<Entity> StriderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<StriderEntity>(0);
}

// ========== 熔岩状态 ==========

bool StriderEntity::isInLava() const
{
    // 炽足兽可以站在熔岩表面
    return Entity::isInLava() || m_onLavaSurface;
}

// ========== 音效 ==========

std::optional<ResourceLocation> StriderEntity::getAmbientSound() const
{
    // 恐慌或被诱惑时不播放环境音
    if (isPanicking() || isBeingTempted()) {
        return std::nullopt;
    }
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> StriderEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> StriderEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

void StriderEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 在熔岩上行走播放 ENTITY_STRIDER_STEP_LAVA，否则播放 ENTITY_STRIDER_STEP
    if (isInLava()) {
        playSound(SoundEvents::ENTITY_STRIDER_STEP_LAVA, 1.0f, 1.0f);
    } else {
        playSound(SoundEvents::ENTITY_STRIDER_STEP, 1.0f, 1.0f);
    }
}

bool StriderEntity::isPanicking() const
{
    return m_panicGoal != nullptr && m_panicGoal->isRunning();
}

bool StriderEntity::isBeingTempted() const
{
    return m_temptGoal != nullptr && m_temptGoal->isRunning();
}

// ========== 骑乘系统 (IRideable) ==========

void StriderEntity::setSaddle(bool saddle)
{
    m_saddled = saddle;
}

f32 StriderEntity::getSteeringSpeed() const
{
    f32 baseSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    return baseSpeed * (isCold() ? MOUNTED_SPEED_COLD : MOUNTED_SPEED_NORMAL);
}

bool StriderEntity::boost()
{
    math::Random& rng = getRandom();
    return m_boostHelper.boost(rng);
}

void StriderEntity::travelTowards(const Vector3& travelVec)
{
    AnimalEntity::travel(travelVec);
}

void StriderEntity::travel(const Vector3& travelVec)
{
    // 设置 AI 移动速度（考虑寒冷状态）
    const f32 moveSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED)) *
        (isCold() ? STRIDE_SPEED_COLD : STRIDE_SPEED_NORMAL);
    setAIMoveSpeed(moveSpeed);

    // 调用 IRideable::ride() 处理骑乘移动
    ride(*this, m_boostHelper, travelVec);
}

// ========== 繁殖 ==========

bool StriderEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 炽足兽使用诡异菌繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    // Items::WARPED_FUNGUS 可能在初始化期间为 nullptr
    if (Items::WARPED_FUNGUS == nullptr) {
        return false;
    }
    return item == Items::WARPED_FUNGUS;
}

std::unique_ptr<AnimalEntity> StriderEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 创建小炽足兽
    auto baby = std::make_unique<StriderEntity>(0);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());
    return baby;
}

// ========== 玩家交互 ==========

ActionResultType StriderEntity::interactMob(Player& player, Hand hand)
{
    ItemStack& heldItem = player.getHeldItem(hand);
    bool isFood = isBreedingItem(heldItem);

    // 非食物 + 已装备鞍 + 无乘客 + 玩家未蹲下 → 玩家骑乘
    // 参考: Strider.mobInteract 中的骑乘逻辑
    if (!isFood && hasSaddle() && getPassengers().empty() && !player.isSneaking()) {
        if (m_world != nullptr && !m_world->isClientSide()) {
            player.startRiding(*this);
        }
        return ActionResultType::Success;
    }

    // 喂食逻辑（与 MC Java Animal.mobInteract 一致）
    // 项目中 AnimalEntity 未覆盖 interactMob()，因此在此直接实现
    if (isFood) {
        i32 age = getGrowingAge();

        if (!isChild() && canBreed()) {
            // 成年且可繁殖 → 进入爱心模式
            if (!player.isCreative()) {
                heldItem.shrink(1);
            }
            setInLove(player.playerId());

            // 播放吃食音效
            if (!isSilent() && m_world != nullptr) {
                f32 pitch = 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f;
                playSound(SoundEvents::ENTITY_STRIDER_EAT, 1.0f, pitch);
            }

            return m_world != nullptr && m_world->isClientSide() ? ActionResultType::Consume
                                                                 : ActionResultType::Success;
        }

        if (isChild()) {
            // 幼年 → 加速成长
            if (!player.isCreative()) {
                heldItem.shrink(1);
            }
            // MC: ageUp((int)(-age / 20.0F), true)
            // 加速剩余成长时间的 10%（以秒为单位）
            i32 speedUpSeconds = static_cast<i32>(-age / 20.0f);
            ageUp(speedUpSeconds);

            // 播放吃食音效
            if (!isSilent() && m_world != nullptr) {
                f32 pitch = 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f;
                playSound(SoundEvents::ENTITY_STRIDER_EAT, 1.0f, pitch);
            }

            return ActionResultType::Success;
        }

        // 成年但已处于爱心模式 → 客户端返回 Consume 预测，服务端继续
        if (m_world != nullptr && m_world->isClientSide()) {
            return ActionResultType::Consume;
        }
    }

    // 喂食未触发 → 检查鞍装备
    // 参考: Strider.mobInteract 中 isEquippableInSlot 逻辑
    // 手持鞍时返回 Pass，由 Player::interactOn 调用 SaddleItem::itemInteractionForEntity 处理
    if (canEquip(heldItem, 0)) {
        return ActionResultType::Pass;
    }

    return ActionResultType::Pass;
}

// ========== 生命周期 ==========

void StriderEntity::tick()
{
    // 诱惑状态：每tick有1/140概率播放 happy 音效
    // 恐慌状态：每tick有1/60概率播放 retreat 音效
    // 两者互斥：先检查诱惑，再检查恐慌
    if (isBeingTempted() && getRandom().nextInt(140) == 0) {
        playSound(SoundEvents::ENTITY_STRIDER_HAPPY, 1.0f, getSoundPitch());
    } else if (isPanicking() && getRandom().nextInt(60) == 0) {
        playSound(SoundEvents::ENTITY_STRIDER_RETREAT, 1.0f, getSoundPitch());
    }

    // 更新寒冷状态
    _updateColdStatus();

    // 调用父类 tick
    AnimalEntity::tick();

    // 处理熔岩行走物理
    _updateLavaWalking();

    // 更新加速计时
    m_boostHelper.tick();

    // 更新寒冷计时器
    if (m_coldTimer > 0) {
        m_coldTimer--;
    }
}

void StriderEntity::_updateColdStatus()
{
    // 检查是否处于寒冷状态
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

    // 检查 BlockTags::STRIDER_WARM_BLOCKS
    // 当前方块或下方方块是否为温暖方块（熔岩方块）
    const BlockState* currentState = m_world->getBlockState(currentPos);
    const BlockState* belowState = m_world->getBlockState(belowPos);
    if (currentState != nullptr && BlockTags::STRIDER_WARM_BLOCKS().contains(*currentState)) {
        inLavaOrWarm = true;
    }
    if (belowState != nullptr && BlockTags::STRIDER_WARM_BLOCKS().contains(*belowState)) {
        inLavaOrWarm = true;
    }

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

void StriderEntity::_updateLavaWalking()
{
    // 处理炽足兽在熔岩上的行走物理
    if (!isInLava()) {
        m_onLavaSurface = false;
        return;
    }

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
    // 炽足兽需要玩家手持诡异菌钓竿才能控制
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

    // 检查玩家主手或副手是否持有诡异菌钓竿
    const ItemStack& mainHand = player->getHeldItem(Hand::MainHand);
    const ItemStack& offHand = player->getHeldItem(Hand::OffHand);

    // 检查主手
    if (!mainHand.isEmpty() && mainHand.getItem() == Items::WARPED_FUNGUS_ON_A_STICK) {
        return true;
    }

    // 检查副手
    if (!offHand.isEmpty() && offHand.getItem() == Items::WARPED_FUNGUS_ON_A_STICK) {
        return true;
    }

    return false;
}

// ========== AI 目标注册 ==========

void StriderEntity::registerGoals()
{
    // 调用父类方法
    AnimalEntity::registerGoals();

    // 炽足兽 AI 目标优先级：
    // 优先级 0: 游泳 - 但炽足兽不需要游泳 AI，它们在熔岩上行走
    // m_goalSelector.addGoal(0, new SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    auto* panicGoal = new entity::ai::goal::PanicGoal(this, 1.65);
    m_panicGoal = panicGoal;
    m_goalSelector.addGoal(1, panicGoal);

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（诡异菌、诡异菌钓竿）
    auto* temptGoal = new entity::ai::goal::TemptGoal(
        this,
        1.4,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            if (item == nullptr) return false;
            // 检查是否为诡异菌或诡异菌钓竿
            // 注意：Items 可能在初始化期间为 nullptr
            if (Items::WARPED_FUNGUS != nullptr && item == Items::WARPED_FUNGUS) {
                return true;
            }
            if (Items::WARPED_FUNGUS_ON_A_STICK != nullptr && item == Items::WARPED_FUNGUS_ON_A_STICK) {
                return true;
            }
            return false;
        },
        false);
    m_temptGoal = temptGoal;
    m_goalSelector.addGoal(3, temptGoal);

    // 优先级 4: 寻找熔岩目标
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MoveToLavaGoal>(this, 1.5));

    // 优先级 5: 跟随父母
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 7: 随机漫步
    m_goalSelector.addGoal(7, new entity::ai::goal::RandomWalkingGoal(this, 1.0, 60));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 优先级 9: 看向其他炽足兽
    m_goalSelector.addGoal(9,
        std::make_unique<entity::ai::goal::LookAtGoal>(this,
            8.0f,
            entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE,
            entity::ai::goal::TypeFilter<StriderEntity>{}));
}

// ========== 属性注册 ==========

void StriderEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 设置炽足兽特定属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, STRIDER_SPEED);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, STRIDER_FOLLOW_RANGE);
}

void StriderEntity::die(DamageSource& cause)
{
    // 先调用父类 die()
    AnimalEntity::die(cause);

    // 如果有鞍，掉落鞍物品
    if (hasSaddle() && m_world != nullptr && !m_world->isClientSide()) {
        // 使用 ItemDropHelper 在实体位置生成鞍物品
        ItemStack saddle(Items::SADDLE, 1);
        math::Random& rng = getRandom();
        ItemDropHelper::spawnItemAtEntity(this, saddle, 0.0f, rng);
    }
}

// ========== IRideable 接口额外方法 ==========

bool StriderEntity::canBeRiddenInWater() const
{
    // 炽足兽可以在熔岩中被骑乘，但不能在水中被骑乘
    return false;
}

// ========== IEquipable 接口实现 ==========

ItemStack StriderEntity::getEquipment(i32 slot) const
{
    // 炽足兽只有一个鞍槽
    if (slot != 0) {
        return ItemStack::EMPTY;
    }

    // 炽足兽不存储实际的鞍 ItemStack，只存储布尔值
    // 当有鞍时返回一个鞍物品堆
    if (hasSaddle()) {
        return ItemStack(Items::SADDLE, 1);
    }

    return ItemStack::EMPTY;
}

void StriderEntity::setEquipment(i32 slot, const ItemStack& item)
{
    // 炽足兽只有一个鞍槽
    if (slot != 0) {
        return;
    }

    // 设置鞍状态
    // 注意：炽足兽不存储实际的物品，只存储布尔值
    bool wasSaddled = hasSaddle();
    bool isSaddle = !item.isEmpty() && item.getItem() == Items::SADDLE;
    setSaddle(isSaddle);

    // 装鞍时播放鞍音效
    if (!wasSaddled && isSaddle && !isSilent()) {
        playSound(SoundEvents::ENTITY_STRIDER_SADDLE, 1.0f, 1.0f);
    }
}

bool StriderEntity::canEquip(const ItemStack& item, i32 slot) const
{
    // 槽位边界检查先于物品判断：炽足兽只有槽位 0
    if (slot != 0) {
        return false;
    }

    // 空物品可以清空槽位
    if (item.isEmpty()) {
        return true;
    }

    // 检查是否是鞍物品
    return item.getItem() == Items::SADDLE;
}

// ========== 寻路权重 ==========

f32 StriderEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 炽足兽偏好岩浆位置
    // 对应 MC Strider.getWalkTargetValue:
    //   return block.fluid.is(LAVA) ? 10.0F
    //        : isInLava() ? Float.NEGATIVE_INFINITY
    //        : 0.0F;
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    BlockPos pos(static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(z));
    const fluid::FluidState* fluid = worldPtr->getFluidState(pos);
    if (fluid != nullptr && !fluid->isEmpty() && fluid->getFluid().isIn(fluid::FluidTags::LAVA())) {
        return 10.0f;
    }

    // 不在岩浆中，但自身当前在岩浆中——强烈避免离开岩浆
    if (isInLava()) {
        return -std::numeric_limits<f32>::infinity();
    }

    return 0.0f;
}

} // namespace mc
