/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "VibrationSignalParticle.hpp"

#include <glm/glm.hpp>

#include "client/world/ClientWorld.hpp"
#include "client/world/entity/ClientEntity.hpp"

namespace mc::client::renderer::trident::particle::particles {

VibrationSignalParticle::VibrationSignalParticle(
    const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks)
    : Particle(pos, glm::vec3(0.0f))
    , m_kind(TargetKind::Block)
    , m_targetPosition(targetPosition)
    , m_arrivalInTicks(arrivalInTicks)
{
    setGravity(0.0f);
    setSize(0.04);
    setHasPhysics(false);

    // 振动粒子颜色：淡蓝色发光
    setColor(glm::vec4(0.75f, 0.85f, 1.0f, 1.0f));

    // 生命周期为到达时间
    setMaxAge(static_cast<f64>(arrivalInTicks));

    setFriction(1.0f);
}

VibrationSignalParticle::VibrationSignalParticle(
    const glm::vec3& pos, EntityId targetEntityId, f32 yOffset, i32 arrivalInTicks)
    : Particle(pos, glm::vec3(0.0f))
    , m_kind(TargetKind::Entity)
    , m_targetEntityId(targetEntityId)
    , m_yOffset(yOffset)
    , m_arrivalInTicks(arrivalInTicks)
{
    setGravity(0.0f);
    setSize(0.04);
    setHasPhysics(false);

    // 振动粒子颜色：淡蓝色发光
    setColor(glm::vec4(0.75f, 0.85f, 1.0f, 1.0f));

    // 生命周期为到达时间
    setMaxAge(static_cast<f64>(arrivalInTicks));

    setFriction(1.0f);
}

std::unique_ptr<Particle> VibrationSignalParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    MC_UNUSED(velocity);
    // 默认工厂：创建一个向正上方飞行 8 tick 的振动粒子
    // 实际使用时应通过 createWithTarget / createWithEntityTarget 创建带目标的粒子
    Vector3d targetPos(pos.x, pos.y + 8.0, pos.z);
    return std::make_unique<VibrationSignalParticle>(pos, targetPos, 8);
}

std::unique_ptr<Particle> VibrationSignalParticle::createWithTarget(
    const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks)
{
    return std::make_unique<VibrationSignalParticle>(pos, targetPosition, arrivalInTicks);
}

std::unique_ptr<Particle> VibrationSignalParticle::createWithEntityTarget(
    const glm::vec3& pos, EntityId targetEntityId, f32 yOffset, i32 arrivalInTicks)
{
    return std::make_unique<VibrationSignalParticle>(pos, targetEntityId, yOffset, arrivalInTicks);
}

void VibrationSignalParticle::tick(mc::client::ClientWorld* world)
{
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 计算剩余 tick 数
    i32 remainingTicks = static_cast<i32>(m_maxAge - m_age);
    if (remainingTicks <= 0) {
        setExpired();
        return;
    }

    // 解析当前目标位置
    // - 方块来源：使用固定目标位置
    // - 实体来源：每 tick 通过 ClientWorld 重新解析实体当前位置
    //   对应 MC Java VibrationSignalParticle.tick() 中
    //   Optional<Vec3> optional = this.target.getPosition(this.level);
    //   if (optional.isEmpty()) { this.remove(); }
    Vector3d currentTarget;
    if (m_kind == TargetKind::Entity) {
        if (world == nullptr) {
            // 无 ClientWorld 可用于解析实体位置，粒子立即过期
            setExpired();
            return;
        }

        const ClientEntity* targetEntity = world->entityManager().getEntity(m_targetEntityId);
        if (targetEntity == nullptr || targetEntity->isRemoved()) {
            // 实体不在客户端视野内或已被标记移除，粒子立即过期
            // 对应 MC Java VibrationSignalParticle.tick 中
            // target.getPosition().isEmpty() -> remove()
            setExpired();
            return;
        }

        currentTarget = Vector3d(static_cast<f64>(targetEntity->x()),
            static_cast<f64>(targetEntity->y()) + static_cast<f64>(m_yOffset),
            static_cast<f64>(targetEntity->z()));
    } else {
        currentTarget = m_targetPosition;
    }

    // 向目标位置插值移动（x = Mth.lerp(1.0 / remainingTicks, x, target.x)）
    // 这会产生指数缓动效果，粒子越接近目标移动越慢
    f64 lerpFactor = 1.0 / static_cast<f64>(remainingTicks);
    m_position.x = static_cast<f32>(glm::mix(static_cast<f64>(m_position.x), currentTarget.x, lerpFactor));
    m_position.y = static_cast<f32>(glm::mix(static_cast<f64>(m_position.y), currentTarget.y, lerpFactor));
    m_position.z = static_cast<f32>(glm::mix(static_cast<f64>(m_position.z), currentTarget.z, lerpFactor));

    // 轻微摆动效果
    m_roll += 0.05;

    // 根据年龄淡出
    f64 ageRatio = m_age / m_maxAge;
    if (ageRatio > 0.7) {
        m_color.a = static_cast<f32>(1.0 - (ageRatio - 0.7) / 0.3);
    }
}

f64 VibrationSignalParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);

    // 根据年龄缩放：开始时小，快速变大，接近结束时缩小
    f64 ageRatio = m_age / m_maxAge;

    if (ageRatio < 0.1) {
        // 开始：从小变大
        return 0.5 + ageRatio / 0.1 * 0.5;
    } else if (ageRatio < 0.8) {
        // 中期：保持正常大小
        return 1.0;
    } else {
        // 末期：逐渐缩小
        return 1.0 - (ageRatio - 0.8) / 0.2 * 0.5;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
