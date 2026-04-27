#include "DripWaterParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

DripWaterParticle::DripWaterParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : DripParticle(pos, velocity, DripType::Water)
{
    mc::math::Random rng;

    // MC 1.16.5: 水滴尺寸
    setSize(0.01f + rng.nextFloat() * 0.01f);

    // MC 1.16.5: 水滴颜色（淡蓝色透明）
    setColor(glm::vec4(0.7f, 0.7f, 1.0f, 0.8f));

    // MC 1.16.5: 生命周期
    setMaxAge(40.0f);
}

std::unique_ptr<Particle> DripWaterParticle::createDripping(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DripWaterParticle>(pos, velocity);
}

std::unique_ptr<Particle> DripWaterParticle::createFalling(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripWaterParticle>(pos, velocity);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0f;
    particle->setGravity(0.06f);  // MC 1.16.5: 水滴下落重力
    return particle;
}

std::unique_ptr<Particle> DripWaterParticle::createLanding(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripWaterParticle>(pos, velocity);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);  // MC 1.16.5: 落地后 16 tick
    particle->setSize(0.04f);
    return particle;
}

u32 DripWaterParticle::getLightColor(mc::client::ClientWorld* world) const {
    // MC 1.16.5: 水滴使用世界光照
    return Particle::getLightColor(world);
}

void DripWaterParticle::onLand(mc::client::ClientWorld* world) {
    // MC 1.16.5: 水滴落地时产生水花效果
    // TODO: 生成 SplashParticle
    // world->spawnParticle(ParticleTypeId::Splash, m_position, glm::vec3(0.0f));

    DripParticle::onLand(world);
}

} // namespace mc::client::renderer::trident::particle::particles
