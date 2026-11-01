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
 * @file recipe_book_category.cpp
 * @brief 差分能力：每个配方落在配方书的**哪个页签**。
 *
 * `list`：数据包里全部配方 ID
 * `run` ：{ "recipe": "minecraft:iron_axe" }
 *         → { "ok": true, "type": "minecraft:crafting_shaped",
 *             "page": "minecraft:crafting_equipment" }
 *
 * ## 比的是什么
 *
 * 原版 `Recipe#recipeBookCategory()` 把**两个维度**压成一个资源位置：
 *
 *   - 配方**类型**：熔炼 / 高炉 / 烟熏 / 营火 / 切石 / 锻造各有一套页签；
 *   - 页签**分类**：数据包 JSON 的 `category` 字段（有序合成：building / redstone /
 *     equipment / misc；烧制：food / blocks / misc）。
 *
 * 例如「有序合成 + equipment」→ `minecraft:crafting_equipment`，
 * 「烟熏 + 任意」→ `minecraft:smoker_food`（烟熏炉只有食物一页），
 * 「高炉 + food」→ `minecraft:blast_furnace_misc`（高炉没有食物页，落杂项）。
 *
 * 这个映射是客户端渲染配方书的真正依据，也是唯一能同时校验「分类字段解析对了没有」
 * 与「类型映射对不对」的观测点：只比 `category` 原值会漏掉后者，只比类型会漏掉前者。
 *
 * 尤其值得压的是**高炉 / 烟熏的取整**：这两类在数据包里带着 food / blocks / misc 三种
 * 分类，但它们没有对应的三页结构——按分类机械映射会凭空造出「高炉食物页」这种原版
 * 不存在的归属。
 */
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/RecipeBookCategories.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 数据包里全部配方 ID（排序去重）。
[[nodiscard]] const std::vector<std::string>& recipeIds()
{
    static std::vector<std::string> s_ids;

    static bool s_initialised = false;
    if (!s_initialised) {
        s_initialised = true;

        datapack::initializeRegistries();

        const auto recipeDir = datapack::root() / "data" / "minecraft" / "recipe";
        mc::RecipeLoader loader;
        for (const auto& file : datapack::jsonFiles(recipeDir)) {
            std::ifstream stream(file);
            if (!stream) {
                continue;
            }
            json contents;
            try {
                stream >> contents;
            }
            catch (const json::parse_error& error) {
                std::fprintf(stderr, "recipe_book_category: %s: %s\n", file.filename().string().c_str(), error.what());
                continue;
            }
            const std::string id = "minecraft:" + file.stem().string();
            auto loaded = loader.loadRecipeJson(ResourceLocation(id), contents.dump());
            if (loaded.failed()) {
                std::fprintf(stderr, "recipe_book_category: %s: %s\n", id.c_str(), loaded.error().message().c_str());
                continue;
            }
            s_ids.push_back(id);
        }
        std::sort(s_ids.begin(), s_ids.end());
        s_ids.erase(std::unique(s_ids.begin(), s_ids.end()), s_ids.end());
    }
    return s_ids;
}

void writeList(FILE* out)
{
    for (const auto& id : recipeIds()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    static_cast<void>(recipeIds());

    const json input = json::parse(inputLine);
    const std::string unit = input.at("recipe").get<std::string>();
    const ResourceLocation id(unit);

    // 配方类型一律用**序列化器 ID**：特殊配方在 `RecipeType` 里统一是 `Special`，
    // 但原版把 12 种特殊配方注册成各自的序列化器（`crafting_special_armordye` 等），
    // 用笼统的 `special` 会和原版的输出对不上，也看不出具体是哪一种。
    // 参数取 std::string 而不是 const char*：`recipeTypeToString` 返回临时 string，
    // 转成裸指针会立刻悬垂。
    const auto describe = [](const std::string& typeId, crafting::RecipeBookPage page) {
        return json{
            {"ok", true}, {"type", typeId}, {"page", "minecraft:" + std::string(crafting::recipeBookPageToPath(page))}};
    };

    const crafting::CraftingRecipe* craftingRecipe = crafting::RecipeManager::instance().getRecipe(id);
    if (craftingRecipe != nullptr) {
        // 特殊配方通过 `SpecialRecipe::getSerializerId()` 给出精确类型。
        const auto* special = dynamic_cast<const crafting::SpecialRecipe*>(craftingRecipe);
        json result = describe(
            special != nullptr ? special->getSerializerId() : crafting::recipeTypeToString(craftingRecipe->getType()),
            craftingRecipe->recipeBookPage());
        result["recipe"] = unit;
        return result.dump();
    }

    // 非工作台配方：熔炼类没有「按 ID 查」的入口（查询都是按输入物品找匹配），
    // 只能按类型桶遍历比对 ID。四个类型的桶各自独立，这正是要比的边界。
    for (const crafting::RecipeType type : {crafting::RecipeType::Smelting,
             crafting::RecipeType::Blasting,
             crafting::RecipeType::Smoking,
             crafting::RecipeType::CampfireCooking}) {
        for (const crafting::SmeltingRecipe* recipe :
            crafting::RecipeManager::instance().getSmeltingRecipesByType(type)) {
            if (recipe->getId() != id) {
                continue;
            }
            return json{{"recipe", unit},
                {"ok", true},
                {"type", crafting::recipeTypeToString(recipe->getType())},
                {"page", "minecraft:" + std::string(crafting::recipeBookPageToPath(recipe->recipeBookPage()))}}
                .dump();
        }
    }

    if (const crafting::StonecuttingRecipe* recipe = crafting::RecipeManager::instance().getStonecuttingRecipe(id);
        recipe != nullptr) {
        return json{{"recipe", unit},
            {"ok", true},
            {"type", crafting::recipeTypeToString(recipe->getType())},
            {"page", "minecraft:" + std::string(crafting::recipeBookPageToPath(recipe->recipeBookPage()))}}
            .dump();
    }

    // 锻造：两种变体各有按 ID 的查询入口，统一归到 smithing 页。
    if (const crafting::SmithingTransformRecipe* recipe =
            crafting::RecipeManager::instance().getSmithingTransformRecipe(id);
        recipe != nullptr) {
        return json{{"recipe", unit},
            {"ok", true},
            {"type", crafting::recipeTypeToString(recipe->getType())},
            {"page", "minecraft:" + std::string(crafting::recipeBookPageToPath(recipe->recipeBookPage()))}}
            .dump();
    }
    if (const crafting::SmithingTrimRecipe* recipe = crafting::RecipeManager::instance().getSmithingTrimRecipe(id);
        recipe != nullptr) {
        return json{{"recipe", unit},
            {"ok", true},
            {"type", crafting::recipeTypeToString(recipe->getType())},
            {"page", "minecraft:" + std::string(crafting::recipeBookPageToPath(recipe->recipeBookPage()))}}
            .dump();
    }

    return json{{"recipe", unit}, {"ok", false}, {"error", "unknown recipe"}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("recipe_book_category", &writeList, &runLine)

} // namespace mc::diff
