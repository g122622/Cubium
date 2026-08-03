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

#include "EmitterParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// EmitterParticle 基类
// ============================================================================

EmitterParticle::EmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime)
    : Particle(pos, velocity)
{
    setMaxAge(lifetime);
    setHasPhysics(false);
}

EmitterParticle::EmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime, u32 emitCount)
    : Particle(pos, velocity)
    , m_emitCount(emitCount)
{
    setMaxAge(lifetime);
    setHasPhysics(false);
}

void EmitterParticle::tick(mc::client::ClientWorld* world)
{
    // 调用父类 tick（增加年龄）
    Particle::tick(world);

    // 增加发射计时器
    ++m_ticksSinceLastEmit;
}

void EmitterParticle::emit(
    mc::client::ClientWorld* world, ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity)
{
    // 使用 Particle 基类的发射回调
    if (emitCallback()) {
        emitCallback()(type, pos, velocity);
        return;
    }

    // emitCallback 由 ParticleManager::tick() 在每个粒子 tick 前设置，
    // 如果此处回调为空，说明此粒子未被 ParticleManager 管理（编程错误）
    MC_ASSERT_RELEASE_MSG(
        false, "EmitterParticle::emit() called without emitCallback set - particle not managed by ParticleManager?");
}

void EmitterParticle::emitWithOffset(mc::client::ClientWorld* world,
    ParticleTypeId type,
    const glm::vec3& center,
    const glm::vec3& offset,
    const glm::vec3& baseVelocity,
    const glm::vec3& velocitySpread)
{
    // 随机偏移
    glm::vec3 pos(center.x + (m_random.nextFloat() * 2.0f - 1.0f) * offset.x,
        center.y + (m_random.nextFloat() * 2.0f - 1.0f) * offset.y,
        center.z + (m_random.nextFloat() * 2.0f - 1.0f) * offset.z);

    // 随机速度
    glm::vec3 vel(baseVelocity.x + (m_random.nextFloat() * 2.0f - 1.0f) * velocitySpread.x,
        baseVelocity.y + (m_random.nextFloat() * 2.0f - 1.0f) * velocitySpread.y,
        baseVelocity.z + (m_random.nextFloat() * 2.0f - 1.0f) * velocitySpread.z);

    emit(world, type, pos, vel);
}

bool EmitterParticle::shouldEmit() const
{
    // 检查是否达到发射间隔
    if (m_ticksSinceLastEmit < m_emitInterval) {
        return false;
    }

    // 检查是否有剩余发射次数（0 表示无限）
    if (m_emitCount > 0) {
        return true; // 由子类减少计数
    }

    return m_emitCount == 0; // 0 表示无限发射
}

// ============================================================================
// HugeExplosionEmitterParticle
// ============================================================================

HugeExplosionEmitterParticle::HugeExplosionEmitterParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : EmitterParticle(pos, velocity, EMITTER_LIFETIME, 1)
{
    m_emitInterval = EMIT_DELAY;
}

std::unique_ptr<Particle> HugeExplosionEmitterParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<HugeExplosionEmitterParticle>(pos, velocity);
}

void HugeExplosionEmitterParticle::tick(mc::client::ClientWorld* world)
{
    EmitterParticle::tick(world);

    // 在延迟后发射大型爆炸粒子
    if (shouldEmit() && m_emitCount > 0) {
        emit(world, ParticleTypeId::LargeExplosion, position(), glm::vec3(0.0f));
        --m_emitCount;
        m_ticksSinceLastEmit = 0;
    }
}

// ============================================================================
// FlameEmitterParticle
// ============================================================================

FlameEmitterParticle::FlameEmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime, u32 emitCount)
    : EmitterParticle(pos, velocity, lifetime, emitCount)
{
    m_emitInterval = EMIT_INTERVAL;
}

std::unique_ptr<Particle> FlameEmitterParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认发射 10 次，持续 20 tick
    return std::make_unique<FlameEmitterParticle>(pos, velocity, 20.0, 10);
}

void FlameEmitterParticle::tick(mc::client::ClientWorld* world)
{
    EmitterParticle::tick(world);

    if (shouldEmit()) {
        // 发射火焰粒子，带随机偏移
        emitWithOffset(world,
            ParticleTypeId::Flame,
            position(),
            glm::vec3(0.1f, 0.1f, 0.1f),     // 位置偏移
            glm::vec3(0.0f, 0.02f, 0.0f),    // 基础速度（向上）
            glm::vec3(0.01f, 0.01f, 0.01f)); // 速度随机范围

        if (m_emitCount > 0) {
            --m_emitCount;
        }
        m_ticksSinceLastEmit = 0;
    }
}

// ============================================================================
// SmokeEmitterParticle
// ============================================================================

SmokeEmitterParticle::SmokeEmitterParticle(const glm::vec3& pos, const glm::vec3& velocity, f64 lifetime, u32 emitCount)
    : EmitterParticle(pos, velocity, lifetime, emitCount)
{
    m_emitInterval = EMIT_INTERVAL;
}

std::unique_ptr<Particle> SmokeEmitterParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认发射 8 次，持续 24 tick
    return std::make_unique<SmokeEmitterParticle>(pos, velocity, 24.0, 8);
}

void SmokeEmitterParticle::tick(mc::client::ClientWorld* world)
{
    EmitterParticle::tick(world);

    if (shouldEmit()) {
        // 发射烟雾粒子
        emitWithOffset(world,
            ParticleTypeId::Smoke,
            position(),
            glm::vec3(0.2f, 0.1f, 0.2f),
            glm::vec3(0.0f, 0.03f, 0.0f),
            glm::vec3(0.02f, 0.01f, 0.02f));

        if (m_emitCount > 0) {
            --m_emitCount;
        }
        m_ticksSinceLastEmit = 0;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
