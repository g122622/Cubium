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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "BubblePopParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

BubblePopParticle::BubblePopParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // quadSize = random.nextFloat() * 0.5F + 0.5F) * 0.1F * 2.0F
    // 即 0.1 ~ 0.2
    setSize(0.1 * (m_random.nextFloat() * 0.5 + 0.5) * 2.0);

    // 速度直接使用传入值，不做额外随机偏移
    // 颜色：白色不透明（继承默认值）
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 重力 0.008（原版值）
    setGravity(BUBBLE_POP_GRAVITY);

    // 摩擦系数（原版 Particle 默认值 0.98，但 BubblePopParticle 的 tick
    // 不使用摩擦，直接移动并做碰撞检测）
    setFriction(0.98);

    // 有碰撞检测
    setHasPhysics(true);

    // 生命周期 4 tick
    setMaxAge(DEFAULT_LIFETIME);
}

std::unique_ptr<Particle> BubblePopParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<BubblePopParticle>(pos, velocity);
}

void BubblePopParticle::tick(mc::client::ClientWorld* world)
{
    // 保存上一帧位置
    m_prevPosition = m_position;

    // 生命周期递增
    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 重力影响（原版：yd -= gravity）
    m_velocity.y -= static_cast<f32>(m_gravity);

    // 碰撞移动（原版使用 move()，hasPhysics = true）
    if (m_hasPhysics && world != nullptr) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
    }

    // 注意：BubblePopParticle 不应用摩擦衰减（原版 tick 中没有 xd *= friction）。
    // 但由于 move() 会将碰撞轴速度归零，这里不需要额外处理。
    // 对于非碰撞轴，速度保持不变（与原版一致）。
}

ResourceLocation BubblePopParticle::getTextureLocation() const
{
    // 根据生命周期进度选择动画帧
    // bubble_pop 有 5 帧：bubble_pop_0 ~ bubble_pop_4
    i32 frame = static_cast<i32>((m_age / m_maxAge) * static_cast<f64>(FRAME_COUNT));
    frame = std::min(frame, FRAME_COUNT - 1);
    return ResourceLocation("minecraft:particle/bubble_pop_" + std::to_string(frame));
}

} // namespace mc::client::renderer::trident::particle::particles
