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

#include "BubbleParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

BubbleParticle::BubbleParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // 尺寸 = 0.02 + rand * 0.02
    setSize(0.02f + m_random.nextFloat() * 0.02f);

    // 速度缩放 0.2 倍加上随机偏移
    m_velocity.x = m_velocity.x * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.y = m_velocity.y * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.z = m_velocity.z * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;

    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));

    setFriction(0.85f);
    setHasPhysics(false);

    // 生命周期 = (int)(8.0 / (rand.nextDouble() * 0.8 + 0.2))
    setMaxAge(static_cast<f64>(static_cast<i32>(8.0 / (m_random.nextFloat() * 0.8f + 0.2f))));
}

std::unique_ptr<Particle> BubbleParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<BubbleParticle>(pos, velocity);
}

void BubbleParticle::tick(mc::client::ClientWorld* world)
{
    // 保存上一帧位置（用于插值）
    m_prevPosition = m_position;

    // 生命周期递增
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 气泡浮力
    m_velocity.y += 0.005f;

    // 摩擦衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 直接移动，不做碰撞检测
    m_position += m_velocity;

    // 检查是否离开水面，如果不在水中则消失
    if (world != nullptr) {
        i32 blockX = mc::math::floorTo<i32>(m_position.x);
        i32 blockY = mc::math::floorTo<i32>(m_position.y);
        i32 blockZ = mc::math::floorTo<i32>(m_position.z);

        const BlockState* state = world->getBlockState(blockX, blockY, blockZ);
        if (state != nullptr && !state->isAir()) {
            const mc::fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState == nullptr || fluidState->isEmpty() ||
                !fluidState->getFluid().isIn(mc::fluid::FluidTags::WATER())) {
                // 气泡离开水面时，生成 BubblePop 粒子
                if (m_emitCallback) {
                    m_emitCallback(ParticleTypeId::BubblePop, m_position, glm::vec3(0.0f, 0.0f, 0.0f));
                }
                setExpired();
                return;
            }
        } else {
            // 不在方块中（空气），气泡破裂，生成 BubblePop 粒子
            if (m_emitCallback) {
                m_emitCallback(ParticleTypeId::BubblePop, m_position, glm::vec3(0.0f, 0.0f, 0.0f));
            }
            setExpired();
            return;
        }
    }
}

// ============================================================================
// CurrentDownParticle
// ============================================================================

CurrentDownParticle::CurrentDownParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // 与 BubbleParticle 相同的尺寸
    setSize(0.02f + m_random.nextFloat() * 0.02f);

    // 速度缩放 0.2 倍加上随机偏移
    m_velocity.x = m_velocity.x * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.y = m_velocity.y * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.z = m_velocity.z * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;

    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));

    setFriction(0.85f);
    setHasPhysics(false);

    // 生命周期 = (int)(8.0 / (rand * 0.8 + 0.2))，与 BubbleParticle 相同
    setMaxAge(static_cast<f64>(static_cast<i32>(8.0 / (m_random.nextFloat() * 0.8f + 0.2f))));
}

std::unique_ptr<Particle> CurrentDownParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CurrentDownParticle>(pos, velocity);
}

void CurrentDownParticle::tick(mc::client::ClientWorld* world)
{
    // 保存上一帧位置
    m_prevPosition = m_position;

    // 生命周期递增
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向下移动（与 BubbleParticle 方向相反）
    m_velocity.y -= 0.005f;

    // 摩擦衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 直接移动，不做碰撞检测
    m_position += m_velocity;

    // 检查是否离开水面：如果不在水中则消失（不生成 BubblePop）
    if (world != nullptr) {
        i32 blockX = mc::math::floorTo<i32>(m_position.x);
        i32 blockY = mc::math::floorTo<i32>(m_position.y);
        i32 blockZ = mc::math::floorTo<i32>(m_position.z);

        const BlockState* state = world->getBlockState(blockX, blockY, blockZ);
        if (state != nullptr && !state->isAir()) {
            const mc::fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState == nullptr || fluidState->isEmpty() ||
                !fluidState->getFluid().isIn(mc::fluid::FluidTags::WATER())) {
                // 离开水面，直接消失
                setExpired();
                return;
            }
        } else {
            // 不在方块中（空气），直接消失
            setExpired();
            return;
        }
    }
}

// ============================================================================
// BubbleColumnUpParticle
// ============================================================================

BubbleColumnUpParticle::BubbleColumnUpParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // 与 BubbleParticle 相同的尺寸
    setSize(0.02f + m_random.nextFloat() * 0.02f);

    // 速度缩放 0.2 倍加上随机偏移
    m_velocity.x = m_velocity.x * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.y = m_velocity.y * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.z = m_velocity.z * 0.2f + (m_random.nextFloat() * 2.0f - 1.0f) * 0.02f;

    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));

    setFriction(0.85f);
    setHasPhysics(false);

    // 生命周期 = (int)(8.0 / (rand * 0.8 + 0.2))，与 BubbleParticle 相同
    setMaxAge(static_cast<f64>(static_cast<i32>(8.0 / (m_random.nextFloat() * 0.8f + 0.2f))));
}

std::unique_ptr<Particle> BubbleColumnUpParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<BubbleColumnUpParticle>(pos, velocity);
}

void BubbleColumnUpParticle::tick(mc::client::ClientWorld* world)
{
    // 保存上一帧位置
    m_prevPosition = m_position;

    // 生命周期递增
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 气泡浮力（与 BubbleParticle 相同，向上）
    m_velocity.y += 0.005f;

    // 摩擦衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 直接移动，不做碰撞检测
    m_position += m_velocity;

    // 检查是否离开水面：静默消失（不生成 BubblePop 粒子）
    if (world != nullptr) {
        i32 blockX = mc::math::floorTo<i32>(m_position.x);
        i32 blockY = mc::math::floorTo<i32>(m_position.y);
        i32 blockZ = mc::math::floorTo<i32>(m_position.z);

        const BlockState* state = world->getBlockState(blockX, blockY, blockZ);
        if (state != nullptr && !state->isAir()) {
            const mc::fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState == nullptr || fluidState->isEmpty() ||
                !fluidState->getFluid().isIn(mc::fluid::FluidTags::WATER())) {
                // 离开水面，静默消失
                setExpired();
                return;
            }
        } else {
            // 不在方块中（空气），静默消失
            setExpired();
            return;
        }
    }
}

} // namespace mc::client::renderer::trident::particle::particles
