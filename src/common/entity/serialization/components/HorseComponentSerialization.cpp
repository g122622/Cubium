/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

#include "common/entity/serialization/components/HorseComponentSerialization.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/ecs/components/HorseAttributeComponent.hpp"
#include "common/entity/ecs/components/HorseJumpComponent.hpp"
#include "common/entity/ecs/components/HorseStatusComponent.hpp"
#include "common/entity/ecs/components/HorseTamingComponent.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <array>

namespace mc::entity::serialization::components {

// ============================================================================
// HorseTamingComponent — Temper + OwnerUUIDMost/Least（双 long）
// priority=0：ownerUuid 先 load 调 setOwnerUuid 触发 setTame(true) 联动写 STATUS_PARAM。
// ============================================================================

static void saveHorseTaming(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::HorseTamingComponent>();
    if (c == nullptr) {
        return; // 非 Horse 族早退
    }
    tag.put(nbt_keys::TEMPER, c->m_temper);

    // 主人UUID（OwnerUUIDMost/OwnerUUIDLeast 双 long 格式）
    if (!c->m_ownerUuid.empty()) {
        auto uuidBytes = ::mc::util::uuidFromString(c->m_ownerUuid);
        if (uuidBytes.size() == 16) {
            i64 most = (static_cast<i64>(uuidBytes[0]) << 56) | (static_cast<i64>(uuidBytes[1]) << 48) |
                (static_cast<i64>(uuidBytes[2]) << 40) | (static_cast<i64>(uuidBytes[3]) << 32) |
                (static_cast<i64>(uuidBytes[4]) << 24) | (static_cast<i64>(uuidBytes[5]) << 16) |
                (static_cast<i64>(uuidBytes[6]) << 8) | static_cast<i64>(uuidBytes[7]);

            i64 least = (static_cast<i64>(uuidBytes[8]) << 56) | (static_cast<i64>(uuidBytes[9]) << 48) |
                (static_cast<i64>(uuidBytes[10]) << 40) | (static_cast<i64>(uuidBytes[11]) << 32) |
                (static_cast<i64>(uuidBytes[12]) << 24) | (static_cast<i64>(uuidBytes[13]) << 16) |
                (static_cast<i64>(uuidBytes[14]) << 8) | static_cast<i64>(uuidBytes[15]);

            tag.put(nbt_keys::HORSE_OWNER_UUID_MOST, most);
            tag.put(nbt_keys::HORSE_OWNER_UUID_LEAST, least);
        }
    }
}

static Result<void> loadHorseTaming(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::HorseTamingComponent>();
    if (c == nullptr) {
        return {}; // 非 Horse 族早退
    }
    auto* horse = dynamic_cast<::mc::AbstractHorseEntity*>(&entity);
    if (horse == nullptr) {
        return {};
    }

    // 驯服进度直写组件
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::TEMPER)) {
        c->m_temper = *val;
    }

    // 主人UUID 调 setOwnerUuid（触发 setTame(true) 联动 + _syncStatusFlags 写 STATUS_PARAM）
    auto ownerMostVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_MOST);
    auto ownerLeastVal = nbt_helper::tryGetLong(tag, nbt_keys::HORSE_OWNER_UUID_LEAST);
    if (ownerMostVal.has_value() && ownerLeastVal.has_value()) {
        i64 most = ownerMostVal.value();
        i64 least = ownerLeastVal.value();
        std::array<u8, 16> uuidBytes{};
        for (i32 i = 7; i >= 0; --i) {
            uuidBytes[i] = static_cast<u8>(most & 0xFF);
            most >>= 8;
        }
        for (i32 i = 15; i >= 8; --i) {
            uuidBytes[i] = static_cast<u8>(least & 0xFF);
            least >>= 8;
        }
        horse->setOwnerUuid(::mc::util::uuidToString(uuidBytes));
    } else {
        horse->clearOwnerUuid();
    }
    return {};
}

// ============================================================================
// HorseJumpComponent — JumpStrength（f32）
// priority=0：load 后同步 AttributeMap（HORSE_JUMP_STRENGTH）。
// ============================================================================

static void saveHorseJump(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::HorseJumpComponent>();
    if (c == nullptr) {
        return;
    }
    tag.put(nbt_keys::HORSE_JUMP_STRENGTH, c->m_jumpStrength);
}

static Result<void> loadHorseJump(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::HorseJumpComponent>();
    if (c == nullptr) {
        return {};
    }
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HORSE_JUMP_STRENGTH)) {
        c->m_jumpStrength = *val;
        // 同步 AttributeMap（B 类属性镜像）。load 期实体已构造完成，attributes() 可用。
        auto* living = dynamic_cast<::mc::LivingEntity*>(&entity);
        if (living != nullptr) {
            living->attributes().setBaseValue(::mc::entity::attribute::Attributes::HORSE_JUMP_STRENGTH, *val);
        }
    }
    return {};
}

// ============================================================================
// HorseStatusComponent — Tame/Bred/Saddle/EatingHaystack（i8 bool）
// priority=10：晚于 HorseTaming(0) load。全部走 setter 触发 _syncStatusFlags 写 STATUS_PARAM。
// NBT 中 Tame 与 OwnerUUID 一致，ownerUuid 联动的 setTame 覆盖幂等。
// ============================================================================

static void saveHorseStatus(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::HorseStatusComponent>();
    if (c == nullptr) {
        return;
    }
    // NBT 中布尔值使用 i8 存储
    if (c->m_tame) {
        tag.put("Tame", static_cast<i8>(1));
    }
    if (c->m_bred) {
        tag.put("Bred", static_cast<i8>(1));
    }
    if (c->m_saddled) {
        tag.put("Saddle", static_cast<i8>(1));
    }
    if (c->m_eating) {
        tag.put(nbt_keys::EATING_HAYSTACK, static_cast<i8>(1));
    }
}

static Result<void> loadHorseStatus(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::HorseStatusComponent>();
    if (c == nullptr) {
        return {};
    }
    auto* horse = dynamic_cast<::mc::AbstractHorseEntity*>(&entity);
    if (horse == nullptr) {
        return {};
    }
    // 全部走 setter（C 类 STATUS_PARAM 镜像副作用硬约束）
    if (auto val = nbt_helper::tryGetBool(tag, "Tame")) {
        horse->setTame(*val);
    }
    if (auto val = nbt_helper::tryGetBool(tag, "Bred")) {
        horse->setBred(*val);
    }
    if (auto val = nbt_helper::tryGetBool(tag, "Saddle")) {
        horse->setSaddle(*val);
    }
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::EATING_HAYSTACK)) {
        horse->setEating(*val);
    }
    return {};
}

// ============================================================================
// HorseAttributeComponent — Speed/HorseHealth（f32）
// priority=20：最后 load，同步 AttributeMap（MAX_HEALTH/MOVEMENT_SPEED）。
// ============================================================================

static void saveHorseAttribute(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto* c = entity.tryGetComponent<ecs::HorseAttributeComponent>();
    if (c == nullptr) {
        return;
    }
    tag.put(nbt_keys::HORSE_SPEED, c->m_speed);
    tag.put(nbt_keys::HORSE_HEALTH, c->m_horseHealth);
}

static Result<void> loadHorseAttribute(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* c = entity.tryGetComponent<ecs::HorseAttributeComponent>();
    if (c == nullptr) {
        return {};
    }
    auto* living = dynamic_cast<::mc::LivingEntity*>(&entity);
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HORSE_SPEED)) {
        c->m_speed = *val;
        if (living != nullptr) {
            living->attributes().setBaseValue(::mc::entity::attribute::Attributes::MOVEMENT_SPEED, *val);
        }
    }
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HORSE_HEALTH)) {
        c->m_horseHealth = *val;
        if (living != nullptr) {
            living->attributes().setBaseValue(::mc::entity::attribute::Attributes::MAX_HEALTH, *val);
        }
    }
    return {};
}

// ============================================================================
// 注册
// ============================================================================

void registerHorseComponentSerializers(ComponentSerializerRegistry& registry)
{
    // priority 分层：Taming=0/Jump=0 先 load（ownerUuid 联动 setTame；jumpStrength 同步 AttributeMap），
    //               Status=10 后 load（tame/bred/saddle/eating 走 setter 写 STATUS_PARAM），
    //               Attribute=20 最后 load（speed/horseHealth 同步 AttributeMap）。
    registry.registerSerializer<ecs::HorseTamingComponent>(saveHorseTaming, loadHorseTaming, 0);
    registry.registerSerializer<ecs::HorseJumpComponent>(saveHorseJump, loadHorseJump, 0);
    registry.registerSerializer<ecs::HorseStatusComponent>(saveHorseStatus, loadHorseStatus, 10);
    registry.registerSerializer<ecs::HorseAttributeComponent>(saveHorseAttribute, loadHorseAttribute, 20);
}

} // namespace mc::entity::serialization::components
