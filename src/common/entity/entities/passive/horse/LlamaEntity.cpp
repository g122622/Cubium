#include "LlamaEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <memory>

namespace mc {

LlamaEntity::LlamaEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    // 随机设置外观和强度
    randomizeAppearance();
}

std::unique_ptr<Entity> LlamaEntity::create(IWorld* /*world*/) {
    return std::make_unique<LlamaEntity>(LegacyEntityType::Unknown, 0);
}

void LlamaEntity::randomizeAppearance() {
    math::Random rng(ticksExisted());

    // 随机选择颜色
    m_color = static_cast<LlamaColor>(rng.nextInt(4));

    // 随机强度 (1-5)
    m_strength = 1 + rng.nextInt(5);
}

bool LlamaEntity::canBeRiddenBy(Player* player) const {
    // 羊驼可以骑乘但不能控制方向
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }
    // 不需要驯服即可骑乘
    return true;
}

i32 LlamaEntity::getInventoryColumns() const {
    // 根据强度决定背包大小：3 + 3 * strength
    return 3 + 3 * m_strength;
}

i32 LlamaEntity::getInventorySize() const {
    // 1格装饰槽 + 背包格数（如果有箱子）
    if (m_hasChest) {
        return 1 + getInventoryColumns() * 3;
    }
    return 1;
}

bool LlamaEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是干草块
    // return itemStack.getItem() == Items::HAY_BLOCK;
    (void)itemStack;
    return false;
}

bool LlamaEntity::isTameItem(const ItemStack& /*itemStack*/) const {
    // 羊驼不响应特定驯服物品
    return false;
}

std::unique_ptr<AnimalEntity> LlamaEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小羊驼
    // auto baby = std::make_unique<LlamaEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    //
    // // 遗传父母的属性
    // LlamaEntity* llamaPartner = dynamic_cast<LlamaEntity*>(&partner);
    // if (llamaPartner) {
    //     math::Random rng(ticksExisted());
    //     // 随机继承父母的一方颜色
    //     if (rng.nextFloat() < 0.5f) {
    //         baby->setColor(m_color);
    //     } else {
    //         baby->setColor(llamaPartner->getColor());
    //     }
    //     // 强度可能更高
    //     i32 babyStrength = std::max(m_strength, llamaPartner->getStrength());
    //     if (rng.nextFloat() < 0.2f) {
    //         babyStrength = std::min(babyStrength + 1, 5);
    //     }
    //     baby->m_strength = babyStrength;
    // }
    //
    // return baby;
    (void)partner;
    return nullptr;
}

void LlamaEntity::tick() {
    AbstractHorseEntity::tick();

    // 更新吐口水冷却
    if (m_spitCooldown > 0) {
        m_spitCooldown--;
    }

    // 商队跟随逻辑
    if (m_inCaravan && m_caravanLeader) {
        // TODO: 跟随商队领袖
    }
}

void LlamaEntity::registerGoals() {
    AbstractHorseEntity::registerGoals();

    // TODO: 羊驼特有 AI 目标
    // - LlamaAttackGoal (吐口水攻击)
    // - LlamaFollowCaravanGoal (跟随商队)
}

void LlamaEntity::registerAttributes() {
    AbstractHorseEntity::registerAttributes();

    // 羊驼的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f + m_strength * 5.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.164f);
}

} // namespace mc
