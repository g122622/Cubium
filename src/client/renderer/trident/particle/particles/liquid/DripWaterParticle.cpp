#include "DripWaterParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

DripWaterParticle::DripWaterParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : DripParticle(pos, velocity)
{
    mc::math::Random rng;

    setSize(0.04f * (0.8f + rng.nextFloat() * 0.4f));

    // 水滴颜色：淡蓝色半透明
    setColor(glm::vec4(0.6f, 0.8f, 1.0f, 0.8f));

    setMaxAge(100.0f);  // 较长生命周期（包含悬挂阶段）
}

std::unique_ptr<Particle> DripWaterParticle::createDripping(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripWaterParticle>(pos, velocity);
    particle->setColor(glm::vec4(0.6f, 0.8f, 1.0f, 0.8f));
    return particle;
}

std::unique_ptr<Particle> DripWaterParticle::createFalling(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    auto particle = std::make_unique<DripWaterParticle>(pos, velocity);
    particle->setColor(glm::vec4(0.6f, 0.8f, 1.0f, 0.8f));
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0f;
    particle->setGravity(0.06f);
    return particle;
}

u32 DripWaterParticle::getLightColor(mc::client::ClientWorld* world) const {
    MC_UNUSED(world);
    // 水滴使用环境光照
    // TODO: 从世界采样光照
    return 0x0F;  // 默认光照
}

void DripWaterParticle::onLand(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    // 水滴落地时产生水花效果
    // TODO: 生成 SplashParticle

    DripParticle::onLand(world);
}

} // namespace mc::client::renderer::trident::particle::particles
