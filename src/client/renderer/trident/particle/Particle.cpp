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

#include "Particle.hpp"
#include "ParticleTextureAtlas.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/core/Types.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <functional>
#include <vector>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>

namespace mc::client::renderer::trident::particle {

namespace {
// 默认纹理位置
const ResourceLocation DEFAULT_TEXTURE("minecraft:particle/generic");
} // namespace

Particle::Particle(const glm::vec3& pos, const glm::vec3& velocity)
    : m_position(pos)
    , m_prevPosition(pos)
    , m_velocity(velocity)
    , m_bboxWidth(physics::PARTICLE_DEFAULT_BBOX_WIDTH)
    , m_bboxHeight(physics::PARTICLE_DEFAULT_BBOX_HEIGHT)
    , m_random(static_cast<u64>(
                   std::hash<double>{}(pos.x) ^ (std::hash<double>{}(pos.y) << 1) ^ (std::hash<double>{}(pos.z) << 2)) ^
          mc::util::TimeUtils::getCurrentTimeUs())
{}

void Particle::tick(mc::client::ClientWorld* world)
{
    // 保存上一帧位置和旋转
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    // 年龄增加
    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 重置碰撞状态
    m_collisionContext.reset();

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 移动并碰撞检测
    if (m_hasPhysics && world != nullptr) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
    }

    // 应用空气阻力
    m_velocity *= static_cast<f32>(m_friction);

    // 地面摩擦
    if (m_collisionContext.onGround) {
        m_velocity.x *= physics::PARTICLE_GROUND_FRICTION;
        m_velocity.z *= physics::PARTICLE_GROUND_FRICTION;
    }

    // 根据年龄淡出（生命后半段淡出）
    if (m_age > m_maxAge * 0.5) {
        f64 fadeProgress = (m_age - m_maxAge * 0.5) / (m_maxAge * 0.5);
        m_color.a = static_cast<f32>(1.0 - fadeProgress);
    }
}

void Particle::buildVertices(const glm::vec3& cameraPos,
    f64 partialTick,
    const ParticleTextureAtlas& atlas,
    std::vector<ParticleVertex>& outVertices) const
{
    // 插值位置
    glm::dvec3 interpPos =
        glm::dvec3(m_prevPosition) + (glm::dvec3(m_position) - glm::dvec3(m_prevPosition)) * partialTick;

    // 插值旋转
    f64 interpRoll = m_prevRoll + (m_roll - m_prevRoll) * partialTick;

    // 计算 billboard 基向量
    glm::dvec3 toCamera = glm::dvec3(cameraPos) - interpPos;
    f64 dist = glm::length(toCamera);
    if (dist < 0.001) {
        return; // 太近了，跳过
    }
    toCamera = glm::normalize(toCamera);

    // 计算右向量和上向量（camera-facing billboard）
    glm::dvec3 right = glm::cross(glm::dvec3(0.0, 1.0, 0.0), toCamera);
    if (glm::length(right) < 0.001) {
        // 相机在正上方或正下方
        right = glm::dvec3(1.0, 0.0, 0.0);
    } else {
        right = glm::normalize(right);
    }
    glm::dvec3 up = glm::cross(toCamera, right);

    // 应用旋转
    if (std::abs(interpRoll) > 0.001) {
        f64 cosR = std::cos(interpRoll);
        f64 sinR = std::sin(interpRoll);
        glm::dvec3 newRight = right * cosR + up * sinR;
        glm::dvec3 newUp = -right * sinR + up * cosR;
        right = newRight;
        up = newUp;
    }

    glm::vec3 interpPosF(static_cast<f32>(interpPos.x), static_cast<f32>(interpPos.y), static_cast<f32>(interpPos.z));
    glm::vec3 rightF(static_cast<f32>(right.x), static_cast<f32>(right.y), static_cast<f32>(right.z));
    glm::vec3 upF(static_cast<f32>(up.x), static_cast<f32>(up.y), static_cast<f32>(up.z));

    // 获取缩放
    f64 scale = getScale(partialTick);
    f64 halfSize = m_size * scale * 0.5;
    const f32 halfSizeF = static_cast<f32>(halfSize);

    // 获取 UV 坐标
    glm::vec4 uv(0.0f, 0.0f, 1.0f, 1.0f); // 默认 UV
    const SpriteInfo* sprite = atlas.getSprite(getTextureLocation());
    if (sprite != nullptr) {
        // 对于动画精灵，基于年龄选择帧
        if (sprite->isAnimated()) {
            uv = atlas.getAnimatedFrameUV(getTextureLocation(), m_age, m_maxAge);
        } else {
            uv = glm::vec4(sprite->uvMin.x, sprite->uvMin.y, sprite->uvMax.x, sprite->uvMax.y);
        }
    }

    // 四个顶点（quad）
    // 左下
    outVertices.push_back({interpPosF - rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(uv.x, uv.w), // UV: 左下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 右下
    outVertices.push_back({interpPosF + rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(uv.z, uv.w), // UV: 右下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 右上
    outVertices.push_back({interpPosF + rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(uv.z, uv.y), // UV: 右上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 左上
    outVertices.push_back({interpPosF - rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(uv.x, uv.y), // UV: 左上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
}

ResourceLocation Particle::getTextureLocation() const
{
    return DEFAULT_TEXTURE;
}

u32 Particle::getLightColor(mc::client::ClientWorld* world) const
{
    // 默认实现：从世界采样光照
    if (world == nullptr) {
        return physics::PARTICLE_MAX_PACKED_LIGHT; // 最大亮度
    }

    // 获取粒子位置的方块坐标
    i32 blockX = mc::math::floorTo<i32>(m_position.x);
    i32 blockY = mc::math::floorTo<i32>(m_position.y);
    i32 blockZ = mc::math::floorTo<i32>(m_position.z);

    // 采样世界光照
    u8 blockLight = world->getBlockLight(blockX, blockY, blockZ);
    u8 skyLight = world->getSkyLight(blockX, blockY, blockZ);

    // 组合成 combined light: (skyLight << 4) | blockLight
    return (static_cast<u32>(skyLight) << 4) | static_cast<u32>(blockLight);
}

f64 Particle::getScale(f64 /*partialTick*/) const
{
    // 默认实现：返回 1.0（无缩放）
    // 子类可以重写以实现缩放动画
    return 1.0;
}

void Particle::move(mc::client::ClientWorld* world, const glm::vec3& delta)
{
    // 完整的 AABB 碰撞检测实现

    if (world == nullptr) {
        // 无世界引用，只移动不检测碰撞
        m_position += delta;
        return;
    }

    // 零移动检查
    if (std::abs(delta.x) < physics::PARTICLE_MIN_MOVEMENT && std::abs(delta.y) < physics::PARTICLE_MIN_MOVEMENT &&
        std::abs(delta.z) < physics::PARTICLE_MIN_MOVEMENT) {
        return;
    }

    // 重置碰撞状态
    m_collisionContext.reset();

    // 获取当前碰撞盒
    AxisAlignedBB bbox = getBoundingBox();

    // 使用 PhysicsEngine 进行碰撞检测
    PhysicsEngine physics(*world);

    // 粒子不需要步进（stepHeight = 0）
    Vector3 actualDelta = physics.moveEntity(bbox,
        Vector3(delta.x, delta.y, delta.z),
        0.0f // stepHeight
    );

    // 更新位置（从碰撞盒中心重算）
    m_position.x = (bbox.minX + bbox.maxX) * 0.5f;
    m_position.y = bbox.minY;
    m_position.z = (bbox.minZ + bbox.maxZ) * 0.5f;

    // 更新碰撞状态
    m_collisionContext.collidedX =
        physics.collidedHorizontally() && (std::abs(actualDelta.x - delta.x) > physics::PARTICLE_MIN_MOVEMENT);
    m_collisionContext.collidedY = physics.collidedVertically();
    m_collisionContext.collidedZ =
        physics.collidedHorizontally() && (std::abs(actualDelta.z - delta.z) > physics::PARTICLE_MIN_MOVEMENT);

    // 地面判定：Y 方向被阻挡且原移动向下
    m_collisionContext.onGround = physics.collidedVertically() && delta.y < 0.0f;

    // 碰撞后速度归零
    if (m_collisionContext.collidedX) {
        m_velocity.x = 0.0f;
    }
    if (m_collisionContext.collidedY) {
        m_velocity.y = 0.0f;
    }
    if (m_collisionContext.collidedZ) {
        m_velocity.z = 0.0f;
    }
}

void Particle::setBoundingBox(f64 width, f64 height)
{
    m_bboxWidth = width;
    m_bboxHeight = height;
}

void Particle::setPosition(const glm::vec3& pos)
{
    m_position = pos;
    m_prevPosition = pos;
}

AxisAlignedBB Particle::getBoundingBox() const
{
    return AxisAlignedBB::fromPosition(Vector3(m_position.x, m_position.y, m_position.z),
        static_cast<f32>(m_bboxWidth),
        static_cast<f32>(m_bboxHeight));
}

} // namespace mc::client::renderer::trident::particle
