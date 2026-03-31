#include "DripParticle.hpp"
#include "../../../../../../common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

DripParticle::DripParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_hangPosition(pos)
{
    setGravity(0.0f);
    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(200.0f);  // 较长的最大年龄
}

void DripParticle::tick(ClientWorld* world) {
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    switch (m_dripState) {
        case DripState::Hanging:
            tickHanging(world);
            break;
        case DripState::Falling:
            tickFalling(world);
            break;
        case DripState::Landed:
            // 已落地，等待消失
            setExpired();
            break;
    }
}

void DripParticle::tickHanging(ClientWorld* world) {
    MC_UNUSED(world);

    // 缓慢积累变大
    mc::math::Random rng;
    m_dripProgress += 0.01f + rng.nextFloat() * 0.02f;

    // 积累满后开始下落
    if (m_dripProgress >= 1.0f) {
        m_dripState = DripState::Falling;
        m_dripProgress = 1.0f;

        // 设置下落速度
        m_velocity.y = -0.01f;
        setGravity(0.02f);
    }

    // 悬挂时位置不变，但有微小摆动
    m_position = m_hangPosition;
}

void DripParticle::tickFalling(ClientWorld* world) {
    // 应用重力
    m_velocity.y -= m_gravity * 0.04f;

    // 限制下落速度
    if (m_velocity.y < -0.3f) {
        m_velocity.y = -0.3f;
    }

    m_position += m_velocity;

    // TODO: 检查与方块的碰撞
    // 当前简化为：下落一定距离后落地
    if (m_position.y < m_hangPosition.y - 2.0f || m_onGround) {
        onLand(world);
    }
}

void DripParticle::onLand(ClientWorld* world) {
    MC_UNUSED(world);
    m_dripState = DripState::Landed;
}

} // namespace mc::client::renderer::trident::particle::particles
