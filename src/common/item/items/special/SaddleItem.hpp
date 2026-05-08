#pragma once

#include "common/item/core/Item.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 鞍物品
 *
 * 用于装备可骑乘实体（猪、炽足兽、马等）。
 * 玩家对实体使用鞍时，如果实体支持装备鞍且未装备，
 * 则装备鞍并消耗一个鞍物品。
 *
 * 参考: net.minecraft.item.SaddleItem
 */
class SaddleItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     */
    explicit SaddleItem(const ItemProperties& properties);

    /**
     * @brief 与实体交互
     *
     * 当玩家右键点击实体时调用。
     * 如果目标实体实现了 IEquipable 接口且可以装备鞍，
     * 则装备鞍并播放音效。
     *
     * @param stack 物品堆
     * @param player 玩家
     * @param target 目标实体
     * @param hand 使用的手
     * @return 是否成功交互
     */
    bool itemInteractionForEntity(ItemStack& stack, Player& player,
                                  LivingEntity& target, Hand hand) override;
};

} // namespace item::items
} // namespace mc
