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

#include "ExperienceOrbEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceConstants.hpp"
#include "common/entity/experience/ExperienceManager.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ExperienceOrbEntity::ExperienceOrbEntity(i32 xpValue, ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), nullptr, registry)
    , m_xpValue(std::clamp(xpValue, 1, MAX_ORB_SIZE))
{
    _initData();
}

ExperienceOrbEntity::ExperienceOrbEntity(IWorld* world, f64 x, f64 y, f64 z, i32 xpValue, ecs::EntityRegistry& registry)
    : Entity(EntityInstanceId(0), world, registry)
    , m_xpValue(std::clamp(xpValue, 1, MAX_ORB_SIZE))
{
    setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    _initData();
}

std::unique_ptr<Entity> ExperienceOrbEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ExperienceOrbEntity>(1, registry);
}

void ExperienceOrbEntity::_initData()
{
    // 设置初始速度为随机值
    math::Random rng(static_cast<u64>(std::hash<i32>{}(static_cast<i32>(m_id))));
    setVelocity(static_cast<f32>(rng.nextDouble() * 0.2 - 0.1),
        static_cast<f32>(rng.nextDouble() * 0.2),
        static_cast<f32>(rng.nextDouble() * 0.2 - 0.1));
}

// ============================================================================
// Entity 接口
// ============================================================================

void ExperienceOrbEntity::tick()
{
    Entity::tick();

    m_age++;
    m_tickCounter++;

    if (m_pickupDelay > 0) {
        m_pickupDelay--;
    }

    _updateMovement();
    _followNearestPlayer();

    if (m_age >= MAX_AGE) {
        remove();
        return;
    }
}

// ============================================================================
// 伤害处理
// ============================================================================

bool ExperienceOrbEntity::hurt(DamageSource& source, f32 amount)
{
    // 经验球可以被间接伤害（火焰、岩浆、爆炸等），但不能被玩家直接攻击

    // 1. 检查无敌状态
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 2. 标记受伤（触发速度同步到客户端）
    markHurt();

    // 3. 减少生命值
    m_health = static_cast<i32>(m_health - amount);

    // 4. 发送 ENTITY_DAMAGE 游戏事件（用于幽匿感测体检测）
    if (m_world != nullptr) {
        BlockPos blockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
        m_world->gameEvent(
            gameevent::GameEvents::ENTITY_DAMAGE, blockPos, gameevent::GameEvent::Context::of(source.getEntity()));
    }

    // 5. 如果生命值降至 0 或以下，销毁经验球
    if (m_health <= 0) {
        discard();
    }

    return true;
}

// ============================================================================
// 经验相关
// ============================================================================

void ExperienceOrbEntity::setXpValue(i32 value)
{
    m_xpValue = std::clamp(value, 1, MAX_ORB_SIZE);
}

i32 ExperienceOrbEntity::getOrbSize() const
{
    return entity::experience::utils::getOrbSize(m_xpValue);
}

// ============================================================================
// 合并
// ============================================================================

bool ExperienceOrbEntity::tryMergeWith(ExperienceOrbEntity& other)
{
    if (!canMergeWith(other)) {
        return false;
    }

    // 合并经验值
    i32 totalXp = m_xpValue + other.m_xpValue;

    // 如果合并后超过最大值，不合并
    if (totalXp > MAX_ORB_SIZE) {
        return false;
    }

    // 设置合并后的经验值
    m_xpValue = totalXp;

    // 重置存活时间为较新的那个
    m_age = std::min(m_age, other.m_age);

    // 移除另一个经验球
    other.remove();

    return true;
}

bool ExperienceOrbEntity::canMergeWith(const ExperienceOrbEntity& other) const
{
    // 不能和自己合并
    if (this == &other) {
        return false;
    }

    // 两个经验球都必须存活且可以被拾取
    if (isRemoved() || other.isRemoved()) {
        return false;
    }

    // 检查距离
    f32 distSq = distanceSqTo(other);
    constexpr f32 MERGE_DISTANCE_SQ =
        entity::experience::constants::ORB_MERGE_DISTANCE * entity::experience::constants::ORB_MERGE_DISTANCE;

    if (distSq > MERGE_DISTANCE_SQ) {
        return false;
    }

    // 检查合并后是否超过最大值
    i32 totalXp = m_xpValue + other.m_xpValue;
    if (totalXp > MAX_ORB_SIZE) {
        return false;
    }

    return true;
}

// ============================================================================
// 拾取
// ============================================================================

void ExperienceOrbEntity::onCollideWithPlayer(Player& player)
{
    // 检查拾取延迟
    if (!canBePickedUp()) {
        return;
    }

    // 检查玩家是否有拾取冷却
    if (!player.canPickupXp()) {
        return;
    }

    // 处理经验修补
    if (_handleMending(player)) {
        // 经验修补消耗了经验，检查是否还有剩余
        if (m_xpValue <= 0) {
            remove();
            return;
        }
    }

    // 给予玩家经验
    i32 xpGiven = _giveExperienceToPlayer(player);

    if (xpGiven > 0) {
        // 设置玩家的拾取冷却
        player.setXpCooldown(2);

        // 播放拾取音效
        if (m_world != nullptr) {
            math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_age));
            f32 pitch = 0.5f * ((rng.nextFloat() - rng.nextFloat()) * 0.7f + 1.8f);
            m_world->playSound(SoundEvents::ENTITY_EXPERIENCE_ORB_PICKUP,
                sound::SoundCategory::Players,
                m_builtIn.stateVector->m_pos,
                0.1f,
                pitch);
        }

        remove();
    }
}

// ============================================================================
// 静态工具方法
// ============================================================================

i32 ExperienceOrbEntity::getXPSplit(i32 totalXp)
{
    return entity::experience::utils::getXPSplit(totalXp);
}

// ============================================================================
// 私有方法
// ============================================================================

void ExperienceOrbEntity::_updateMovement()
{
    Vector3 vel = velocity();

    if (isInWater()) {
        vel.x *= 0.99;
        vel.y = std::min(vel.y + 0.0005f, 0.06f);
        vel.z *= 0.99;
    } else if (isInLava()) {
        math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_age));
        vel.x = static_cast<f32>((rng.nextFloat() - rng.nextFloat()) * 0.2);
        vel.y = 0.2f;
        vel.z = static_cast<f32>((rng.nextFloat() - rng.nextFloat()) * 0.2);
    } else if (!hasNoGravity()) {
        vel.y -= entity::experience::constants::ORB_GRAVITY;
    }

    Vector3 actual = moveWithCollision(vel.x, vel.y, vel.z);
    setVelocity(actual);

    vel = velocity();
    if (onGround()) {
        // 获取脚下方块的实际滑度
        f32 slipperiness = 0.6f; // 默认滑度
        if (m_world != nullptr) {
            BlockPos groundPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y - 0.2)), // 脚下方块
                static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
            const BlockState* groundState = m_world->getBlockState(groundPos);
            if (groundState != nullptr) {
                slipperiness = groundState->getBlock().getSlipperiness(*groundState, m_world, &groundPos, this);
            }
        }
        f32 groundFriction = slipperiness * 0.98f;
        vel.x *= groundFriction;
        vel.z *= groundFriction;
        vel.y *= -0.9f; // Y轴反弹
    } else {
        vel.x *= 0.98;
        vel.z *= 0.98;
        vel.y *= 0.98;
    }

    constexpr f32 MOTION_THRESHOLD = 0.003f;
    if (std::abs(vel.x) < MOTION_THRESHOLD) vel.x = 0.0f;
    if (std::abs(vel.z) < MOTION_THRESHOLD) vel.z = 0.0f;

    setVelocity(vel);
}

void ExperienceOrbEntity::_followNearestPlayer()
{
    constexpr i32 SEARCH_INTERVAL_BASE = 20;

    if (m_lastSearchTick < m_tickCounter - SEARCH_INTERVAL_BASE + (static_cast<i32>(id()) % 100)) {
        if (m_trackingPlayer == nullptr || m_trackingPlayer->isRemoved() || m_trackingPlayer->isSpectator() ||
            distanceSqTo(*m_trackingPlayer) > TRACKING_RANGE * TRACKING_RANGE) {
            m_trackingPlayer = _findNearestPlayer();
        }
        m_lastSearchTick = m_tickCounter;
    }

    if (m_trackingPlayer != nullptr && m_trackingPlayer->isSpectator()) {
        m_trackingPlayer = nullptr;
    }

    if (m_trackingPlayer == nullptr) {
        return;
    }

    f32 dx = static_cast<f32>(m_trackingPlayer->x() - x());
    f32 dy = static_cast<f32>(m_trackingPlayer->y() + m_trackingPlayer->eyeHeight() / 2.0f - y());
    f32 dz = static_cast<f32>(m_trackingPlayer->z() - z());
    f32 distSq = dx * dx + dy * dy + dz * dz;

    constexpr f32 TRACKING_RANGE_SQ = TRACKING_RANGE * TRACKING_RANGE;
    if (distSq < TRACKING_RANGE_SQ && distSq > 0.001f) {
        f32 dist = std::sqrt(distSq);
        f32 attraction = 1.0f - dist / TRACKING_RANGE;
        attraction = attraction * attraction * 0.1f;

        Vector3 vel = velocity();
        vel.x += (dx / dist) * attraction;
        vel.y += (dy / dist) * attraction;
        vel.z += (dz / dist) * attraction;
        setVelocity(vel);

        constexpr f32 PICKUP_DISTANCE_SQ = PICKUP_DISTANCE * PICKUP_DISTANCE;
        if (distSq < PICKUP_DISTANCE_SQ && canBePickedUp()) {
            onCollideWithPlayer(*m_trackingPlayer);
        }
    }
}

Player* ExperienceOrbEntity::_findNearestPlayer() const
{
    return EntityUtils::findClosestEntity<Player>(
        m_world, position(), TRACKING_RANGE, nullptr, [](Player* p) { return p->isAlive() && !p->isSpectator(); });
}

bool ExperienceOrbEntity::_handleMending(Player& player)
{
    // 经验修补：每2点经验修复1点耐久
    // 查找玩家装备中有经验修补附魔且损坏的物品
    // 检查顺序：主手、副手、头盔、胸甲、护腿、靴子
    PlayerInventory& inventory = player.inventory();

    // 收集有经验修补且损坏的物品槽位
    std::vector<i32> damagedSlots;

    // 主手物品
    i32 mainHandSlot = inventory.getSelectedSlot();
    ItemStack mainHandStack = inventory.getItem(mainHandSlot);
    if (!mainHandStack.isEmpty() &&
        item::enchant::EnchantmentHelper::hasEnchantment(mainHandStack, "minecraft:mending") &&
        mainHandStack.isDamaged()) {
        damagedSlots.push_back(mainHandSlot);
    }

    // 副手物品
    ItemStack offhandStack = inventory.getOffhandItem();
    if (!offhandStack.isEmpty() &&
        item::enchant::EnchantmentHelper::hasEnchantment(offhandStack, "minecraft:mending") &&
        offhandStack.isDamaged()) {
        damagedSlots.push_back(InventorySlots::OFFHAND);
    }

    // 护甲
    constexpr i32 armorSlots[] = {
        InventorySlots::ARMOR_HEAD,  // 头盔
        InventorySlots::ARMOR_CHEST, // 胸甲
        InventorySlots::ARMOR_LEGS,  // 护腿
        InventorySlots::ARMOR_FEET   // 靴子
    };

    for (i32 slot : armorSlots) {
        ItemStack armorStack = inventory.getItem(slot);
        if (!armorStack.isEmpty() &&
            item::enchant::EnchantmentHelper::hasEnchantment(armorStack, "minecraft:mending") &&
            armorStack.isDamaged()) {
            damagedSlots.push_back(slot);
        }
    }

    if (damagedSlots.empty()) {
        return false; // 没有需要修复的物品
    }

    // 随机选择一个损坏的物品
    math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_age));
    size_t selectedIndex = static_cast<size_t>(rng.nextInt(static_cast<i32>(damagedSlots.size())));
    i32 selectedSlot = damagedSlots[selectedIndex];

    // 获取物品
    ItemStack stack = inventory.getItem(selectedSlot);

    // 计算可以修复的耐久值
    i32 durabilityToRepair = entity::experience::utils::xpToDurability(m_xpValue);
    if (durabilityToRepair <= 0) {
        return false;
    }

    // 获取当前损坏值
    i32 currentDamage = stack.getDamage();
    i32 maxDamage = stack.getMaxDamage();

    if (currentDamage <= 0 || maxDamage <= 0) {
        return false;
    }

    // 计算实际修复量
    i32 actualRepair = std::min(durabilityToRepair, currentDamage);
    i32 newDamage = currentDamage - actualRepair;
    stack.setDamage(newDamage);

    // 更新物品
    inventory.setItem(selectedSlot, stack);

    // 计算消耗的经验值
    i32 xpConsumed = entity::experience::utils::durabilityToXp(actualRepair);
    m_xpValue -= xpConsumed;

    if (m_xpValue < 0) {
        m_xpValue = 0;
    }

    return true;
}

i32 ExperienceOrbEntity::_giveExperienceToPlayer(Player& player)
{
    if (m_xpValue <= 0) {
        return 0;
    }

    i32 xpToGive = m_xpValue;
    m_xpValue = 0;

    // 给予玩家经验
    player.addExperience(xpToGive);

    return xpToGive;
}

} // namespace mc
