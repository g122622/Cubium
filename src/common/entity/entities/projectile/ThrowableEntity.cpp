#include "ThrowableEntity.hpp"
#include "../../core/LivingEntity.hpp"

namespace mc {
namespace entity {

ThrowableEntity::ThrowableEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
}

void ThrowableEntity::tick() {
    // 调用父类tick
    ProjectileEntity::tick();

    // TODO: 添加水中的粒子效果
    // if (isInWater()) {
    //     for (int i = 0; i < 4; ++i) {
    //         world->addParticle(ParticleTypes::BUBBLE, ...);
    //     }
    // }
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
