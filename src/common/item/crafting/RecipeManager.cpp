#include "item/crafting/RecipeManager.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include <algorithm>

namespace mc {
namespace crafting {

RecipeManager& RecipeManager::instance()
{
    static RecipeManager instance;
    return instance;
}

bool RecipeManager::registerRecipe(std::unique_ptr<CraftingRecipe> recipe)
{
    if (!recipe) {
        return false;
    }

    ResourceLocation id = recipe->getId();

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_recipesById.find(id) != m_recipesById.end() ||
        m_smeltingRecipesById.find(id) != m_smeltingRecipesById.end()) {
        return false;
    }

    RecipeType type = recipe->getType();
    const Item* resultItem = recipe->getResultItem().getItem();

    const CraftingRecipe* recipePtr = recipe.get();
    m_recipesById[id] = std::move(recipe);

    m_recipesByType[type].push_back(recipePtr);

    if (resultItem != nullptr) {
        m_recipesByResult[resultItem->itemId()].push_back(recipePtr);
    }

    return true;
}

bool RecipeManager::registerSmeltingRecipe(std::unique_ptr<SmeltingRecipe> recipe)
{
    if (!recipe) {
        return false;
    }

    ResourceLocation id = recipe->getId();

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_smeltingRecipesById.find(id) != m_smeltingRecipesById.end() ||
        m_recipesById.find(id) != m_recipesById.end()) {
        return false;
    }

    RecipeType type = recipe->getType();
    const SmeltingRecipe* recipePtr = recipe.get();
    m_smeltingRecipesById[id] = std::move(recipe);

    m_smeltingRecipesByType[type].push_back(recipePtr);
    return true;
}

const CraftingRecipe* RecipeManager::getRecipe(const ResourceLocation& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_recipesById.find(id);
    if (it != m_recipesById.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool RecipeManager::hasRecipe(const ResourceLocation& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recipesById.find(id) != m_recipesById.end() ||
        m_smeltingRecipesById.find(id) != m_smeltingRecipesById.end();
}

std::vector<const CraftingRecipe*> RecipeManager::getAllRecipes() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const CraftingRecipe*> result;
    result.reserve(m_recipesById.size());

    for (const auto& pair : m_recipesById) {
        result.push_back(pair.second.get());
    }

    return result;
}

std::vector<const CraftingRecipe*> RecipeManager::getRecipesByType(RecipeType type) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_recipesByType.find(type);
    if (it != m_recipesByType.end()) {
        return it->second;
    }
    return {};
}

std::vector<const SmeltingRecipe*> RecipeManager::getSmeltingRecipesByType(RecipeType type) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_smeltingRecipesByType.find(type);
    if (it != m_smeltingRecipesByType.end()) {
        return it->second;
    }
    return {};
}

const SmeltingRecipe* RecipeManager::getSmeltingRecipe(const ItemStack& input, RecipeType type) const
{
    if (input.isEmpty()) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_smeltingRecipesByType.find(type);
    if (it == m_smeltingRecipesByType.end()) {
        return nullptr;
    }

    blockentity::FurnaceInventory inventory;
    inventory.setInputItem(input);

    for (const SmeltingRecipe* recipe : it->second) {
        if (recipe != nullptr && recipe->matches(inventory)) {
            return recipe;
        }
    }

    return nullptr;
}

const CraftingRecipe* RecipeManager::findMatchingRecipe(const CraftingInventory& inventory) const
{

    std::lock_guard<std::mutex> lock(m_mutex);

    auto shapedIt = m_recipesByType.find(RecipeType::ShapedCrafting);
    if (shapedIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : shapedIt->second) {
            if (recipe->matches(inventory)) {
                return recipe;
            }
        }
    }

    auto shapelessIt = m_recipesByType.find(RecipeType::ShapelessCrafting);
    if (shapelessIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : shapelessIt->second) {
            if (recipe->matches(inventory)) {
                return recipe;
            }
        }
    }

    auto craftingIt = m_recipesByType.find(RecipeType::Crafting);
    if (craftingIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : craftingIt->second) {
            if (recipe->matches(inventory)) {
                return recipe;
            }
        }
    }

    // 最后检查特殊配方（动态配方，如物品修复、染色等）
    auto specialIt = m_recipesByType.find(RecipeType::Special);
    if (specialIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : specialIt->second) {
            if (recipe->matches(inventory)) {
                return recipe;
            }
        }
    }

    return nullptr;
}

std::vector<const CraftingRecipe*> RecipeManager::findAllMatchingRecipes(const CraftingInventory& inventory) const
{

    std::vector<const CraftingRecipe*> result;
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& pair : m_recipesById) {
        if (pair.second->matches(inventory)) {
            result.push_back(pair.second.get());
        }
    }

    return result;
}

std::vector<const CraftingRecipe*> RecipeManager::getRecipesForResult(const Item& result) const
{

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_recipesByResult.find(result.itemId());
    if (it != m_recipesByResult.end()) {
        return it->second;
    }
    return {};
}

std::vector<const CraftingRecipe*> RecipeManager::getRecipesForResult(const ItemStack& result) const
{

    if (result.isEmpty()) {
        return {};
    }
    return getRecipesForResult(*result.getItem());
}

size_t RecipeManager::getRecipeCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recipesById.size() + m_smeltingRecipesById.size();
}

void RecipeManager::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recipesById.clear();
    m_smeltingRecipesById.clear();
    m_recipesByType.clear();
    m_smeltingRecipesByType.clear();
    m_recipesByResult.clear();
}

void RecipeManager::forEachRecipe(std::function<void(const CraftingRecipe&)> callback) const
{

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& pair : m_recipesById) {
        callback(*pair.second);
    }
}

} // namespace crafting
} // namespace mc