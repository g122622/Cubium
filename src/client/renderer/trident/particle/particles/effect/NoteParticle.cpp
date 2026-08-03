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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "NoteParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <cstdlib>
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

namespace {

/**
 * @brief HSV 转 RGB
 *
 * @param h 色相 [0, 6)
 * @param s 饱和度 [0, 1]
 * @param v 明度 [0, 1]
 * @return RGB 向量，各分量 [0, 1]
 */
glm::vec3 hsvToRgb(f32 h, f32 s, f32 v)
{
    f32 c = v * s;
    f32 x = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
    f32 m = v - c;

    glm::vec3 rgb;
    if (h < 1.0f) {
        rgb = glm::vec3(c, x, 0.0f);
    } else if (h < 2.0f) {
        rgb = glm::vec3(x, c, 0.0f);
    } else if (h < 3.0f) {
        rgb = glm::vec3(0.0f, c, x);
    } else if (h < 4.0f) {
        rgb = glm::vec3(0.0f, x, c);
    } else if (h < 5.0f) {
        rgb = glm::vec3(x, 0.0f, c);
    } else {
        rgb = glm::vec3(c, 0.0f, x);
    }
    return rgb + glm::vec3(m);
}

} // anonymous namespace

NoteParticle::NoteParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0);
    setSize(0.02 + m_random.nextFloat() * 0.01);
    setFriction(0.95);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);
}

std::unique_ptr<Particle> NoteParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    auto particle = std::make_unique<NoteParticle>(pos, velocity);

    // 从 velocity.y 提取音高值（0.0~1.0），编码为彩虹色
    f32 note = mc::math::clamp(static_cast<f32>(velocity.y), 0.0f, 1.0f);

    // 色相 = note * 6.0，将 0~1 映射到色轮上
    f32 hue = note * 6.0f;
    glm::vec3 rgb = hsvToRgb(hue, 1.0f, 1.0f);

    particle->setColor(glm::vec4(rgb.r, rgb.g, rgb.b, 1.0f));

    // 设置向上漂浮速度
    particle->m_velocity.y = 0.02f;
    particle->m_velocity.x = 0.0f;
    particle->m_velocity.z = 0.0f;

    return particle;
}

void NoteParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机左右摆动
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.02f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.02f;

    // 向上漂浮速度逐渐衰减
    m_velocity.y *= 0.98f;

    // 摩擦衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    m_position += m_velocity;

    // 随生命周期淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(1.0f - lifeRatio * 0.5f);
}

} // namespace mc::client::renderer::trident::particle::particles
