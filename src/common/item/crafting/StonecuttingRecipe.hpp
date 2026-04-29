#pragma once

#include "IRecipe.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {
namespace crafting {

/**
 * @brief 切石机配方
 *
 * 切石机配方将一个输入物品转换为输出物品。
 * 与熔炼配方类似，但不需要熔炼时间和经验值。
 * 参考: net.minecraft.item.crafting.StonecuttingRecipe
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:stonecutting",
 *   "ingredient": { "item": "minecraft:stone" },
 *   "result": "minecraft:stone_bricks",
 *   "count": 1
 * }
 * @endcode
 */
class StonecuttingRecipe : public IRecipe<IInventory> {
public:
    /**
     * @brief 构造函数
     * @param id 配方ID
     * @param group 分组名（切石机配方通常不使用分组）
     * @param ingredient 输入原料
     * @param result 结果物品
     * @param count 结果数量
     */
    StonecuttingRecipe(
        const ResourceLocation& id,
        const String& group,
        const Ingredient& ingredient,
        const ItemStack& result,
        i32 count = 1
    );

    ~StonecuttingRecipe() override = default;

    // ========== IRecipe 接口实现 ==========

    [[nodiscard]] bool matches(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override;
    [[nodiscard]] const String& getGroup() const override { return m_group; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] RecipeType getType() const override { return RecipeType::Stonecutting; }

    /**
     * @brief 切石机配方始终可以适应（只有一个输入槽）
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override {
        (void)width;
        (void)height;
        return true;
    }

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 容器
     * @return 每个槽位的剩余物品堆列表
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const IInventory& inventory) const override;

    // ========== 切石机特有方法 ==========

    /**
     * @brief 获取输入原料
     * @return 输入原料
     */
    [[nodiscard]] const Ingredient& getIngredient() const { return m_ingredient; }

    /**
     * @brief 获取结果数量
     * @return 结果数量
     */
    [[nodiscard]] i32 getCount() const { return m_count; }

private:
    ResourceLocation m_id;
    String m_group;
    Ingredient m_ingredient;
    ItemStack m_result;
    i32 m_count;
    mutable std::vector<Ingredient> m_ingredients;  ///< 缓存的原料列表
};

} // namespace crafting
} // namespace mc
