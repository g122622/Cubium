#pragma once

#include "../../core/Item.hpp"
#include "../../core/UseAction.hpp"

namespace mc {

class Player;
class IWorld;
class Entity;

namespace item {
namespace special {

/**
 * @brief 牛奶桶物品
 *
 * 牛奶桶用于清除玩家身上所有药水效果。
 * - 饮用时间：32 ticks (1.6秒)
 * - 饮用后返回空桶
 * - 清除所有效果（包括正面和负面效果）
 * - 不恢复饥饿值
 *
 * 参考: net.minecraft.item.MilkBucketItem
 */
class MilkBucketItem : public Item {
public:
    /**
     * @brief 构造牛奶桶
     * @param properties 物品属性
     */
    explicit MilkBucketItem(ItemProperties properties);

    ~MilkBucketItem() override = default;

    /**
     * @brief 获取使用时长
     *
     * 牛奶桶饮用时间为 32 ticks (1.6秒)
     *
     * @param stack 物品堆
     * @return 使用时长 (ticks)
     */
    [[nodiscard]] i32 getUseDuration(const ItemStack& stack) const override;

    /**
     * @brief 获取使用动作
     *
     * 牛奶桶返回 Drink 动作（饮用）
     *
     * @param stack 物品堆
     * @return 饮用动作
     */
    [[nodiscard]] UseAction getUseAction(const ItemStack& stack) const override;

    /**
     * @brief 右键使用物品
     *
     * 检查玩家是否可以饮用牛奶。
     * 创造模式总是可以饮用，生存模式需要至少有一个效果才能饮用。
     *
     * @param world 世界引用
     * @param player 玩家
     * @param hand 使用的手
     * @return 动作结果
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    /**
     * @brief 使用完成（饮用完成）
     *
     * 清除玩家身上所有药水效果，然后返回空桶。
     *
     * @param stack 物品堆
     * @param world 世界引用
     * @param entity 使用者实体（通常是玩家）
     * @return 结果物品堆（空桶）
     */
    ItemStack onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) override;

    /**
     * @brief 是否可以食用
     *
     * 牛奶桶可以在任何时候饮用，即使没有药水效果。
     * 创造模式可以饮用，生存模式也可以饮用。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @return 是否可以饮用
     */
    [[nodiscard]] bool canEat(const ItemStack& stack, const Player& player) const override;
};

} // namespace special
} // namespace item
} // namespace mc
