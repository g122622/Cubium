#include "RainParticle.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle {

class ClientWorld {
public:
    virtual ~ClientWorld() = default;

    /**
     * @brief 获取指定方块状态
     *
     * 这里只保留雨滴碰撞所需的最小接口，不引入额外世界依赖。
     *
     * @param x 方块 x 坐标
     * @param y 方块 y 坐标
     * @param z 方块 z 坐标
     * @return 方块状态指针
     */
    [[nodiscard]] virtual const mc::BlockState* getBlockState(mc::i32 x, mc::i32 y, mc::i32 z) const = 0;
};

} // namespace mc::client::renderer::trident::particle

namespace mc::client::renderer::trident::particle::particles {

namespace {

constexpr f32 RAIN_BBOX_WIDTH = 0.02f;
constexpr f32 RAIN_BBOX_HEIGHT = 0.04f;
constexpr f32 GROUND_PROBE_EPSILON = 0.01f;

/**
 * @brief 检查雨滴包围盒是否与地面方块相交
 *
 * 只使用 `ClientWorld::getBlockState(...)` 做轻量采样，不引入额外世界接口。
 * 只要任意采样到的方块碰撞形状与粒子包围盒相交，就认为已经落地。
 *
 * @param world 客户端世界
 * @param box 用于检测的粒子包围盒
 * @return 如果命中可碰撞方块则返回 true
 */
[[nodiscard]] bool hasGroundCollision(mc::client::renderer::trident::particle::ClientWorld* world, const AxisAlignedBB& box)
{
    MC_ASSERT_RELEASE(world != nullptr);

    const i32 minX = static_cast<i32>(std::floor(box.minX));
    const i32 maxX = static_cast<i32>(std::ceil(box.maxX) - 1.0f);
    const i32 minY = static_cast<i32>(std::floor(box.minY));
    const i32 maxY = static_cast<i32>(std::ceil(box.maxY) - 1.0f);
    const i32 minZ = static_cast<i32>(std::floor(box.minZ));
    const i32 maxZ = static_cast<i32>(std::ceil(box.maxZ) - 1.0f);

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                const BlockState* state = world->getBlockState(x, y, z);
                if (state == nullptr || state->isAir()) {
                    continue;
                }

                const CollisionShape& shape = state->getCollisionShape();
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
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE);
    setBoundingBox(RAIN_BBOX_WIDTH, RAIN_BBOX_HEIGHT);
    setColor(glm::vec4(0.7f, 0.8f, 1.0f, 0.6f));  // 淡蓝色半透明
    setFriction(0.98f);
    setHasPhysics(false);  // 雨滴暂时不走通用实体碰撞

    // 雨滴生命周期较短
    // 参考 MC: maxAge = (int)(8.0D / (Math.random() * 0.8D + 0.2D))
    mc::math::Random rng;
    f64 lifeMultiplier = 0.2f + rng.nextFloat() * 0.8f;
    setMaxAge(8.0f / lifeMultiplier);
}

std::unique_ptr<Particle> RainParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::renderer::trident::particle::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<RainParticle>(pos, velocity);
}

void RainParticle::tick(mc::client::renderer::trident::particle::ClientWorld* world)
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
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04f);

    // 限制下落速度（终端速度）
    if (m_velocity.y < TERMINAL_VELOCITY) {
        m_velocity.y = TERMINAL_VELOCITY;
    }

    // 应用速度
    m_position += m_velocity;
    setBoundingBox(m_bboxWidth, m_bboxHeight);

    // 应用阻力
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    m_onGround = false;

    // 检查地面碰撞
    if (world != nullptr) {
        // 稍微向下探测，避免刚好贴着方块表面时漏检
        const AxisAlignedBB probeBox(
            m_bboxMin.x,
            m_bboxMin.y - GROUND_PROBE_EPSILON,
            m_bboxMin.z,
            m_bboxMax.x,
            m_bboxMax.y - GROUND_PROBE_EPSILON,
            m_bboxMax.z
        );

        if (hasGroundCollision(world, probeBox)) {
            m_onGround = true;
            m_velocity.y = 0.0f;
        }
    }

    if (m_onGround) {
        // 雨滴碰到地面有概率消失
        mc::math::Random rng;
        if (rng.nextFloat() < 0.5f) {
            setExpired();
        }
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 雨滴不淡出，直接保持颜色
}

} // namespace mc::client::renderer::trident::particle::particles
