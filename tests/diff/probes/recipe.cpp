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

/**
 * @file recipe.cpp
 * @brief 差分能力：配方的加载结果。
 *
 * `list`：数据包里全部配方 ID
 * `run` ：{ "recipe": "minecraft:oak_planks" } → 该配方的结构化摘要
 *
 * 走服务端同一条加载路径（`RecipeLoader::loadRecipeJson`）：JSON 由它解析成配方对象、
 * 注册进 `RecipeManager`，探针再从管理器里按 ID 取回来做摘要。比的是「配方表最终
 * 是什么样」——类型、分组、原料集合、结果物品与数量、烹饪时间/经验、锻造模板/底材/
 * 添加物、特殊配方的类别。
 */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/RecipeSerializers.hpp"
#include "common/item/crafting/ShapedRecipe.hpp"
#include "common/item/crafting/ShapelessRecipe.hpp"
#include "common/item/crafting/SmeltingRecipe.hpp"
#include "common/item/crafting/SmithingTransformRecipe.hpp"
#include "common/item/crafting/SmithingTrimRecipe.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/item/crafting/StonecuttingRecipe.hpp"
#include "common/item/crafting/TransmuteRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using crafting::CraftingRecipe;
using crafting::Ingredient;
using nlohmann::json;

/// 输入目录里成功加载的配方 ID（排序去重）
std::vector<std::string>& loadedIds()
{
    static std::vector<std::string> s_ids;
    return s_ids;
}

/// 原料 → 摘要
///
/// 输出每个原料的候选物品集合（与原版 `Ingredient#items` 同一口径：标签展开成成员、
/// 多个候选取并集，排序去重）。标签要展开而不是原样输出 `#tag`：配方吃的是成员物品，
/// 标签成员集合本身就是原版 `placementInfo` 的一部分——服务端标签为空时，配方在
/// 服务端根本匹配不到任何物品，这正是要比对出来的差异（`item_tags` harness 单独
/// 校验标签注册表，这里校验标签在配方里的实际落点）。
[[nodiscard]] json summariseIngredient(const Ingredient& ingredient)
{
    std::vector<std::string> items;
    for (const auto* item : ingredient.getAllMatchingItems()) {
        if (item != nullptr) {
            items.push_back(item->itemLocation().toString());
        }
    }
    std::sort(items.begin(), items.end());
    items.erase(std::unique(items.begin(), items.end()), items.end());
    return json{{"items", std::move(items)}};
}

[[nodiscard]] json summariseIngredients(const std::vector<Ingredient>& ingredients)
{
    json result = json::array();
    for (const auto& ingredient : ingredients) {
        result.push_back(summariseIngredient(ingredient));
    }
    return result;
}

[[nodiscard]] json summariseStack(const ItemStack& stack)
{
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        return json(nullptr);
    }
    return json{{"id", stack.getItem()->itemLocation().toString()}, {"count", stack.getCount()}};
}

/// 特殊配方的摘要
///
/// 数据包里只有 `type`，行为由子类实现：`type` 取序列化器 ID（原版配方注册表的键）。
/// 有固定网格布局的特殊配方（地图扩展）给出逐槽原料与 3×3 尺寸，结果按数据包语义
/// 为 null；没有布局的（修复、染色、复制……）只有类型。
[[nodiscard]] json summariseSpecial(const crafting::SpecialRecipe& recipe)
{
    json result;
    result["recipe"] = recipe.getId().toString();
    result["type"] = recipe.getSerializerId();
    result["group"] = "";
    result["ingredients"] = summariseIngredients(crafting::RecipeSerializers::placementIngredients(
        recipe, ResourceLocation("minecraft:filled_map"), ResourceLocation("minecraft:paper")));
    // 没有固定网格布局的特殊配方（修复、染色、复制……）：原版这里只给类型。
    if (result["ingredients"].empty()) {
        return result;
    }
    // 地图扩展是固定的 3×3 网格；数据包不写结果，原版按 null 输出。
    result["result"] = json(nullptr);
    result["width"] = 3;
    result["height"] = 3;
    return result;
}

[[nodiscard]] json summariseRecipe(const CraftingRecipe& recipe)
{
    json result;
    result["recipe"] = recipe.getId().toString();
    result["type"] = crafting::recipeTypeToString(recipe.getType());

    if (const auto* special = dynamic_cast<const crafting::SpecialRecipe*>(&recipe); special != nullptr) {
        return summariseSpecial(*special);
    }

    result["group"] = recipe.getGroup();
    result["result"] = summariseStack(recipe.getResultItem());
    result["ingredients"] = summariseIngredients(recipe.getIngredients());

    if (const auto* shaped = dynamic_cast<const crafting::ShapedRecipe*>(&recipe); shaped != nullptr) {
        result["width"] = shaped->getRecipeWidth();
        result["height"] = shaped->getRecipeHeight();
    } else if (const auto* smelting = dynamic_cast<const crafting::SmeltingRecipe*>(&recipe); smelting != nullptr) {
        result["cookTime"] = smelting->getCookTime();
        result["experience"] = smelting->getExperience();
    }
    return result;
}

[[nodiscard]] const crafting::SmeltingRecipe* findSmelting(const ResourceLocation& id)
{
    constexpr crafting::RecipeType TYPES[] = {crafting::RecipeType::Smelting,
        crafting::RecipeType::Blasting,
        crafting::RecipeType::Smoking,
        crafting::RecipeType::CampfireCooking};
    for (const auto type : TYPES) {
        for (const auto* recipe : crafting::RecipeManager::instance().getSmeltingRecipesByType(type)) {
            if (recipe != nullptr && recipe->getId() == id) {
                return recipe;
            }
        }
    }
    return nullptr;
}

[[nodiscard]] json summariseCooking(const crafting::SmeltingRecipe* recipe)
{
    json result;
    result["recipe"] = recipe->getId().toString();
    result["type"] = crafting::recipeTypeToString(recipe->getType());
    result["group"] = recipe->getGroup();
    result["ingredients"] = summariseIngredients(recipe->getIngredients());
    result["result"] = summariseStack(recipe->getResultItem());
    result["cookTime"] = recipe->getCookTime();
    result["experience"] = recipe->getExperience();
    return result;
}

[[nodiscard]] json summariseStonecutting(const crafting::StonecuttingRecipe* recipe)
{
    json result;
    result["recipe"] = recipe->getId().toString();
    result["type"] = crafting::recipeTypeToString(recipe->getType());
    result["group"] = recipe->getGroup();
    result["ingredients"] = summariseIngredients(recipe->getIngredients());
    result["result"] = summariseStack(recipe->getResultItem());
    return result;
}

[[nodiscard]] json summariseSmithing(const crafting::SmithingTransformRecipe* recipe)
{
    json result;
    result["recipe"] = recipe->getId().toString();
    result["type"] = crafting::recipeTypeToString(recipe->getType());
    result["group"] = recipe->getGroup();
    result["ingredients"] = summariseIngredients(recipe->getIngredients());
    result["result"] = summariseStack(recipe->getResultItem());
    return result;
}

/// 纹饰结果：原版 `SmithingTrimRecipe#assemble` 对「底材原料的首个物品 + 添加物原料的
/// 首个物品」做真实合成——底材不是可纹饰盔甲、添加物不是纹饰材料时返回空。
[[nodiscard]] json summariseTrimResult(const crafting::SmithingTrimRecipe& recipe)
{
    const auto baseItems = recipe.getBase().getAllMatchingItems();
    const auto additionItems = recipe.getAddition().getAllMatchingItems();
    if (baseItems.empty() || additionItems.empty()) {
        return json(nullptr);
    }
    CraftingInventory inventory(1, 3);
    inventory.setItem(0, ItemStack::EMPTY);
    inventory.setItem(1, ItemStack(*baseItems.front(), 1));
    inventory.setItem(2, ItemStack(*additionItems.front(), 1));
    return summariseStack(recipe.assemble(inventory));
}

[[nodiscard]] json summariseSmithing(const crafting::SmithingTrimRecipe* recipe)
{
    json result;
    result["recipe"] = recipe->getId().toString();
    result["type"] = crafting::recipeTypeToString(recipe->getType());
    result["group"] = recipe->getGroup();
    result["ingredients"] = summariseIngredients(recipe->getIngredients());
    result["result"] = summariseTrimResult(*recipe);
    return result;
}

void loadAll()
{
    static bool s_initialised = false;
    if (s_initialised) {
        return;
    }
    s_initialised = true;

    datapack::initializeRegistries();

    const auto recipeDir = datapack::root() / "data" / "minecraft" / "recipe";
    mc::RecipeLoader loader;
    for (const auto& file : datapack::jsonFiles(recipeDir)) {
        std::ifstream stream(file);
        if (!stream) {
            continue;
        }
        nlohmann::json contents;
        try {
            stream >> contents;
        }
        catch (const nlohmann::json::parse_error& error) {
            std::fprintf(stderr, "recipe: %s: %s\n", file.filename().string().c_str(), error.what());
            continue;
        }
        const std::string id = "minecraft:" + file.stem().string();
        auto loaded = loader.loadRecipeJson(ResourceLocation(id), contents.dump());
        if (loaded.failed()) {
            // 加载失败不静默：原版侧会有这个单元而探针没有，比对报告里会体现。
            std::fprintf(stderr, "recipe: %s: %s\n", id.c_str(), loaded.error().message().c_str());
            continue;
        }
        loadedIds().push_back(id);
    }
    std::sort(loadedIds().begin(), loadedIds().end());
}

void writeList(FILE* out)
{
    loadAll();
    for (const auto& id : loadedIds()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    loadAll();
    const json input = json::parse(inputLine);
    const std::string id = input.at("recipe").get<std::string>();

    const auto found = std::lower_bound(loadedIds().begin(), loadedIds().end(), id);
    if (found == loadedIds().end() || *found != id) {
        return json{{"recipe", id}, {"missing", true}}.dump();
    }
    const ResourceLocation recipeId(id);
    if (const CraftingRecipe* recipe = crafting::RecipeManager::instance().getRecipe(recipeId); recipe != nullptr) {
        return summariseRecipe(*recipe).dump();
    }
    if (const crafting::SmeltingRecipe* recipe = findSmelting(recipeId); recipe != nullptr) {
        return summariseCooking(recipe).dump();
    }
    if (const crafting::StonecuttingRecipe* recipe =
            crafting::RecipeManager::instance().getStonecuttingRecipe(recipeId);
        recipe != nullptr) {
        return summariseStonecutting(recipe).dump();
    }
    if (const crafting::SmithingTransformRecipe* recipe =
            crafting::RecipeManager::instance().getSmithingTransformRecipe(recipeId);
        recipe != nullptr) {
        return summariseSmithing(recipe).dump();
    }
    if (const crafting::SmithingTrimRecipe* recipe =
            crafting::RecipeManager::instance().getSmithingTrimRecipe(recipeId);
        recipe != nullptr) {
        return summariseSmithing(recipe).dump();
    }
    return json{{"recipe", id}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("recipe", &writeList, &runLine)

} // namespace mc::diff
