#include "DiggingParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

DiggingParticle::DiggingParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState)
    : Particle(pos, velocity)
    , m_blockState(blockState)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.5f + rng.nextFloat() * 0.5f));
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));  // 使用纹理原色

    setFriction(0.92f);
    setHasPhysics(true);  // 方块粒子有物理碰撞
    setMaxAge(DEFAULT_LIFETIME * (0.8f + rng.nextFloat() * 0.4f));
}

std::unique_ptr<Particle> DiggingParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认使用石头方块
    // 实际使用时应通过 createWithBlock 创建
    return nullptr;
}

std::unique_ptr<Particle> DiggingParticle::createWithBlock(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    const BlockState& blockState)
{
    return std::make_unique<DiggingParticle>(pos, velocity, blockState);
}

void DiggingParticle::tick(mc::client::ClientWorld* world) {
    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= m_gravity * 0.04f;

    // 随机旋转
    m_roll += 0.1f;

    // 移动并碰撞
    move(world, m_velocity);

    // 应用阻力
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7f) {
        m_color.a = 1.0f - (lifeRatio - 0.7f) / 0.3f;
    }
}

ResourceLocation DiggingParticle::getTextureLocation() const {
    // TODO: 根据方块状态获取对应的纹理位置
    // 当前返回默认方块纹理
    return ResourceLocation("minecraft:block/stone");
}

} // namespace mc::client::renderer::trident::particle::particles
