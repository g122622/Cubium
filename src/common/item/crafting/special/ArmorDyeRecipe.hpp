#pragma once

#include "item/crafting/SpecialRecipe.hpp"
#include "item/core/ItemStack.hpp"
#include "core/Types.hpp"

namespace mc {
namespace crafting {

/**
 * @brief 盔甲染色配方
 *
 * 允许玩家在工作台中使用染料为可染色盔甲上色。
 * 支持混合多种染料，结果颜色通过颜色混合算法计算。
 *
 * 参考: net.minecraft.item.crafting.ArmorDyeRecipe
 *
 * 匹配条件：
 * - 必须有恰好一个可染色盔甲
 * - 必须至少有一个染料
 *
 * 使用示例：
 * @code
 * // 玩家在工作台中放入皮革盔甲和多种染料
 * // 结果：染色的皮革盔甲
 * @endcode
 */
class ArmorDyeRecipe : public SpecialRecipe {
public:
    explicit ArmorDyeRecipe(const ResourceLocation& id);

    /**
     * @brief 检查是否匹配染色配方
     * @param inventory 合成网格
     * @return 如果有一个可染色盔甲和至少一个染料返回 true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成染色后的盔甲
     * @param inventory 合成网格
     * @return 染色后的盔甲
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 空列表（染料被消耗）
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 检查物品是否为可染色盔甲
     * @param stack 物品堆
     * @return 如果是可染色盔甲返回 true
     */
    [[nodiscard]] static bool isDyeableArmor(const ItemStack& stack);

    /**
     * @brief 检查物品是否为染料
     * @param stack 物品堆
     * @return 如果是染料返回 true
     */
    [[nodiscard]] static bool isDye(const ItemStack& stack);

    /**
     * @brief 混合两种颜色
     * @param color1 第一种颜色（ARGB格式）
     * @param color2 第二种颜色（ARGB格式）
     * @return 混合后的颜色
     */
    [[nodiscard]] static u32 mixColors(u32 color1, u32 color2);
};

} // namespace crafting
} // namespace mc
