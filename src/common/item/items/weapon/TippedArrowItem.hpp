#pragma once

#include "ArrowItem.hpp"
#include "../../potion/Potion.hpp"
#include <vector>

namespace mc {
namespace item {

/**
 * @brief 药水箭物品
 *
 * 带有药水效果的箭矢。命中生物时应用药水效果。
 * MC 1.16.5 中药水箭不受益于无限附魔。
 *
 * 参考 MC 1.16.5 TippedArrowItem
 */
class TippedArrowItem : public ArrowItem {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit TippedArrowItem(const ItemProperties& properties);

    ~TippedArrowItem() override = default;

    /**
     * @brief 创建箭矢实体
     *
     * 创建带有药水效果的箭矢实体。
     *
     * @param world 世界
     * @param stack 箭矢物品堆
     * @param shooter 射击者
     * @return 箭矢实体指针（调用者负责管理）
     */
    [[nodiscard]] entity::AbstractArrowEntity* createArrow(
        IWorld& world,
        const ItemStack& stack,
        LivingEntity& shooter) const override;

    /**
     * @brief 检查箭矢是否无限
     *
     * MC 1.16.5: 药水箭不受益于无限附魔。
     *
     * @param arrowStack 箭矢物品堆
     * @param bowStack 弓物品堆
     * @param player 玩家
     * @return 是否无限（总是返回 false）
     */
    [[nodiscard]] bool isInfinite(
        const ItemStack& arrowStack,
        const ItemStack& bowStack,
        Player& player) const override;

    /**
     * @brief 获取药水类型
     * @param stack 物品堆
     * @return 药水指针，无效返回 nullptr
     */
    [[nodiscard]] static const potion::Potion* getPotion(const ItemStack& stack);

    /**
     * @brief 获取药水效果列表
     * @param stack 物品堆
     * @return 效果列表
     */
    [[nodiscard]] static std::vector<entity::effect::EffectInstance> getEffects(const ItemStack& stack);

    /**
     * @brief 设置药水类型
     * @param stack 物品堆
     * @param potion 药水类型
     */
    static void setPotion(ItemStack& stack, const potion::Potion* potion);
};

} // namespace item
} // namespace mc
