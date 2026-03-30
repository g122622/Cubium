#include "DrownedEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include <random>

namespace mc {

DrownedEntity::DrownedEntity(LegacyEntityType type, EntityId id)
    : ZombieEntity(type, id)
{
    // 注册属性
    registerAttributes();

    // 随机决定是否手持三叉戟
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<i32> dist(1, 100);
    m_hasTrident = dist(gen) <= 15; // 15% 概率
}

std::unique_ptr<Entity> DrownedEntity::create(IWorld* /*world*/) {
    return std::make_unique<DrownedEntity>(LegacyEntityType::Unknown, 0);
}

bool DrownedEntity::isInWater() const {
    // TODO: 检查是否在水中
    return false;
}

bool DrownedEntity::shouldBurnInDaylight() const {
    // 在水中不燃烧
    return !isInWater();
}

void DrownedEntity::tick() {
    ZombieEntity::tick();

    // 在水中时的特殊行为
    if (isInWater()) {
        // 可以游泳
        // TODO: 设置游泳状态
    }
}

void DrownedEntity::registerAttributes() {
    // 调用父类方法
    ZombieEntity::registerAttributes();

    // 溺尸的属性与僵尸相同
}

} // namespace mc
