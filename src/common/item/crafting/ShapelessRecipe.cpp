#include "item/crafting/ShapelessRecipe.hpp"
#include <algorithm>

namespace mc {
namespace crafting {

ShapelessRecipe::ShapelessRecipe(const ResourceLocation& id,
                                 std::vector<Ingredient> ingredients,
                                 ItemStack result,
                                 const std::string& group)
    : m_id(id)
    , m_ingredients(std::move(ingredients))
    , m_result(std::move(result))
    , m_group(group) {
    // 计算是否为简单配方
    m_isSimple = true;
    for (const Ingredient& ingredient : m_ingredients) {
        if (!ingredient.isSimple()) {
            m_isSimple = false;
            break;
        }
    }
}

bool ShapelessRecipe::matches(const CraftingInventory& inventory) const {
    // 统计网格中的非空槽位数量
    i32 nonEmptySlots = 0;
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!inventory.getItem(i).isEmpty()) {
            ++nonEmptySlots;
        }
    }

    // 原料数量必须等于非空槽位数量
    if (static_cast<i32>(m_ingredients.size()) != nonEmptySlots) {
        return false;
    }

    // 跟踪已使用的槽位
    std::vector<bool> used(inventory.getContainerSize(), false);

    // 使用回溯算法进行匹配
    // MC 原版对于简单配方使用 RecipeItemHelper 优化，复杂配方使用回溯
    // 这里统一使用回溯算法以确保正确性
    return matchWithBacktracking(inventory, used, 0);
}

bool ShapelessRecipe::matchWithBacktracking(const CraftingInventory& inventory,
                                             std::vector<bool>& used,
                                             i32 ingredientIndex) const {
    // 所有原料都已匹配
    if (ingredientIndex >= static_cast<i32>(m_ingredients.size())) {
        return true;
    }

    const Ingredient& ingredient = m_ingredients[ingredientIndex];

    // 尝试为当前原料找一个匹配的槽位
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!used[i] && ingredient.test(inventory.getItem(i))) {
            used[i] = true;

            // 递归匹配下一个原料
            if (matchWithBacktracking(inventory, used, ingredientIndex + 1)) {
                return true;
            }

            // 回溯，尝试其他槽位
            used[i] = false;
        }
    }

    // 没有找到匹配
    return false;
}

ItemStack ShapelessRecipe::assemble(const CraftingInventory& inventory) const {
    (void)inventory;
    return m_result.copy();
}

bool ShapelessRecipe::canFitIn(i32 width, i32 height) const {
    return static_cast<i32>(m_ingredients.size()) <= width * height;
}

std::vector<ItemStack> ShapelessRecipe::getRemainingItems(const CraftingInventory& inventory) const {
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
