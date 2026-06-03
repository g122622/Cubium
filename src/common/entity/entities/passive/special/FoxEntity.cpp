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

#include "FoxEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/special/FoxGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../utils/ItemDropHelper.hpp"

namespace mc {

FoxEntity::FoxEntity(EntityId id)
    : AnimalEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> FoxEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FoxEntity>(0);
}

// ========== 信任系统 ==========

bool FoxEntity::trusts(u64 playerId) const
{
    for (u64 trustedId : m_trustedPlayers) {
        if (trustedId == playerId) {
            return true;
        }
    }
    return false;
}

void FoxEntity::addTrustedPlayer(u64 playerId)
{
    if (trusts(playerId)) {
        return;
    }

    if (m_trustedPlayers.size() < MAX_TRUSTED_PLAYERS) {
        m_trustedPlayers.push_back(playerId);
    } else {
        // 替换最早的信任
        m_trustedPlayers.erase(m_trustedPlayers.begin());
        m_trustedPlayers.push_back(playerId);
    }
}

void FoxEntity::removeTrustedPlayer(u64 playerId)
{
    auto it = std::find(m_trustedPlayers.begin(), m_trustedPlayers.end(), playerId);
    if (it != m_trustedPlayers.end()) {
        m_trustedPlayers.erase(it);
    }
}

std::optional<u64> FoxEntity::getFirstTrustedPlayer() const
{
    if (m_trustedPlayers.empty()) {
        return std::nullopt;
    }
    return m_trustedPlayers[0];
}

// ========== 状态标志位 ==========

bool FoxEntity::isSitting() const
{
    return (m_stateFlags & FLAG_SITTING) != 0;
}

void FoxEntity::setSitting(bool sitting)
{
    if (sitting) {
        m_stateFlags |= FLAG_SITTING;
    } else {
        m_stateFlags &= ~FLAG_SITTING;
    }
}

bool FoxEntity::isCrouching() const
{
    return (m_stateFlags & FLAG_CROUCHING) != 0;
}

void FoxEntity::setCrouching(bool crouching)
{
    if (crouching) {
        m_stateFlags |= FLAG_CROUCHING;
    } else {
        m_stateFlags &= ~FLAG_CROUCHING;
        m_crouchAmount = 0.0f;
    }
}

bool FoxEntity::isInterested() const
{
    return (m_stateFlags & FLAG_INTERESTED) != 0;
}

void FoxEntity::setInterested(bool interested)
{
    if (interested) {
        m_stateFlags |= FLAG_INTERESTED;
    } else {
        m_stateFlags &= ~FLAG_INTERESTED;
    }
}

bool FoxEntity::isPounceReady() const
{
    return (m_stateFlags & FLAG_POUNCE_READY) != 0;
}

void FoxEntity::setPounceReady(bool ready)
{
    if (ready) {
        m_stateFlags |= FLAG_POUNCE_READY;
    } else {
        m_stateFlags &= ~FLAG_POUNCE_READY;
    }
}

void FoxEntity::setSleeping(bool sleeping)
{
    if (sleeping && !isSleeping()) {
        m_sleepTimer = 100 + (getRandom().nextInt(100)); // 5-10秒
    }
    if (sleeping) {
        m_stateFlags |= FLAG_SLEEPING;
    } else {
        m_stateFlags &= ~FLAG_SLEEPING;
    }
}

bool FoxEntity::isStuck() const
{
    return (m_stateFlags & FLAG_STUCK) != 0;
}

void FoxEntity::setStuck(bool stuck)
{
    if (stuck) {
        m_stateFlags |= FLAG_STUCK;
    } else {
        m_stateFlags &= ~FLAG_STUCK;
    }
}

bool FoxEntity::isFoxAggroed() const
{
    return (m_stateFlags & FLAG_FOX_AGGROED) != 0;
}

void FoxEntity::setFoxAggroed(bool aggroed)
{
    if (aggroed) {
        m_stateFlags |= FLAG_FOX_AGGROED;
    } else {
        m_stateFlags &= ~FLAG_FOX_AGGROED;
    }
}

// ========== 叼物品 ==========

bool FoxEntity::isHoldingItem() const
{
    return m_heldItem != nullptr && !m_heldItem->isEmpty();
}

void FoxEntity::setHeldItem(std::unique_ptr<ItemStack> item)
{
    m_heldItem = std::move(item);
}

ItemStack FoxEntity::getHeldItem(Hand hand) const
{
    if (hand == Hand::MainHand) {
        if (m_heldItem != nullptr) {
            return *m_heldItem;
        }
    }
    return ItemStack();
}

void FoxEntity::setHeldItem(Hand hand, ItemStack stack)
{
    if (hand == Hand::MainHand) {
        m_heldItem = std::make_unique<ItemStack>(std::move(stack));
    }
}

void FoxEntity::dropHeldItem()
{
    // 在世界生成掉落物
    if (m_heldItem == nullptr || m_heldItem->isEmpty()) {
        return;
    }

    IWorld* worldPtr = this->world();
    if (worldPtr == nullptr) {
        return;
    }

    // 使用 ItemDropHelper 在实体位置生成物品实体
    math::Random rng = getRandom();
    ItemDropHelper::spawnItemAtEntity(this, *m_heldItem, 0.0f, rng, 10);

    // 清空物品引用
    m_heldItem.reset();
}

// ========== 行为辅助方法 ==========

bool FoxEntity::canAct() const
{
    // 可以行动的条件：非坐下、非蹲伏、非睡眠、非卡住、非激怒
    return !isSitting() && !isCrouching() && !isSleeping() && !isStuck() && !isFoxAggroed();
}

void FoxEntity::resetAllStates()
{
    // 重置所有状态：坐下、蹲伏、睡眠等
    setSitting(false);
    setCrouching(false);
    setInterested(false);
    setPounceReady(false);
    setStuck(false);
    m_crouchAmount = 0.0f;
    m_prevCrouchAmount = 0.0f;
}

void FoxEntity::wakeUp()
{
    // 唤醒（停止睡眠、坐下等）
    setSleeping(false);
    setSitting(false);
}

// ========== 繁殖 ==========

bool FoxEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 只有甜浆果可以用来繁殖狐狸
    // 注意：发光浆果是后续版本添加的，本项目只支持甜浆果
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::SWEET_BERRIES;
}

std::unique_ptr<AnimalEntity> FoxEntity::spawnBaby(AnimalEntity& partner)
{
    auto baby = std::make_unique<FoxEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 遗传皮肤类型：50% 概率从任一父母继承皮肤类型
    FoxEntity* partnerFox = dynamic_cast<FoxEntity*>(&partner);
    math::Random rng = getRandom();
    if (rng.nextBoolean()) {
        baby->setFoxType(m_foxType);
    } else if (partnerFox != nullptr) {
        baby->setFoxType(partnerFox->getFoxType());
    } else {
        // 如果配偶不是狐狸（不应该发生），使用自己的类型
        baby->setFoxType(m_foxType);
    }

    // 幼狐继承父母的信任玩家
    for (u64 playerId : m_trustedPlayers) {
        baby->addTrustedPlayer(playerId);
    }
    if (partnerFox != nullptr) {
        for (u64 playerId : partnerFox->getTrustedPlayers()) {
            baby->addTrustedPlayer(playerId);
        }
    }

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

// ========== 状态更新 ==========

void FoxEntity::_updateCrouchAmount()
{
    // 蹲伏量更新逻辑
    m_prevCrouchAmount = m_crouchAmount;

    if (isCrouching()) {
        // 蹲伏时逐渐增加到 3.0
        m_crouchAmount = std::min(3.0f, m_crouchAmount + 0.15f);
    } else {
        // 不蹲伏时逐渐减少到 0
        m_crouchAmount = std::max(0.0f, m_crouchAmount - 0.15f);
    }
}

// ========== 刻更新 ==========

void FoxEntity::tick()
{
    AnimalEntity::tick();

    // 更新蹲伏量
    _updateCrouchAmount();

    // 睡眠计时器
    if (isSleeping()) {
        m_sleepTimer--;
        if (m_sleepTimer <= 0) {
            setSleeping(false);
        }
    }

    // 如果卡在雪中，减少卡住计时器
    // TODO: 实现卡住状态检测和脱离雪块的逻辑
}

// ========== AI 目标注册 ==========

void FoxEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 狐狸特有目标注册顺序
    // 注意：优先级数值越小，优先级越高

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // TODO: 优先级 1: 卡住时跳跃目标（用于从雪中逃脱）

    // 优先级 2: 恐慌逃跑
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::PanicGoal>(this, 2.2));

    // 优先级 3: 繁殖
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 4: 躲避玩家（未信任的玩家）
    m_goalSelector.addGoal(4,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(this,
            16.0f, // 检测距离：16格
            1.6,   // 近距离逃跑速度（更快）
            1.4,   // 远距离逃跑速度
            [this](const LivingEntity* entity) -> bool {
                if (entity == nullptr) return false;
                // 只躲避玩家
                const Player* player = dynamic_cast<const Player*>(entity);
                if (player == nullptr) return false;
                // 不躲避旁观者或创造模式玩家
                if (player->isSpectator() || player->isCreative()) return false;
                // 不躲避信任的玩家
                if (trusts(player->id())) return false;
                // 不躲避当狐狸处于攻击状态时
                return !isFoxAggroed();
            }));

    // 优先级 4: 躲避狼和北极熊
    m_goalSelector.addGoal(4,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(this,
            8.0f, // 检测距离
            1.6,  // 近距离逃跑速度
            1.4,  // 远距离逃跑速度
            [](const LivingEntity* entity) -> bool {
                if (entity == nullptr) return false;
                auto type = entity->typeId();
                return type == entity::EntityTypeIdNumber::WOLF || type == entity::EntityTypeIdNumber::POLAR_BEAR;
            }));

    // 优先级 5: 跟踪猎物（扑击的前置阶段）
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::FoxFollowTargetGoal>(this));

    // 优先级 6: 扑击攻击
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::FoxPounceGoal>(this));

    // 优先级 6: 寻找庇护所（白天躲避阳光）
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::FoxFindShelterGoal>(this, 1.25));

    // 优先级 7: 咬击攻击（近战攻击）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::FoxBiteGoal>(this, 1.2, true));

    // 优先级 7: 睡眠
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::FoxSleepGoal>(this));

    // 优先级 8: 跟随父母（幼体）
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.25));

    // 优先级 9: 村庄漫步
    // 简化实现：使用随机行走
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 1.0, 32));

    // 优先级 10: 吃浆果
    m_goalSelector.addGoal(10, std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(this, 1.2, 12, 2));

    // 优先级 10: 跳跃攻击（备用）
    m_goalSelector.addGoal(10, std::make_unique<entity::ai::goal::LeapAtTargetGoal>(this, 0.4f));

    // 优先级 11: 避水随机行走
    m_goalSelector.addGoal(11, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 11: 寻找物品
    m_goalSelector.addGoal(11, std::make_unique<entity::ai::goal::FoxFindItemsGoal>(this));

    // 优先级 3: 食物诱惑（甜浆果）
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.0, // 跟随速度
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item == Items::SWEET_BERRIES;
            },
            false // 不被玩家移动吓跑
            ));

    // 优先级 12: 看向玩家
    m_goalSelector.addGoal(12,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 24.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 13: 坐下观察
    m_goalSelector.addGoal(13, std::make_unique<entity::ai::goal::FoxSitAndLookGoal>(this));

    // 目标选择器
    // TODO: 实现复仇目标 - 当信任玩家被攻击时触发
    // m_targetSelector.addGoal(3, std::make_unique<entity::ai::goal::FoxRevengeGoal>(this));
}

// ========== 属性注册 ==========

void FoxEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 狐狸的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

// ========== 音效 ==========

std::optional<ResourceLocation> FoxEntity::getAmbientSound() const
{
    // 白狐使用 screech 音效
    if (m_foxType == FoxType::Snow) {
        return SoundEvents::ENTITY_FOX_SCREECH;
    }
    return SoundEvents::ENTITY_FOX_AMBIENT;
}

std::optional<ResourceLocation> FoxEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_FOX_HURT;
}

std::optional<ResourceLocation> FoxEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_FOX_DEATH;
}

void FoxEntity::playSleepSound()
{
    playSound(SoundEvents::ENTITY_FOX_SLEEP, 1.0f, 1.0f);
}

void FoxEntity::playSniffSound()
{
    playSound(SoundEvents::ENTITY_FOX_SNIFF, 1.0f, 1.0f);
}

void FoxEntity::playBiteSound()
{
    playSound(SoundEvents::ENTITY_FOX_BITE, 1.0f, 1.0f);
}

void FoxEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_FOX_EAT, 1.0f, 1.0f);
}

void FoxEntity::playSpitSound()
{
    playSound(SoundEvents::ENTITY_FOX_SPIT, 1.0f, 1.0f);
}

void FoxEntity::playScreechSound()
{
    playSound(SoundEvents::ENTITY_FOX_SCREECH, 1.0f, 1.0f);
}

} // namespace mc
