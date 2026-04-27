#include "BubbleParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "client/world/ClientWorld.hpp"

namespace mc::client::renderer::trident::particle::particles {

BubbleParticle::BubbleParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    mc::math::Random rng;

    // MC 1.16.5: 尺寸 0.02 x 0.02
    // 尺寸 = 0.02 + rand * 0.02
    setSize(0.02f + rng.nextFloat() * 0.02f);

    // MC 1.16.5: 速度缩放 0.2 倍加上随机偏移
    // motionX = (rand.nextDouble() - rand.nextDouble()) * 0.02 + vx * 0.2
    // motionY = (rand.nextDouble() - rand.nextDouble()) * 0.02 + vy * 0.2
    // motionZ = (rand.nextDouble() - rand.nextDouble()) * 0.02 + vz * 0.2
    m_velocity.x = m_velocity.x * 0.2f + (rng.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.y = m_velocity.y * 0.2f + (rng.nextFloat() * 2.0f - 1.0f) * 0.02f;
    m_velocity.z = m_velocity.z * 0.2f + (rng.nextFloat() * 2.0f - 1.0f) * 0.02f;

    // MC 1.16.5: 颜色为淡蓝色半透明
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));

    // MC 1.16.5: 摩擦 0.85
    setFriction(0.85f);
    setHasPhysics(false);  // 气泡不做碰撞检测

    // MC 1.16.5: 生命周期 = (int)(8.0 / (rand.nextDouble() * 0.8 + 0.2))
    setMaxAge(static_cast<f64>(static_cast<i32>(8.0 / (rng.nextFloat() * 0.8f + 0.2f))));
}

std::unique_ptr<Particle> BubbleParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<BubbleParticle>(pos, velocity);
}

void BubbleParticle::tick(mc::client::ClientWorld* world) {
    // 保存上一帧位置（用于插值）
    m_prevPosition = m_position;

    // 生命周期递增
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // MC 1.16.5: 气泡向上升起
    // motionY += 0.005D (向上的浮力)
    m_velocity.y += 0.005f;

    // MC 1.16.5: 摩擦 0.85
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 直接移动，不做碰撞检测
    m_position += m_velocity;

    // MC 1.16.5: 检查是否离开水面
    // 如果当前位置不在水中，则消失
    if (world != nullptr) {
        i32 blockX = mc::math::floorTo<i32>(m_position.x);
        i32 blockY = mc::math::floorTo<i32>(m_position.y);
        i32 blockZ = mc::math::floorTo<i32>(m_position.z);

        const BlockState* state = world->getBlockState(blockX, blockY, blockZ);
        if (state != nullptr && !state->isAir()) {
            const mc::fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState == nullptr || fluidState->isEmpty() ||
                !fluidState->getFluid().isIn(mc::fluid::FluidTags::WATER())) {
                // 离开水面，气泡破裂
                // TODO: 生成 BubblePop 粒子
                setExpired();
                return;
            }
        } else {
            // 不在方块中（空气或空），气泡破裂
            setExpired();
            return;
        }
    }
}

} // namespace mc::client::renderer::trident::particle::particles
