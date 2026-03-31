#include "item/crafting/RecipeSerializers.hpp"
#include "item/core/ItemRegistry.hpp"
#include <algorithm>
#include <limits>

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

    if (type == "minecraft:crafting_shaped") {
        auto result = parseShapedRecipe(id, json);
        if (result.success()) {
            std::unique_ptr<CraftingRecipe> recipe(result.value().release());
            return recipe;
        }
        return result.error();
    }
    else if (type == "minecraft:crafting_shapeless") {
        auto result = parseShapelessRecipe(id, json);
        if (result.success()) {
            std::unique_ptr<CraftingRecipe> recipe(result.value().release());
            return recipe;
        }
        return result.error();
    }
    else {
        return Error(ErrorCode::ResourceParseError,
                     "Unsupported recipe type: " + type);
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

    // 计算尺寸
    i32 width, height;
    if (!parsePatternDimensions(pattern, width, height)) {
        return Error(ErrorCode::ResourceParseError, "Invalid pattern dimensions");
    }

    // 解析key
    if (!json.contains("key") || !json["key"].is_object()) {
        return Error(ErrorCode::ResourceParseError, "Shaped recipe missing 'key' object");
    }

    auto ingredientsResult = parsePatternIngredients(pattern, json["key"], width);
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
        ingredients.push_back(ingResult.value());
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

Result<Ingredient> RecipeSerializers::parseIngredient(const nlohmann::json& json) {
    // 数组形式：多选项
    if (json.is_array()) {
        std::vector<ItemStack> stacks;
        for (const auto& item : json) {
            auto ingResult = parseIngredient(item);
            if (!ingResult.success()) {
                return ingResult.error();
            }
            // 合并匹配堆栈
            const auto& matchingStacks = ingResult.value().getMatchingStacks();
            for (const auto& stack : matchingStacks) {
                stacks.push_back(stack);
            }
        }
        return Ingredient::fromStacks(std::move(stacks));
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
            // 物品未注册，返回空原料
            return Ingredient();
        }

        return Ingredient::fromItem(*item);
    }

    return Error(ErrorCode::ResourceParseError, "Ingredient must have 'item' or 'tag' field");
}

Result<ItemStack> RecipeSerializers::parseResult(const nlohmann::json& json) {
    if (!json.is_object()) {
        return Error(ErrorCode::ResourceParseError, "Result must be an object");
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

    return ItemStack(*item, count);
}

bool RecipeSerializers::parsePatternDimensions(const std::vector<String>& pattern,
                                                i32& outWidth,
                                                i32& outHeight) {
    if (pattern.empty()) {
        return false;
    }

    outHeight = static_cast<i32>(pattern.size());

    // 找到非空部分的宽度
    i32 minWidth = std::numeric_limits<i32>::max();
    i32 maxWidth = 0;

    for (const String& row : pattern) {
        i32 rowWidth = 0;
        bool foundNonSpace = false;

        for (char c : row) {
            if (c != ' ') {
                foundNonSpace = true;
                ++rowWidth;
            }
        }

        if (foundNonSpace) {
            minWidth = std::min(minWidth, static_cast<i32>(row.find_first_not_of(' ')));
            maxWidth = std::max(maxWidth, rowWidth);
        }
    }

    if (minWidth == std::numeric_limits<i32>::max()) {
        // 全空pattern
        outWidth = 0;
        return true;
    }

    outWidth = maxWidth;
    return true;
}

Result<std::vector<Ingredient>> RecipeSerializers::parsePatternIngredients(
    const std::vector<String>& pattern,
    const nlohmann::json& key,
    [[maybe_unused]] i32 width) {

    std::vector<Ingredient> ingredients;

    // 构建键到原料的映射
    std::unordered_map<char, Ingredient> keyMap;
    for (auto it = key.begin(); it != key.end(); ++it) {
        char c = it.key()[0]; // 键是单个字符
        auto ingResult = parseIngredient(it.value());
        if (!ingResult.success()) {
            return ingResult.error();
        }
        keyMap[c] = ingResult.value();
    }

    // 计算pattern的实际边界
    i32 minCol = std::numeric_limits<i32>::max();
    i32 maxCol = 0;
    i32 minRow = -1;
    i32 maxRow = -1;

    for (i32 row = 0; row < static_cast<i32>(pattern.size()); ++row) {
        const String& rowStr = pattern[row];
        for (i32 col = 0; col < static_cast<i32>(rowStr.size()); ++col) {
            if (rowStr[col] != ' ') {
                if (minRow < 0) minRow = row;
                maxRow = row;
                minCol = std::min(minCol, col);
                maxCol = std::max(maxCol, col);
            }
        }
    }

    // 如果pattern全空
    if (minRow < 0) {
        return ingredients;
    }

    // 按行列顺序解析原料
    for (i32 row = minRow; row <= maxRow; ++row) {
        const String& rowStr = pattern[row];
        for (i32 col = minCol; col <= maxCol; ++col) {
            char c = ' ';
            if (col < static_cast<i32>(rowStr.size())) {
                c = rowStr[col];
            }

            if (c == ' ') {
                // 空格表示空槽位
                ingredients.push_back(Ingredient());
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
