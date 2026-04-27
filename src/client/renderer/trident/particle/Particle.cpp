#include "Particle.hpp"
#include "ParticleTextureAtlas.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/core/Constants.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/world/IWorld.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "client/world/ClientWorld.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include <algorithm>
#include <cmath>

namespace mc::client::renderer::trident::particle {

// 默认纹理位置
static const ResourceLocation DEFAULT_TEXTURE("minecraft:particle/generic");

Particle::Particle(const glm::vec3& pos, const glm::vec3& velocity)
    : m_position(pos)
    , m_prevPosition(pos)
    , m_velocity(velocity)
    , m_bboxWidth(physics::PARTICLE_DEFAULT_BBOX_WIDTH)
    , m_bboxHeight(physics::PARTICLE_DEFAULT_BBOX_HEIGHT)
{
}

void Particle::tick(mc::client::ClientWorld* world) {
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

    // 应用重力（MC 的重力系数约为 0.04 blocks/tick²）
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

    // 根据年龄淡出（MC 在生命后半段淡出）
    if (m_age > m_maxAge * 0.5f) {
        f64 fadeProgress = (m_age - m_maxAge * 0.5f) / (m_maxAge * 0.5f);
        m_color.a = static_cast<f32>(1.0 - fadeProgress);
    }
}

void Particle::buildVertices(
    const glm::vec3& cameraPos,
    f64 partialTick,
    const ParticleTextureAtlas& atlas,
    std::vector<ParticleVertex>& outVertices) const
{
    // 插值位置
    glm::dvec3 interpPos = glm::dvec3(m_prevPosition) +
        (glm::dvec3(m_position) - glm::dvec3(m_prevPosition)) * partialTick;

    // 插值旋转
    f64 interpRoll = m_prevRoll + (m_roll - m_prevRoll) * partialTick;

    // 计算 billboard 基向量
    glm::dvec3 toCamera = glm::dvec3(cameraPos) - interpPos;
    f64 dist = glm::length(toCamera);
    if (dist < 0.001) {
        return;  // 太近了，跳过
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

    glm::vec3 interpPosF(static_cast<f32>(interpPos.x),
                         static_cast<f32>(interpPos.y),
                         static_cast<f32>(interpPos.z));
    glm::vec3 rightF(static_cast<f32>(right.x), static_cast<f32>(right.y), static_cast<f32>(right.z));
    glm::vec3 upF(static_cast<f32>(up.x), static_cast<f32>(up.y), static_cast<f32>(up.z));

    // 获取缩放
    f64 scale = getScale(partialTick);
    f64 halfSize = m_size * scale * 0.5;
    const f32 halfSizeF = static_cast<f32>(halfSize);

    // 获取 UV 坐标
    glm::vec4 uv(0.0f, 0.0f, 1.0f, 1.0f);  // 默认 UV
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
    outVertices.push_back({
        interpPosF - rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(uv.x, uv.w),  // UV: 左下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a
    });
    // 右下
    outVertices.push_back({
        interpPosF + rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(uv.z, uv.w),  // UV: 右下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a
    });
    // 右上
    outVertices.push_back({
        interpPosF + rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(uv.z, uv.y),  // UV: 右上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a
    });
    // 左上
    outVertices.push_back({
        interpPosF - rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(uv.x, uv.y),  // UV: 左上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a
    });
}

ResourceLocation Particle::getTextureLocation() const {
    return DEFAULT_TEXTURE;
}

u32 Particle::getLightColor(mc::client::ClientWorld* world) const {
    // 默认实现：从世界采样光照
    // 参考 MC 1.16.5 Particle.getBrightnessForRender()
    if (world == nullptr) {
        return physics::PARTICLE_MAX_PACKED_LIGHT;  // 最大亮度
    }

    // 获取粒子位置的方块坐标
    i32 blockX = mc::math::floorTo<i32>(m_position.x);
    i32 blockY = mc::math::floorTo<i32>(m_position.y);
    i32 blockZ = mc::math::floorTo<i32>(m_position.z);

    // 采样世界光照
    u8 blockLight = world->getBlockLight(blockX, blockY, blockZ);
    u8 skyLight = world->getSkyLight(blockX, blockY, blockZ);

    // 组合成 combined light: (skyLight << 4) | blockLight
    // 参考 WorldRenderer.getCombinedLight()
    return (static_cast<u32>(skyLight) << 4) | static_cast<u32>(blockLight);
}

f64 Particle::getScale(f64 /*partialTick*/) const {
    // 默认实现：返回 1.0（无缩放）
    // 子类可以重写以实现缩放动画
    return 1.0f;
}

void Particle::move(mc::client::ClientWorld* world, const glm::vec3& delta) {
    // 参考 MC 1.16.5 Entity.move() 和 Particle.move()
    if (world == nullptr) {
        // 无世界引用，只移动不检测碰撞
        m_position += delta;
        return;
    }

    // 零移动检查
    if (std::abs(delta.x) < physics::PARTICLE_MIN_MOVEMENT &&
        std::abs(delta.y) < physics::PARTICLE_MIN_MOVEMENT &&
        std::abs(delta.z) < physics::PARTICLE_MIN_MOVEMENT) {
        return;
    }

    // TODO: 实现完整的碰撞检测
    // 目前使用简化版本：直接移动，只检测地面碰撞
    glm::vec3 actualDelta(delta.x, delta.y, delta.z);

    // 应用重力后直接移动
    m_position += actualDelta;

    // 简化的地面检测：检查下方是否有方块
    AxisAlignedBB bbox = getBoundingBox();
    i32 checkY = mc::math::floorTo<i32>(bbox.minY - 0.01f);
    const BlockState* state = world->getBlockState(
        mc::math::floorTo<i32>((bbox.minX + bbox.maxX) * 0.5f),
        checkY,
        mc::math::floorTo<i32>((bbox.minZ + bbox.maxZ) * 0.5f)
    );

    if (state != nullptr && !state->isAir()) {
        const CollisionShape& shape = state->getCollisionShape();
        if (!shape.isEmpty()) {
            // 简化：假设方块是完整立方体
            f32 blockTop = static_cast<f32>(checkY + 1);
            if (m_position.y < blockTop) {
                m_position.y = blockTop;
                m_velocity.y = 0.0f;
                m_collisionContext.onGround = true;
                m_collisionContext.collidedY = true;
            }
        }
    }

    // 碰撞后速度归零
    if (m_collisionContext.collidedX) {
        m_velocity.x = 0.0f;
    }
    if (m_collisionContext.collidedZ) {
        m_velocity.z = 0.0f;
    }
}

void Particle::setBoundingBox(f64 width, f64 height) {
    m_bboxWidth = width;
    m_bboxHeight = height;
}

void Particle::setPosition(const glm::vec3& pos) {
    m_position = pos;
    m_prevPosition = pos;
}

AxisAlignedBB Particle::getBoundingBox() const {
    return AxisAlignedBB::fromPosition(
        Vector3(m_position.x, m_position.y, m_position.z),
        static_cast<f32>(m_bboxWidth),
        static_cast<f32>(m_bboxHeight)
    );
}

} // namespace mc::client::renderer::trident::particle
