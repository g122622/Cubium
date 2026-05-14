#pragma once

#include "core/Result.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/crafting/ShapedRecipe.hpp"
#include "item/crafting/ShapelessRecipe.hpp"
#include "item/crafting/SmeltingRecipe.hpp"
#include "item/crafting/SmithingRecipe.hpp"
#include "item/crafting/StonecuttingRecipe.hpp"
#include <memory>
#include <nlohmann/json.hpp>

namespace mc {
namespace crafting {

/**
 * @brief 配方序列化器
 *
 * 提供从JSON解析配方的功能，兼容MC 1.16.5数据包格式。
 *
 * 支持的配方类型：
 * - minecraft:crafting_shaped - 有序合成
 * - minecraft:crafting_shapeless - 无序合成
 * - minecraft:smelting - 熔炉配方
 * - minecraft:blasting - 高炉配方
 * - minecraft:smoking - 烟熏炉配方
 * - minecraft:campfire_cooking - 营火烹饪配方
 *
 * JSON格式示例（有序合成）：
 * @code
 * {
 *   "type": "minecraft:crafting_shaped",
 *   "pattern": ["###", " # ", "###"],
 *   "key": {
 *     "#": { "item": "minecraft:oak_planks" }
 *   },
 *   "result": {
 *     "item": "minecraft:crafting_table",
 *     "count": 1
 *   },
 *   "group": "crafting_tables"
 * }
 * @endcode
 *
 * JSON格式示例（无序合成）：
 * @code
 * {
 *   "type": "minecraft:crafting_shapeless",
 *   "ingredients": [
 *     { "item": "minecraft:iron_ingot" },
 *     { "item": "minecraft:stick" }
 *   ],
 *   "result": {
 *     "item": "minecraft:iron_nugget",
 *     "count": 9
 *   }
 * }
 * @endcode
 */
class RecipeSerializers {
public:
    /// 有序/无序合成的最大宽度（MC 1.16.5 默认值）
    static constexpr i32 MAX_RECIPE_WIDTH = 3;

    /// 有序/无序合成的最大高度（MC 1.16.5 默认值）
    static constexpr i32 MAX_RECIPE_HEIGHT = 3;

    /// 默认熔炼时间（tick）
    static constexpr i32 DEFAULT_SMELTING_TIME = 200;

    /// 默认高炉/烟熏炉/营火烹饪时间（tick）
    static constexpr i32 DEFAULT_COOKING_TIME = 100;

    /// 配方类型字符串常量
    struct Type {
        static constexpr const char* CRAFTING_SHAPED = "minecraft:crafting_shaped";
        static constexpr const char* CRAFTING_SHAPELESS = "minecraft:crafting_shapeless";
        static constexpr const char* SMELTING = "minecraft:smelting";
        static constexpr const char* BLASTING = "minecraft:blasting";
        static constexpr const char* SMOKING = "minecraft:smoking";
        static constexpr const char* CAMPFIRE_COOKING = "minecraft:campfire_cooking";
        static constexpr const char* STONECUTTING = "minecraft:stonecutting";
        static constexpr const char* SMITHING = "minecraft:smithing";
    };

    /**
     * @brief 从JSON解析配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<CraftingRecipe>> fromJson(const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 从JSON解析熔炼类配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<SmeltingRecipe>> fromSmeltingJson(
        const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 解析有序合成配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<ShapedRecipe>> parseShapedRecipe(
        const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 解析无序合成配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<ShapelessRecipe>> parseShapelessRecipe(
        const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 解析熔炉配方
     * @param id 配方ID
     * @param json JSON数据
     * @param defaultCookTime 默认熔炼时间
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<SmeltingRecipe>> parseSmeltingRecipe(
        const ResourceLocation& id, const nlohmann::json& json, i32 defaultCookTime = DEFAULT_SMELTING_TIME);

    /**
     * @brief 解析高炉配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<SmeltingRecipe>> parseBlastingRecipe(
        const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 解析烟熏炉配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<SmeltingRecipe>> parseSmokingRecipe(
        const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 解析营火烹饪配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<SmeltingRecipe>> parseCampfireCookingRecipe(
        const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 解析原料
     * @param json JSON数据
     * @return 解析的原料，或错误
     *
     * 支持格式：
     * - 单个物品: { "item": "minecraft:stone" }
     * - 物品标签: { "tag": "minecraft:planks" }
     * - 多选项: [ { "item": "a" }, { "item": "b" } ]
     */
    static Result<Ingredient> parseIngredient(const nlohmann::json& json);

    /**
     * @brief 解析结果物品堆
     * @param json JSON数据
     * @return 解析的物品堆，或错误
     *
     * 支持格式：
     * - 字符串形式: "minecraft:stone"
     * - 对象形式: { "item": "minecraft:stone", "count": 1 }
     */
    static Result<ItemStack> parseResult(const nlohmann::json& json);

private:
    /**
     * @brief 压缩pattern，移除空边
     * @param pattern 原始pattern数组
     * @return 压缩后的pattern数组
     *
     * MC 原版 shrink() 方法：
     * - 移除顶部的空行
     * - 移除底部的空行
     * - 移除左边的空列
     * - 移除右边的空列
     */
    static std::vector<std::string> shrinkPattern(const std::vector<std::string>& pattern);

    /**
     * @brief 验证pattern数组
     * @param pattern 字符串数组
     * @return 错误信息，如果验证通过则为空
     *
     * 验证规则：
     * - 行数不超过 MAX_RECIPE_HEIGHT
     * - 每行长度不超过 MAX_RECIPE_WIDTH
     * - 所有行长度相同
     */
    static std::string validatePattern(const std::vector<std::string>& pattern);

    /**
     * @brief 从pattern和key解析原料列表
     * @param pattern 字符串数组（已压缩）
     * @param key 键映射
     * @return 解析的原料列表，或错误
     */
    static Result<std::vector<Ingredient>> parsePatternIngredients(
        const std::vector<std::string>& pattern, const nlohmann::json& key);
};

} // namespace crafting
} // namespace mc
