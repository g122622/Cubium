#pragma once

#include "../../core/Item.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {

// 前向声明
namespace potion {
class Potion;
}

namespace item {

/**
 * @brief 药水物品基类
 *
 * 可饮用的药水，饮用后应用效果。
 * 包括普通药水、喷溅药水、滞留药水。
 *
 * 参考: net.minecraft.item.PotionItem
 */
class PotionItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit PotionItem(const ItemProperties& properties);

    // ========== 使用行为 ==========

    /**
     * @brief 获取使用时间
     * @param stack 物品堆
     * @return 使用时间（tick），药水为32
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     * @param stack 物品堆
     * @return 饮用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 物品使用完成
     *
     * 当玩家完成饮用时调用，应用药水效果。
     *
     * @param stack 物品堆
     * @param world 世界
     * @param entity 使用实体
     * @return 使用后的物品堆
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, LivingEntity& entity) override;

    /**
     * @brief 右键使用物品
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 是否有附魔光效
     * @param stack 物品堆
     * @return 如果药水有效果则返回true
     */
    [[nodiscard]] bool hasEffect(const ItemStack& stack) const;

    /**
     * @brief 获取翻译键
     * @param stack 物品堆
     * @return 带药水类型的翻译键
     */
    [[nodiscard]] String getTranslationKey(const ItemStack& stack) const override;

private:
    /**
     * @brief 应用药水效果到实体
     * @param potion 药水类型
     * @param entity 目标实体
     * @param world 世界
     */
    void applyEffects(const potion::Potion* potion, LivingEntity& entity, IWorld& world);
};

} // namespace item
} // namespace mc
