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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHERWISE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "LlamaEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractChestedHorseEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace mc {

// ============================================================================
// 常量定义
// ============================================================================

namespace {

// 商队系统常量
constexpr f64 CARAVAN_SEARCH_RADIUS = 9.0;            // 搜索半径
constexpr f64 CARAVAN_SEARCH_HEIGHT = 4.0;            // 搜索高度
constexpr f64 CARAVAN_MIN_JOIN_DISTANCE_SQ = 4.0;     // 最小加入距离平方 (2格)
constexpr f64 CARAVAN_MAX_FOLLOW_DISTANCE_SQ = 676.0; // 最大跟随距离平方 (26格)
constexpr f64 CARAVAN_FOLLOW_DISTANCE = 2.0;          // 跟随间距
constexpr i32 CARAVAN_MAX_LENGTH = 8;                 // 商队最大长度

// 远程攻击常量
constexpr f32 LLAMA_SPIT_SPEED = 1.5f;       // 口水速度
constexpr f32 LLAMA_SPIT_INACCURACY = 10.0f; // 口水散布
constexpr f32 LLAMA_SPIT_DAMAGE = 1.0f;      // 口水伤害

// AI 速度常量
constexpr f64 LLAMA_CARAVAN_SPEED = 2.1;          // 商队跟随速度
constexpr f64 LLAMA_RANGED_ATTACK_SPEED = 1.25;   // 远程攻击移动速度
constexpr f32 LLAMA_RANGED_ATTACK_RADIUS = 20.0f; // 远程攻击半径
constexpr i32 LLAMA_ATTACK_INTERVAL = 40;         // 攻击间隔 ticks

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

LlamaEntity::LlamaEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractChestedHorseEntity(id, registry)
{
    randomizeAppearance();

    // 补调 registerGoals / registerAttributes：AnimalEntity 构造只调基类版（vtable 指向 AnimalEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。Llama 的 registerGoals 加专属
    // HurtByTarget / LlamaDefend 目标。详见 AbstractHorseEntity 构造注释。
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> LlamaEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<LlamaEntity>(0, registry);
}

void LlamaEntity::randomizeAppearance()
{
    math::Random random(ticksExisted());
    m_color = static_cast<LlamaColor>(random.nextInt(4));
    setStrength(1 + random.nextInt(5));
}

bool LlamaEntity::canBeRiddenBy(Player* player) const
{
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }

    return true;
}

i32 LlamaEntity::getInventoryColumns() const
{
    return m_strength;
}

void LlamaEntity::setStrength(i32 strength)
{
    m_strength = std::clamp(strength, 1, 5);
}

// ============================================================================
// 商队系统
// ============================================================================

void LlamaEntity::joinCaravan(LlamaEntity* head)
{
    if (head == nullptr) {
        return;
    }

    // 先离开当前商队
    leaveCaravan();

    // 设置新的商队头领
    m_caravanHead = head;
    // 设置头领的尾部为当前羊驼
    head->m_caravanTail = this;
}

void LlamaEntity::leaveCaravan()
{
    if (m_caravanHead != nullptr) {
        // 清除头领的尾部引用
        m_caravanHead->m_caravanTail = nullptr;
        // 清除自己的头领引用
        m_caravanHead = nullptr;
    }
}

// ============================================================================
// 食物与繁殖
// ============================================================================

bool LlamaEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 用于 TemptGoal AI 目标（玩家手持食物时会被诱惑）
    // 注意：只有干草块会触发繁殖（在 handleEating 中处理）
    return isFoodItem(itemStack);
}

bool LlamaEntity::isTameItem(const ItemStack& /*itemStack*/) const
{
    return false;
}

bool LlamaEntity::canMateWith(const AnimalEntity& other) const
{
    // 羊驼只能与羊驼交配
    if (this == &other) {
        return false;
    }

    const LlamaEntity* otherLlama = dynamic_cast<const LlamaEntity*>(&other);
    if (otherLlama == nullptr) {
        return false;
    }

    // 检查双方都满足繁殖条件（成体且不在爱心状态）
    return canBreed() && otherLlama->canBreed();
}

std::unique_ptr<AnimalEntity> LlamaEntity::spawnBaby(AnimalEntity& partner)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    math::Random rng(ticksExisted());

    auto baby = std::make_unique<LlamaEntity>(0, *registry);
    baby->setChild(true);
    baby->setPosition(x(), y(), z());

    // 遗传属性
    setOffspringAttributes(partner, *baby);

    // 遗传强度和颜色
    const LlamaEntity* partnerLlama = dynamic_cast<const LlamaEntity*>(&partner);
    if (partnerLlama != nullptr) {
        // 强度遗传：取父母强度的最大值，然后随机 +1/+0
        i32 parentStrength = std::max(getStrength(), partnerLlama->getStrength());
        i32 babyStrength = parentStrength + rng.nextInt(2); // +0 或 +1
        baby->setStrength(babyStrength);

        // 颜色遗传：随机选择父本或母本的颜色
        if (rng.nextBoolean()) {
            baby->setColor(getColor());
        } else {
            baby->setColor(partnerLlama->getColor());
        }
    }

    return baby;
}

bool LlamaEntity::handleEating(Player* player, ItemStack& itemStack)
{
    // 羊驼的食物效果与马不同：
    // - 小麦：治疗 2，成长 10 ticks，驯服 +3
    // - 干草块：治疗 10，成长 90 ticks，驯服 +6，可触发繁殖

    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    bool isWheat = (item == Items::WHEAT);
    bool isHayBlock = (item == Items::HAY_BLOCK);

    if (!isWheat && !isHayBlock) {
        return false;
    }

    i32 healAmount = 0;
    i32 growthTime = 0;
    i32 temperIncrease = 0;

    if (isWheat) {
        // 小麦效果
        healAmount = 2;
        growthTime = 10;
        temperIncrease = 3;
    } else { // isHayBlock
        // 干草块效果
        healAmount = 10;
        growthTime = 90;
        temperIncrease = 6;
    }

    bool hadEffect = false;

    // 治疗生命值
    if (health() < maxHealth()) {
        heal(static_cast<f32>(healAmount));
        hadEffect = true;
    }

    // 加速幼体成长
    if (isChild()) {
        addGrowingAge(growthTime);
        hadEffect = true;
    }

    // 增加驯服进度
    if (!isTame()) {
        increaseTemper(temperIncrease);
        hadEffect = true;
    }

    // 播放进食音效
    auto eatSound = getEatSound();
    if (eatSound.has_value()) {
        playSound(eatSound.value(), 1.0f, 1.0f);
    }

    // 触发繁殖（只有干草块可以触发繁殖）
    if (isHayBlock && getGrowingAge() == 0 && canBreed()) {
        setInLove();
        hadEffect = true;
    }

    // 消耗一个物品（非创造模式玩家）
    if (player != nullptr && !player->isCreative()) {
        itemStack.shrink(1);
    }

    return hadEffect;
}

bool LlamaEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 羊驼食物：小麦、干草块
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    return item == Items::WHEAT || item == Items::HAY_BLOCK;
}

const ResourceLocation& LlamaEntity::getChestEquipSound() const
{
    return SoundEvents::ENTITY_LLAMA_CHEST;
}

std::optional<ResourceLocation> LlamaEntity::getEatSound() const
{
    return SoundEvents::ENTITY_LLAMA_EAT;
}

std::optional<ResourceLocation> LlamaEntity::getAngrySound() const
{
    return SoundEvents::ENTITY_LLAMA_ANGRY;
}

// ============================================================================
// 远程攻击
// ============================================================================

void LlamaEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    _spit(target);
}

void LlamaEntity::_spit(LivingEntity* target)
{
    if (target == nullptr || m_world == nullptr) {
        return;
    }

    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }

    // 创建口水实体
    auto spitEntity = std::make_unique<entity::LlamaSpitEntity>(EntityInstanceId(0), *registry);
    spitEntity->setTypeId(entity::EntityTypeKeys::LLAMA_SPIT); // 工厂绕过补救：直接构造缺 typeId

    // 设置发射者
    spitEntity->setShooter(this);

    // 计算发射位置：从羊驼眼睛高度发射，稍微偏向前方
    f32 renderYawOffset = yaw(); // 使用 yaw 作为渲染偏移
    f32 sinYaw = std::sin(renderYawOffset * math::DEG_TO_RAD);
    f32 cosYaw = std::cos(renderYawOffset * math::DEG_TO_RAD);

    f32 spawnX = static_cast<f32>(x()) - (width() + 1.0f) * 0.5f * sinYaw;
    f32 spawnY = static_cast<f32>(getEyeY()) - 0.1f;
    f32 spawnZ = static_cast<f32>(z()) + (width() + 1.0f) * 0.5f * cosYaw;

    spitEntity->setPosition(spawnX, spawnY, spawnZ);

    // 计算射击向量：瞄准目标 1/3 高度
    f64 targetX = target->x() - x();
    f64 targetY = target->y() + target->height() / 3.0 - spawnY;
    f64 targetZ = target->z() - z();

    // 添加抛物线补偿
    f32 horizontalDist = std::sqrt(static_cast<f32>(targetX * targetX + targetZ * targetZ));
    f32 compensation = horizontalDist * 0.2f;

    // 发射
    spitEntity->shoot(static_cast<f32>(targetX),
        static_cast<f32>(targetY + compensation),
        static_cast<f32>(targetZ),
        LLAMA_SPIT_SPEED,
        LLAMA_SPIT_INACCURACY);

    // 生成实体
    m_world->spawnEntity(std::move(spitEntity));

    // 设置吐口水状态和冷却
    m_spitting = true;
    m_spitCooldown = LLAMA_ATTACK_INTERVAL;

    // 播放音效
    playSound(SoundEvents::ENTITY_LLAMA_SPIT, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
}

// ============================================================================
// 生命周期
// ============================================================================

void LlamaEntity::tick()
{
    AbstractChestedHorseEntity::tick();

    // 更新吐口水冷却
    if (m_spitCooldown > 0) {
        --m_spitCooldown;
    }

    // 商队跟随逻辑由 LlamaFollowCaravanGoal 处理
    // 这里只需要维护商队链表的完整性（检测头领是否还存在）
    if (m_caravanHead != nullptr) {
        // 检查头领是否还活着
        if (!m_caravanHead->isAlive() || m_caravanHead->isRemoved()) {
            leaveCaravan();
        }
    }
}

void LlamaEntity::registerGoals()
{
    AbstractChestedHorseEntity::registerGoals();

    // 对齐 vanilla Llama.registerGoals（Llama.java:117-131）：vanilla 完全 override，不继承
    // AbstractHorse 的 MountPanicGoal(priority 1)，而是把普通 PanicGoal 与 RangedAttackGoal
    // 同置 priority 3，且 RangedAttackGoal 注册在前。
    //
    // 本项目 GoalSelector 不按优先级排序、按注册顺序遍历，且同优先级 goal 不可互相抢占
    // (isPreemptedBy 要求 other.priority < m.priority)。若沿用父类 PanicGoal(priority 1)，
    // 受击后 PanicGoal(1) 先被遍历、先占据 Move flag，RangedAttackGoal(3) 因优先级更低无法
    // 抢占而永不启动——羊驼受击只会 panic 逃跑而非吐口水反击，与 vanilla 不一致。
    //
    // 修复：移除继承的 PanicGoal(1)，在 RangedAttackGoal 之后以 priority 3 重新注册 PanicGoal。
    // 这样受击后 RangedAttackGoal 先遍历、先占 Move flag（attackTarget 已由 HurtByTargetGoal
    // 设上），同优先级 PanicGoal 无法抢占，羊驼吐口水反击；仅当无有效 target 时 PanicGoal 才接管。
    m_goalSelector.removeGoalsOfType<entity::ai::goal::PanicGoal>();

    // 优先级 2: 商队跟随目标
    m_goalSelector.addGoal(
        2, std::make_unique<entity::ai::goal::LlamaFollowCaravanGoal>(this, static_cast<f32>(LLAMA_CARAVAN_SPEED)));

    // 优先级 3: 远程攻击目标（必须先于 PanicGoal 注册，使其在同优先级下先占据 Move flag）
    // 参数：速度 1.25, 攻击间隔 40 ticks, 攻击半径 20 格
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::RangedAttackGoal>(
            this, LLAMA_RANGED_ATTACK_SPEED, LLAMA_ATTACK_INTERVAL, LLAMA_ATTACK_INTERVAL, LLAMA_RANGED_ATTACK_RADIUS));

    // 优先级 3: 恐慌逃跑目标（同优先级，注册在远程攻击之后；仅当 RangedAttackGoal 未启动时接管）
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::PanicGoal>(this, 1.2));

    // Target 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this));

    // Target 优先级 2: 防御目标 - 攻击未驯服的狼
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::LlamaDefendTargetGoal>(this));
}

void LlamaEntity::registerAttributes()
{
    AbstractChestedHorseEntity::registerAttributes();
    // 羊驼生命值 = 15 + strength * 5
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f + static_cast<f32>(m_strength) * 5.0f);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.175f);
    // 羊驼的跟随范围是 40 格
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0f);
}

bool LlamaEntity::isValidArmorForSlot(const ItemStack& item) const
{
    // 检查物品是否在 ItemTags.CARPETS 中
    const Item* itemPtr = item.getItem();
    if (itemPtr == nullptr) {
        return false;
    }

    return itemPtr->isIn(item::tag::ItemTags::CARPETS());
}

} // namespace mc
