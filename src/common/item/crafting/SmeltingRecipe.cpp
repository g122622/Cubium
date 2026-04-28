#include "SmeltingRecipe.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"

namespace mc {
namespace crafting {

SmeltingRecipe::SmeltingRecipe(
    const ResourceLocation& id,
    const String& group,
    const Ingredient& ingredient,
    const ItemStack& result,
    f32 experience,
    i32 cookTime
)
    : m_id(id)
    , m_group(group)
    , m_ingredient(ingredient)
    , m_result(result)
    , m_experience(experience)
    , m_cookTime(cookTime) {
    m_ingredients.push_back(m_ingredient);
}

bool SmeltingRecipe::matches(const blockentity::FurnaceInventory& inventory) const {
    // MC 原版：检查输入槽（槽位0）是否匹配原料
    return m_ingredient.test(inventory.getItem(0));
}

ItemStack SmeltingRecipe::assemble(const blockentity::FurnaceInventory& inventory) const {
    (void)inventory;
    return m_result.copy();
}

const std::vector<Ingredient>& SmeltingRecipe::getIngredients() const {
    return m_ingredients;
}

} // namespace crafting
} // namespace mc
