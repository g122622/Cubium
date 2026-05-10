#include "DrownedEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

DrownedEntity::DrownedEntity(LegacyEntityType type, EntityId id)
    : ZombieEntity(type, id)
{
    // 注册属性
    registerAttributes();

    // 随机决定是否手持三叉戟
    math::Random rng = getRandom();
    m_hasTrident = rng.nextInt(1, 100) <= 15; // 15% 概率
}

std::unique_ptr<Entity> DrownedEntity::create(IWorld* /*world*/) {
    return std::make_unique<DrownedEntity>(LegacyEntityType::Unknown, 0);
}

bool DrownedEntity::isInWater() const {
    // 调用父类的 isInWater() 方法
    // Entity::isInWater() 已经在 updateEnvironmentState() 中正确更新
    return ZombieEntity::isInWater();
}

bool DrownedEntity::shouldBurnInDaylight() const {
    // 在水中不燃烧
    return !isInWater();
}

void DrownedEntity::tick() {
    ZombieEntity::tick();

    // 在水中时的特殊行为
    // MC 1.16.5: 溺尸在水中可以游泳，游泳状态由AI目标控制
    // SwimGoal 会在水中自动启用游泳导航
    (void)isInWater();  // 暂时避免未使用警告，AI目标实现后会使用
}

void DrownedEntity::registerAttributes() {
    // 调用父类方法
    ZombieEntity::registerAttributes();

    // 溺尸的属性与僵尸相同
}

} // namespace mc
