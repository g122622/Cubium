#pragma once

#include "item/crafting/SpecialRecipe.hpp"
#include "item/core/ItemStack.hpp"

namespace mc {
namespace crafting {

/**
 * @brief 地图复制配方
 *
 * 允许玩家使用已填充地图和空地图复制地图。
 * 复制的地图与原地图共享相同的地图数据。
 *
 * 参考: net.minecraft.item.crafting.MapCloningRecipe
 *
 * 匹配条件：
 * - 必须有一张已填充地图（FilledMapItem）
 * - 必须有至少一张空地图（EmptyMapItem）
 *
 * 结果：
 * - 输出数量 = 空地图数量 + 1（原地图保留）
 */
class MapCloningRecipe : public SpecialRecipe {
public:
    explicit MapCloningRecipe(const ResourceLocation& id);

    /**
     * @brief 检查是否匹配地图复制配方
     * @param inventory 合成网格
     * @return 如果有一张已填充地图和至少一张空地图返回 true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成复制的地图
     * @param inventory 合成网格
     * @return 复制的地图堆（数量 = 空地图数量 + 1）
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 原地图保留在原位置
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 检查物品是否为已填充地图
     * @param stack 物品堆
     * @return 如果是已填充地图返回 true
     */
    [[nodiscard]] static bool isFilledMap(const ItemStack& stack);

    /**
     * @brief 检查物品是否为空地图
     * @param stack 物品堆
     * @return 如果是空地图返回 true
     */
    [[nodiscard]] static bool isEmptyMap(const ItemStack& stack);
};

} // namespace crafting
} // namespace mc
