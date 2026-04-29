#include "StonecuttingRecipe.hpp"

namespace mc {
namespace crafting {

StonecuttingRecipe::StonecuttingRecipe(
    const ResourceLocation& id,
    const String& group,
    const Ingredient& ingredient,
    const ItemStack& result,
    i32 count
)
    : m_id(id)
    , m_group(group)
    , m_ingredient(ingredient)
    , m_result(result)
    , m_count(count) {
    // 缓存原料列表
    m_ingredients.push_back(m_ingredient);
}

bool StonecuttingRecipe::matches(const IInventory& inventory) const {
    // MC 原版：检查输入槽（槽位0）是否匹配原料
    if (inventory.getContainerSize() == 0) {
        return false;
    }
    return m_ingredient.test(inventory.getItem(0));
}

ItemStack StonecuttingRecipe::assemble(const IInventory& inventory) const {
    (void)inventory;
    ItemStack result = m_result;
    result.setCount(m_count);
    return result;
}

const std::vector<Ingredient>& StonecuttingRecipe::getIngredients() const {
    return m_ingredients;
}

std::vector<ItemStack> StonecuttingRecipe::getRemainingItems(const IInventory& inventory) const {
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
