#include "DispenseItemBehaviorRegistry.hpp"
#include "IDispenseItemBehavior.hpp"

namespace mc {
namespace blocks {

DispenseItemBehaviorRegistry::DispenseItemBehaviorRegistry()
    : m_defaultBehavior(std::make_unique<DefaultDispenseItemBehavior>()) {
}

DispenseItemBehaviorRegistry& DispenseItemBehaviorRegistry::instance() {
    static DispenseItemBehaviorRegistry instance;
    return instance;
}

void DispenseItemBehaviorRegistry::registerBehavior(const String& itemId, std::unique_ptr<IDispenseItemBehavior> behavior) {
    m_behaviors[itemId] = std::move(behavior);
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const ItemStack& stack) const {
    if (stack.isEmpty()) {
        return nullptr;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return nullptr;
    }
    return getBehavior(item->itemLocation().toString());
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getBehavior(const String& itemId) const {
    auto it = m_behaviors.find(itemId);
    if (it != m_behaviors.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool DispenseItemBehaviorRegistry::hasBehavior(const String& itemId) const {
    return m_behaviors.find(itemId) != m_behaviors.end();
}

IDispenseItemBehavior* DispenseItemBehaviorRegistry::getDefaultBehavior() {
    return m_defaultBehavior.get();
}

void DispenseItemBehaviorRegistry::initDefaultBehaviors() {
    // TODO: 在物品系统完善后注册默认行为
    // 当前框架已就绪，等待以下物品注册：
    //
    // 投掷物:
    // - Arrow (箭矢)
    // - TippedArrow (药箭)
    // - SpectralArrow (光灵箭)
    // - Snowball (雪球)
    // - Egg (鸡蛋)
    // - ExperienceBottle (附魔之瓶)
    // - SplashPotion (喷溅药水)
    // - LingeringPotion (滞留药水)
    //
    // 实体生成:
    // - Spawn Eggs (各种刷怪蛋)
    //
    // 特殊行为:
    // - FireCharge (火焰弹) - 发射火球
    // - FireworkRocket (烟花火箭) - 发射烟花
    // - Boat variants (各种船) - 放置船
    // - Bucket variants (各种桶) - 放置/收集流体
    // - FlintAndSteel (打火石) - 点火
    // - BoneMeal (骨粉) - 催熟作物
    // - TNT (TNT) - 生成点燃的TNT
    // - ShulkerBox (潜影盒) - 放置方块
    // - GlassBottle (玻璃瓶) - 收集液体
    // - Glowstone (萤石) - 充能重生锚
    // - Shears (剪刀) - 采集蜂巢
}

} // namespace blocks
} // namespace mc
