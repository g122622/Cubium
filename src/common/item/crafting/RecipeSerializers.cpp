#include "item/crafting/RecipeSerializers.hpp"
#include "item/core/ItemRegistry.hpp"
#include <algorithm>
#include <sstream>

namespace mc {
namespace crafting {

Result<std::unique_ptr<CraftingRecipe>> RecipeSerializers::fromJson(
    const ResourceLocation& id,
    const nlohmann::json& json) {

    // 解析类型
    if (!json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Recipe missing 'type' field");
    }

    String type = json["type"].get<String>();

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
    // 熔炉
    else if (type == "minecraft:smelting") {
        auto result = parseSmeltingRecipe(id, json, DEFAULT_SMELTING_TIME);
        if (result.success()) {
            // 转换为 CraftingRecipe 不适用，熔炼配方独立存储
            return Error(ErrorCode::ResourceParseError,
                "Smelting recipes should be loaded via fromSmeltingJson");
        }
        return result.error();
    }
    // 高炉
    else if (type == "minecraft:blasting") {
        auto result = parseBlastingRecipe(id, json);
        if (result.success()) {
            return Error(ErrorCode::ResourceParseError,
                "Blasting recipes should be loaded via fromSmeltingJson");
        }
        return result.error();
    }
    // 烟熏炉
    else if (type == "minecraft:smoking") {
        auto result = parseSmokingRecipe(id, json);
        if (result.success()) {
            return Error(ErrorCode::ResourceParseError,
                "Smoking recipes should be loaded via fromSmeltingJson");
        }
        return result.error();
    }
    // 营火烹饪
    else if (type == "minecraft:campfire_cooking") {
        auto result = parseCampfireCookingRecipe(id, json);
        if (result.success()) {
            return Error(ErrorCode::ResourceParseError,
                "Campfire cooking recipes should be loaded via fromSmeltingJson");
        }
        return result.error();
    }
    else {
        return Error(ErrorCode::ResourceParseError,
                     "Unsupported recipe type: " + type);
    }
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::fromSmeltingJson(
    const ResourceLocation& id,
    const nlohmann::json& json) {

    if (!json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Recipe missing 'type' field");
    }

    String type = json["type"].get<String>();

    if (type == "minecraft:smelting") {
        return parseSmeltingRecipe(id, json, DEFAULT_SMELTING_TIME);
    }
    else if (type == "minecraft:blasting") {
        return parseBlastingRecipe(id, json);
    }
    else if (type == "minecraft:smoking") {
        return parseSmokingRecipe(id, json);
    }
    else if (type == "minecraft:campfire_cooking") {
        return parseCampfireCookingRecipe(id, json);
    }
    else {
        return Error(ErrorCode::ResourceParseError,
                     "Not a smelting recipe type: " + type);
    }
}

Result<std::unique_ptr<ShapedRecipe>> RecipeSerializers::parseShapedRecipe(
    const ResourceLocation& id,
    const nlohmann::json& json) {

    // 解析pattern
    if (!json.contains("pattern") || !json["pattern"].is_array()) {
        return Error(ErrorCode::ResourceParseError, "Shaped recipe missing 'pattern' array");
    }

    std::vector<String> pattern;
    for (const auto& row : json["pattern"]) {
        if (!row.is_string()) {
            return Error(ErrorCode::ResourceParseError, "Pattern row must be a string");
        }
        pattern.push_back(row.get<String>());
    }

    if (pattern.empty()) {
        return Error(ErrorCode::ResourceParseError, "Pattern cannot be empty");
    }

    // 验证pattern
    String validationError = validatePattern(pattern);
    if (!validationError.empty()) {
        return Error(ErrorCode::ResourceParseError, validationError);
    }

    // 压缩pattern（移除空边）
    std::vector<String> shrunkPattern = shrinkPattern(pattern);
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

    auto ingredientsResult = parsePatternIngredients(shrunkPattern, json["key"]);
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
    String group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<String>();
    }

    return std::make_unique<ShapedRecipe>(
        id,
        width,
        height,
        std::move(ingredientsResult.value()),
        resultStack.value(),
        group
    );
}

Result<std::unique_ptr<ShapelessRecipe>> RecipeSerializers::parseShapelessRecipe(
    const ResourceLocation& id,
    const nlohmann::json& json) {

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
        // MC 原版：过滤空原料
        if (!ingResult.value().hasNoMatchingItems()) {
            ingredients.push_back(ingResult.value());
        }
    }

    // MC 原版校验：空原料数组
    if (ingredients.empty()) {
        return Error(ErrorCode::ResourceParseError, "No ingredients for shapeless recipe");
    }

    // MC 原版校验：原料数量上限
    if (ingredients.size() > static_cast<size_t>(MAX_RECIPE_WIDTH * MAX_RECIPE_HEIGHT)) {
        std::ostringstream oss;
        oss << "Too many ingredients for shapeless recipe, max is "
            << (MAX_RECIPE_WIDTH * MAX_RECIPE_HEIGHT);
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
    String group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<String>();
    }

    return std::make_unique<ShapelessRecipe>(
        id,
        std::move(ingredients),
        resultStack.value(),
        group
    );
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseSmeltingRecipe(
    const ResourceLocation& id,
    const nlohmann::json& json,
    i32 defaultCookTime) {

    // 解析group（可选）
    String group;
    if (json.contains("group") && json["group"].is_string()) {
        group = json["group"].get<String>();
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

    return std::make_unique<SmeltingRecipe>(
        id, group, ingResult.value(), resultStack.value(), experience, cookTime
    );
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseBlastingRecipe(
    const ResourceLocation& id,
    const nlohmann::json& json) {

    auto result = parseSmeltingRecipe(id, json, DEFAULT_COOKING_TIME);
    if (result.success()) {
        // 转换为 BlastingRecipe
        // 目前 BlastingRecipe 继承自 SmeltingRecipe，仅类型不同
        // 实际创建时需要使用 BlastingRecipe 类
        // 这里暂时返回 SmeltingRecipe，后续需要重构
    }
    return result;
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseSmokingRecipe(
    const ResourceLocation& id,
    const nlohmann::json& json) {

    return parseSmeltingRecipe(id, json, DEFAULT_COOKING_TIME);
}

Result<std::unique_ptr<SmeltingRecipe>> RecipeSerializers::parseCampfireCookingRecipe(
    const ResourceLocation& id,
    const nlohmann::json& json) {

    return parseSmeltingRecipe(id, json, DEFAULT_COOKING_TIME);
}

Result<Ingredient> RecipeSerializers::parseIngredient(const nlohmann::json& json) {
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

    // 对象形式
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Ingredient must be an object or array");
    }

    // 物品标签
    if (json.contains("tag")) {
        if (!json["tag"].is_string()) {
            return Error(ErrorCode::ResourceParseError, "Tag must be a string");
        }
        return Ingredient::fromTag(json["tag"].get<String>());
    }

    // 单个物品
    if (json.contains("item")) {
        if (!json["item"].is_string()) {
            return Error(ErrorCode::ResourceParseError, "Item must be a string");
        }

        String itemId = json["item"].get<String>();
        ResourceLocation loc(itemId);

        Item* item = ItemRegistry::instance().getItem(loc);
        if (!item) {
            // MC 原版：物品未注册时返回空原料（hasNoMatchingItems == true）
            return Ingredient();
        }

        return Ingredient::fromItem(*item);
    }

    return Error(ErrorCode::ResourceParseError, "Ingredient must have 'item' or 'tag' field");
}

Result<ItemStack> RecipeSerializers::parseResult(const nlohmann::json& json) {
    // 字符串形式：仅物品ID
    if (json.is_string()) {
        String itemId = json.get<String>();
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

    if (!json.contains("item") || !json["item"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Result missing 'item' field");
    }

    String itemId = json["item"].get<String>();
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

    // TODO: 支持NBT数据解析
    // if (json.contains("nbt")) { ... }

    return ItemStack(*item, count);
}

std::vector<String> RecipeSerializers::shrinkPattern(const std::vector<String>& pattern) {
    if (pattern.empty()) {
        return {};
    }

    // 找到非空边界
    i32 minRow = -1;
    i32 maxRow = -1;
    i32 minCol = std::numeric_limits<i32>::max();
    i32 maxCol = 0;

    for (i32 row = 0; row < static_cast<i32>(pattern.size()); ++row) {
        const String& rowStr = pattern[row];
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
    std::vector<String> result;
    for (i32 row = minRow; row <= maxRow; ++row) {
        String rowStr;
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

String RecipeSerializers::validatePattern(const std::vector<String>& pattern) {
    if (pattern.size() > static_cast<size_t>(MAX_RECIPE_HEIGHT)) {
        std::ostringstream oss;
        oss << "Pattern has too many rows, max is " << MAX_RECIPE_HEIGHT;
        return oss.str();
    }

    // 检查每行长度
    size_t expectedWidth = 0;
    bool widthSet = false;

    for (const String& row : pattern) {
        if (row.size() > static_cast<size_t>(MAX_RECIPE_WIDTH)) {
            std::ostringstream oss;
            oss << "Pattern row is too long, max is " << MAX_RECIPE_WIDTH;
            return oss.str();
        }

        // MC 原版：每行长度必须相同
        if (widthSet && row.size() != expectedWidth) {
            return "Pattern rows must have the same width";
        }

        if (!widthSet) {
            expectedWidth = row.size();
            widthSet = true;
        }
    }

    return "";  // 验证通过
}

Result<std::vector<Ingredient>> RecipeSerializers::parsePatternIngredients(
    const std::vector<String>& pattern,
    const nlohmann::json& key) {

    std::vector<Ingredient> ingredients;

    // 构建键到原料的映射
    std::unordered_map<char, Ingredient> keyMap;
    for (auto it = key.begin(); it != key.end(); ++it) {
        char c = it.key()[0];  // 键是单个字符
        auto ingResult = parseIngredient(it.value());
        if (!ingResult.success()) {
            return ingResult.error();
        }
        keyMap[c] = ingResult.value();
    }

    // 按行列顺序解析原料
    for (const String& row : pattern) {
        for (char c : row) {
            if (c == ' ') {
                // 空格表示空槽位（MC 原版使用 Ingredient.EMPTY）
                ingredients.push_back(Ingredient::EMPTY);
            }
            else {
                auto it = keyMap.find(c);
                if (it == keyMap.end()) {
                    return Error(ErrorCode::ResourceParseError,
                                 "Pattern uses undefined key: " + String(1, c));
                }
                ingredients.push_back(it->second);
            }
        }
    }

    return ingredients;
}

} // namespace crafting
} // namespace mc
