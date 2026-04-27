#include "ThrowableEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

namespace mc {
namespace entity {

ThrowableEntity::ThrowableEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
}

void ThrowableEntity::tick() {
    // 调用父类tick
    ProjectileEntity::tick();

    // MC 1.16.5: 水中的气泡粒子
    if (m_world && isInWater()) {
        mc::math::Random rng = getRandom();
        if (rng.nextInt(4) == 0) {
            Vector3 pos(x(), y() + 0.2f, z());
            Vector3 vel(
                (rng.nextFloat() * 2.0f - 1.0f) * 0.1f,
                0.1f + rng.nextFloat() * 0.1f,
                (rng.nextFloat() * 2.0f - 1.0f) * 0.1f);

            m_world->addParticle(
                client::renderer::trident::particle::ParticleTypeId::Bubble,
                pos, vel);
        }
    }
}

std::unique_ptr<ThrowableEntity> ThrowableEntity::createFromThrower(
    LegacyEntityType type, EntityId id, LivingEntity& thrower) {
    auto throwable = std::make_unique<ThrowableEntity>(type, id);
    // 设置位置为投掷者眼睛位置
    throwable->setPosition(thrower.x(),
                           thrower.y() + thrower.eyeHeight() - 0.1f,
                           thrower.z());
    throwable->setShooter(&thrower);
    return throwable;
}

} // namespace entity
} // namespace mc

std::unique_ptr<ThrowableEntity> ThrowableEntity::createFromThrower(
    LegacyEntityType type, EntityId id, LivingEntity& thrower) {
    auto throwable = std::make_unique<ThrowableEntity>(type, id);
    // 设置位置为投掷者眼睛位置
    throwable->setPosition(thrower.x(),
                           thrower.y() + thrower.eyeHeight() - 0.1f,
                           thrower.z());
    throwable->setShooter(&thrower);
    return throwable;
}

} // namespace entity
} // namespace mc
