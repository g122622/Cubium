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

#include "item/crafting/RecipeSerializers.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/ShapedRecipe.hpp"
#include "common/item/crafting/ShapelessRecipe.hpp"
#include "common/item/crafting/SmeltingRecipe.hpp"
#include "common/item/crafting/SmithingRecipe.hpp"
#include "common/item/crafting/SmithingTransformRecipe.hpp"
#include "common/item/crafting/SmithingTrimRecipe.hpp"
#include "common/item/crafting/StonecuttingRecipe.hpp"
#include "common/item/crafting/TransmuteRecipe.hpp"
#include "common/item/crafting/special/ArmorDyeRecipe.hpp"
#include "common/item/crafting/special/BannerDuplicateRecipe.hpp"
#include "common/item/crafting/special/BookCloningRecipe.hpp"
#include "common/item/crafting/special/DecoratedPotRecipe.hpp"
#include "common/item/crafting/special/FireworkRocketRecipe.hpp"
#include "common/item/crafting/special/FireworkStarFadeRecipe.hpp"
#include "common/item/crafting/special/FireworkStarRecipe.hpp"
#include "common/item/crafting/special/MapCloningRecipe.hpp"
#include "common/item/crafting/special/MapExtendingRecipe.hpp"
#include "common/item/crafting/special/RepairItemRecipe.hpp"
#include "common/item/crafting/special/ShieldDecorationRecipe.hpp"
#include "common/item/crafting/special/TippedArrowRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/core/ItemRegistry.hpp"
#include "util/nbt/Nbt.hpp"
#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace crafting {

// ============================================================================
// NBT 转 JSON 辅助函数
// ============================================================================

namespace {

/**
 * @brief 将 NBT 标签转换为 nlohmann::json
 * @param tag NBT 标签
 * @return 转换后的 JSON 值
 */
nlohmann::json nbtToJson(const nbt::tags::tag& tag);

nlohmann::json nbtListToJson(const nbt::tags::list_tag& list)
{
    nlohmann::json result = nlohmann::json::array();
    for (size_t i = 0; i < list.size(); ++i) {
        auto elem = list[i];
        if (elem) {
            result.push_back(nbtToJson(*elem));
        }
    }
    return result;
}

nlohmann::json nbtToJson(const nbt::tags::tag& tag)
{
    using namespace nbt::tags;

    switch (tag.id()) {
        case nbt::TagId::Byte: {
            const auto& t = dynamic_cast<const byte_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Short: {
            const auto& t = dynamic_cast<const short_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Int: {
            const auto& t = dynamic_cast<const int_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Long: {
            const auto& t = dynamic_cast<const long_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Float: {
            const auto& t = dynamic_cast<const float_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::Double: {
            const auto& t = dynamic_cast<const double_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::ByteArray: {
            const auto& t = dynamic_cast<const bytearray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (i8 val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::String: {
            const auto& t = dynamic_cast<const string_tag&>(tag);
            return t.value;
        }
        case nbt::TagId::List: {
            const auto& t = dynamic_cast<const list_tag&>(tag);
            return nbtListToJson(t);
        }
        case nbt::TagId::Compound: {
            const auto& t = dynamic_cast<const compound_tag&>(tag);
            nlohmann::json result = nlohmann::json::object();
            for (const auto& [key, value] : t.value) {
                if (value) {
                    result[key] = nbtToJson(*value);
                }
            }
            return result;
        }
        case nbt::TagId::IntArray: {
            const auto& t = dynamic_cast<const intarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (i32 val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::LongArray: {
            const auto& t = dynamic_cast<const longarray_tag&>(tag);
            nlohmann::json result = nlohmann::json::array();
            for (i64 val : t.value) {
                result.push_back(val);
            }
            return result;
        }
        case nbt::TagId::End:
        default:
            return nlohmann::json();
    }
}

} // anonymous namespace

// ============================================================================
// RecipeSerializers 实现
// ============================================================================

Result<std::unique_ptr<CraftingRecipe>> RecipeSerializers::fromJson(
    const ResourceLocation& id, const nlohmann::json& json)
{

    // 解析类型
    if (!json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Recipe missing 'type' field");
    }

    std::string type = json["type"].get<std::string>();

    // 有序合成
    if (type == "minecraft:crafting_shaped") {
        auto result = parseShapedRecipe(id, json);
        if (result.success()) {
            std::unique_ptr<CraftingRecipe> recipe(result.value().release());
            return recipe;
        }
        return result.error();
    }
    // 无序合成
    else if (type == "minecraft:crafting_shapeless") {
        auto result = parseShapelessRecipe(id, json);
        if (result.success()) {
            std::unique_ptr<CraftingRecipe> recipe(result.value().release());
            return recipe;
        }
        return result.error();
    }
    // 转化配方（crafting_transmute）
    else if (type == "minecraft:crafting_transmute") {
        auto result = parseTransmuteRecipe(id, json);
        if (result.success()) {
            std::unique_ptr<CraftingRecipe> recipe(result.value().release());
            return recipe;
        }
        return result.error();
    }
    // 切石机 - 返回错误提示使用专门的方法
    else if (type == "minecraft:stonecutting") {
        return Error(ErrorCode::ResourceParseError,
            "Stonecutting recipes should be loaded via parseStonecuttingRecipe. "
            "Use RecipeLoader::loadRecipeJson which handles all recipe types.");
    }
    // 锻造台 - 返回错误提示使用专门的方法
    else if (type == "minecraft:smithing") {
        return Error(ErrorCode::ResourceParseError,
            "Smithing recipes should be loaded via parseSmithingRecipe. "
            "Use RecipeLoader::loadRecipeJson which handles all recipe types.");
    }
    // 锻造升级/盔甲纹饰（MC 1.21+）- 返回错误提示使用专门的方法
    else if (type == "minecraft:smithing_transform") {
        return Error(ErrorCode::ResourceParseError,
            "Smithing transform recipes should be loaded via parseSmithingTransformRecipe. "
            "Use RecipeLoader::loadRecipeJson which handles all recipe types.");
    } else if (type == "minecraft:smithing_trim") {
        return Error(ErrorCode::ResourceParseError,
            "Smithing trim recipes should be loaded via parseSmithingTrimRecipe. "
            "Use RecipeLoader::loadRecipeJson which handles all recipe types.");
    }
    // 特殊合成配方（crafting_special_* / crafting_decorated_pot）- 返回错误提示使用专门的方法
    else if (type.rfind("minecraft:crafting_special_", 0) == 0 || type == "minecraft:crafting_decorated_pot") {
        return Error(ErrorCode::ResourceParseError,
            "Special crafting recipes should be loaded via parseSpecialRecipe. "
            "Use RecipeLoader::loadRecipeJson which handles all recipe types.");
    }
    // 熔炼类配方 - 返回错误提示使用专门的方法
    else if (type == "minecraft:smelting" || type == "minecraft:blasting" || type == "minecraft:smoking" ||
        type == "minecraft:campfire_cooking") {
        return Error(ErrorCode::ResourceParseError,
            "Smelting recipes should be loaded via fromSmeltingJson. "
            "Use RecipeLoader::loadRecipeJson which handles all recipe types.");
    } else {
        return Error(ErrorCode::ResourceParseError, "Unsupported recipe type: " + type);
    }
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::fromSmeltingJson(
    const ResourceLocation& id, const nlohmann::json& json)
{

    if (!json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Recipe missing 'type' field");
    }

    std::string type = json["type"].get<std::string>();

    if (type == "minecraft:smelting") {
        return parseSmeltingRecipe(id, json, DEFAULT_SMELTING_TIME);
    } else if (type == "minecraft:blasting") {
        return parseBlastingRecipe(id, json);
    } else if (type == "minecraft:smoking") {
        return parseSmokingRecipe(id, json);
    } else if (type == "minecraft:campfire_cooking") {
        return parseCampfireCookingRecipe(id, json);
    } else {
        return Error(ErrorCode::ResourceParseError, "Not a smelting recipe type: " + type);
    }
}

Result<std::unique_ptr<ShapedRecipe>> RecipeSerializers::parseShapedRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{

    // 解析pattern
    if (!json.contains("pattern") || !json["pattern"].is_array()) {
        return Error(ErrorCode::ResourceParseError, "Shaped recipe missing 'pattern' array");
    }

    std::vector<std::string> pattern;
    for (const auto& row : json["pattern"]) {
        if (!row.is_string()) {
            return Error(ErrorCode::ResourceParseError, "Pattern row must be a string");
        }
        pattern.push_back(row.get<std::string>());
    }

    if (pattern.empty()) {
        return Error(ErrorCode::ResourceParseError, "Pattern cannot be empty");
    }

    // 验证pattern
    std::string validationError = _validatePattern(pattern);
    if (!validationError.empty()) {
        return Error(ErrorCode::ResourceParseError, validationError);
    }

    // 压缩pattern（移除空边）
    std::vector<std::string> shrunkPattern = _shrinkPattern(pattern);
    if (shrunkPattern.empty()) {
        return Error(ErrorCode::ResourceParseError, "Pattern is all spaces");
    }

    // 计算压缩后的尺寸
    i32 width = static_cast<i32>(shrunkPattern[0].size());
    i32 height = static_cast<i32>(shrunkPattern.size());

    // 解析key
    if (!json.contains("key") || !json["key"].is_object()) {
        return Error(ErrorCode::ResourceParseError, "Shaped recipe missing 'key' object");
    }

    auto ingredientsResult = _parsePatternIngredients(shrunkPattern, json["key"]);
    if (!ingredientsResult.success()) {
        return ingredientsResult.error();
    }

    // 解析result
    if (!json.contains("result")) {
        return Error(ErrorCode::ResourceParseError, "Recipe missing 'result'");
    }

    auto resultStack = parseResult(json["result"]);
    if (!resultStack.success()) {
        return resultStack.error();
    }

    // 解析group（可选）
    std::string group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<std::string>();
    }

    return std::make_unique<ShapedRecipe>(
        id, width, height, std::move(ingredientsResult.value()), resultStack.value(), group);
}

Result<std::unique_ptr<ShapelessRecipe>> RecipeSerializers::parseShapelessRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{

    // 解析ingredients
    if (!json.contains("ingredients") || !json["ingredients"].is_array()) {
        return Error(ErrorCode::ResourceParseError, "Shapeless recipe missing 'ingredients' array");
    }

    std::vector<Ingredient> ingredients;
    for (const auto& ingJson : json["ingredients"]) {
        auto ingResult = parseIngredient(ingJson);
        if (!ingResult.success()) {
            return ingResult.error();
        }
        // 过滤空原料
        if (!ingResult.value().hasNoMatchingItems()) {
            ingredients.push_back(ingResult.value());
        }
    }

    // 校验：空原料数组
    if (ingredients.empty()) {
        return Error(ErrorCode::ResourceParseError, "No ingredients for shapeless recipe");
    }

    // 校验：原料数量上限
    if (ingredients.size() > static_cast<size_t>(MAX_RECIPE_WIDTH * MAX_RECIPE_HEIGHT)) {
        std::ostringstream oss;
        oss << "Too many ingredients for shapeless recipe, max is " << (MAX_RECIPE_WIDTH * MAX_RECIPE_HEIGHT);
        return Error(ErrorCode::ResourceParseError, oss.str());
    }

    // 解析result
    if (!json.contains("result")) {
        return Error(ErrorCode::ResourceParseError, "Recipe missing 'result'");
    }

    auto resultStack = parseResult(json["result"]);
    if (!resultStack.success()) {
        return resultStack.error();
    }

    // 解析group（可选）
    std::string group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<std::string>();
    }

    return std::make_unique<ShapelessRecipe>(id, std::move(ingredients), resultStack.value(), group);
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseSmeltingRecipe(
    const ResourceLocation& id, const nlohmann::json& json, i32 defaultCookTime)
{

    // 解析group（可选）
    std::string group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<std::string>();
    }

    // 解析ingredient
    if (!json.contains("ingredient")) {
        return Error(ErrorCode::ResourceParseError, "Smelting recipe missing 'ingredient'");
    }

    auto ingResult = parseIngredient(json["ingredient"]);
    if (!ingResult.success()) {
        return ingResult.error();
    }

    // 解析result
    if (!json.contains("result")) {
        return Error(ErrorCode::ResourceParseError, "Smelting recipe missing 'result'");
    }

    auto resultStack = parseResult(json["result"]);
    if (!resultStack.success()) {
        return resultStack.error();
    }

    // 解析experience（可选，默认0）
    f32 experience = 0.0f;
    if (json.contains("experience") && json["experience"].is_number()) {
        experience = json["experience"].get<f32>();
    }

    // 解析cookingtime（可选，使用默认值）
    i32 cookTime = defaultCookTime;
    if (json.contains("cookingtime") && json["cookingtime"].is_number_integer()) {
        cookTime = json["cookingtime"].get<i32>();
    }

    return std::make_unique<SmeltingRecipe>(id, group, ingResult.value(), resultStack.value(), experience, cookTime);
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseBlastingRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{

    // 高炉配方默认熔炼时间为 100 tick
    constexpr i32 BLASTING_COOK_TIME = 100;

    auto result = parseSmeltingRecipe(id, json, BLASTING_COOK_TIME);
    if (result.success()) {
        // 获取 SmeltingRecipe 并转换为 BlastingRecipe
        auto smelting = result.value();
        std::unique_ptr<SmeltingRecipe> blasting = std::make_unique<BlastingRecipe>(smelting->getId(),
            smelting->getGroup(),
            smelting->getIngredient(),
            smelting->getResultItem(),
            smelting->getExperience(),
            smelting->getCookTime());
        return blasting;
    }
    return result;
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseSmokingRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{

    // 烟熏炉配方默认熔炼时间为 100 tick
    constexpr i32 SMOKING_COOK_TIME = 100;

    auto result = parseSmeltingRecipe(id, json, SMOKING_COOK_TIME);
    if (result.success()) {
        // 获取 SmeltingRecipe 并转换为 SmokingRecipe
        auto smelting = result.value();
        std::unique_ptr<SmeltingRecipe> smoking = std::make_unique<SmokingRecipe>(smelting->getId(),
            smelting->getGroup(),
            smelting->getIngredient(),
            smelting->getResultItem(),
            smelting->getExperience(),
            smelting->getCookTime());
        return smoking;
    }
    return result;
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseCampfireCookingRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{

    // 营火烹饪配方默认熔炼时间为 600 tick（30秒）
    constexpr i32 CAMPFIRE_COOK_TIME = 600;

    auto result = parseSmeltingRecipe(id, json, CAMPFIRE_COOK_TIME);
    if (result.success()) {
        // 获取 SmeltingRecipe 并转换为 CampfireCookingRecipe
        auto smelting = result.value();
        std::unique_ptr<SmeltingRecipe> campfire = std::make_unique<CampfireCookingRecipe>(smelting->getId(),
            smelting->getGroup(),
            smelting->getIngredient(),
            smelting->getResultItem(),
            smelting->getExperience(),
            smelting->getCookTime());
        return campfire;
    }
    return result;
}

Result<Ingredient> RecipeSerializers::parseIngredient(const nlohmann::json& json)
{
    // 数组形式：多选项
    if (json.is_array()) {
        std::vector<Ingredient> ingredients;
        for (const auto& item : json) {
            auto ingResult = parseIngredient(item);
            if (!ingResult.success()) {
                return ingResult.error();
            }
            ingredients.push_back(ingResult.value());
        }
        return Ingredient::merge(ingredients);
    }

    // MC 1.21+ 字符串形式：直接物品ID（如 "minecraft:raw_iron"）或标签引用（如 "#minecraft:bundles"）
    if (json.is_string()) {
        std::string itemId = json.get<std::string>();

        // 标签引用：以 '#' 开头
        if (!itemId.empty() && itemId[0] == '#') {
            return Ingredient::fromTag(itemId.substr(1));
        }

        ResourceLocation loc(itemId);

        Item* item = ItemRegistry::instance().getItem(loc);
        if (!item) {
            // 物品未注册时返回空原料
            return Ingredient();
        }

        return Ingredient::fromItem(*item);
    }

    // 对象形式
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Ingredient must be a string, object, or array");
    }

    // 物品标签
    if (json.contains("tag")) {
        if (!json["tag"].is_string()) {
            return Error(ErrorCode::ResourceParseError, "Tag must be a string");
        }
        return Ingredient::fromTag(json["tag"].get<std::string>());
    }

    // 单个物品
    if (json.contains("item")) {
        if (!json["item"].is_string()) {
            return Error(ErrorCode::ResourceParseError, "Item must be a string");
        }

        std::string itemId = json["item"].get<std::string>();
        ResourceLocation loc(itemId);

        Item* item = ItemRegistry::instance().getItem(loc);
        if (!item) {
            // 物品未注册时返回空原料
            return Ingredient();
        }

        return Ingredient::fromItem(*item);
    }

    return Error(ErrorCode::ResourceParseError, "Ingredient must have 'item' or 'tag' field");
}

Result<ItemStack> RecipeSerializers::parseResult(const nlohmann::json& json)
{
    // 字符串形式：仅物品ID
    if (json.is_string()) {
        std::string itemId = json.get<std::string>();
        ResourceLocation loc(itemId);
        Item* item = ItemRegistry::instance().getItem(loc);
        if (!item) {
            return Error(ErrorCode::ResourceParseError, "Unknown item: " + itemId);
        }
        return ItemStack(*item, 1);
    }

    // 对象形式
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Result must be a string or object");
    }

    // MC 1.21+ 使用 "id" 字段，旧格式使用 "item" 字段
    std::string itemId;
    if (json.contains("item") && json["item"].is_string()) {
        itemId = json["item"].get<std::string>();
    } else if (json.contains("id") && json["id"].is_string()) {
        itemId = json["id"].get<std::string>();
    } else {
        return Error(ErrorCode::ResourceParseError, "Result missing 'item' or 'id' field");
    }
    ResourceLocation loc(itemId);

    Item* item = ItemRegistry::instance().getItem(loc);
    if (!item) {
        return Error(ErrorCode::ResourceParseError, "Unknown item: " + itemId);
    }

    i32 count = 1;
    if (json.contains("count") && json["count"].is_number_integer()) {
        count = json["count"].get<i32>();
        if (count < 1) {
            count = 1;
        }
    }

    // 创建物品堆
    ItemStack stack(*item, count);

    // 解析 NBT 数据
    // NBT 字段可以是两种格式：
    // 1. 字符串形式（Mojangson 格式）："{display:{Name:\"Custom Name\"}}"
    // 2. JSON 对象形式：{"display":{"Name":"Custom Name"}}
    if (json.contains("nbt")) {
        const auto& nbtValue = json["nbt"];

        if (nbtValue.is_string()) {
            // Mojangson 字符串格式
            std::string nbtString = nbtValue.get<std::string>();
            try {
                // 使用 Mojangson 格式解析 NBT 字符串
                std::istringstream iss(nbtString);
                iss >> nbt::contexts::mojangson;

                auto parsedTagPtr = nbt::tags::compound_tag::read(iss);
                if (parsedTagPtr && !iss.fail()) {
                    // 将 NBT 转换为 JSON 并合并到 ItemStack
                    nlohmann::json jsonTag = nbtToJson(*parsedTagPtr);
                    if (jsonTag.is_object() && !jsonTag.empty()) {
                        stack.mergeTag(jsonTag);
                    }
                }
            }
            catch (const std::exception&) {
                // 解析失败，忽略 NBT 数据
                // 在实际游戏中可能需要记录警告日志
            }
        } else if (nbtValue.is_object()) {
            // JSON 对象格式，直接合并
            stack.mergeTag(nbtValue);
        }
    }

    return stack;
}

std::vector<std::string> RecipeSerializers::_shrinkPattern(const std::vector<std::string>& pattern)
{
    if (pattern.empty()) {
        return {};
    }

    // 找到非空边界
    i32 minRow = -1;
    i32 maxRow = -1;
    i32 minCol = std::numeric_limits<i32>::max();
    i32 maxCol = 0;

    for (i32 row = 0; row < static_cast<i32>(pattern.size()); ++row) {
        const std::string& rowStr = pattern[row];
        bool hasContent = false;
        for (i32 col = 0; col < static_cast<i32>(rowStr.size()); ++col) {
            if (rowStr[col] != ' ') {
                hasContent = true;
                minCol = std::min(minCol, col);
                maxCol = std::max(maxCol, col);
            }
        }
        if (hasContent) {
            if (minRow < 0) minRow = row;
            maxRow = row;
        }
    }

    // 全空
    if (minRow < 0) {
        return {};
    }

    // 提取压缩后的pattern
    std::vector<std::string> result;
    for (i32 row = minRow; row <= maxRow; ++row) {
        std::string rowStr;
        for (i32 col = minCol; col <= maxCol; ++col) {
            if (col < static_cast<i32>(pattern[row].size())) {
                rowStr += pattern[row][col];
            } else {
                rowStr += ' ';
            }
        }
        result.push_back(rowStr);
    }

    return result;
}

std::string RecipeSerializers::_validatePattern(const std::vector<std::string>& pattern)
{
    if (pattern.size() > static_cast<size_t>(MAX_RECIPE_HEIGHT)) {
        std::ostringstream oss;
        oss << "Pattern has too many rows, max is " << MAX_RECIPE_HEIGHT;
        return oss.str();
    }

    // 检查每行长度
    size_t expectedWidth = 0;
    bool widthSet = false;

    for (const std::string& row : pattern) {
        if (row.size() > static_cast<size_t>(MAX_RECIPE_WIDTH)) {
            std::ostringstream oss;
            oss << "Pattern row is too long, max is " << MAX_RECIPE_WIDTH;
            return oss.str();
        }

        // 每行长度必须相同
        if (widthSet && row.size() != expectedWidth) {
            return "Pattern rows must have the same width";
        }

        if (!widthSet) {
            expectedWidth = row.size();
            widthSet = true;
        }
    }

    return ""; // 验证通过
}

Result<std::vector<Ingredient>> RecipeSerializers::_parsePatternIngredients(
    const std::vector<std::string>& pattern, const nlohmann::json& key)
{

    std::vector<Ingredient> ingredients;

    // 构建键到原料的映射
    std::unordered_map<char, Ingredient> keyMap;
    std::unordered_set<char> usedKeys; // 记录pattern中使用的key

    // 先收集pattern中使用的所有字符
    for (const std::string& row : pattern) {
        for (char c : row) {
            if (c != ' ') {
                usedKeys.insert(c);
            }
        }
    }

    // 解析key对象
    for (auto it = key.begin(); it != key.end(); ++it) {
        // key必须是单字符
        if (it.key().size() != 1) {
            return Error(ErrorCode::ResourceParseError,
                "Invalid key entry: '" + it.key() + "' is an invalid symbol (must be 1 character only).");
        }

        char c = it.key()[0];

        // 空格是保留符号
        if (c == ' ') {
            return Error(ErrorCode::ResourceParseError, "Invalid key entry: ' ' is a reserved symbol.");
        }

        auto ingResult = parseIngredient(it.value());
        if (!ingResult.success()) {
            return ingResult.error();
        }
        keyMap[c] = ingResult.value();
    }

    // 检查key中定义但未在pattern中使用的符号
    for (const auto& [keyChar, _] : keyMap) {
        if (usedKeys.find(keyChar) == usedKeys.end()) {
            return Error(ErrorCode::ResourceParseError,
                "Key defines symbols that aren't used in pattern: '" + std::string(1, keyChar) + "'");
        }
    }

    // 按行列顺序解析原料
    for (const std::string& row : pattern) {
        for (char c : row) {
            if (c == ' ') {
                // 空格表示空槽位
                ingredients.push_back(Ingredient::EMPTY);
            } else {
                auto it = keyMap.find(c);
                if (it == keyMap.end()) {
                    return Error(ErrorCode::ResourceParseError, "Pattern uses undefined key: " + std::string(1, c));
                }
                ingredients.push_back(it->second);
            }
        }
    }

    return ingredients;
}

// ============================================================================
// 切石机配方解析
// ============================================================================

Result<std::unique_ptr<StonecuttingRecipe>> RecipeSerializers::parseStonecuttingRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{
    // 解析group（可选，切石机配方通常不使用）
    std::string group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<std::string>();
    }

    // 解析ingredient
    if (!json.contains("ingredient")) {
        return Error(ErrorCode::ResourceParseError, "Stonecutting recipe missing 'ingredient'");
    }

    auto ingResult = parseIngredient(json["ingredient"]);
    if (!ingResult.success()) {
        return ingResult.error();
    }

    // 切石机配方的result是字符串形式，count是单独字段
    // 格式1: "result": "minecraft:stone_bricks", "count": 1
    // 格式2: "result": { "item": "minecraft:stone_bricks", "count": 1 }
    if (!json.contains("result")) {
        return Error(ErrorCode::ResourceParseError, "Stonecutting recipe missing 'result'");
    }

    // 解析结果物品
    ItemStack resultStack;
    const auto& resultValue = json["result"];

    if (resultValue.is_string()) {
        // 字符串形式：仅物品ID
        std::string itemId = resultValue.get<std::string>();
        ResourceLocation loc(itemId);
        Item* item = ItemRegistry::instance().getItem(loc);
        if (!item) {
            return Error(ErrorCode::ResourceParseError, "Unknown item: " + itemId);
        }

        // 解析count（可选，默认为1）
        i32 count = 1;
        if (json.contains("count") && json["count"].is_number_integer()) {
            count = json["count"].get<i32>();
            if (count < 1) {
                count = 1;
            }
        }

        resultStack = ItemStack(*item, count);
    } else if (resultValue.is_object()) {
        // 对象形式：使用parseResult解析
        auto parseResult = RecipeSerializers::parseResult(resultValue);
        if (!parseResult.success()) {
            return parseResult.error();
        }
        resultStack = parseResult.value();
    } else {
        return Error(ErrorCode::ResourceParseError, "Stonecutting recipe 'result' must be string or object");
    }

    return std::make_unique<StonecuttingRecipe>(id, group, ingResult.value(), resultStack, resultStack.getCount());
}

// ============================================================================
// 锻造台配方解析
// ============================================================================

Result<std::unique_ptr<SmithingRecipe>> RecipeSerializers::parseSmithingRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{
    // 解析base（基础物品）
    if (!json.contains("base")) {
        return Error(ErrorCode::ResourceParseError, "Smithing recipe missing 'base'");
    }

    auto baseResult = parseIngredient(json["base"]);
    if (!baseResult.success()) {
        return baseResult.error();
    }

    // 解析addition（添加物）
    if (!json.contains("addition")) {
        return Error(ErrorCode::ResourceParseError, "Smithing recipe missing 'addition'");
    }

    auto additionResult = parseIngredient(json["addition"]);
    if (!additionResult.success()) {
        return additionResult.error();
    }

    // 解析result
    if (!json.contains("result")) {
        return Error(ErrorCode::ResourceParseError, "Smithing recipe missing 'result'");
    }

    auto resultStack = parseResult(json["result"]);
    if (!resultStack.success()) {
        return resultStack.error();
    }

    return std::make_unique<SmithingRecipe>(id, baseResult.value(), additionResult.value(), resultStack.value());
}

// ============================================================================
// 锻造升级配方解析（smithing_transform，MC 1.21+）
// ============================================================================

Result<std::unique_ptr<SmithingTransformRecipe>> RecipeSerializers::parseSmithingTransformRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{
    // template（可选）：缺失时使用空 Ingredient
    Ingredient templateIngredient = Ingredient::EMPTY;
    if (json.contains("template")) {
        auto templateResult = parseIngredient(json["template"]);
        if (!templateResult.success()) {
            return templateResult.error();
        }
        templateIngredient = templateResult.value();
    }

    // base（必选）
    if (!json.contains("base")) {
        return Error(ErrorCode::ResourceParseError, "Smithing transform recipe missing 'base'");
    }
    auto baseResult = parseIngredient(json["base"]);
    if (!baseResult.success()) {
        return baseResult.error();
    }

    // addition（可选）：缺失时使用空 Ingredient
    Ingredient additionIngredient = Ingredient::EMPTY;
    if (json.contains("addition")) {
        auto additionResult = parseIngredient(json["addition"]);
        if (!additionResult.success()) {
            return additionResult.error();
        }
        additionIngredient = additionResult.value();
    }

    // result（必选，TransmuteResult 格式，复用 parseResult 支持 1.21 id 字段）
    if (!json.contains("result")) {
        return Error(ErrorCode::ResourceParseError, "Smithing transform recipe missing 'result'");
    }
    auto resultStack = parseResult(json["result"]);
    if (!resultStack.success()) {
        return resultStack.error();
    }

    return std::make_unique<SmithingTransformRecipe>(
        id, std::move(templateIngredient), baseResult.value(), std::move(additionIngredient), resultStack.value());
}

// ============================================================================
// 盔甲纹饰配方解析（smithing_trim，MC 1.21+）
// ============================================================================

Result<std::unique_ptr<SmithingTrimRecipe>> RecipeSerializers::parseSmithingTrimRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{
    // template（必选）
    if (!json.contains("template")) {
        return Error(ErrorCode::ResourceParseError, "Smithing trim recipe missing 'template'");
    }
    auto templateResult = parseIngredient(json["template"]);
    if (!templateResult.success()) {
        return templateResult.error();
    }

    // base（必选）
    if (!json.contains("base")) {
        return Error(ErrorCode::ResourceParseError, "Smithing trim recipe missing 'base'");
    }
    auto baseResult = parseIngredient(json["base"]);
    if (!baseResult.success()) {
        return baseResult.error();
    }

    // addition（必选）
    if (!json.contains("addition")) {
        return Error(ErrorCode::ResourceParseError, "Smithing trim recipe missing 'addition'");
    }
    auto additionResult = parseIngredient(json["addition"]);
    if (!additionResult.success()) {
        return additionResult.error();
    }

    // pattern（必选，TrimPattern 的 ResourceLocation）
    // TODO: 纹饰系统（TrimPattern 注册表）未实现，当前仅解析为 ResourceLocation 暂存，
    // 待纹饰注册表接入后解析为 Holder<TrimPattern>。
    if (!json.contains("pattern") || !json["pattern"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Smithing trim recipe missing 'pattern' string");
    }
    ResourceLocation pattern(json["pattern"].get<std::string>());

    return std::make_unique<SmithingTrimRecipe>(
        id, templateResult.value(), baseResult.value(), additionResult.value(), std::move(pattern));
}

// ============================================================================
// 特殊合成配方解析（crafting_special_* / crafting_decorated_pot）
// ============================================================================

namespace {
/// 特殊合成配方 type -> 工厂函数映射。
/// 这些配方的 JSON 仅含 type + category，无原料/结果字段。行为由对应代码类实现，
/// 此处按 type 创建对应 SpecialRecipe 子类实例（用数据包 ID）。
/// 参考 MC 1.21.11 SpecialRecipeSerializer：fromJson 不解析字段，仅按 type 建实例。
const std::unordered_map<std::string, std::function<std::unique_ptr<CraftingRecipe>(const ResourceLocation&)>>&
specialRecipeFactories()
{
    static const std::unordered_map<std::string,
        std::function<std::unique_ptr<CraftingRecipe>(const ResourceLocation&)>>
        factories = {
            {"minecraft:crafting_special_repairitem",
                [](const ResourceLocation& id) { return std::make_unique<RepairItemRecipe>(id); }},
            {"minecraft:crafting_special_armordye",
                [](const ResourceLocation& id) { return std::make_unique<ArmorDyeRecipe>(id); }},
            {"minecraft:crafting_special_bookcloning",
                [](const ResourceLocation& id) { return std::make_unique<BookCloningRecipe>(id); }},
            {"minecraft:crafting_special_mapcloning",
                [](const ResourceLocation& id) { return std::make_unique<MapCloningRecipe>(id); }},
            {"minecraft:crafting_special_mapextending",
                [](const ResourceLocation& id) { return std::make_unique<MapExtendingRecipe>(id); }},
            {"minecraft:crafting_special_tippedarrow",
                [](const ResourceLocation& id) { return std::make_unique<TippedArrowRecipe>(id); }},
            {"minecraft:crafting_decorated_pot",
                [](const ResourceLocation& id) { return std::make_unique<DecoratedPotRecipe>(id); }},
            {"minecraft:crafting_special_bannerduplicate",
                [](const ResourceLocation& id) { return std::make_unique<BannerDuplicateRecipe>(id); }},
            {"minecraft:crafting_special_shielddecoration",
                [](const ResourceLocation& id) { return std::make_unique<ShieldDecorationRecipe>(id); }},
            {"minecraft:crafting_special_firework_rocket",
                [](const ResourceLocation& id) { return std::make_unique<FireworkRocketRecipe>(id); }},
            {"minecraft:crafting_special_firework_star",
                [](const ResourceLocation& id) { return std::make_unique<FireworkStarRecipe>(id); }},
            {"minecraft:crafting_special_firework_star_fade",
                [](const ResourceLocation& id) { return std::make_unique<FireworkStarFadeRecipe>(id); }},
        };
    return factories;
}
} // namespace

Result<std::unique_ptr<CraftingRecipe>> RecipeSerializers::parseSpecialRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{
    // 读取 type 字段
    if (!json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Special recipe missing 'type' field");
    }
    std::string type = json["type"].get<std::string>();

    // 按 type 查工厂表
    const auto& factories = specialRecipeFactories();
    auto it = factories.find(type);
    if (it == factories.end()) {
        return Error(ErrorCode::ResourceParseError, "Unknown special recipe type: " + type);
    }

    // 特殊配方的 JSON 无原料/结果字段，不解析其他内容，直接按 type 创建实例
    return it->second(id);
}

// ============================================================================
// 转化配方解析（crafting_transmute）
// ============================================================================

Result<std::unique_ptr<TransmuteRecipe>> RecipeSerializers::parseTransmuteRecipe(
    const ResourceLocation& id, const nlohmann::json& json)
{
    // 解析group（可选）
    std::string group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<std::string>();
    }

    // 解析input（被转化的物品）
    if (!json.contains("input")) {
        return Error(ErrorCode::ResourceParseError, "Transmute recipe missing 'input'");
    }

    auto inputResult = parseIngredient(json["input"]);
    if (!inputResult.success()) {
        return inputResult.error();
    }

    // 解析material（材料）
    if (!json.contains("material")) {
        return Error(ErrorCode::ResourceParseError, "Transmute recipe missing 'material'");
    }

    auto materialResult = parseIngredient(json["material"]);
    if (!materialResult.success()) {
        return materialResult.error();
    }

    // 解析result
    if (!json.contains("result")) {
        return Error(ErrorCode::ResourceParseError, "Transmute recipe missing 'result'");
    }

    auto resultStack = parseResult(json["result"]);
    if (!resultStack.success()) {
        return resultStack.error();
    }

    return std::make_unique<TransmuteRecipe>(
        id, inputResult.value(), materialResult.value(), resultStack.value(), group);
}

} // namespace crafting
} // namespace mc
