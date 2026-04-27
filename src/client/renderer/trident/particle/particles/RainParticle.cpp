#include "RainParticle.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/world/block/Block.hpp"
#include "client/world/ClientWorld.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

namespace {

constexpr f32 RAIN_BBOX_WIDTH = 0.02f;
constexpr f32 RAIN_BBOX_HEIGHT = 0.04f;
constexpr f32 GROUND_PROBE_EPSILON = 0.01f;

/**
 * @brief 检查雨滴包围盒是否与地面方块相交
 *
 * 只要任意采样到的方块碰撞形状与粒子包围盒相交，就认为已经落地。
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

RainParticle::RainParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // 雨滴参数
    setGravity(physics::RAIN_GRAVITY);
    setSize(DEFAULT_SIZE);
    setBoundingBox(RAIN_BBOX_WIDTH, RAIN_BBOX_HEIGHT);
    setColor(glm::vec4(0.7f, 0.8f, 1.0f, 0.6f));  // 淡蓝色半透明
    setFriction(physics::DRAG_AIR);
    setHasPhysics(false);  // 雨滴使用自定义碰撞检测

    // 雨滴生命周期较短
    // 参考 MC: maxAge = (int)(8.0D / (Math.random() * 0.8D + 0.2D))
    mc::math::Random rng;
    f64 lifeMultiplier = 0.2f + rng.nextFloat() * 0.8f;
    setMaxAge(8.0f / lifeMultiplier);
}

std::unique_ptr<Particle> RainParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<RainParticle>(pos, velocity);
}

void RainParticle::tick(mc::client::ClientWorld* world)
{
    // 保存上一帧位置
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    // 年龄增加
    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力（MC 粒子重力系数 0.04）
    m_velocity.y -= static_cast<f32>(m_gravity * physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 限制下落速度（终端速度）
    if (m_velocity.y < TERMINAL_VELOCITY) {
        m_velocity.y = TERMINAL_VELOCITY;
    }

    // 应用速度
    m_position += m_velocity;

    // 应用阻力
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 重置碰撞状态
    m_collisionContext.reset();

    // 检查地面碰撞
    if (world != nullptr) {
        AxisAlignedBB bbox = getBoundingBox();

        // 稍微向下探测，避免刚好贴着方块表面时漏检
        AxisAlignedBB probeBox(
            bbox.minX,
            bbox.minY - GROUND_PROBE_EPSILON,
            bbox.minZ,
            bbox.maxX,
            bbox.minY,
            bbox.maxZ
        );

        if (hasGroundCollision(world, probeBox)) {
            m_collisionContext.onGround = true;
            m_collisionContext.collidedY = true;
            m_velocity.y = 0.0f;
        }
    }

    if (m_collisionContext.onGround) {
        // 雨滴碰到地面有概率消失
        mc::math::Random rng;
        if (rng.nextFloat() < 0.5f) {
            setExpired();
        }
        m_velocity.x *= physics::PARTICLE_GROUND_FRICTION;
        m_velocity.z *= physics::PARTICLE_GROUND_FRICTION;
    }

    // 雨滴不淡出，直接保持颜色
}

} // namespace mc::client::renderer::trident::particle::particles
