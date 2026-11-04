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
 * @brief 差分能力：营火烹饪配方表（`CampfireCookingRecipe`）。
 *
 * `list`：配方 ID + 一组「能否烹饪」查询单元
 * `run` ：一行 -> { "cookTime", "result", "experience" }
 *
 * ## 为什么单独做一条
 *
 * 营火只认 `RecipeType.CAMPFIRE_COOKING`，**不读熔炉或烟熏机配方**。三者的数据文件
 * 各自独立（`data/<ns>/recipe/*.json` 里按 `type` 字段区分），因此同一物品在三种
 * 加工设施下的可熔炼性与耗时都可能不同。把营火单独比对，才能确认：
 *
 * 1. Cubium 的 `getSmeltingRecipe(stack, CampfireCooking)` 只命中营火配方；
 * 2. 每个配方的 `cookingtime` 没有串到熔炉（200）或烟熏机（100）的值上。
 *
 * ## 与 `CampfireBlockEntity` 的关系
 *
 * `placeFood` 先查配方再取 `cookingTime`；`cookTick` 每 tick 递增进度，达到
 * `cookingTime` 时掉落结果；`cooldownTick` 在熄灭时每次减 2。因此**这张表里的
 * `cookTime` 就是营火的实际烹饪耗时**，抄错任何一个都会让该物品熟得太早或太晚。
 *
 * ## 单元格式
 *
 * - 配方单元：`recipe:<配方id>`
 * - 查询单元：`lookup:<输入物品id>`
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/crafting/RecipeManager.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 营火烘焙的输入物品（原版 `data/minecraft/recipe/` 里 campfire_cooking 类型的覆盖物）。
constexpr const char* INPUTS[] = {"minecraft:beef",
    "minecraft:chicken",
    "minecraft:cod",
    "minecraft:mutton",
    "minecraft:porkchop",
    "minecraft:rabbit",
    "minecraft:salmon",
    "minecraft:potato",
    "minecraft:kelp",
    "minecraft:brown_mushroom",
    "minecraft:red_mushroom",
    // 这些**不是**营火配方，用来验证不会误命中熔炉/烟熏机配方
    "minecraft:cobblestone",
    "minecraft:iron_ore",
    "minecraft:sand",
    "minecraft:porkchop",
    "minecraft:stick",
    "minecraft:apple"};

std::string floatBits(f32 value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

std::string itemIdOf(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return "";
    }
    return stack.getItem()->itemLocation().toString();
}

void loadAll()
{
    static bool s_initialised = false;
    if (s_initialised) {
        return;
    }
    s_initialised = true;

    datapack::initializeRegistries();

    // 走服务端同一条加载路径：数据包里的每个 recipe JSON 都交给 RecipeLoader 解析。
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
            std::fprintf(stderr, "campfire_cooking: %s: %s\n", file.filename().string().c_str(), error.what());
            continue;
        }
        const std::string id = "minecraft:" + file.stem().string();
        auto loaded = loader.loadRecipeJson(ResourceLocation(id), contents.dump());
        if (loaded.failed()) {
            std::fprintf(stderr, "campfire_cooking: %s: %s\n", id.c_str(), loaded.error().message().c_str());
        }
    }
}

void writeList(FILE* out)
{
    loadAll();
    for (const auto* recipe :
        crafting::RecipeManager::instance().getSmeltingRecipesByType(crafting::RecipeType::CampfireCooking)) {
        std::fprintf(out, "recipe:%s\n", recipe->getId().toString().c_str());
    }
    for (const char* input : INPUTS) {
        std::fprintf(out, "lookup:%s\n", input);
    }
}

std::string runLine(std::string_view inputLine)
{
    loadAll();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    json result{{"unit", unit}};

    if (unit.rfind("lookup:", 0) == 0) {
        const std::string itemId = unit.substr(7);
        const Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
        if (item == nullptr) {
            result["missing"] = true;
            return result.dump();
        }

        ItemStack stack(*item, 1);
        const crafting::SmeltingRecipe* recipe =
            crafting::RecipeManager::instance().getSmeltingRecipe(stack, crafting::RecipeType::CampfireCooking);

        if (recipe == nullptr) {
            result["found"] = false;
            return result.dump();
        }

        result["found"] = true;
        result["recipe"] = recipe->getId().toString();
        result["cookTime"] = recipe->getCookTime();
        result["result"] = itemIdOf(recipe->getResultItem());
        result["experience"] = floatBits(recipe->getExperience());
        return result.dump();
    }

    if (unit.rfind("recipe:", 0) == 0) {
        const std::string recipeId = unit.substr(7);
        for (const auto* recipe :
            crafting::RecipeManager::instance().getSmeltingRecipesByType(crafting::RecipeType::CampfireCooking)) {
            if (recipe->getId().toString() == recipeId) {
                result["cookTime"] = recipe->getCookTime();
                result["result"] = itemIdOf(recipe->getResultItem());
                result["experience"] = floatBits(recipe->getExperience());
                return result.dump();
            }
        }
        result["missing"] = true;
        return result.dump();
    }

    throw std::runtime_error("campfire_cooking: 未知单元类型：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(campfire_cooking, &writeList, &runLine)

} // namespace mc::diff
