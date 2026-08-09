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

#include "common/entity/serialization/components/EntityComponentSerialization.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityFlags.hpp"
#include "common/entity/ecs/components/EntityFlagsComponent.hpp"
#include "common/entity/ecs/components/EntityRotationComponent.hpp"
#include "common/entity/ecs/components/EntityStateComponent.hpp"
#include "common/entity/ecs/components/FireComponent.hpp"
#include "common/entity/ecs/components/FreezeComponent.hpp"
#include "common/entity/ecs/components/PhysicsStateComponent.hpp"
#include "common/entity/ecs/components/PortalComponent.hpp"
#include "common/entity/ecs/components/StateVectorComponent.hpp"
#include "common/entity/ecs/components/VelocityComponent.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <algorithm>

namespace mc::entity::serialization::components {

// ============================================================================
// StateVectorComponent — Pos（位置 double list）
// ============================================================================

static void saveStateVector(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto& pos = entity.position();
    nbt_helper::putDoubleList(
        tag, nbt_keys::POS, {static_cast<f64>(pos.x), static_cast<f64>(pos.y), static_cast<f64>(pos.z)});
}

static Result<void> loadStateVector(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto pos = nbt_helper::getDoubleList(tag, nbt_keys::POS);
    if (pos.size() < 3) {
        return {};
    }
    // 直写组件绕过 setPosition 副作用（setPosition 会污染 m_posPrev + 重建 AABB），
    // 与原 Entity::readFromNBT 直写 m_builtIn.stateVector->m_pos 语义一致。
    auto* stateVector = entity.tryGetComponent<ecs::StateVectorComponent>();
    if (stateVector == nullptr) {
        return {};
    }
    stateVector->m_pos.x = static_cast<f32>(pos[0]);
    stateVector->m_pos.y = static_cast<f32>(pos[1]);
    stateVector->m_pos.z = static_cast<f32>(pos[2]);
    return {};
}

// ============================================================================
// VelocityComponent — Motion（运动 double list，分量限 ±10.0）
// ============================================================================

static void saveVelocity(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const auto vel = entity.velocity();
    nbt_helper::putDoubleList(
        tag, nbt_keys::MOTION, {static_cast<f64>(vel.x), static_cast<f64>(vel.y), static_cast<f64>(vel.z)});
}

static Result<void> loadVelocity(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto motion = nbt_helper::getDoubleList(tag, nbt_keys::MOTION);
    if (motion.size() < 3) {
        return {};
    }
    // 运动分量限制在 ±10.0（对齐原 Entity::readFromNBT clamp 逻辑）
    entity.setVelocity(static_cast<f32>(std::clamp(motion[0], -10.0, 10.0)),
        static_cast<f32>(std::clamp(motion[1], -10.0, 10.0)),
        static_cast<f32>(std::clamp(motion[2], -10.0, 10.0)));
    return {};
}

// ============================================================================
// EntityRotationComponent — Rotation（旋转 float list: yaw, pitch）
// ============================================================================

static void saveRotation(const Entity& entity, nbt::tags::compound_tag& tag)
{
    nbt_helper::putFloatList(tag, nbt_keys::ROTATION, {entity.yaw(), entity.pitch()});
}

static Result<void> loadRotation(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto rotation = nbt_helper::getFloatList(tag, nbt_keys::ROTATION);
    if (rotation.size() < 2) {
        return {};
    }
    // 直写组件绕过 setRotation 副作用（setRotation 会污染 m_rotPrev），
    // 与原 Entity::readFromNBT 直写 m_builtIn.rotation->m_rot 语义一致。
    auto* rot = entity.tryGetComponent<ecs::EntityRotationComponent>();
    if (rot == nullptr) {
        return {};
    }
    rot->m_rot.x = rotation[0];
    rot->m_rot.y = rotation[1];

    // 同步身体/头部旋转为 yaw（对齐 MC 1.21.11 Entity#load）
    // MC 在加载 NBT 后调用 setYHeadRot(getYRot()) / setYBodyRot(getYRot())，保证从存档/
    // 结构模板 NBT 加载的实体身体与头部朝向与 yaw 一致，而非保持构造初值 0。
    // Entity 基类 setYBodyRot/setYHeadRot 默认空实现，LivingEntity 子类重写后才真正写入字段。
    entity.setYHeadRot(rotation[0]);
    entity.setYBodyRot(rotation[0]);
    return {};
}

// ============================================================================
// PhysicsStateComponent — FallDistance + OnGround
// ============================================================================

static void savePhysicsState(const Entity& entity, nbt::tags::compound_tag& tag)
{
    tag.put(nbt_keys::FALL_DISTANCE, entity.fallDistance());
    tag.put(nbt_keys::ON_GROUND, static_cast<i8>(entity.onGround() ? 1 : 0));
}

static Result<void> loadPhysicsState(Entity& entity, const nbt::tags::compound_tag& tag)
{
    // 坠落距离：setFallDistance 纯直写无副作用
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::FALL_DISTANCE)) {
        entity.setFallDistance(*val);
    }
    // 地面标记：直写组件绕过 setOnGround 副作用（落地瞬间会清空 m_lastClimbPos），
    // 与原 Entity::readFromNBT 直写 m_builtIn.physicsState->m_onGround 语义一致。
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::ON_GROUND)) {
        auto* physics = entity.tryGetComponent<ecs::PhysicsStateComponent>();
        if (physics != nullptr) {
            physics->m_onGround = *val;
        }
    }
    return {};
}

// ============================================================================
// FireComponent — Fire（火焰剩余 tick，i16）
// ============================================================================

static void saveFire(const Entity& entity, nbt::tags::compound_tag& tag)
{
    tag.put(nbt_keys::FIRE, static_cast<i16>(entity.getRemainingFireTicks()));
}

static Result<void> loadFire(Entity& entity, const nbt::tags::compound_tag& tag)
{
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::FIRE)) {
        entity.setRemainingFireTicks(*val);
    }
    return {};
}

// ============================================================================
// PortalComponent — PortalCooldown（传送门冷却，i32）
// ============================================================================

static void savePortal(const Entity& entity, nbt::tags::compound_tag& tag)
{
    tag.put(nbt_keys::PORTAL_COOLDOWN, entity.portalCooldown());
}

static Result<void> loadPortal(Entity& entity, const nbt::tags::compound_tag& tag)
{
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::PORTAL_COOLDOWN)) {
        entity.setPortalCooldown(*val);
    }
    return {};
}

// ============================================================================
// FreezeComponent — TicksFrozen（冰冻计时器，i32，仅当 > 0 时保存）
// ============================================================================

static void saveFreeze(const Entity& entity, nbt::tags::compound_tag& tag)
{
    const i32 ticksFrozen = entity.getTicksFrozen();
    if (ticksFrozen > 0) {
        tag.put(nbt_keys::TICKS_FROZEN, ticksFrozen);
    }
}

static Result<void> loadFreeze(Entity& entity, const nbt::tags::compound_tag& tag)
{
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::TICKS_FROZEN)) {
        entity.setTicksFrozen(*val);
    }
    return {};
}

// ============================================================================
// EntityStateComponent — Air + CustomName + CustomNameVisible + Silent + NoGravity
// ============================================================================

static void saveEntityState(const Entity& entity, nbt::tags::compound_tag& tag)
{
    // 空气剩余 tick（i16）
    tag.put(nbt_keys::AIR, static_cast<i16>(entity.air()));

    // 自定义名称（仅当存在时保存）
    if (entity.hasCustomName()) {
        tag.put(nbt_keys::CUSTOM_NAME, entity.customNameText());
    }

    // 自定义名称可见 / 静默 / 无重力（byte 0/1）
    tag.put(nbt_keys::CUSTOM_NAME_VISIBLE, static_cast<i8>(entity.isCustomNameVisible() ? 1 : 0));
    tag.put(nbt_keys::SILENT, static_cast<i8>(entity.isSilent() ? 1 : 0));
    tag.put(nbt_keys::NO_GRAVITY, static_cast<i8>(entity.hasNoGravity() ? 1 : 0));
}

static Result<void> loadEntityState(Entity& entity, const nbt::tags::compound_tag& tag)
{
    // 空气
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::AIR)) {
        entity.setAir(static_cast<i32>(*val));
    }
    // 自定义名称
    if (auto val = nbt_helper::tryGetString(tag, nbt_keys::CUSTOM_NAME)) {
        entity.setCustomName(*val);
    }
    // 自定义名称可见
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::CUSTOM_NAME_VISIBLE)) {
        entity.setCustomNameVisible(*val);
    }
    // 静默
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::SILENT)) {
        entity.setSilent(*val);
    }
    // 无重力
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::NO_GRAVITY)) {
        entity.setNoGravity(*val);
    }
    return {};
}

// ============================================================================
// EntityFlagsComponent — FallFlying（鞘翅飞行标志，byte 0/1）
// 原在 LivingEntity 层处理，本批上提为按 EntityFlagsComponent 注册的自由函数
// （仅依赖 Entity 基类 public 接口 isElytraFlying/addFlag/removeFlag）。
// ============================================================================

static void saveFallFlying(const Entity& entity, nbt::tags::compound_tag& tag)
{
    tag.put(nbt_keys::FALL_FLYING, static_cast<i8>(entity.isElytraFlying() ? 1 : 0));
}

static Result<void> loadFallFlying(Entity& entity, const nbt::tags::compound_tag& tag)
{
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::FALL_FLYING)) {
        if (*val) {
            entity.addFlag(EntityFlags::FallFlying);
        } else {
            entity.removeFlag(EntityFlags::FallFlying);
        }
    }
    return {};
}

// ============================================================================
// 注册
// ============================================================================

void registerEntityComponentSerializers(ComponentSerializerRegistry& registry)
{
    registry.registerSerializer<ecs::StateVectorComponent>(saveStateVector, loadStateVector);
    registry.registerSerializer<ecs::VelocityComponent>(saveVelocity, loadVelocity);
    registry.registerSerializer<ecs::EntityRotationComponent>(saveRotation, loadRotation);
    registry.registerSerializer<ecs::PhysicsStateComponent>(savePhysicsState, loadPhysicsState);
    registry.registerSerializer<ecs::FireComponent>(saveFire, loadFire);
    registry.registerSerializer<ecs::PortalComponent>(savePortal, loadPortal);
    registry.registerSerializer<ecs::FreezeComponent>(saveFreeze, loadFreeze);
    registry.registerSerializer<ecs::EntityStateComponent>(saveEntityState, loadEntityState);
    registry.registerSerializer<ecs::EntityFlagsComponent>(saveFallFlying, loadFallFlying);
}

} // namespace mc::entity::serialization::components
