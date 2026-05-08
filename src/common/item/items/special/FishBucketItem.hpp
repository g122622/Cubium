#pragma once

#include "../../core/Item.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc {
namespace entity {
    class EntityType;
}

namespace item {

/**
 * @brief 鱼桶物品
 *
 * 右键使用时放置水并生成鱼实体。
 * 参考 MC 1.16.5: net.minecraft.item.FishBucketItem
 */
class FishBucketItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param fishTypeName 鱼实体类型名称（如 "minecraft:cod"）
     * @param properties 物品属性
     */
    FishBucketItem(
        const char* fishTypeName,
        const ItemProperties& properties);

    ~FishBucketItem() override = default;

    /**
     * @brief 获取鱼类型名称
     */
    [[nodiscard]] const std::string& getFishTypeName() const { return m_fishTypeName; }

    /**
     * @brief 方块交互 - 放置水并生成鱼
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    /**
     * @brief 右键使用 - 在水中生成鱼
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

private:
    /**
     * @brief 在指定位置生成鱼
     * @param world 世界
     * @param pos 位置
     * @return 是否成功生成
     */
    bool spawnFish(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 返回空桶给玩家
     *
     * 参考 MC 1.16.5 BucketItem.emptyBucket()
     * - 如果当前物品堆已空，直接替换为空桶
     * - 否则尝试添加到背包，背包满则掉落
     *
     * @param player 玩家
     * @param stack 当前物品堆（可能被修改）
     */
    void returnEmptyBucket(Player& player, ItemStack& stack) const;

    std::string m_fishTypeName;
};

} // namespace item
} // namespace mc
