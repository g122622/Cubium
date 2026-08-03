/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "item/crafting/RecipeManager.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/SmeltingRecipe.hpp"
#include "common/item/crafting/SmithingRecipe.hpp"
#include "common/item/crafting/StonecuttingRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

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
        m_smeltingRecipesById.find(id) != m_smeltingRecipesById.end() ||
        m_stonecuttingRecipesById.find(id) != m_stonecuttingRecipesById.end() ||
        m_smithingRecipesById.find(id) != m_smithingRecipesById.end()) {
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
        m_recipesById.find(id) != m_recipesById.end() ||
        m_stonecuttingRecipesById.find(id) != m_stonecuttingRecipesById.end() ||
        m_smithingRecipesById.find(id) != m_smithingRecipesById.end()) {
        return false;
    }

    RecipeType type = recipe->getType();
    const SmeltingRecipe* recipePtr = recipe.get();
    m_smeltingRecipesById[id] = std::move(recipe);

    m_smeltingRecipesByType[type].push_back(recipePtr);
    return true;
}

bool RecipeManager::registerStonecuttingRecipe(std::unique_ptr<StonecuttingRecipe> recipe)
{
    if (!recipe) {
        return false;
    }

    ResourceLocation id = recipe->getId();

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_stonecuttingRecipesById.find(id) != m_stonecuttingRecipesById.end() ||
        m_recipesById.find(id) != m_recipesById.end() ||
        m_smeltingRecipesById.find(id) != m_smeltingRecipesById.end() ||
        m_smithingRecipesById.find(id) != m_smithingRecipesById.end()) {
        return false;
    }

    const StonecuttingRecipe* recipePtr = recipe.get();
    m_stonecuttingRecipesById[id] = std::move(recipe);

    m_stonecuttingRecipesByType[RecipeType::Stonecutting].push_back(recipePtr);
    return true;
}

bool RecipeManager::registerSmithingRecipe(std::unique_ptr<SmithingRecipe> recipe)
{
    if (!recipe) {
        return false;
    }

    ResourceLocation id = recipe->getId();

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_smithingRecipesById.find(id) != m_smithingRecipesById.end() ||
        m_recipesById.find(id) != m_recipesById.end() ||
        m_smeltingRecipesById.find(id) != m_smeltingRecipesById.end() ||
        m_stonecuttingRecipesById.find(id) != m_stonecuttingRecipesById.end()) {
        return false;
    }

    const SmithingRecipe* recipePtr = recipe.get();
    m_smithingRecipesById[id] = std::move(recipe);

    m_smithingRecipesByType[RecipeType::Smithing].push_back(recipePtr);
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
        m_smeltingRecipesById.find(id) != m_smeltingRecipesById.end() ||
        m_stonecuttingRecipesById.find(id) != m_stonecuttingRecipesById.end() ||
        m_smithingRecipesById.find(id) != m_smithingRecipesById.end();
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
        if (recipe->matches(inventory)) {
            return recipe;
        }
    }

    return nullptr;
}

const StonecuttingRecipe* RecipeManager::getStonecuttingRecipe(const ResourceLocation& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_stonecuttingRecipesById.find(id);
    if (it != m_stonecuttingRecipesById.end()) {
        return it->second.get();
    }
    return nullptr;
}

const SmithingRecipe* RecipeManager::getSmithingRecipe(const ResourceLocation& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_smithingRecipesById.find(id);
    if (it != m_smithingRecipesById.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<const StonecuttingRecipe*> RecipeManager::getAllStonecuttingRecipes() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const StonecuttingRecipe*> result;
    result.reserve(m_stonecuttingRecipesById.size());

    for (const auto& pair : m_stonecuttingRecipesById) {
        result.push_back(pair.second.get());
    }

    return result;
}

std::vector<const SmithingRecipe*> RecipeManager::getAllSmithingRecipes() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<const SmithingRecipe*> result;
    result.reserve(m_smithingRecipesById.size());

    for (const auto& pair : m_smithingRecipesById) {
        result.push_back(pair.second.get());
    }

    return result;
}

std::vector<const StonecuttingRecipe*> RecipeManager::findStonecuttingRecipes(const ItemStack& input) const
{
    std::vector<const StonecuttingRecipe*> result;

    if (input.isEmpty()) {
        return result;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& pair : m_stonecuttingRecipesById) {
        const StonecuttingRecipe* recipe = pair.second.get();
        const Ingredient& ingredient = recipe->getIngredient();
        if (ingredient.test(input)) {
            result.push_back(recipe);
        }
    }

    return result;
}

std::vector<const SmithingRecipe*> RecipeManager::findSmithingRecipes(const ItemStack& input) const
{
    std::vector<const SmithingRecipe*> result;

    if (input.isEmpty()) {
        return result;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& pair : m_smithingRecipesById) {
        const SmithingRecipe* recipe = pair.second.get();
        const Ingredient& base = recipe->getBase();
        if (base.test(input)) {
            result.push_back(recipe);
        }
    }

    return result;
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

    // 检查转化配方（crafting_transmute，如收纳袋染色）
    auto transmuteIt = m_recipesByType.find(RecipeType::Transmute);
    if (transmuteIt != m_recipesByType.end()) {
        for (const CraftingRecipe* recipe : transmuteIt->second) {
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
    return m_recipesById.size() + m_smeltingRecipesById.size() + m_stonecuttingRecipesById.size() +
        m_smithingRecipesById.size();
}

void RecipeManager::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recipesById.clear();
    m_smeltingRecipesById.clear();
    m_stonecuttingRecipesById.clear();
    m_smithingRecipesById.clear();
    m_recipesByType.clear();
    m_smeltingRecipesByType.clear();
    m_stonecuttingRecipesByType.clear();
    m_smithingRecipesByType.clear();
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