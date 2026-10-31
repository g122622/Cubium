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
 * @file crafting_match.cpp
 * @brief 差分能力：合成配方在**固定网格语料**下的匹配结果（`matches`）与产出（`assemble`）。
 *
 * `list`：单元名的全集（配方 ID 全部下发，由 harness 展开成「配方 × 网格」）
 * `run` ：{ "unit": "minecraft:oak_planks#grid", "recipe": "...", "width": 2, "height": 2,
 *           "grid": [{"id": "minecraft:oak_log", "count": 1}, null, null, null] }
 *         → { "unit": ..., "ok": true, "matches": true, "result": {"id": ..., "count": ...} }
 *
 * ## 为什么需要这个能力
 *
 * 已有的 `recipe` 能力比的是「配方表加载成了什么」：类型、原料、结果、烹饪时间。
 * 它只对**锻造**配方做过 `matches` / `assemble`——因为锻造的输入恰好是三个槽位，
 * 塞进一个列表就能表达。有序合成与无形状合成从来没有比过匹配，而它们正是玩家最常
 * 触碰的一层。
 *
 * ## 比的是什么
 *
 * 匹配语义在两侧的实现路径**完全不同**，任何一处偏差都会让玩家「摆对了却合不出来」
 * 或「摆错了却合得出东西」：
 *
 * - **原版**先把网格交给 `CraftingInput.ofPositioned`：裁掉四周的空行空列，得到
 *   「最小包围盒」；`ShapedRecipePattern.matches` 再要求**包围盒尺寸与配方尺寸严格
 *   相等**，然后比对正向与镜像两种排布，并先用 `ingredientCount` 挡住多余物品。
 * - **Cubium** 的 `ShapedRecipe::matches` 直接在原网格上滑动窗口试所有偏移、每个偏移
 *   试镜像与正向，网格内但配方外的位置用 `Ingredient::EMPTY` 测试。
 *
 * 两条路径在绝大多数输入上应当同真同假，但边界上的行为可以不同：把 1×1 配方摆在
 * 3×3 网格的角落、2×2 配方放进 3×3 网格、要素重复但排布不同的相近配方、镜像后
 * 撞上别的配方……这些正是语料要压的地方。
 *
 * 无形状一侧同理：原版走 `StackedContents.canCraft`（把网格内容压成计数表再从配方
 * 原料里逐项扣减），Cubium 走回溯匹配。两者的失败模式不同——前者对「同一种物品被
 * 两个不同原料同时接受」的处理依赖扣减顺序，后者依赖回溯顺序。
 *
 * ## 语料
 *
 * 网格由 harness 逐场景给出（可变尺寸 1×1 / 2×2 / 3×3），探针不自己造输入：这样
 * 「哪些摆法值得比」这份知识只有一处，两侧读到的是同一份。
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
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/ShapedRecipe.hpp"
#include "common/item/crafting/ShapelessRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using crafting::CraftingRecipe;
using nlohmann::json;

/// 网格里的一格：`nullptr` 表示空。
struct GridCell {
    const Item* item = nullptr;
    i32 count = 0;
};

/// 加载数据包里**全部**合成配方（有序 + 无形状），返回按 ID 排序后的清单。
[[nodiscard]] const std::vector<std::string>& craftingRecipeIds()
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
                std::fprintf(stderr, "crafting_match: %s: %s\n", file.filename().string().c_str(), error.what());
                continue;
            }
            const std::string id = "minecraft:" + file.stem().string();
            auto loaded = loader.loadRecipeJson(ResourceLocation(id), contents.dump());
            if (loaded.failed()) {
                // 加载失败的配方不会出现在 list 里；原版侧有而探针没有，报告里会点名。
                std::fprintf(stderr, "crafting_match: %s: %s\n", id.c_str(), loaded.error().message().c_str());
                continue;
            }
            const ResourceLocation recipeId(id);
            const CraftingRecipe* recipe = crafting::RecipeManager::instance().getRecipe(recipeId);
            if (recipe == nullptr) {
                continue;
            }
            // 只有有序与无形状参与本能力：锻造的输入是三个槽位而不是网格，熔炼/切石
            // 是单物品输入，它们由 `recipe` 能力覆盖。
            const auto type = recipe->getType();
            if (type == crafting::RecipeType::ShapedCrafting || type == crafting::RecipeType::ShapelessCrafting) {
                s_ids.push_back(id);
            }
        }
        std::sort(s_ids.begin(), s_ids.end());
        s_ids.erase(std::unique(s_ids.begin(), s_ids.end()), s_ids.end());
    }
    return s_ids;
}

/// 物品堆 → `{id, count}`；空堆写成 JSON null（两侧同口径）。
[[nodiscard]] json summariseStack(const ItemStack& stack)
{
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        return json(nullptr);
    }
    return json{{"id", stack.getItem()->itemLocation().toString()}, {"count", stack.getCount()}};
}

void writeList(FILE* out)
{
    // 只列本侧**真的加载出来**的合成配方：`deriveMissing` 据此把「探针认不出的配方」
    // 整组剔除，比列一堆永远 `ok:false` 的单元更容易读报告。
    for (const auto& id : craftingRecipeIds()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    // 先把数据包加载起来：`run` 不经过 `list`，不显式触发的话配方管理器是空的，
    // 每个单元都会报 "unknown recipe"——那种失败看起来像数据缺口，实际只是没初始化。
    static_cast<void>(craftingRecipeIds());

    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const std::string recipeId = input.at("recipe").get<std::string>();

    const ResourceLocation id(recipeId);
    const CraftingRecipe* recipe = crafting::RecipeManager::instance().getRecipe(id);
    if (recipe == nullptr) {
        return json{{"unit", unit}, {"ok", false}, {"error", "unknown recipe"}}.dump();
    }

    const i32 width = input.at("width").get<i32>();
    const i32 height = input.at("height").get<i32>();
    const auto& grid = input.at("grid");
    if (grid.size() != static_cast<std::size_t>(width * height)) {
        return json{{"unit", unit}, {"ok", false}, {"error", "grid size mismatch"}}.dump();
    }

    // 网格按行优先填：与原版 `CraftingInput.of(width, height, items)` 的列表顺序一致。
    // 物品不认识的格子按空处理并报错——它意味着场景本身在当前注册表下不可构造。
    CraftingInventory inventory(width, height);
    for (std::size_t index = 0; index < grid.size(); ++index) {
        const auto& cell = grid.at(index);
        if (cell.is_null()) {
            inventory.setItem(static_cast<i32>(index), ItemStack::EMPTY);
            continue;
        }
        const auto cellId = cell.at("id").get<std::string>();
        const Item* item = ItemRegistry::instance().getItem(ResourceLocation(cellId));
        if (item == nullptr) {
            return json{{"unit", unit}, {"ok", false}, {"error", "unknown item: " + cellId}}.dump();
        }
        inventory.setItem(static_cast<i32>(index), ItemStack(item, cell.value("count", 1)));
    }

    const bool matches = recipe->matches(inventory);
    json result = json{{"unit", unit}, {"ok", true}, {"matches", matches}};
    // 只有匹配时才求值：`assemble` 对不匹配的输入没有语义，两侧都不该去碰。
    result["result"] = matches ? summariseStack(recipe->assemble(inventory)) : json(nullptr);
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("crafting_match", &writeList, &runLine)

} // namespace mc::diff
