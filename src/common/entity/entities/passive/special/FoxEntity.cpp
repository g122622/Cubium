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
#include "../../../../item/core/Item.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/food/FoodItem.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/WorldEvents.hpp"
#include "../../../../world/block/BlockState.hpp"
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
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../entities/item/ItemEntity.hpp"
#include "../../../entities/passive/special/TurtleEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../registry/VanillaEntityTypeKeys.hpp"
#include "../../../utils/ItemDropHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace mc {

FoxEntity::FoxEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AnimalEntity(id, registry)
{
    // 对齐 vanilla Fox 构造（Fox.java:148）：setCanPickUpLoot(true) 使狐狸能拾取掉落物。
    // 由 MobEntity::tick 的 looting 扫描段驱动，经 wantsToPickUp(canHoldItem) 判定后调
    // FoxEntity::pickUpItem（手持物品语义：取 1 个入主手，多余掉落，吐出旧手持物）。
    setCanPickUpLoot(true);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> FoxEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<FoxEntity>(0, registry);
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
    math::Random& rng = getRandom();
    ItemDropHelper::spawnItemAtEntity(this, *m_heldItem, 0.0f, rng, 10);

    // 清空物品引用
    m_heldItem.reset();
}

void FoxEntity::spitOutItem(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return;
    }

    IWorld* worldPtr = this->world();
    if (worldPtr == nullptr) {
        return;
    }

    // 沿视线方向前方生成物品实体，带 40 tick 拾取延迟
    f32 lookX = -std::sin(math::toRadians(yaw()));
    f32 lookZ = std::cos(math::toRadians(yaw()));

    math::Random& rng = getRandom();
    f32 spawnX = static_cast<f32>(x()) + lookX;
    f32 spawnY = static_cast<f32>(y()) + 1.0f;
    f32 spawnZ = static_cast<f32>(z()) + lookZ;

    ItemDropHelper::spawnItemEntity(worldPtr, stack, spawnX, spawnY, spawnZ, rng, 40);

    // 播放吐出音效
    playSpitSound();
}

bool FoxEntity::canHoldItem(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return false;
    }

    // 主手为空时可拾取任何物品
    if (!isHoldingItem()) {
        return true;
    }

    // 当正在进食时（ticksSinceEaten > 0），只有新物品是食物而当前物品不是食物时才替换
    const ItemStack* currentHeld = getHeldItem();
    if (currentHeld != nullptr && m_ticksSinceEaten > 0) {
        return isConsumableFood(stack) && !isConsumableFood(*currentHeld);
    }

    return false;
}

bool FoxEntity::isConsumableFood(const ItemStack& stack) const
{
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }
    // 判断物品是否是食物
    return item->isFood();
}

bool FoxEntity::canEat() const
{
    if (!isHoldingItem()) {
        return false;
    }
    const ItemStack* held = getHeldItem();
    if (held == nullptr || !isConsumableFood(*held)) {
        return false;
    }
    // 没有攻击目标、在地面上、不在睡觉
    return attackTarget() == nullptr && onGround() && !isSleeping();
}

void FoxEntity::pickUpItem(ItemEntity& itemEntity)
{
    const ItemStack& itemStack = itemEntity.getItemStack();
    if (!canHoldItem(itemStack)) {
        return;
    }

    i32 count = itemStack.getCount();
    if (count > 1) {
        // 多余的物品在实体位置生成掉落物
        IWorld* worldPtr = this->world();
        if (worldPtr != nullptr) {
            ItemStack extra(*itemStack.getItem(), count - 1);
            math::Random& rng = getRandom();
            ItemDropHelper::spawnItemAtEntity(this, extra, 0.0f, rng, 10);
        }
    }

    // 吐出当前手持物品
    if (isHoldingItem()) {
        const ItemStack* current = getHeldItem();
        if (current != nullptr) {
            spitOutItem(*current);
        }
        m_heldItem.reset();
    }

    // 将新物品（只取1个）放入主手
    ItemStack toHold(*itemStack.getItem(), 1);
    m_heldItem = std::make_unique<ItemStack>(std::move(toHold));

    // 移除物品实体
    itemEntity.remove();

    // 重置进食计时器
    m_ticksSinceEaten = 0;
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
    // 甜浆果和发光浆果可以用来繁殖狐狸
    // MC 原版 FOX_FOOD 标签包含 sweet_berries 和 glow_berries
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::SWEET_BERRIES || item == Items::GLOW_BERRIES;
}

std::unique_ptr<AnimalEntity> FoxEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    auto baby = std::make_unique<FoxEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 遗传皮肤类型：50% 概率从任一父母继承皮肤类型
    FoxEntity* partnerFox = dynamic_cast<FoxEntity*>(&partner);
    math::Random& rng = getRandom();
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

    // 进食逻辑：每 tick 递增计时器，达到阈值后食用物品
    // MC 原版: Fox.aiStep() 中 ticksSinceEaten > 600 时调用 finishUsingItem
    IWorld* worldPtr = this->world();
    if (worldPtr != nullptr && !worldPtr->isClientSide() && isAlive()) {
        m_ticksSinceEaten++;

        if (isHoldingItem() && canEat()) {
            if (m_ticksSinceEaten > MIN_TICKS_BEFORE_EAT) {
                // 食用完成：调用 onItemUseFinish 处理物品消耗和效果应用
                // FoodItem 子类（蘑菇煲等）会在 onItemUseFinish 中处理 shrink 和容器物品返回
                // 普通食物（甜浆果等，注册为 Item 而非 FoodItem）的 onItemUseFinish 不处理消耗，
                // 需要在此手动 shrink
                // 对应 MC 原版: itemstack.finishUsingItem(this.level(), this)
                const ItemStack* held = getHeldItem();
                if (held != nullptr && isConsumableFood(*held)) {
                    ItemStack heldCopy = *held;
                    // 注意：const_cast 是安全的，因为 Items 在注册后是不可变的
                    const Item* item = heldCopy.getItem();
                    ItemStack result = const_cast<Item*>(item)->onItemUseFinish(heldCopy, *worldPtr, *this);

                    // 对于非 FoodItem 的食物（如甜浆果），onItemUseFinish 不会消耗物品
                    // 需要手动处理物品消耗
                    if (item != nullptr && !dynamic_cast<const item::items::FoodItem*>(item)) {
                        heldCopy.shrink(1);
                        // 如果物品有容器物品（非食物路径不太可能，但保持一致性）
                        if (heldCopy.isEmpty() && item->hasContainerItem()) {
                            result = ItemStack(item->containerItem(), 1);
                        } else {
                            result = heldCopy;
                        }
                    }

                    // 如果消耗后返回物品不为空（如蘑菇煲返回碗），放入嘴中
                    // 如果消耗后返回物品为空（普通食物完全消耗），清除嘴中物品
                    // 对应 MC 原版: if (!itemstack1.isEmpty()) { this.setItemSlot(MAINHAND, itemstack1); }
                    if (!result.isEmpty()) {
                        m_heldItem = std::make_unique<ItemStack>(std::move(result));
                    } else {
                        m_heldItem.reset();
                    }
                }
                m_ticksSinceEaten = 0;
            } else if (m_ticksSinceEaten > EAT_ANIMATION_START_TICKS && getRandom().nextFloat() < 0.1f) {
                // 接近完成时有 10% 概率播放吃音效
                playEatSound();
            }
        }

        // 如果攻击目标失效，取消蹲伏和感兴趣状态
        LivingEntity* target = attackTarget();
        if (target == nullptr || !target->isAlive()) {
            setCrouching(false);
            setInterested(false);
        }
    }

    // 卡在雪中时播放方块破坏粒子效果
    // 对应 MC Java: Fox.tick() 中 isFaceplanted() 时 levelEvent(2001, blockpos, Block.getId(blockstate))
    if (isStuck() && worldPtr != nullptr && getRandom().nextFloat() < 0.2f) {
        BlockPos pos(static_cast<i32>(x()), static_cast<i32>(y()), static_cast<i32>(z()));
        const BlockState* state = worldPtr->getBlockState(pos);
        if (state != nullptr) {
            worldPtr->playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, static_cast<i32>(state->stateId()));
        }
    }
    // TODO: fox 攻击 AI 链路未完全闭环（simpleMobTest 超时）。移动系统已修复（见 memory:
    // mobentity-navigator-pathfinder-null-global-bug，MobEntity 寻路三层接线 + WorldRegion），
    // fox 现能寻路接近 chicken。剩余缺口：FoxFollowTargetGoal START_FOLLOW_DISTANCE_SQ=36(6格)
    // 致近距离(<6格)猎物不启动 follow → fox 永不 crouch → FoxPounceGoal(isFullyCrouched 门控)
    // 永不启动（FoxGoals.hpp:130-131）。vanilla 对应类为 StalkPreyGoal，距离阈值逻辑一致但
    // 项目缺蹲伏触发链路。另:setFoxAggroed() 零调用点，vanilla 设 attackTarget 时应同步设
    // aggroed flag（FoxEntity.hpp:231）。详见 memory: fox-ai-follow-distance-threshold-bug。
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

    // 优先级 1: 卡住时脱离雪块（扑击落地后卡在雪中的自动脱离）
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(this));

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
                if (trusts(player->playerId())) return false;
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
                auto type = entity->entityType();
                return type == entity::VanillaEntityTypeKeys::WOLF || type == entity::VanillaEntityTypeKeys::POLAR_BEAR;
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

    // 优先级 3: 保卫信任玩家 - 当信任玩家被攻击时反击
    m_targetSelector.addGoal(3, std::make_unique<entity::ai::goal::FoxRevengeGoal>(this));

    // 优先级 4: 攻击小鸡和兔子
    m_targetSelector.addGoal(4,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            false, // checkSight = false
            10,    // chance = 10（随机检查间隔）
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                auto type = entity->entityType();
                return type == entity::VanillaEntityTypeKeys::CHICKEN || type == entity::VanillaEntityTypeKeys::RABBIT;
            }));

    // 优先级 4: 攻击幼年海龟（陆地上不在水中的幼体）
    m_targetSelector.addGoal(4,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<TurtleEntity>>(this,
            false, // checkSight = false
            10,    // chance = 10
            [](const LivingEntity* entity) -> bool {
                const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
                if (!turtle) return false;
                return turtle->isChild() && !turtle->isInWater();
            }));

    // 优先级 6: 攻击鱼群（仅群居鱼类：鳕鱼、鲑鱼、热带鱼，不包括河豚）
    m_targetSelector.addGoal(6,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            false, // checkSight = false
            20,    // chance = 20（比陆地猎物更低的检查频率）
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                auto type = entity->entityType();
                // 仅攻击群居鱼类：鳕鱼、鲑鱼、热带鱼（不包括河豚）
                return type == entity::VanillaEntityTypeKeys::COD || type == entity::VanillaEntityTypeKeys::SALMON ||
                    type == entity::VanillaEntityTypeKeys::TROPICAL_FISH;
            }));
}

// ========== 属性注册 ==========

void FoxEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 狐狸的属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
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
