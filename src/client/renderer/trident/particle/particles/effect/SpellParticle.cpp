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

#include "SpellParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// SpellParticle
// ============================================================================

SpellParticle::SpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.04f);
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 设置颜色，添加一点随机性
    f32 r = color.r * (0.8f + m_random.nextFloat() * 0.2f);
    f32 g = color.g * (0.8f + m_random.nextFloat() * 0.2f);
    f32 b = color.b * (0.8f + m_random.nextFloat() * 0.2f);
    setColor(glm::vec4(r, g, b, 0.8f));

    // 轻微向上漂浮
    m_velocity.y += 0.01f;
}

std::unique_ptr<Particle> SpellParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 从 velocity 中提取 RGB 颜色信息
    // velocity.x = R, velocity.y = G, velocity.z = B，范围为 [0,1]
    // 若 velocity 为零向量（未指定颜色），回退到默认紫色
    f32 r = static_cast<f32>(velocity.x);
    f32 g = static_cast<f32>(velocity.y);
    f32 b = static_cast<f32>(velocity.z);
    if (r == 0.0f && g == 0.0f && b == 0.0f) {
        // 默认紫色（药水效果粒子）
        r = 0.5f;
        g = 0.0f;
        b = 1.0f;
    }
    return std::make_unique<SpellParticle>(pos, velocity, glm::vec4(r, g, b, 1.0f));
}

void SpellParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向上漂浮
    m_velocity.y += 0.002f;

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

// ============================================================================
// InstantSpellParticle
// ============================================================================

InstantSpellParticle::InstantSpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.02f);
    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 设置颜色，更亮
    setColor(glm::vec4(color.r * 1.2f, color.g * 1.2f, color.b * 1.2f, 1.0f));
    m_velocity.y += 0.02f;
}

std::unique_ptr<Particle> InstantSpellParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 从 velocity 中提取 RGB 颜色信息
    // velocity.x = R, velocity.y = G, velocity.z = B，范围为 [0,1]
    // 若 velocity 为零向量（未指定颜色），回退到默认白色
    f32 r = static_cast<f32>(velocity.x);
    f32 g = static_cast<f32>(velocity.y);
    f32 b = static_cast<f32>(velocity.z);
    if (r == 0.0f && g == 0.0f && b == 0.0f) {
        // 默认白色（即时药水效果粒子）
        r = 1.0f;
        g = 1.0f;
        b = 1.0f;
    }
    return std::make_unique<InstantSpellParticle>(pos, velocity, glm::vec4(r, g, b, 1.0f));
}

void InstantSpellParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    m_velocity.y += 0.005f;
    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(1.0f - lifeRatio);
}

// ============================================================================
// EntityEffectParticle
// ============================================================================

EntityEffectParticle::EntityEffectParticle(
    const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color, bool ambient)
    : Particle(pos, velocity)
    , m_ambient(ambient)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.03f);
    setHasPhysics(false);

    if (m_ambient) {
        // 环境模式：alpha=38/255≈0.149，更慢的漂浮速度，更长的生命周期
        setFriction(0.98f);
        setMaxAge(AMBIENT_LIFETIME + m_random.nextFloat() * 6.0);
        setColor(glm::vec4(color.r, color.g, color.b, AMBIENT_ALPHA));
    } else {
        // 普通模式：alpha=0.5
        setFriction(0.95f);
        setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);
        setColor(color);
    }
}

std::unique_ptr<Particle> EntityEffectParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 从 velocity 中提取 RGB 颜色信息（与 SpellcastingIllagerEntity 等实体
    // 通过 addParticle(ParticleTypeId::EntityEffect, pos, colorVector) 传递颜色的方式一致）
    // velocity.x = R, velocity.y = G, velocity.z = B，范围为 [0,1]
    // 若 velocity 为零向量（未指定颜色），回退到默认紫色
    f32 r = static_cast<f32>(velocity.x);
    f32 g = static_cast<f32>(velocity.y);
    f32 b = static_cast<f32>(velocity.z);
    if (r == 0.0f && g == 0.0f && b == 0.0f) {
        // 默认紫色（药水效果粒子）
        r = 0.5f;
        g = 0.0f;
        b = 0.5f;
    }
    return std::make_unique<EntityEffectParticle>(pos, velocity, glm::vec4(r, g, b, 0.5f));
}

std::unique_ptr<Particle> EntityEffectParticle::createAmbient(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 从 velocity 中提取 RGB 颜色信息，与 create() 相同
    // 但使用环境模式：更低的 alpha (38/255≈0.149)，更慢的漂浮，更长的生命周期
    f32 r = static_cast<f32>(velocity.x);
    f32 g = static_cast<f32>(velocity.y);
    f32 b = static_cast<f32>(velocity.z);
    if (r == 0.0f && g == 0.0f && b == 0.0f) {
        // 默认蓝色（信标效果粒子）
        r = 0.5f;
        g = 0.5f;
        b = 1.0f;
    }
    return std::make_unique<EntityEffectParticle>(pos, velocity, glm::vec4(r, g, b, 1.0f), true);
}

std::unique_ptr<Particle> EntityEffectParticle::createWithColor(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world, const glm::vec4& color)
{
    MC_UNUSED(world);
    // 直接使用传入的 RGBA 颜色构造粒子（来自粒子数据管线的 EntityEffectParticleData）
    return std::make_unique<EntityEffectParticle>(pos, velocity, color, false);
}

void EntityEffectParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    if (m_ambient) {
        // 环境模式：更慢的漂浮和漂移速度
        m_velocity.y += 0.001f;
        m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.001f;
        m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.001f;
    } else {
        // 普通模式
        m_velocity.y += 0.002f;
        m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.003f;
        m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.003f;
    }

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出：根据环境/普通模式使用不同的基础 alpha
    f64 lifeRatio = m_age / m_maxAge;
    f32 baseAlpha = m_ambient ? AMBIENT_ALPHA : 0.5f;
    m_color.a = static_cast<f32>(baseAlpha * (1.0f - lifeRatio));
}

// ============================================================================
// WitchParticle
// ============================================================================

WitchParticle::WitchParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.04f);
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 紫色
    f32 purpleIntensity = 0.6f + m_random.nextFloat() * 0.4f;
    setColor(glm::vec4(purpleIntensity * 0.8f, 0.0f, purpleIntensity, 0.8f));

    m_velocity.y += 0.01f;
}

std::unique_ptr<Particle> WitchParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<WitchParticle>(pos, velocity);
}

void WitchParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    m_velocity.y += 0.002f;
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

} // namespace mc::client::renderer::trident::particle::particles
