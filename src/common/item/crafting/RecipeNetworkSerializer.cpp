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

#include "item/crafting/RecipeNetworkSerializer.hpp"
#include "item/crafting/IRecipe.hpp"

namespace mc {
namespace crafting {

void RecipeNetworkSerializer::serialize(const CraftingRecipe& recipe, network::PacketSerializer& ser)
{
    // 写入配方类型
    writeRecipeType(recipe.getType(), ser);

    // 写入配方ID
    ser.writeString(recipe.getId().toString());

    // 写入配方组
    ser.writeString(recipe.getGroup());

    // 根据配方类型写入特定数据
    switch (recipe.getType()) {
        case RecipeType::ShapedCrafting:
            serializeShaped(static_cast<const ShapedRecipe&>(recipe), ser);
            break;
        case RecipeType::ShapelessCrafting:
            serializeShapeless(static_cast<const ShapelessRecipe&>(recipe), ser);
            break;
        case RecipeType::Special:
            // 特殊配方不需要额外数据
            break;
        default:
            // 其他类型暂不支持序列化
            break;
    }
}

Result<std::unique_ptr<CraftingRecipe>> RecipeNetworkSerializer::deserialize(network::PacketDeserializer& deser)
{
    // 读取配方类型
    auto typeResult = readRecipeType(deser);
    if (typeResult.failed()) {
        return typeResult.error();
    }
    RecipeType type = typeResult.value();

    // 读取配方ID
    auto idResult = deser.readString();
    if (idResult.failed()) {
        return idResult.error();
    }
    ResourceLocation id(idResult.value());

    // 读取配方组
    auto groupResult = deser.readString();
    if (groupResult.failed()) {
        return groupResult.error();
    }
    std::string group = groupResult.value();

    // 根据配方类型读取特定数据
    switch (type) {
        case RecipeType::ShapedCrafting: {
            auto recipeResult = deserializeShaped(deser, id, group);
            if (recipeResult.failed()) {
                return recipeResult.error();
            }
            return std::unique_ptr<CraftingRecipe>(recipeResult.value());
        }
        case RecipeType::ShapelessCrafting: {
            auto recipeResult = deserializeShapeless(deser, id, group);
            if (recipeResult.failed()) {
                return recipeResult.error();
            }
            return std::unique_ptr<CraftingRecipe>(recipeResult.value());
        }
        case RecipeType::Special:
            // 特殊配方需要从注册表获取
            return Error(ErrorCode::InvalidData, "Special recipes must be registered, not deserialized");
        default:
            return Error(ErrorCode::InvalidData,
                "Unsupported recipe type for CraftingRecipe: " + std::to_string(static_cast<i32>(type)));
    }
}

void RecipeNetworkSerializer::serializeSmelting(const SmeltingRecipe& recipe, network::PacketSerializer& ser)
{
    // 写入配方组
    ser.writeString(recipe.getGroup());

    // 写入原料
    const Ingredient& ingredient = recipe.getIngredient();
    ingredient.serialize(ser);

    // 写入结果
    recipe.getResultItem().serialize(ser);

    // 写入经验和烹饪时间
    ser.writeF32(recipe.getExperience());
    ser.writeVarInt(recipe.getCookTime());
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeNetworkSerializer::deserializeSmelting(
    network::PacketDeserializer& deser, RecipeType type)
{

    // 读取配方组
    auto groupResult = deser.readString();
    if (groupResult.failed()) {
        return groupResult.error();
    }
    std::string group = groupResult.value();

    // 读取原料
    auto ingredientResult = Ingredient::deserialize(deser);
    if (ingredientResult.failed()) {
        return ingredientResult.error();
    }

    // 读取结果
    auto resultStackResult = ItemStack::deserialize(deser);
    if (resultStackResult.failed()) {
        return resultStackResult.error();
    }

    // 读取经验和烹饪时间
    auto expResult = deser.readF32();
    if (expResult.failed()) {
        return expResult.error();
    }
    f32 experience = expResult.value();

    auto cookTimeResult = deser.readVarInt();
    if (cookTimeResult.failed()) {
        return cookTimeResult.error();
    }
    i32 cookTime = cookTimeResult.value();

    // 创建配方
    auto recipe = std::make_unique<SmeltingRecipe>(
        ResourceLocation("", ""), group, ingredientResult.value(), resultStackResult.value(), experience, cookTime);

    return recipe;
}

void RecipeNetworkSerializer::serializeShaped(const ShapedRecipe& recipe, network::PacketSerializer& ser)
{
    // 写入宽度和高度
    ser.writeVarInt(recipe.getRecipeWidth());
    ser.writeVarInt(recipe.getRecipeHeight());

    // 写入原料列表
    const auto& ingredients = recipe.getIngredients();
    ser.writeVarUInt(static_cast<u32>(ingredients.size()));
    for (const Ingredient& ingredient : ingredients) {
        ingredient.serialize(ser);
    }

    // 写入结果
    recipe.getResultItem().serialize(ser);
}

Result<std::unique_ptr<ShapedRecipe>> RecipeNetworkSerializer::deserializeShaped(
    network::PacketDeserializer& deser, const ResourceLocation& id, const std::string& group)
{

    // 读取宽度和高度
    auto widthResult = deser.readVarInt();
    if (widthResult.failed()) {
        return widthResult.error();
    }
    i32 width = widthResult.value();

    auto heightResult = deser.readVarInt();
    if (heightResult.failed()) {
        return heightResult.error();
    }
    i32 height = heightResult.value();

    // 读取原料数量
    auto countResult = deser.readVarUInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    u32 count = countResult.value();

    // 验证原料数量
    if (count != static_cast<u32>(width * height)) {
        return Error(ErrorCode::InvalidData,
            "Ingredient count mismatch: expected " + std::to_string(width * height) + ", got " + std::to_string(count));
    }

    // 读取原料
    std::vector<Ingredient> ingredients;
    ingredients.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        auto ingredientResult = Ingredient::deserialize(deser);
        if (ingredientResult.failed()) {
            return ingredientResult.error();
        }
        ingredients.push_back(std::move(ingredientResult.value()));
    }

    // 读取结果
    auto resultStackResult = ItemStack::deserialize(deser);
    if (resultStackResult.failed()) {
        return resultStackResult.error();
    }

    auto recipe =
        std::make_unique<ShapedRecipe>(id, width, height, std::move(ingredients), resultStackResult.value(), group);

    return recipe;
}

void RecipeNetworkSerializer::serializeShapeless(const ShapelessRecipe& recipe, network::PacketSerializer& ser)
{
    // 写入原料列表
    const auto& ingredients = recipe.getIngredients();
    ser.writeVarUInt(static_cast<u32>(ingredients.size()));
    for (const Ingredient& ingredient : ingredients) {
        ingredient.serialize(ser);
    }

    // 写入结果
    recipe.getResultItem().serialize(ser);
}

Result<std::unique_ptr<ShapelessRecipe>> RecipeNetworkSerializer::deserializeShapeless(
    network::PacketDeserializer& deser, const ResourceLocation& id, const std::string& group)
{

    // 读取原料数量
    auto countResult = deser.readVarUInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    u32 count = countResult.value();

    // 读取原料
    std::vector<Ingredient> ingredients;
    ingredients.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        auto ingredientResult = Ingredient::deserialize(deser);
        if (ingredientResult.failed()) {
            return ingredientResult.error();
        }
        ingredients.push_back(std::move(ingredientResult.value()));
    }

    // 读取结果
    auto resultStackResult = ItemStack::deserialize(deser);
    if (resultStackResult.failed()) {
        return resultStackResult.error();
    }

    auto recipe = std::make_unique<ShapelessRecipe>(id, std::move(ingredients), resultStackResult.value(), group);

    return recipe;
}

void RecipeNetworkSerializer::serializeStonecutting(const StonecuttingRecipe& recipe, network::PacketSerializer& ser)
{
    // 写入原料
    recipe.getIngredient().serialize(ser);

    // 写入结果
    recipe.getResultItem().serialize(ser);

    // 写入结果数量
    ser.writeVarInt(recipe.getCount());
}

Result<std::unique_ptr<StonecuttingRecipe>> RecipeNetworkSerializer::deserializeStonecutting(
    network::PacketDeserializer& deser, const ResourceLocation& id)
{

    // 读取原料
    auto ingredientResult = Ingredient::deserialize(deser);
    if (ingredientResult.failed()) {
        return ingredientResult.error();
    }

    // 读取结果
    auto resultStackResult = ItemStack::deserialize(deser);
    if (resultStackResult.failed()) {
        return resultStackResult.error();
    }

    // 读取结果数量
    auto countResult = deser.readVarInt();
    if (countResult.failed()) {
        return countResult.error();
    }
    i32 count = countResult.value();

    auto recipe =
        std::make_unique<StonecuttingRecipe>(id, "", ingredientResult.value(), resultStackResult.value(), count);

    return recipe;
}

void RecipeNetworkSerializer::serializeSmithing(const SmithingRecipe& recipe, network::PacketSerializer& ser)
{
    // 写入基础原料
    recipe.getBase().serialize(ser);

    // 写入添加物原料
    recipe.getAddition().serialize(ser);

    // 写入结果
    recipe.getResultItem().serialize(ser);
}

Result<std::unique_ptr<SmithingRecipe>> RecipeNetworkSerializer::deserializeSmithing(
    network::PacketDeserializer& deser, const ResourceLocation& id)
{

    // 读取基础原料
    auto baseResult = Ingredient::deserialize(deser);
    if (baseResult.failed()) {
        return baseResult.error();
    }

    // 读取添加物原料
    auto additionResult = Ingredient::deserialize(deser);
    if (additionResult.failed()) {
        return additionResult.error();
    }

    // 读取结果
    auto resultStackResult = ItemStack::deserialize(deser);
    if (resultStackResult.failed()) {
        return resultStackResult.error();
    }

    auto recipe =
        std::make_unique<SmithingRecipe>(id, baseResult.value(), additionResult.value(), resultStackResult.value());

    return recipe;
}

void RecipeNetworkSerializer::writeRecipeType(RecipeType type, network::PacketSerializer& ser)
{
    ser.writeVarInt(static_cast<i32>(type));
}

Result<RecipeType> RecipeNetworkSerializer::readRecipeType(network::PacketDeserializer& deser)
{
    auto typeResult = deser.readVarInt();
    if (typeResult.failed()) {
        return typeResult.error();
    }
    i32 typeValue = typeResult.value();

    // 验证类型值
    if (typeValue < 0 || typeValue > static_cast<i32>(RecipeType::Special)) {
        return Error(ErrorCode::InvalidData, "Invalid recipe type: " + std::to_string(typeValue));
    }

    return static_cast<RecipeType>(typeValue);
}

} // namespace crafting
} // namespace mc
