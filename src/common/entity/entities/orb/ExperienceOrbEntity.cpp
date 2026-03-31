#include "ExperienceOrbEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../player/Player.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../experience/ExperienceUtils.hpp"
#include "../../experience/ExperienceManager.hpp"
#include "../../inventory/PlayerInventory.hpp"
#include "../../../item/enchantment/EnchantmentHelper.hpp"
#include "../../../util/math/random/Random.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ExperienceOrbEntity::ExperienceOrbEntity(i32 xpValue)
    : Entity(LegacyEntityType::ExperienceOrb, EntityId(0))
    , m_xpValue(std::clamp(xpValue, 1, MAX_ORB_SIZE))
{
    initData();
}

ExperienceOrbEntity::ExperienceOrbEntity(IWorld* world, f64 x, f64 y, f64 z, i32 xpValue)
    : Entity(LegacyEntityType::ExperienceOrb, EntityId(0), world)
    , m_xpValue(std::clamp(xpValue, 1, MAX_ORB_SIZE))
{
    setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    initData();
}

std::unique_ptr<Entity> ExperienceOrbEntity::create(IWorld* /*world*/) {
    return std::make_unique<ExperienceOrbEntity>(1);
}

void ExperienceOrbEntity::initData() {
    // 设置初始速度为随机值
    math::Random rng(static_cast<u64>(std::hash<i32>{}(static_cast<i32>(m_id))));
    setVelocity(
        static_cast<f32>(rng.nextDouble() * 0.2 - 0.1),
        static_cast<f32>(rng.nextDouble() * 0.2),
        static_cast<f32>(rng.nextDouble() * 0.2 - 0.1)
    );
}

// ============================================================================
// Entity 接口
// ============================================================================

void ExperienceOrbEntity::tick() {
    Entity::tick();

    // 增加存活时间
    m_age++;

    // 减少拾取延迟
    if (m_pickupDelay > 0) {
        m_pickupDelay--;
    }

    // 更新物理运动
    updateMovement();

    // 追踪最近的玩家
    followNearestPlayer();

    // 检查是否过期（5分钟后消失）
    if (m_age >= MAX_AGE) {
        remove();
        return;
    }

    // 检查是否在水中或岩浆中（应用浮力）
    if (isInWater() || isInLava()) {
        // 在流体中漂浮
        Vector3 vel = velocity();
        vel.y += 0.01f;  // 轻微上浮
        setVelocity(vel);
    }
}

void ExperienceOrbEntity::baseTick() {
    Entity::baseTick();

    // 减少着火时间
    if (isOnFire()) {
        setFire(fire() - 1);
    }
}

// ============================================================================
// 经验相关
// ============================================================================

void ExperienceOrbEntity::setXpValue(i32 value) {
    m_xpValue = std::clamp(value, 1, MAX_ORB_SIZE);
}

i32 ExperienceOrbEntity::getOrbSize() const {
    return entity::experience::utils::getOrbSize(m_xpValue);
}

// ============================================================================
// 合并
// ============================================================================

bool ExperienceOrbEntity::tryMergeWith(ExperienceOrbEntity& other) {
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

bool ExperienceOrbEntity::canMergeWith(const ExperienceOrbEntity& other) const {
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
    constexpr f32 MERGE_DISTANCE_SQ = entity::experience::constants::ORB_MERGE_DISTANCE * entity::experience::constants::ORB_MERGE_DISTANCE;

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

void ExperienceOrbEntity::onCollideWithPlayer(Player& player) {
    // 检查拾取延迟
    if (!canBePickedUp()) {
        return;
    }

    // 检查玩家是否有拾取冷却
    if (!player.canPickupXp()) {
        return;
    }

    // 处理经验修补
    if (handleMending(player)) {
        // 经验修补消耗了经验，检查是否还有剩余
        if (m_xpValue <= 0) {
            remove();
            return;
        }
    }

    // 给予玩家经验
    i32 xpGiven = giveExperienceToPlayer(player);

    if (xpGiven > 0) {
        // 设置玩家的拾取冷却（参考 MC 1.16.5: 2 ticks）
        player.setXpCooldown(2);

        // TODO: 播放拾取音效
        // world.playSound(position, SoundEvents.ENTITY_EXPERIENCE_ORB_PICKUP, ...)

        remove();
    }
}

// ============================================================================
// 静态工具方法
// ============================================================================

i32 ExperienceOrbEntity::getXPSplit(i32 totalXp) {
    return entity::experience::utils::getXPSplit(totalXp);
}

// ============================================================================
// 私有方法
// ============================================================================

void ExperienceOrbEntity::updateMovement() {
    // 应用重力
    Vector3 vel = velocity();

    // 在水中或岩浆中有浮力
    if (isInWater() || isInLava()) {
        vel.y += entity::experience::constants::ORB_GRAVITY * 0.8f;  // 减弱重力
        vel.x *= 0.99f;
        vel.y *= 0.99f;
        vel.z *= 0.99f;
    } else {
        vel.y -= entity::experience::constants::ORB_GRAVITY;
    }

    // 应用摩擦力
    if (onGround()) {
        vel.x *= entity::experience::constants::ORB_GROUND_FRICTION;
        vel.z *= entity::experience::constants::ORB_GROUND_FRICTION;
    } else {
        vel.x *= 0.99f;
        vel.z *= 0.99f;
    }

    // 重置过小的速度为零
    constexpr f32 MOTION_THRESHOLD = 0.003f;
    if (std::abs(vel.x) < MOTION_THRESHOLD) vel.x = 0.0f;
    if (std::abs(vel.y) < MOTION_THRESHOLD && !onGround()) vel.y = 0.0f;
    if (std::abs(vel.z) < MOTION_THRESHOLD) vel.z = 0.0f;

    setVelocity(vel);

    // 移动
    Vector3 actual = moveWithCollision(vel.x, vel.y, vel.z);
    setVelocity(actual);
}

void ExperienceOrbEntity::followNearestPlayer() {
    // 查找最近的玩家
    Player* nearestPlayer = findNearestPlayer();

    if (!nearestPlayer) {
        m_trackingPlayer = nullptr;
        return;
    }

    m_trackingPlayer = nearestPlayer;

    // 计算到玩家的距离
    f32 dx = nearestPlayer->x() - x();
    f32 dy = nearestPlayer->y() - y();
    f32 dz = nearestPlayer->z() - z();
    f32 distSq = dx * dx + dy * dy + dz * dz;

    // 在追踪范围内时被吸引
    constexpr f32 TRACKING_RANGE_SQ = TRACKING_RANGE * TRACKING_RANGE;
    if (distSq < TRACKING_RANGE_SQ && distSq > 0.001f) {
        f32 dist = std::sqrt(distSq);

        // 吸引力计算：距离越近，吸引力越强
        // 参考 MC: 1.0 - sqrt(dist) / 8.0
        f32 attraction = 1.0f - dist / entity::experience::constants::ORB_ATTRACT_DISTANCE;
        attraction = attraction * attraction * entity::experience::constants::ORB_ATTRACT_SPEED;

        // 向玩家方向移动
        Vector3 vel = velocity();
        vel.x += (dx / dist) * attraction;
        vel.y += (dy / dist) * attraction;
        vel.z += (dz / dist) * attraction;

        setVelocity(vel);

        // 检查是否可以拾取
        constexpr f32 PICKUP_DISTANCE_SQ = PICKUP_DISTANCE * PICKUP_DISTANCE;
        if (distSq < PICKUP_DISTANCE_SQ && canBePickedUp()) {
            onCollideWithPlayer(*nearestPlayer);
        }
    }
}

Player* ExperienceOrbEntity::findNearestPlayer() const {
    if (!m_world) {
        return nullptr;
    }

    // 获取范围内的所有实体
    std::vector<Entity*> entities = m_world->getEntitiesInRange(
        Vector3(static_cast<f32>(x()), static_cast<f32>(y()), static_cast<f32>(z())),
        TRACKING_RANGE
    );

    // 查找最近的玩家
    Player* nearestPlayer = nullptr;
    f32 nearestDistSq = std::numeric_limits<f32>::max();

    for (Entity* entity : entities) {
        // 检查是否为玩家
        if (entity && entity->getTypeId() == entity::EntityTypes::PLAYER) {
            Player* player = static_cast<Player*>(entity);
            f32 dx = static_cast<f32>(player->x() - x());
            f32 dy = static_cast<f32>(player->y() - y());
            f32 dz = static_cast<f32>(player->z() - z());
            f32 distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearestPlayer = player;
            }
        }
    }

    return nearestPlayer;
}

bool ExperienceOrbEntity::handleMending(Player& player) {
    // 经验修补：每2点经验修复1点耐久
    // 参考 MC 1.16.5 ExperienceOrbEntity.damageItem

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
        InventorySlots::ARMOR_HEAD,   // 头盔
        InventorySlots::ARMOR_CHEST,  // 胸甲
        InventorySlots::ARMOR_LEGS,   // 护腿
        InventorySlots::ARMOR_FEET    // 靴子
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
        return false;  // 没有需要修复的物品
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

i32 ExperienceOrbEntity::giveExperienceToPlayer(Player& player) {
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
