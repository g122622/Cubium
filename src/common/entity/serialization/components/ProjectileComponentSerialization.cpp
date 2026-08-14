/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the above copyright notice
 * and this permission notice shall be included in all copies or substantial portions
 * of the Software.
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

#include "common/entity/serialization/components/ProjectileComponentSerialization.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/PickupStatus.hpp"
#include "common/entity/ecs/components/DamagingProjectileComponent.hpp"
#include "common/entity/ecs/components/EvokerFangsComponent.hpp"
#include "common/entity/ecs/components/EyeOfEnderComponent.hpp"
#include "common/entity/ecs/components/FireballStateComponent.hpp"
#include "common/entity/ecs/components/FireworkRocketComponent.hpp"
#include "common/entity/ecs/components/ProjectileArrowStateComponent.hpp"
#include "common/entity/ecs/components/ProjectileItemComponent.hpp"
#include "common/entity/ecs/components/ProjectileOwnerComponent.hpp"
#include "common/entity/ecs/components/ShulkerBulletComponent.hpp"
#include "common/entity/ecs/components/TridentStateComponent.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/projectile/TridentEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <array>
#include <memory>
#include <utility>

namespace mc::entity::serialization::components {

using namespace nbt_keys;
using nbt_helper::tryGetBool;
using nbt_helper::tryGetByte;
using nbt_helper::tryGetCompound;
using nbt_helper::tryGetDouble;
using nbt_helper::tryGetFloat;
using nbt_helper::tryGetInt;
using nbt_helper::tryGetLong;

// ============================================================================
// ProjectileOwnerComponent — owner UUID(双 long) + LeftOwner + HasBeenShot
// 对齐 vanilla 1.21.11 Projectile.addAdditionalSaveData()。
// owner UUID 用双 long 格式（OwnerUUIDMost/Least），与项目既有 EvokerFangs/AreaEffectCloud
// 一致（非 vanilla EntityReference 单键格式）。
// owner 字段无 DataParameter 同步副作用，序列化器直写组件（与 EvokerFangs 既有 OOP 实现一致）。
// ============================================================================

static void writeOwnerUuid(nbt::tags::compound_tag& tag, const std::string& uuidStr)
{
    if (uuidStr.empty()) {
        return;
    }
    auto uuidBytes = util::uuidFromString(uuidStr);
    if (uuidBytes.size() != 16) {
        return;
    }
    i64 most = 0;
    i64 least = 0;
    for (i32 i = 0; i < 8; ++i) {
        most = (most << 8) | static_cast<i64>(uuidBytes[i]);
    }
    for (i32 i = 8; i < 16; ++i) {
        least = (least << 8) | static_cast<i64>(uuidBytes[i]);
    }
    tag.put(PROJECTILE_OWNER_UUID_MOST, most);
    tag.put(PROJECTILE_OWNER_UUID_LEAST, least);
}

static std::string readOwnerUuid(const nbt::tags::compound_tag& tag)
{
    auto mostVal = tryGetLong(tag, PROJECTILE_OWNER_UUID_MOST);
    auto leastVal = tryGetLong(tag, PROJECTILE_OWNER_UUID_LEAST);
    if (!mostVal.has_value() || !leastVal.has_value()) {
        return {};
    }
    i64 m = *mostVal;
    i64 l = *leastVal;
    std::array<u8, 16> uuidBytes{};
    for (i32 i = 7; i >= 0; --i) {
        uuidBytes[i] = static_cast<u8>(m & 0xFF);
        m >>= 8;
    }
    for (i32 i = 15; i >= 8; --i) {
        uuidBytes[i] = static_cast<u8>(l & 0xFF);
        l >>= 8;
    }
    return util::uuidToString(uuidBytes);
}

static void saveProjectileOwner(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::ProjectileOwnerComponent>();
    if (c == nullptr) {
        return;
    }
    writeOwnerUuid(tag, c->m_shooterUuid);
    // vanilla 仅在 LeftOwner=true 时写该键；项目对齐。
    if (c->m_leftShooter) {
        tag.put(PROJECTILE_LEFT_OWNER, static_cast<i8>(1));
    }
    tag.put(PROJECTILE_HAS_BEEN_SHOT, static_cast<i8>(c->m_hasBeenShot ? 1 : 0));
}

static Result<void> loadProjectileOwner(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::ProjectileOwnerComponent>();
    if (c == nullptr) {
        return {};
    }
    c->m_shooterUuid = readOwnerUuid(tag);
    if (auto val = tryGetBool(tag, PROJECTILE_LEFT_OWNER)) {
        c->m_leftShooter = *val;
    }
    if (auto val = tryGetBool(tag, PROJECTILE_HAS_BEEN_SHOT)) {
        c->m_hasBeenShot = *val;
    }
    return {};
}

// ============================================================================
// ProjectileArrowStateComponent — AbstractArrow 8 字段 + dealtDamage
// 对齐 vanilla 1.21.11 AbstractArrow.addAdditionalSaveData()。
// 字段：life/shake/inGround/pickup/damage/crit/PierceLevel/item/dealtDamage。
// dealtDamage 是 ThrownTrident 的字段但复用父类 ProjectileArrowStateComponent.m_dealtDamage，
// 故归入本序列化器（vanilla ThrownTrident 单独存 DealtDamage，项目统一进父组件）。
// priority=10：晚于 TridentState(priority=0) load，确保三叉戟 item 先读以重算 loyalty。
// ============================================================================

static void saveArrowState(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c == nullptr) {
        return;
    }
    tag.put(ARROW_LIFE, static_cast<i16>(c->m_ticksInGround));
    tag.put(ARROW_SHAKE, static_cast<i8>(c->m_arrowShake));
    tag.put(ARROW_IN_GROUND, static_cast<i8>(c->m_inGround ? 1 : 0));
    tag.put(ARROW_PICKUP, static_cast<i8>(c->m_pickupStatus));
    tag.put(ARROW_DAMAGE, c->m_damage);
    tag.put(ARROW_CRIT, static_cast<i8>(c->m_critical ? 1 : 0));
    tag.put(ARROW_PIERCE_LEVEL, static_cast<i8>(c->m_pierceLevel));
    tag.put(TRIDENT_DEALT_DAMAGE, static_cast<i8>(c->m_dealtDamage ? 1 : 0));
}

static Result<void> loadArrowState(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::ProjectileArrowStateComponent>();
    if (c == nullptr) {
        return {};
    }
    if (auto val = tryGetInt(tag, ARROW_LIFE)) {
        c->m_ticksInGround = *val;
    }
    if (auto val = tryGetByte(tag, ARROW_SHAKE)) {
        c->m_arrowShake = *val;
    }
    if (auto val = tryGetBool(tag, ARROW_IN_GROUND)) {
        c->m_inGround = *val;
    }
    if (auto val = tryGetByte(tag, ARROW_PICKUP)) {
        i8 v = *val;
        if (v >= static_cast<i8>(PickupStatus::Disallowed) && v <= static_cast<i8>(PickupStatus::CreativeOnly)) {
            c->m_pickupStatus = static_cast<PickupStatus>(v);
        }
    }
    if (auto val = tryGetFloat(tag, ARROW_DAMAGE)) {
        c->m_damage = *val;
    }
    if (auto val = tryGetBool(tag, ARROW_CRIT)) {
        c->m_critical = *val;
    }
    if (auto val = tryGetByte(tag, ARROW_PIERCE_LEVEL)) {
        c->m_pierceLevel = static_cast<u8>(*val);
    }
    if (auto val = tryGetBool(tag, TRIDENT_DEALT_DAMAGE)) {
        c->m_dealtDamage = *val;
    }
    return {};
}

// ============================================================================
// TridentStateComponent — Trident item（loyalty 从 item 重算不存盘）
// 对齐 vanilla 1.21.11 ThrownTrident.addAdditionalSaveData()：仅存 Trident item。
// DealtDamage 归入 ProjectileArrowStateComponent（父组件），此处不存。
// priority=0：早于 ArrowState(priority=10) load。load 时调 TridentEntity::setItemStack
// 重算 loyalty（从 item 忠诚附魔）并镜像同步 DATA_LOYALTY/DATA_FOIL。
// ============================================================================

static void saveTridentState(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::TridentStateComponent>();
    if (c == nullptr || c->m_tridentStack == nullptr) {
        return;
    }
    nbt::tags::compound_tag itemTag;
    c->m_tridentStack->toNbt(itemTag);
    tag.value.emplace(TRIDENT_ITEM, std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
}

static Result<void> loadTridentState(Entity& entity, const nbt::tags::compound_tag& tag)
{
    // 仅 TridentEntity 持有此组件；load item 时调 setItemStack 重算 loyalty + 镜像同步。
    // 非 TridentEntity（理论上不会 attach 此组件）早退。
    auto* trident = dynamic_cast<TridentEntity*>(&entity);
    const nbt::tags::compound_tag* itemTag = tryGetCompound(tag, TRIDENT_ITEM);
    if (trident != nullptr && itemTag != nullptr) {
        auto stackResult = ItemStack::fromNbt(*itemTag);
        if (stackResult.success()) {
            trident->setItemStack(stackResult.value());
        }
        return {};
    }
    // 降级路径：非 TridentEntity 或缺 item 键时直写组件（不重算 loyalty）。
    auto* c = entity.tryGetComponent<ecs::TridentStateComponent>();
    if (c != nullptr && c->m_tridentStack != nullptr && itemTag != nullptr) {
        auto stackResult = ItemStack::fromNbt(*itemTag);
        if (stackResult.success()) {
            *c->m_tridentStack = stackResult.value();
        }
    }
    return {};
}

// ============================================================================
// ProjectileItemComponent — ThrowableItemProjectile / Spear item
// 对齐 vanilla 1.21.11 ThrowableItemProjectile.addAdditionalSaveData()（Item 键）。
// Arrow/Spear 的 item 也走此组件（Spear 复用 ProjectileItemComponent 承载 m_spearStack）。
// 注意：AbstractArrow 子树的 item 键是 "item"（ARROW_ITEM），与 ThrowableItemProjectile 的 "Item"
// 不同大小写。本项目 ProjectileItemComponent 跨两类子树共用，按实体类型分发键名：
//   - AbstractArrow 子树（Spear）用 "item"
//   - ThrowableItemProjectile 子树（Snowball/Egg/Potion/ExperienceBottle/EnderPearl）用 "Item"
// 区分依据：dynamic_cast<AbstractArrowEntity*> 成功则用 "item"，否则用 "Item"。
// ============================================================================

static void saveProjectileItem(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::ProjectileItemComponent>();
    if (c == nullptr || c->m_itemStack == nullptr) {
        return;
    }
    // AbstractArrow 子树（Spear）用小写 "item" 键，ThrowableItemProjectile 子树用 "Item" 键。
    const std::string& key =
        dynamic_cast<const AbstractArrowEntity*>(&entity) != nullptr ? ARROW_ITEM : EYE_OF_ENDER_ITEM;
    nbt::tags::compound_tag itemTag;
    c->m_itemStack->toNbt(itemTag);
    tag.value.emplace(key, std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
}

static Result<void> loadProjectileItem(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::ProjectileItemComponent>();
    if (c == nullptr || c->m_itemStack == nullptr) {
        return {};
    }
    const std::string& key =
        dynamic_cast<const AbstractArrowEntity*>(&entity) != nullptr ? ARROW_ITEM : EYE_OF_ENDER_ITEM;
    if (const nbt::tags::compound_tag* itemTag = tryGetCompound(tag, key)) {
        auto stackResult = ItemStack::fromNbt(*itemTag);
        if (stackResult.success()) {
            *c->m_itemStack = stackResult.value();
        }
    }
    return {};
}

// ============================================================================
// DamagingProjectileComponent — acceleration_power
// 对齐 vanilla 1.21.11 AbstractHurtingProjectile.addAdditionalSaveData()。
// vanilla 存单一 acceleration_power(double)，项目分 XYZ 三分量（运行时加速度向量）。
// 存盘取三分量模长写 acceleration_power；读盘按模长无法还原分量方向，故读盘时均分到三分量
// （TODO: 项目与 vanilla 字段结构差异，方向信息存盘后丢失，待统一字段结构后修正）。
// ============================================================================

static void saveDamagingProjectile(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::DamagingProjectileComponent>();
    if (c == nullptr) {
        return;
    }
    // 模长 = sqrt(x^2+y^2+z^2)，对齐 vanilla acceleration_power 语义（加速力大小）。
    f64 power = std::sqrt(static_cast<f64>(c->m_accelerationX) * c->m_accelerationX +
        static_cast<f64>(c->m_accelerationY) * c->m_accelerationY +
        static_cast<f64>(c->m_accelerationZ) * c->m_accelerationZ);
    tag.put(ACCELERATION_POWER, power);
}

static Result<void> loadDamagingProjectile(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::DamagingProjectileComponent>();
    if (c == nullptr) {
        return {};
    }
    if (auto val = tryGetDouble(tag, ACCELERATION_POWER)) {
        // TODO: 项目 DamagingProjectile 用 XYZ 三分量，vanilla 用单一 acceleration_power。
        // 读盘时方向信息丢失，均分到三分量（近似，待字段结构统一后修正）。
        f32 comp = static_cast<f32>(*val / 3.0);
        c->m_accelerationX = comp;
        c->m_accelerationY = comp;
        c->m_accelerationZ = comp;
    }
    return {};
}

// ============================================================================
// FireballStateComponent — Fireball(ExplosionPower) / WitherSkull(dangerous)
// 对齐 vanilla 1.21.11 Fireball/WitherSkull。
// Fireball 与 WitherSkull 共用此组件：m_explosionPower(Fireball)/m_blue(WitherSkull)。
// 按实体类型分发键名：WitherSkull 存 dangerous(m_blue)，其余(Fireball)存 ExplosionPower(m_explosionPower)。
// 注意：vanilla Fireball 存 Item 不存 ExplosionPower（ExplosionPower 是 LargeFireball/恶魂的键）；
// 项目 FireballEntity 用 m_explosionPower 作爆炸威力，此为项目既有设计差异，沿用项目现状。
// ============================================================================

static void saveFireballState(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::FireballStateComponent>();
    if (c == nullptr) {
        return;
    }
    // 判定是否为凋灵之首：FireballEntity 无 m_blue 语义（恒 false），WitherSkull 用 m_blue。
    // 用组件内 m_blue 是否被设置过不可靠（Fireball 的 m_blue 恒 false），改用实体类型名判定。
    // 简化：m_explosionPower 默认 1（Fireball 构造 setDamage(6) 不改 explosionPower），
    // WitherSkull 构造也不改 explosionPower。两者都用 m_explosionPower=1 默认值，无法区分。
    // 故按 entity 是否能 cast 到 WitherSkull 分发——但此处避免引入 WitherSkullEntity 依赖，
    // 改用 typeId 字符串判定。
    // TODO: 补 WitherSkullEntity 的 typeId 判定后精确分发；当前两键都写（vanilla 客户端按类型只读其一）。
    tag.put(FIREBALL_EXPLOSION_POWER, static_cast<i8>(c->m_explosionPower));
    tag.put(WITHER_SKULL_DANGEROUS, static_cast<i8>(c->m_blue ? 1 : 0));
}

static Result<void> loadFireballState(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::FireballStateComponent>();
    if (c == nullptr) {
        return {};
    }
    if (auto val = tryGetByte(tag, FIREBALL_EXPLOSION_POWER)) {
        c->m_explosionPower = *val;
    }
    if (auto val = tryGetBool(tag, WITHER_SKULL_DANGEROUS)) {
        c->m_blue = *val;
    }
    return {};
}

// ============================================================================
// FireworkRocketComponent — Life/LifeTime/FireworksItem/ShotAtAngle
// 对齐 vanilla 1.21.11 FireworkRocketEntity.addAdditionalSaveData()。
// 搬迁自 FireworkRocketEntity 既有 OOP override（删除原 override）。
// load 时 FireworksItem 经 setFireworkItem（重算 flightTime + 镜像同步 DATA_FIREWORKS_ITEM）。
// ============================================================================

static void saveFireworkRocket(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::FireworkRocketComponent>();
    if (c == nullptr) {
        return;
    }
    if (c->m_fireworkItem != nullptr && !c->m_fireworkItem->isEmpty()) {
        nbt::tags::compound_tag itemTag;
        c->m_fireworkItem->toNbt(itemTag);
        tag.value.emplace(FIREWORKS_ITEM, std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
    }
    tag.put(LIFE, c->m_lifetime);
    // 总生命时间仅在已计算时写出（-1 是占位符，对齐原 OOP 实现）。
    if (c->m_lifeTime >= 0) {
        tag.put(LIFE_TIME, c->m_lifeTime);
    }
    tag.put(SHOT_AT_ANGLE, static_cast<i8>(c->m_shotFromCrossbow ? 1 : 0));
}

static Result<void> loadFireworkRocket(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::FireworkRocketComponent>();
    // 烟花物品经 setFireworkItem 重算 flightTime + 镜像同步 DATA_FIREWORKS_ITEM。
    auto* rocket = dynamic_cast<FireworkRocketEntity*>(&entity);
    if (const nbt::tags::compound_tag* itemTag = tryGetCompound(tag, FIREWORKS_ITEM)) {
        auto stackResult = ItemStack::fromNbt(*itemTag);
        if (stackResult.success() && rocket != nullptr) {
            rocket->setFireworkItem(stackResult.value());
        }
    }
    if (c == nullptr) {
        return {};
    }
    if (auto val = tryGetInt(tag, LIFE)) {
        c->m_lifetime = *val;
    }
    // 覆盖 setFireworkItem 中的 -1 重置（对齐原 OOP 实现）。
    if (auto val = tryGetInt(tag, LIFE_TIME)) {
        c->m_lifeTime = *val;
    }
    if (auto val = tryGetByte(tag, SHOT_AT_ANGLE)) {
        c->m_shotFromCrossbow = (*val != 0);
    }
    return {};
}

// ============================================================================
// EvokerFangsComponent — Warmup + Owner UUID(双 long)
// 对齐 vanilla 1.21.11 EvokerFangs.addAdditionalSaveData()。
// 搬迁自 EvokerFangsEntity 既有 OOP override（删除原 override）。
// owner 字段无 DataParameter 同步，直写组件（与原 OOP 实现一致）。
// ============================================================================

static void saveEvokerFangs(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::EvokerFangsComponent>();
    if (c == nullptr) {
        return;
    }
    tag.put(WARMUP, c->m_warmupDelay);
    writeOwnerUuid(tag, c->m_ownerUuid);
}

static Result<void> loadEvokerFangs(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::EvokerFangsComponent>();
    if (c == nullptr) {
        return {};
    }
    if (auto val = tryGetInt(tag, WARMUP)) {
        c->m_warmupDelay = *val;
    }
    c->m_ownerUuid = readOwnerUuid(tag);
    c->m_owner = nullptr; // 运行时指针经 UUID 懒加载查找，不持久化。
    return {};
}

// ============================================================================
// ShulkerBulletComponent — Target/Dir/Steps/TXD/TYD/TZD
// 对齐 vanilla 1.21.11 ShulkerBullet.addAdditionalSaveData()。
// target 是运行时 Entity* 指针非持久，持久化 m_targetUuid；direction 用 Direction 枚举值
// （vanilla Dir 键 legacy 3D id，与项目 Direction 枚举值一致）。
// ============================================================================

static void saveShulkerBullet(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::ShulkerBulletComponent>();
    if (c == nullptr) {
        return;
    }
    if (!c->m_targetUuid.empty()) {
        writeOwnerUuid(tag, c->m_targetUuid);
        // TODO: vanilla Target 键是单一 UUID（EntityReference），项目用双 long 复用 writeOwnerUuid。
        // 键名复用 OwnerUUIDMost/Least（与 owner 同格式），vanilla 客户端按 ShulkerBullet 类型读取。
    }
    if (c->m_direction != Direction::None) {
        tag.put(SHULKER_BULLET_DIR, static_cast<i8>(c->m_direction));
    }
    tag.put(SHULKER_BULLET_STEPS, c->m_flightSteps);
    tag.put(SHULKER_BULLET_TXD, static_cast<f64>(c->m_targetDelta.x));
    tag.put(SHULKER_BULLET_TYD, static_cast<f64>(c->m_targetDelta.y));
    tag.put(SHULKER_BULLET_TZD, static_cast<f64>(c->m_targetDelta.z));
}

static Result<void> loadShulkerBullet(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::ShulkerBulletComponent>();
    if (c == nullptr) {
        return {};
    }
    c->m_targetUuid = readOwnerUuid(tag);
    c->m_target = nullptr; // 运行时指针经 UUID 懒加载查找。
    if (auto val = tryGetByte(tag, SHULKER_BULLET_DIR)) {
        i8 v = *val;
        if (v >= static_cast<i8>(Direction::Down) && v <= static_cast<i8>(Direction::East)) {
            c->m_direction = static_cast<Direction>(v);
        }
    }
    if (auto val = tryGetInt(tag, SHULKER_BULLET_STEPS)) {
        c->m_flightSteps = *val;
    }
    if (auto val = tryGetDouble(tag, SHULKER_BULLET_TXD)) {
        c->m_targetDelta.x = static_cast<f64>(*val);
    }
    if (auto val = tryGetDouble(tag, SHULKER_BULLET_TYD)) {
        c->m_targetDelta.y = static_cast<f64>(*val);
    }
    if (auto val = tryGetDouble(tag, SHULKER_BULLET_TZD)) {
        c->m_targetDelta.z = static_cast<f64>(*val);
    }
    return {};
}

// ============================================================================
// EyeOfEnderComponent — vanilla 存 Item（项目无 item 字段）
// 对齐 vanilla 1.21.11 EyeOfEnder.addAdditionalSaveData()：仅存 Item，不调 super（不存 Owner）。
// 项目 EyeOfEnderEntity 当前无 item 字段（运行时算），此序列化器仅占位标 TODO，
// 不读写任何键（待补 item 字段后镜像）。
// ============================================================================

static void saveEyeOfEnder(const Entity& /*entity*/, nbt::tags::compound_tag& /*tag*/)
{
    // TODO: 项目 EyeOfEnderEntity 当前无 item 字段，vanilla EyeOfEnder 持久化 "Item" 键。
    // 补齐 item 字段后此处写 EYE_OF_ENDER_ITEM。
}

static Result<void> loadEyeOfEnder(Entity& /*entity*/, const nbt::tags::compound_tag& /*tag*/)
{
    // TODO: 同 save，待补 item 字段后读 EYE_OF_ENDER_ITEM。
    return {};
}

// ============================================================================
// 注册
// ============================================================================

void registerProjectileComponentSerializers(ComponentSerializerRegistry& registry)
{
    // priority=0（无跨组件依赖）：owner/item/fireball/firework/evokerfangs/shulkerbullet/eyeofender/damaging
    registry.registerSerializer<ecs::ProjectileOwnerComponent>(saveProjectileOwner, loadProjectileOwner);
    registry.registerSerializer<ecs::ProjectileItemComponent>(saveProjectileItem, loadProjectileItem);
    registry.registerSerializer<ecs::DamagingProjectileComponent>(saveDamagingProjectile, loadDamagingProjectile);
    registry.registerSerializer<ecs::FireballStateComponent>(saveFireballState, loadFireballState);
    registry.registerSerializer<ecs::FireworkRocketComponent>(saveFireworkRocket, loadFireworkRocket);
    registry.registerSerializer<ecs::EvokerFangsComponent>(saveEvokerFangs, loadEvokerFangs);
    registry.registerSerializer<ecs::ShulkerBulletComponent>(saveShulkerBullet, loadShulkerBullet);
    registry.registerSerializer<ecs::EyeOfEnderComponent>(saveEyeOfEnder, loadEyeOfEnder);

    // priority 分层：TridentState=0 先 load（读 item 重算 loyalty），ArrowState=10 后 load（读 dealtDamage）。
    // dealtDamage 在父组件 ProjectileArrowStateComponent，三叉戟复用，须晚于 TridentState load
    // 确保三叉戟 item 已就位（虽然 dealtDamage 与 item 无直接依赖，但沿用计划 priority 设计
    // 保证未来扩展时 loyalty/dealtDamage 顺序可控）。
    registry.registerSerializer<ecs::TridentStateComponent>(saveTridentState, loadTridentState, 0);
    registry.registerSerializer<ecs::ProjectileArrowStateComponent>(saveArrowState, loadArrowState, 10);
}

} // namespace mc::entity::serialization::components
