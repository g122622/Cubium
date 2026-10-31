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
 * @file single_item_recipe.cpp
 * @brief 差分能力：**单物品配方**的查询语义（熔炼 / 高炉 / 烟熏 / 营火 / 切石）。
 *
 * `list`：单元名 = 输入物品 ID（数据包里所有单物品配方的原料并集）
 * `run` ：{ "unit": "minecraft:iron_ore" }
 *         → { "ok": true, "furnace": {...}, "blast": ..., "smoke": ..., "campfire": ...,
 *             "stonecutting": [...] }
 *
 * ## 为什么需要这个能力
 *
 * `recipe` 能力比的是「配方表里每一条长什么样」——按配方 ID 逐个导出类型、原料、结果、
 * 烹饪时间。它回答不了玩家的实际问题：**手里拿着一个铁矿石，炉子会烧出什么**。两者
 * 的差别在「查询」这一层：
 *
 * - 一个输入可能匹配**多条**配方（多个木板切石成不同台阶、生鳕鱼既能烧也能烟熏）；
 *   查询只返回「第一条」，返回哪一条取决于注册顺序。顺序错一位，玩家得到的就是别的
 *   产物——而这在按 ID 比对的 `recipe` 能力里完全看不出来。
 * - 配方按**类型**分桶。熔炼与烟熏的原料完全相同（都是生肉），只在 cookTime 上不同；
 *   桶装错或查询时串桶，玩家就会把肉放进高炉还烧得出来。
 * - 空输入、以及「原料是标签」的配方，命中与否取决于标签注册表而不是配方本身。
 *
 * 切石单独输出成列表而不是 `findFirst`：原版切石机界面列出该输入的全部可切产物，
 * 少一条就少一个选项。整张列表比对才能发现漏掉的条目。
 */
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/SmeltingRecipe.hpp"
#include "common/item/crafting/StonecuttingRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 参与比对的四种「烧制」类型。切石单独处理（它返回列表而不是单条）。
struct CookingKind {
    const char* field;         ///< 输出 JSON 里的字段名
    crafting::RecipeType type; ///< 配方类型
};

constexpr CookingKind COOKING_KINDS[] = {
    {"furnace", crafting::RecipeType::Smelting},
    {"blast", crafting::RecipeType::Blasting},
    {"smoke", crafting::RecipeType::Smoking},
    {"campfire", crafting::RecipeType::CampfireCooking},
};

/// 物品堆 → `{id, count}`；空堆写成 null。
[[nodiscard]] json summariseStack(const ItemStack& stack)
{
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        return json(nullptr);
    }
    return json{{"id", stack.getItem()->itemLocation().toString()}, {"count", stack.getCount()}};
}

[[nodiscard]] json summariseCooking(const crafting::SmeltingRecipe* recipe)
{
    if (recipe == nullptr) {
        return json(nullptr);
    }
    json result = json{
        {"recipe", recipe->getId().toString()},
        {"result", summariseStack(recipe->getResultItem())},
        {"group", recipe->getGroup()},
        {"cookTime", recipe->getCookTime()},
        {"experience", recipe->getExperience()},
    };
    return result;
}

/**
 * 加载数据包里的全部配方，返回「所有单物品配方的原料并集」按 ID 排序后的清单。
 *
 * 并集而不是配方清单：单元是**输入物品**，一个物品可能同时是好几条配方的原料
 * （铁矿石既是熔炼原料、也可能被别的配方接受），按物品枚举才不会重复比对同一件事。
 */
[[nodiscard]] const std::vector<std::string>& inputItemIds()
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
                std::fprintf(stderr, "single_item_recipe: %s: %s\n", file.filename().string().c_str(), error.what());
                continue;
            }
            const std::string id = "minecraft:" + file.stem().string();
            auto loaded = loader.loadRecipeJson(ResourceLocation(id), contents.dump());
            if (loaded.failed()) {
                std::fprintf(stderr, "single_item_recipe: %s: %s\n", id.c_str(), loaded.error().message().c_str());
                continue;
            }
        }

        // 原料并集取自**所有**单物品配方：烧制四种 + 切石。
        for (const CookingKind& kind : COOKING_KINDS) {
            for (const crafting::SmeltingRecipe* recipe :
                crafting::RecipeManager::instance().getSmeltingRecipesByType(kind.type)) {
                for (const auto* item : recipe->getIngredients().front().getAllMatchingItems()) {
                    if (item != nullptr) {
                        s_ids.push_back(item->itemLocation().toString());
                    }
                }
            }
        }
        for (const crafting::StonecuttingRecipe* recipe :
            crafting::RecipeManager::instance().getAllStonecuttingRecipes()) {
            for (const auto* item : recipe->getIngredients().front().getAllMatchingItems()) {
                if (item != nullptr) {
                    s_ids.push_back(item->itemLocation().toString());
                }
            }
        }

        std::sort(s_ids.begin(), s_ids.end());
        s_ids.erase(std::unique(s_ids.begin(), s_ids.end()), s_ids.end());
    }
    return s_ids;
}

void writeList(FILE* out)
{
    for (const auto& id : inputItemIds()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    static_cast<void>(inputItemIds());

    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(unit));
    if (item == nullptr) {
        return json{{"unit", unit}, {"ok", false}, {"error", "unknown item"}}.dump();
    }

    // 输入物品堆的数量固定为 1：原版 `SingleRecipeInput` 只从槽位取一个物品，
    // 匹配不看数量；用 1 避免把「数量参与匹配」这类实现错误混进来。
    const ItemStack stack(item, 1);

    json result = json{{"unit", unit}, {"ok", true}};
    for (const CookingKind& kind : COOKING_KINDS) {
        result[kind.field] = summariseCooking(crafting::RecipeManager::instance().getSmeltingRecipe(stack, kind.type));
    }

    // 切石：原版界面列出该输入的全部可切产物，因此这里给整张列表。
    // `findStonecuttingRecipes` 遍历的是 unordered_map，顺序不确定；两侧都按配方 ID
    // 排序后再输出，比对的就只剩「有哪些条目」。
    json stonecutting = json::array();
    for (const crafting::StonecuttingRecipe* recipe :
        crafting::RecipeManager::instance().findStonecuttingRecipes(stack)) {
        stonecutting.push_back(json{
            {"recipe", recipe->getId().toString()},
            {"result", summariseStack(recipe->getResultItem())},
            {"group", recipe->getGroup()},
        });
    }
    std::sort(stonecutting.begin(), stonecutting.end(), [](const json& left, const json& right) {
        return left.at("recipe").get<std::string>() < right.at("recipe").get<std::string>();
    });
    result["stonecutting"] = std::move(stonecutting);

    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("single_item_recipe", &writeList, &runLine)

} // namespace mc::diff
