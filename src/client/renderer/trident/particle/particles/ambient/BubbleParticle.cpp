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
#include "client/world/ClientWorld.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"

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

} // namespace mc::client::renderer::trident::particle::particles
