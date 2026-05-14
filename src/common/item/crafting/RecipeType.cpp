#include "item/crafting/IRecipe.hpp"
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
