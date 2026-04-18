#pragma once

#include "../../core/Item.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {

namespace potion {
class Potion;
}

namespace item {

/**
 * @brief 药水物品基类
 */
class PotionItem : public Item {
public:
    /**
     * @brief 构造药水物品
     */
    explicit PotionItem(const ItemProperties& properties);

    /**
     * @brief 获取使用时长
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 使用完成
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 右键使用物品
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 是否有药水效果
     */
    [[nodiscard]] bool hasEffect(const ItemStack& stack) const;

    /**
     * @brief 获取翻译键
     */
    [[nodiscard]] String getTranslationKey(const ItemStack& stack) const override;

private:
    /**
     * @brief 将药水效果应用到实体
     */
    void applyEffects(const potion::Potion* potion, Entity& entity, IWorld& world);
};

} // namespace item
} // namespace mc
