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

#include "SnowParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/core/Types.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/block/BlockState.hpp"
#include <cmath>
#include <memory>
#include <vector>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

namespace {

constexpr f32 SNOW_BBOX_WIDTH = 0.02f;
constexpr f32 SNOW_BBOX_HEIGHT = 0.02f;
constexpr f32 TERMINAL_VELOCITY = -0.5f; // 雪花终端速度（比雨慢）

/**
 * @brief 检查雪花包围盒是否与地面方块相交
 *
 * @param world 客户端世界
 * @param box 用于检测的粒子包围盒
 * @return 如果命中可碰撞方块则返回 true
 */
[[nodiscard]] bool hasGroundCollision(mc::client::ClientWorld* world, const AxisAlignedBB& box)
{
    MC_ASSERT_RELEASE(world != nullptr);

    const i32 minX = mc::math::floorTo<i32>(box.minX);
    const i32 maxX = mc::math::floorTo<i32>(box.maxX);
    const i32 minY = mc::math::floorTo<i32>(box.minY);
    const i32 maxY = mc::math::floorTo<i32>(box.maxY);
    const i32 minZ = mc::math::floorTo<i32>(box.minZ);
    const i32 maxZ = mc::math::floorTo<i32>(box.maxZ);

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                const mc::BlockState* state = world->getBlockState(x, y, z);
                if (state == nullptr || state->isAir()) {
                    continue;
                }

                const mc::CollisionShape& shape = state->getCollisionShape();
                if (!shape.isEmpty() && shape.intersects(box, x, y, z)) {
                    return true;
                }
            }
        }
    }

    return false;
}

} // namespace

SnowParticle::SnowParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_swingPhase(0.0f)
    , m_swingAmplitude(SWING_AMPLITUDE)
{
    // 随机初始相位和振幅
    m_swingPhase = m_random.nextFloat() * mc::math::TWO_PI;
    m_swingAmplitude = SWING_AMPLITUDE * (0.5f + m_random.nextFloat());

    // 雪花参数
    setGravity(physics::SNOW_GRAVITY);
    setSize(0.05f + m_random.nextFloat() * 0.05f); // 0.05 - 0.1
    setBoundingBox(SNOW_BBOX_WIDTH, SNOW_BBOX_HEIGHT);
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.9f)); // 白色几乎不透明
    setFriction(0.95f);
    setHasPhysics(false); // 雪花使用自定义碰撞检测

    // 雪花生命周期较长
    f64 lifeMultiplier = 0.8f + m_random.nextFloat() * 0.2f;
    setMaxAge(200.0f / lifeMultiplier);
}

std::unique_ptr<Particle> SnowParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SnowParticle>(pos, velocity);
}

void SnowParticle::tick(mc::client::ClientWorld* world)
{
    // 保存上一帧位置
    m_prevPosition = m_position;

    // 年龄增加
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 限制下落速度（终端速度）
    if (m_velocity.y < TERMINAL_VELOCITY) {
        m_velocity.y = TERMINAL_VELOCITY;
    }

    // 雪花摇摆效果（正弦波水平漂移）
    m_swingPhase += SWING_FREQUENCY;
    f64 swing = std::sin(m_swingPhase) * m_swingAmplitude;
    m_velocity.x += static_cast<f32>(swing * 0.01);

    // 重置碰撞状态
    m_collisionContext.reset();

    // 检查地面碰撞
    if (world != nullptr) {
        AxisAlignedBB bbox = getBoundingBox();

        // 稍微向下探测，避免刚好贴着方块表面时漏检
        AxisAlignedBB probeBox(
            bbox.minX, bbox.minY - mc::math::EPSILON_GROUND_PROBE, bbox.minZ, bbox.maxX, bbox.minY, bbox.maxZ);

        if (hasGroundCollision(world, probeBox)) {
            m_collisionContext.onGround = true;
            m_collisionContext.collidedY = true;
            m_velocity.y = 0.0f;

            // 雪花落地后立即消失
            setExpired();
            return;
        }
    }

    // 应用速度
    m_position += m_velocity;

    // 应用阻力
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 根据年龄淡出（雪花在生命后半段淡出）
    if (m_age > m_maxAge * 0.8f) {
        f64 fadeProgress = (m_age - m_maxAge * 0.8f) / (m_maxAge * 0.2f);
        m_color.a = static_cast<f32>(0.9f * (1.0f - fadeProgress));
    }
}

void SnowParticle::buildVertices(const glm::vec3& cameraPos,
    f64 partialTick,
    const ParticleTextureAtlas& atlas,
    std::vector<ParticleVertex>& outVertices) const
{
    // 使用基类实现（支持纹理图集）
    Particle::buildVertices(cameraPos, partialTick, atlas, outVertices);
}

} // namespace mc::client::renderer::trident::particle::particles
