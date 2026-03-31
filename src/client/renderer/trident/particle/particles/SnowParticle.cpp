#include "SnowParticle.hpp"
#include "../../../../../common/util/math/random/Random.hpp"
#include "../../../../../common/util/assert/AssertAll.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

SnowParticle::SnowParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_swingPhase(0.0f)
    , m_swingAmplitude(SWING_AMPLITUDE)
{
    // 使用项目的随机数生成器
    mc::math::Random rng;

    // 随机初始相位和振幅
    m_swingPhase = rng.nextFloat() * 6.28318f;  // 0 - 2π
    m_swingAmplitude = SWING_AMPLITUDE * (0.5f + rng.nextFloat());

    // 雪花参数
    setGravity(DEFAULT_GRAVITY);
    setSize(0.05f + rng.nextFloat() * 0.05f);  // 0.05 - 0.1
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));  // 白色几乎不透明
    setFriction(0.95f);
    setHasPhysics(false);  // 雪花暂时不进行碰撞检测

    // 雪花生命周期较长
    // 参考 MC: maxAge = (int)(200.0F / (Math.random() * 0.2F + 0.8F))
    f32 lifeMultiplier = 0.8f + rng.nextFloat() * 0.2f;
    setMaxAge(200.0f / lifeMultiplier);
}

std::unique_ptr<Particle> SnowParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SnowParticle>(pos, velocity);
}

void SnowParticle::tick(ClientWorld* world) {
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

    // 雪花摇摆效果
    m_swingPhase += SWING_FREQUENCY;
    f32 swing = std::sin(m_swingPhase) * m_swingAmplitude;
    m_velocity.x += swing * 0.01f;

    // 应用速度
    m_position += m_velocity;

    // 应用阻力
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 根据年龄淡出
    if (m_age > m_maxAge * 0.8f) {
        f32 fadeProgress = (m_age - m_maxAge * 0.8f) / (m_maxAge * 0.2f);
        m_color.a = 0.9f * (1.0f - fadeProgress);
    }

    MC_UNUSED(world);
}

void SnowParticle::buildVertices(
    const glm::vec3& cameraPos,
    f32 partialTick,
    const ParticleTextureAtlas& atlas,
    std::vector<ParticleVertex>& outVertices) const
{
    // 使用基类实现（支持纹理图集）
    Particle::buildVertices(cameraPos, partialTick, atlas, outVertices);
}

} // namespace mc::client::renderer::trident::particle::particles
