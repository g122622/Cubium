#include "DripParticle.hpp"
#include "common/util/math/random/Random.hpp"

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

std::unique_ptr<Particle> DripParticle::createDrippingLava(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity);
    particle->setColor(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
    particle->setSize(0.02f);
    return particle;
}

std::unique_ptr<Particle> DripParticle::createFallingLava(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity);
    particle->setColor(glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
    particle->setSize(0.02f);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0f;
    particle->setGravity(0.06f);
    return particle;
}

std::unique_ptr<Particle> DripParticle::createLandingLava(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity);
    particle->setColor(glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));
    particle->setSize(0.04f);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);
    return particle;
}

std::unique_ptr<Particle> DripParticle::createDrippingHoney(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity);
    particle->setColor(glm::vec4(1.0f, 0.7f, 0.2f, 1.0f));
    particle->setSize(0.02f);
    return particle;
}

std::unique_ptr<Particle> DripParticle::createFallingHoney(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity);
    particle->setColor(glm::vec4(1.0f, 0.7f, 0.2f, 1.0f));
    particle->setSize(0.02f);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0f;
    particle->setGravity(0.01f);  // 蜂蜜下落更慢
    return particle;
}

std::unique_ptr<Particle> DripParticle::createLandingHoney(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity);
    particle->setColor(glm::vec4(1.0f, 0.7f, 0.2f, 1.0f));
    particle->setSize(0.04f);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);
    return particle;
}

void DripParticle::tick(mc::client::ClientWorld* world) {
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

void DripParticle::tickHanging(mc::client::ClientWorld* world) {
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

void DripParticle::tickFalling(mc::client::ClientWorld* world) {
    // 应用重力
    m_velocity.y -= m_gravity * 0.04f;

    // 限制下落速度
    if (m_velocity.y < -0.3f) {
        m_velocity.y = -0.3f;
    }

    m_position += m_velocity;

    // TODO: 检查与方块的碰撞
    // 当前简化为：下落一定距离后落地
    if (m_position.y < m_hangPosition.y - 2.0f || m_collisionContext.onGround) {
        onLand(world);
    }
}

void DripParticle::onLand(mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    m_dripState = DripState::Landed;
}

f64 DripParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    // 滴落粒子根据积累进度缩放
    return m_dripProgress;
}

} // namespace mc::client::renderer::trident::particle::particles
