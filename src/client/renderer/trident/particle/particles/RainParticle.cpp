#include "RainParticle.hpp"
#include "../../../../../common/util/math/random/Random.hpp"
#include "../../../../../common/util/assert/AssertAll.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

RainParticle::RainParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // 雨滴参数
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE);
    setColor(glm::vec4(0.7f, 0.8f, 1.0f, 0.6f));  // 淡蓝色半透明
    setFriction(0.98f);
    setHasPhysics(false);  // 雨滴暂时不进行碰撞检测

    // 雨滴生命周期较短
    // 参考 MC: maxAge = (int)(8.0D / (Math.random() * 0.8D + 0.2D))
    mc::math::Random rng;
    f64 lifeMultiplier = 0.2f + rng.nextFloat() * 0.8f;
    setMaxAge(8.0f / lifeMultiplier);
}

std::unique_ptr<Particle> RainParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<RainParticle>(pos, velocity);
}

void RainParticle::tick(ClientWorld* world) {
    // 保存上一帧位置
    m_prevPosition = m_position;

    // 年龄增加
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= m_gravity * 0.04f;

    // 限制下落速度（终端速度）
    if (m_velocity.y < TERMINAL_VELOCITY) {
        m_velocity.y = TERMINAL_VELOCITY;
    }

    // 应用速度
    m_position += m_velocity;

    // 应用阻力
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 检查地面碰撞
    // TODO: 当实现碰撞检测后，使用 world 进行检测
    MC_UNUSED(world);
    if (m_onGround) {
        // 雨滴碰撞地面时有概率消失
        mc::math::Random rng;
        if (rng.nextFloat() < 0.5f) {
            setExpired();
        }
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 雨滴不淡出，直接消失
    // 不修改 alpha
}

} // namespace mc::client::renderer::trident::particle::particles
