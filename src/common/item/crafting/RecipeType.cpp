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

#include "item/crafting/IRecipe.hpp"
#include <optional>
#include <string>
#include <unordered_map>

namespace mc {
namespace crafting {

namespace {
const std::unordered_map<RecipeType, std::string> typeToStringMap = {{RecipeType::Crafting, "minecraft:crafting"},
    {RecipeType::ShapedCrafting, "minecraft:crafting_shaped"},
    {RecipeType::ShapelessCrafting, "minecraft:crafting_shapeless"},
    {RecipeType::Smelting, "minecraft:smelting"},
    {RecipeType::Blasting, "minecraft:blasting"},
    {RecipeType::Smoking, "minecraft:smoking"},
    {RecipeType::CampfireCooking, "minecraft:campfire_cooking"},
    {RecipeType::Stonecutting, "minecraft:stonecutting"},
    {RecipeType::Smithing, "minecraft:smithing"},
    {RecipeType::SmithingTransform, "minecraft:smithing_transform"},
    {RecipeType::SmithingTrim, "minecraft:smithing_trim"},
    {RecipeType::Transmute, "minecraft:crafting_transmute"},
    {RecipeType::Special, "minecraft:special"}};

const std::unordered_map<std::string, RecipeType> stringToTypeMap = {{"minecraft:crafting", RecipeType::Crafting},
    {"minecraft:crafting_shaped", RecipeType::ShapedCrafting},
    {"crafting_shaped", RecipeType::ShapedCrafting},
    {"minecraft:crafting_shapeless", RecipeType::ShapelessCrafting},
    {"crafting_shapeless", RecipeType::ShapelessCrafting},
    {"minecraft:smelting", RecipeType::Smelting},
    {"smelting", RecipeType::Smelting},
    {"minecraft:blasting", RecipeType::Blasting},
    {"blasting", RecipeType::Blasting},
    {"minecraft:smoking", RecipeType::Smoking},
    {"smoking", RecipeType::Smoking},
    {"minecraft:campfire_cooking", RecipeType::CampfireCooking},
    {"campfire_cooking", RecipeType::CampfireCooking},
    {"minecraft:stonecutting", RecipeType::Stonecutting},
    {"stonecutting", RecipeType::Stonecutting},
    {"minecraft:smithing", RecipeType::Smithing},
    {"smithing", RecipeType::Smithing},
    {"minecraft:smithing_transform", RecipeType::SmithingTransform},
    {"smithing_transform", RecipeType::SmithingTransform},
    {"minecraft:smithing_trim", RecipeType::SmithingTrim},
    {"smithing_trim", RecipeType::SmithingTrim},
    {"minecraft:crafting_transmute", RecipeType::Transmute},
    {"crafting_transmute", RecipeType::Transmute},
    {"minecraft:special", RecipeType::Special},
    {"special", RecipeType::Special}};
} // namespace

std::string recipeTypeToString(RecipeType type)
{
    auto it = typeToStringMap.find(type);
    if (it != typeToStringMap.end()) {
        return it->second;
    }
    return "minecraft:crafting";
}

std::optional<RecipeType> recipeTypeFromString(const std::string& str)
{
    auto it = stringToTypeMap.find(str);
    if (it != stringToTypeMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace crafting
} // namespace mc
