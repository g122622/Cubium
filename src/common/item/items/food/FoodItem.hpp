#pragma once

#include "../../core/Item.hpp"
#include "../../food/Food.hpp"
#include "../../core/UseAction.hpp"

namespace mc {

// Forward declarations
class Player;
class IWorld;
class LivingEntity;

namespace item::items {

/**
 * @brief 食物物品基类
 *
 * 所有可食用物品的基类。
 * 参考: net.minecraft.item.FoodItem
 *
 * 用法示例:
 * @code
 * // 注册苹果
 * auto& apple = ItemRegistry::instance().registerItem<FoodItem>(
 *     ResourceLocation("minecraft:apple"),
 *     ItemProperties().maxStackSize(64).food(&Foods::APPLE)
 * );
 * @endcode
 */
class FoodItem : public Item {
public:
    /**
     * @brief 构造食物物品
     * @param food 食物属性
     * @param properties 物品属性
     */
    FoodItem(const food::Food* food, ItemProperties properties);

    // ========== 物品重写方法 ==========

    /**
     * @brief 是否为食物
     */
    [[nodiscard]] bool isFood() const override { return m_food != nullptr; }

    /**
     * @brief 获取食物属性
     */
    [[nodiscard]] const food::Food* getFood() const override { return m_food; }

    /**
     * @brief 获取使用时间
     *
     * 普通食物32ticks，快速食用食物16ticks。
     *
     * @param stack 物品堆
     * @return 使用时间（ticks）
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     * @param stack 物品堆
     * @return 使用动作类型
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 如果玩家可以食用，返回成功结果。
     *
     * @param world 世界
     * @param player 玩家
     * @param hand 使用的手
     * @return 动作结果
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 物品使用完成
     *
     * 应用食物效果：
     * - 恢复饥饿值和饱和度
     * - 应用药水效果（如金苹果）
     * - 返回容器物品（如碗、瓶）
     *
     * @param stack 物品堆
     * @param world 世界
     * @param entity 使用者
     * @return 使用后的物品堆
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, LivingEntity& entity) override;

    /**
     * @brief 检查是否可以食用
     * @param stack 物品堆
     * @param player 玩家
     * @return 是否可以食用
     */
    [[nodiscard]] bool canEat(const ItemStack& stack, const Player& player) const override;

protected:
    const food::Food* m_food;
};

} // namespace item::items
} // namespace mc
