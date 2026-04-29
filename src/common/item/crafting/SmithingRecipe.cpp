#include "SmithingRecipe.hpp"

namespace mc {
namespace crafting {

const String SmithingRecipe::EMPTY_GROUP = "";

SmithingRecipe::SmithingRecipe(
    const ResourceLocation& id,
    const Ingredient& base,
    const Ingredient& addition,
    const ItemStack& result
)
    : m_id(id)
    , m_base(base)
    , m_addition(addition)
    , m_result(result) {
    // 缓存原料列表
    m_ingredients.push_back(m_base);
    m_ingredients.push_back(m_addition);
}

bool SmithingRecipe::matches(const IInventory& inventory) const {
    // MC 原版：检查基础槽位和添加物槽位
    if (inventory.getContainerSize() < 2) {
        return false;
    }

    ItemStack baseStack = inventory.getItem(SLOT_BASE);
    ItemStack additionStack = inventory.getItem(SLOT_ADDITION);

    return m_base.test(baseStack) && m_addition.test(additionStack);
}

ItemStack SmithingRecipe::assemble(const IInventory& inventory) const {
    (void)inventory;
    // MC 原版：锻造结果复制基础物品的 NBT 数据
    // 这里简化实现，只返回结果物品
    return m_result.copy();
}

const std::vector<Ingredient>& SmithingRecipe::getIngredients() const {
    return m_ingredients;
}

std::vector<ItemStack> SmithingRecipe::getRemainingItems(const IInventory& inventory) const {
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
