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

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "core/Result.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/crafting/ShapedRecipe.hpp"
#include "item/crafting/ShapelessRecipe.hpp"
#include "item/crafting/SmeltingRecipe.hpp"
#include "item/crafting/SmithingRecipe.hpp"
#include "item/crafting/StonecuttingRecipe.hpp"
#include "item/crafting/TransmuteRecipe.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace crafting {

/**
 * @brief 配方序列化器
 *
 * 提供从JSON解析配方的功能，支持标准数据包格式。
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
    /// 有序/无序合成的最大宽度
    static constexpr i32 MAX_RECIPE_WIDTH = 3;

    /// 有序/无序合成的最大高度
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
        static constexpr const char* CRAFTING_TRANSMUTE = "minecraft:crafting_transmute";
    };

    /**
     * @brief 从JSON解析合成配方（有序/无序）
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     *
     * 支持的配方类型：
     * - minecraft:crafting_shaped
     * - minecraft:crafting_shapeless
     */
    static Result<std::unique_ptr<CraftingRecipe>> fromJson(const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 从JSON解析切石机配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<StonecuttingRecipe>> parseStonecuttingRecipe(
        const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 从JSON解析锻造台配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    static Result<std::unique_ptr<SmithingRecipe>> parseSmithingRecipe(
        const ResourceLocation& id, const nlohmann::json& json);

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
     * @brief 解析转化配方（crafting_transmute）
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     *
     * JSON 格式：
     * @code
     * {
     *   "type": "minecraft:crafting_transmute",
     *   "category": "equipment",        // 可选，目前未使用
     *   "group": "bundle_dye",          // 可选
     *   "input": "#minecraft:bundles",  // 被转化的物品（物品ID/标签/对象）
     *   "material": "minecraft:white_dye", // 材料（物品ID/标签/对象）
     *   "result": {                      // 转化结果（仅物品类型和数量）
     *     "id": "minecraft:white_bundle"
     *   }
     * }
     * @endcode
     */
    static Result<std::unique_ptr<TransmuteRecipe>> parseTransmuteRecipe(
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
     * - 带 NBT 数据: { "item": "minecraft:stone_sword", "count": 1, "nbt": "{display:{Name:\"Custom Name\"}}" }
     * - NBT JSON 对象: { "item": "minecraft:stone_sword", "nbt": {"display":{"Name":"Custom Name"}} }
     *
     * NBT 字段支持两种格式：
     * 1. Mojangson 字符串格式："{display:{Name:\"{\\\"text\\\":\\\"Custom Name\\\"}\"}}"
     * 2. JSON 对象格式：{"display":{"Name":{"text":"Custom Name"}}}
     */
    static Result<ItemStack> parseResult(const nlohmann::json& json);

private:
    /**
     * @brief 压缩pattern，移除空边
     * @param pattern 原始pattern数组
     * @return 压缩后的pattern数组
     */
    static std::vector<std::string> _shrinkPattern(const std::vector<std::string>& pattern);

    /**
     * @brief 验证pattern数组
     * @param pattern 字符串数组
     * @return 错误信息，如果验证通过则为空
     */
    static std::string _validatePattern(const std::vector<std::string>& pattern);

    /**
     * @brief 从pattern和key解析原料列表
     * @param pattern 字符串数组（已压缩）
     * @param key 键映射
     * @return 解析的原料列表，或错误
     */
    static Result<std::vector<Ingredient>> _parsePatternIngredients(
        const std::vector<std::string>& pattern, const nlohmann::json& key);
};

} // namespace crafting
} // namespace mc
