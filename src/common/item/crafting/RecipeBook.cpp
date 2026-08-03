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

#include "RecipeBook.hpp"
#include "RecipeManager.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/nbt/Nbt.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace crafting {

// ========== RecipeBookCategory ==========

const char* recipeBookCategoryToString(RecipeBookCategory category) noexcept
{
    switch (category) {
        case RecipeBookCategory::Crafting:
            return "crafting";
        case RecipeBookCategory::Furnace:
            return "furnace";
        case RecipeBookCategory::BlastFurnace:
            return "blast_furnace";
        case RecipeBookCategory::Smoker:
            return "smoker";
        default:
            return "unknown";
    }
}

std::optional<RecipeBookCategory> recipeBookCategoryFromString(const std::string& str) noexcept
{
    if (str == "crafting" || str == "minecraft:crafting") {
        return RecipeBookCategory::Crafting;
    }
    if (str == "furnace" || str == "minecraft:furnace") {
        return RecipeBookCategory::Furnace;
    }
    if (str == "blast_furnace" || str == "minecraft:blast_furnace") {
        return RecipeBookCategory::BlastFurnace;
    }
    if (str == "smoker" || str == "minecraft:smoker") {
        return RecipeBookCategory::Smoker;
    }
    return std::nullopt;
}

// ========== RecipeBookStatus ==========

RecipeBookStatus::RecipeBookStatus()
{
    // 默认构造：所有分类状态为 false
    for (size_t i = 0; i < static_cast<size_t>(RecipeBookCategory::Count); ++i) {
        m_status[i] = CategoryStatus(false, false);
    }
}

bool RecipeBookStatus::isGuiOpen(RecipeBookCategory category) const noexcept
{
    const size_t index = static_cast<size_t>(category);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(RecipeBookCategory::Count));
    return m_status[index].guiOpen;
}

void RecipeBookStatus::setGuiOpen(RecipeBookCategory category, bool open) noexcept
{
    const size_t index = static_cast<size_t>(category);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(RecipeBookCategory::Count));
    m_status[index].guiOpen = open;
}

bool RecipeBookStatus::isFilteringCraftable(RecipeBookCategory category) const noexcept
{
    const size_t index = static_cast<size_t>(category);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(RecipeBookCategory::Count));
    return m_status[index].filteringCraftable;
}

void RecipeBookStatus::setFilteringCraftable(RecipeBookCategory category, bool filtering) noexcept
{
    const size_t index = static_cast<size_t>(category);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(RecipeBookCategory::Count));
    m_status[index].filteringCraftable = filtering;
}

void RecipeBookStatus::setCategoryStatus(RecipeBookCategory category, bool open, bool filtering) noexcept
{
    const size_t index = static_cast<size_t>(category);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(RecipeBookCategory::Count));
    m_status[index].guiOpen = open;
    m_status[index].filteringCraftable = filtering;
}

void RecipeBookStatus::copyFrom(const RecipeBookStatus& other) noexcept
{
    for (size_t i = 0; i < static_cast<size_t>(RecipeBookCategory::Count); ++i) {
        m_status[i] = other.m_status[i].copy();
    }
}

RecipeBookStatus RecipeBookStatus::copy() const noexcept
{
    RecipeBookStatus result;
    result.copyFrom(*this);
    return result;
}

bool RecipeBookStatus::operator==(const RecipeBookStatus& other) const noexcept
{
    for (size_t i = 0; i < static_cast<size_t>(RecipeBookCategory::Count); ++i) {
        if (!(m_status[i] == other.m_status[i])) {
            return false;
        }
    }
    return true;
}

// ========== RecipeBook ==========

void RecipeBook::unlock(const ResourceLocation& recipeId)
{
    m_recipes.insert(recipeId);
}

void RecipeBook::lock(const ResourceLocation& recipeId)
{
    m_recipes.erase(recipeId);
    m_newRecipes.erase(recipeId);
}

bool RecipeBook::isUnlocked(const ResourceLocation& recipeId) const noexcept
{
    return m_recipes.contains(recipeId);
}

bool RecipeBook::isBookRecipe(const ResourceLocation& recipeId) const noexcept
{
    // 未解锁的配方不在配方书中
    if (!m_recipes.contains(recipeId)) {
        return false;
    }

    // 动态配方不在配方书中显示
    // 参考 MC 1.21.1: ServerRecipeBook.addRecipes() 中的 !recipe.isSpecial() 过滤逻辑
    // 动态配方（染色、修复、书本复制等）虽然可以合成，但不列入配方书
    if (ServerRecipeBook::isDynamicRecipe(recipeId)) {
        return false;
    }

    return true;
}

bool RecipeBook::isNew(const ResourceLocation& recipeId) const noexcept
{
    return m_newRecipes.contains(recipeId);
}

void RecipeBook::markNew(const ResourceLocation& recipeId)
{
    m_newRecipes.insert(recipeId);
}

void RecipeBook::markSeen(const ResourceLocation& recipeId)
{
    m_newRecipes.erase(recipeId);
}

void RecipeBook::copyFrom(const RecipeBook& other)
{
    m_recipes = other.m_recipes;
    m_newRecipes = other.m_newRecipes;
    m_status.copyFrom(other.m_status);
}

void RecipeBook::clear() noexcept
{
    m_recipes.clear();
    m_newRecipes.clear();
}

// ========== ServerRecipeBook ==========

bool ServerRecipeBook::isDynamicRecipe(const ResourceLocation& recipeId)
{
    const CraftingRecipe* recipe = RecipeManager::instance().getRecipe(recipeId);
    return recipe != nullptr && recipe->isDynamic();
}

nbt::tags::compound_tag ServerRecipeBook::write() const
{
    nbt::tags::compound_tag tag;

    // 写入 GUI 状态
    // Crafting
    tag.put("isGuiOpen", static_cast<i8>(m_status.isGuiOpen(RecipeBookCategory::Crafting)));
    tag.put("isFilteringCraftable", static_cast<i8>(m_status.isFilteringCraftable(RecipeBookCategory::Crafting)));

    // Furnace
    tag.put("isFurnaceGuiOpen", static_cast<i8>(m_status.isGuiOpen(RecipeBookCategory::Furnace)));
    tag.put("isFurnaceFilteringCraftable", static_cast<i8>(m_status.isFilteringCraftable(RecipeBookCategory::Furnace)));

    // Blast Furnace
    tag.put("isBlastingFurnaceGuiOpen", static_cast<i8>(m_status.isGuiOpen(RecipeBookCategory::BlastFurnace)));
    tag.put("isBlastingFurnaceFilteringCraftable",
        static_cast<i8>(m_status.isFilteringCraftable(RecipeBookCategory::BlastFurnace)));

    // Smoker
    tag.put("isSmokerGuiOpen", static_cast<i8>(m_status.isGuiOpen(RecipeBookCategory::Smoker)));
    tag.put("isSmokerFilteringCraftable", static_cast<i8>(m_status.isFilteringCraftable(RecipeBookCategory::Smoker)));

    // 写入已解锁配方列表
    auto recipesList = std::make_unique<nbt::tags::string_list_tag>();
    recipesList->value.reserve(m_recipes.size());
    for (const auto& recipeId : m_recipes) {
        recipesList->value.push_back(recipeId.toString());
    }
    tag.value["recipes"] = std::move(recipesList);

    // 写入新解锁配方列表（待显示）
    auto newRecipesList = std::make_unique<nbt::tags::string_list_tag>();
    newRecipesList->value.reserve(m_newRecipes.size());
    for (const auto& recipeId : m_newRecipes) {
        newRecipesList->value.push_back(recipeId.toString());
    }
    tag.value["toBeDisplayed"] = std::move(newRecipesList);

    return tag;
}

void ServerRecipeBook::read(const nbt::tags::compound_tag& tag)
{
    // 读取 GUI 状态
    // 使用 try-catch 或检查键是否存在
    auto tryGetBool = [&tag](const std::string& key) -> bool {
        auto it = tag.value.find(key);
        if (it != tag.value.end()) {
            auto* byteTag = dynamic_cast<const nbt::tags::byte_tag*>(it->second.get());
            if (byteTag) {
                return byteTag->value != 0;
            }
        }
        return false;
    };

    // Crafting
    m_status.setCategoryStatus(
        RecipeBookCategory::Crafting, tryGetBool("isGuiOpen"), tryGetBool("isFilteringCraftable"));

    // Furnace
    m_status.setCategoryStatus(
        RecipeBookCategory::Furnace, tryGetBool("isFurnaceGuiOpen"), tryGetBool("isFurnaceFilteringCraftable"));

    // Blast Furnace
    m_status.setCategoryStatus(RecipeBookCategory::BlastFurnace,
        tryGetBool("isBlastingFurnaceGuiOpen"),
        tryGetBool("isBlastingFurnaceFilteringCraftable"));

    // Smoker
    m_status.setCategoryStatus(
        RecipeBookCategory::Smoker, tryGetBool("isSmokerGuiOpen"), tryGetBool("isSmokerFilteringCraftable"));

    // 读取已解锁配方列表
    // 参考 MC 1.21.1: ServerRecipeBook.loadUntrusted() 在加载时过滤掉不存在的配方
    m_recipes.clear();
    auto recipesIt = tag.value.find("recipes");
    if (recipesIt != tag.value.end()) {
        auto* recipesList = dynamic_cast<const nbt::tags::string_list_tag*>(recipesIt->second.get());
        if (recipesList) {
            for (const auto& recipeStr : recipesList->value) {
                ResourceLocation recipeId(recipeStr);
                // 过滤掉动态配方：旧存档可能包含动态配方ID，加载时应排除
                if (!isDynamicRecipe(recipeId)) {
                    m_recipes.insert(std::move(recipeId));
                }
            }
        }
    }

    // 读取新解锁配方列表
    m_newRecipes.clear();
    auto newRecipesIt = tag.value.find("toBeDisplayed");
    if (newRecipesIt != tag.value.end()) {
        auto* newRecipesList = dynamic_cast<const nbt::tags::string_list_tag*>(newRecipesIt->second.get());
        if (newRecipesList) {
            for (const auto& recipeStr : newRecipesList->value) {
                ResourceLocation recipeId(recipeStr);
                // 同样过滤掉动态配方
                if (!isDynamicRecipe(recipeId)) {
                    m_newRecipes.insert(std::move(recipeId));
                }
            }
        }
    }
}

std::vector<ResourceLocation> ServerRecipeBook::consumeNewRecipes()
{
    std::vector<ResourceLocation> result(m_newRecipes.begin(), m_newRecipes.end());
    m_newRecipes.clear();
    return result;
}

std::vector<ResourceLocation> ServerRecipeBook::getAllUnlockedRecipes() const
{
    return std::vector<ResourceLocation>(m_recipes.begin(), m_recipes.end());
}

} // namespace crafting
} // namespace mc
