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
 * @brief 差分能力：四类烹饪配方的横向对照（熔炉 / 烟熏机 / 高炉 / 营火）。
 *
 * `list`：每个输入物品 × 每种设施
 * `run` ：一行 -> { "found", "cookTime", "result", "experience", "recipe" }
 *
 * ## 为什么值得单独做一条
 *
 * 四类烹饪配方在数据包里是**四套独立的数据**（同一个 `recipe` 目录、按 `type` 区分），
 * `cookingtime` 也各自写在 JSON 里而不是代码里。它们在游戏里的语义差异很大：
 *
 * - **熔炉**（`smelting`）：通用，食物与矿石都熔；
 * - **烟熏机**（`smoking`）：只做食物，速度是熔炉的**两倍**；
 * - **高炉**（`blasting`）：只做矿石与金属，速度同样是两倍；
 * - **营火**（`campfire_cooking`）：只做食物，速度是熔炉的**三倍慢**。
 *
 * 最容易出错的是「串配方」——把 `smoking` 的查找走成 `smelting`，或反之。这种错误
 * 不会崩溃，只会让某些物品在错误的设施里可加工、或者耗时差一倍。逐设施比对是唯一
 * 能可靠发现它的办法。
 *
 * ## 单元格式
 *
 * `<facility>|<输入物品id>`，facility 取 `smelting` / `smoking` / `blasting` / `campfire_cooking`。
 */

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

/// 四种烹饪设施，顺序与单元枚举一致。
constexpr const char* FACILITIES[] = {"smelting", "smoking", "blasting", "campfire_cooking"};

/**
 * 参与比对的输入物品。
 *
 * 覆盖四类配方的典型入口，并刻意包含交叉项：
 * - `beef` / `potato` / `cod`：只该在 smelting / smoking / campfire_cooking 里出现
 * - `iron_ore` / `sand` / `cobblestone`：只该在 smelting / blasting 里出现
 * - `cobblestone` -> stone 同时出现在 smelting 与 blasting
 * - `kelp`：只该在 smelting 与 campfire_cooking 里出现（烟熏机不加工海带）
 */
constexpr const char* INPUTS[] = {"minecraft:beef",
    "minecraft:porkchop",
    "minecraft:chicken",
    "minecraft:cod",
    "minecraft:salmon",
    "minecraft:potato",
    "minecraft:kelp",
    "minecraft:iron_ore",
    "minecraft:gold_ore",
    "minecraft:copper_ore",
    "minecraft:sand",
    "minecraft:cobblestone",
    "minecraft:clay_ball",
    "minecraft:iron_ingot",
    "minecraft:stick",
    "minecraft:apple"};

crafting::RecipeType typeOf(const std::string& facility)
{
    if (facility == "smoking") {
        return crafting::RecipeType::Smoking;
    }
    if (facility == "blasting") {
        return crafting::RecipeType::Blasting;
    }
    if (facility == "campfire_cooking") {
        return crafting::RecipeType::CampfireCooking;
    }
    return crafting::RecipeType::Smelting;
}

std::string floatBits(f32 value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
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
            std::fprintf(stderr, "cooking_recipe_types: %s: %s\n", file.filename().string().c_str(), error.what());
            continue;
        }
        const std::string id = "minecraft:" + file.stem().string();
        auto loaded = loader.loadRecipeJson(ResourceLocation(id), contents.dump());
        if (loaded.failed()) {
            std::fprintf(stderr, "cooking_recipe_types: %s: %s\n", id.c_str(), loaded.error().message().c_str());
        }
    }
}

void writeList(FILE* out)
{
    for (const char* facility : FACILITIES) {
        for (const char* input : INPUTS) {
            std::fprintf(out, "%s|%s\n", facility, input);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    loadAll();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t separator = unit.find('|');
    if (separator == std::string::npos) {
        throw std::runtime_error("cooking_recipe_types: 单元应为 'facility|item'：" + unit);
    }
    const std::string facility = unit.substr(0, separator);
    const std::string itemId = unit.substr(separator + 1);

    json result{{"unit", unit}};

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
    if (item == nullptr) {
        result["missing"] = true;
        return result.dump();
    }

    ItemStack stack(*item, 1);
    const crafting::SmeltingRecipe* recipe =
        crafting::RecipeManager::instance().getSmeltingRecipe(stack, typeOf(facility));

    if (recipe == nullptr) {
        result["found"] = false;
        return result.dump();
    }

    result["found"] = true;
    result["recipe"] = recipe->getId().toString();
    result["cookTime"] = recipe->getCookTime();
    result["result"] = recipe->getResultItem().getItem()->itemLocation().toString();
    result["experience"] = floatBits(recipe->getExperience());
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("cooking_recipe_types", &writeList, &runLine)

} // namespace mc::diff
