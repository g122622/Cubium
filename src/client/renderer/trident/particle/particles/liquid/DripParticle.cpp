#include "DripParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "client/world/ClientWorld.hpp"

namespace mc::client::renderer::trident::particle::particles {

DripParticle::DripParticle(const glm::vec3& pos, const glm::vec3& velocity, DripType type)
    : Particle(pos, velocity)
    , m_hangPosition(pos)
    , m_type(type)
{
    // MC 1.16.5: DripParticle 基类构造
    // 尺寸 = 0.01 x 0.01
    setBoundingBox(0.01f, 0.01f);
    setSize(0.01f);

    // 悬挂状态下的重力为 0.02
    setGravity(0.02f);
    setFriction(0.98f);
    setHasPhysics(true);

    // MC 1.16.5: 最大寿命 40 tick（悬挂阶段）
    setMaxAge(40.0f);

    // 根据类型设置颜色
    switch (type) {
        case DripType::Lava:
            setColor(glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));  // 橙红色
            break;
        case DripType::Honey:
            setColor(glm::vec4(1.0f, 0.7f, 0.2f, 1.0f));  // 金黄色
            break;
        case DripType::ObsidianTear:
            setColor(glm::vec4(0.2f, 0.1f, 0.3f, 1.0f));  // 紫色
            break;
        case DripType::Water:
        default:
            setColor(glm::vec4(0.7f, 0.7f, 1.0f, 1.0f));  // 淡蓝色
            break;
    }
}

ResourceLocation DripParticle::getTextureLocation() const {
    // 根据类型和状态返回不同纹理
    if (m_dripState == DripState::Landed) {
        switch (m_type) {
            case DripType::Lava:
                return ResourceLocation("minecraft:particle/drip_fall_lava");
            case DripType::Honey:
                return ResourceLocation("minecraft:particle/drip_fall_honey");
            case DripType::ObsidianTear:
                return ResourceLocation("minecraft:particle:drip_fall_obsidian_tear");
            case DripType::Water:
            default:
                return ResourceLocation("minecraft:particle/splash");
        }
    } else if (m_dripState == DripState::Falling) {
        return ResourceLocation("minecraft:particle/drip_fall");
    } else {
        return ResourceLocation("minecraft:particle/drip_hang");
    }
}

u32 DripParticle::getLightColor(mc::client::ClientWorld* world) const {
    // MC 1.16.5: 熔岩滴是发光粒子
    if (m_type == DripType::Lava) {
        // 熔岩滴发光：基础光照 + 随生命周期的变化
        // 随着生命周期增加，光照增加
        f64 lifeRatio = m_age / m_maxAge;
        u8 blockLight = 15;
        u8 skyLight = static_cast<u8>(std::min(15.0, lifeRatio * 15.0));
        return (static_cast<u32>(skyLight) << 4) | static_cast<u32>(blockLight);
    }
    return Particle::getLightColor(world);
}

std::unique_ptr<Particle> DripParticle::createDrippingLava(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity, DripType::Lava);
    return particle;
}

std::unique_ptr<Particle> DripParticle::createFallingLava(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity, DripType::Lava);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0f;
    particle->setGravity(0.06f);  // MC 1.16.5: 下落时重力 0.06
    return particle;
}

std::unique_ptr<Particle> DripParticle::createLandingLava(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity, DripType::Lava);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);  // MC 1.16.5: 落地后 16 tick
    particle->setSize(0.04f);
    return particle;
}

std::unique_ptr<Particle> DripParticle::createDrippingHoney(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity, DripType::Honey);
    return particle;
}

std::unique_ptr<Particle> DripParticle::createFallingHoney(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity, DripType::Honey);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0f;
    particle->setGravity(0.01f);  // MC 1.16.5: 蜂蜜下落重力更小
    return particle;
}

std::unique_ptr<Particle> DripParticle::createLandingHoney(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripParticle>(pos, velocity, DripType::Honey);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);
    particle->setSize(0.04f);
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
            // MC 1.16.5: 落地状态只更新颜色淡出
            // 落地粒子已经在 onLand 中设置了最大寿命
            break;
    }

    // MC 1.16.5: 熔岩滴颜色随时间变化（红色 -> 黄色）
    if (m_type == DripType::Lava && m_dripState == DripState::Hanging) {
        // 绿色和蓝色逐渐增加，产生从红色到黄色的渐变
        f64 progress = m_dripProgress;
        m_color.g = static_cast<f32>(16.0 / (40.0 - m_maxAge + 16.0 + progress * 24.0));
        m_color.b = static_cast<f32>(4.0 / (40.0 - m_maxAge + 8.0 + progress * 12.0));
    }
}

void DripParticle::tickHanging(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    // MC 1.16.5 Dripping.tick():
    // 进度增加非常慢，通常需要 40 tick 积累满
    // 实际上进度由外部控制，这里简化处理
    mc::math::Random rng;
    m_dripProgress += 0.01f + rng.nextFloat() * 0.01f;

    // 积累满后开始下落
    if (m_dripProgress >= 1.0f) {
        m_dripState = DripState::Falling;
        m_dripProgress = 1.0f;
        // 速度清零，准备下落
        m_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    // 悬挂时位置不变
    m_position = m_hangPosition;
}

void DripParticle::tickFalling(mc::client::ClientWorld* world) {
    // MC 1.16.5 FallingLiquidParticle.tick():
    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 空气阻力
    m_velocity *= static_cast<f32>(m_friction);

    // 使用完整碰撞检测
    if (m_hasPhysics && world != nullptr) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
    }

    // 检测落地
    if (m_collisionContext.onGround || m_collisionContext.collidedY) {
        onLand(world);
    }

    // MC 1.16.5: 检测是否进入同种液体
    if (world != nullptr) {
        i32 blockX = mc::math::floorTo<i32>(m_position.x);
        i32 blockY = mc::math::floorTo<i32>(m_position.y);
        i32 blockZ = mc::math::floorTo<i32>(m_position.z);

        const BlockState* state = world->getBlockState(blockX, blockY, blockZ);
        if (state != nullptr && !state->isAir()) {
            const mc::fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                const mc::fluid::Fluid& fluid = fluidState->getFluid();
                bool inSameFluid = false;
                switch (m_type) {
                    case DripType::Water:
                        inSameFluid = fluid.isIn(mc::fluid::FluidTags::WATER());
                        break;
                    case DripType::Lava:
                        inSameFluid = fluid.isIn(mc::fluid::FluidTags::LAVA());
                        break;
                    case DripType::Honey:
                    case DripType::ObsidianTear:
                        // 蜂蜜和黑曜石眼泪没有流体检测
                        break;
                }

                if (inSameFluid) {
                    setExpired();
                }
            }
        }
    }
}

void DripParticle::onLand(mc::client::ClientWorld* world) {
    MC_UNUSED(world);
    m_dripState = DripState::Landed;
    m_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    setMaxAge(m_age + 16.0f);  // 落地后存在 16 tick

    // MC 1.16.5: 根据 m_type 生成对应的落地粒子
    // Water -> SplashParticle, Lava -> LandingLava, Honey -> LandingHoney
    // 需要在 ParticleManager 支持粒子生成时实现
}

f64 DripParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    // MC 1.16.5: 悬挂时根据进度缩放，下落后固定
    if (m_dripState == DripState::Hanging) {
        return m_dripProgress;
    }
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
