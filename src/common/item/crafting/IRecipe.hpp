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
#include "core/Result.hpp"
#include "item/crafting/Ingredient.hpp"
#include "resource/ResourceLocation.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace crafting {

/**
 * @brief 配方类型枚举
 */
enum class RecipeType : u8 {
    Crafting,          ///< 有序/无序合成（通用）
    ShapedCrafting,    ///< 有序合成
    ShapelessCrafting, ///< 无序合成
    Smelting,          ///< 熔炼
    Blasting,          ///< 高炉
    Smoking,           ///< 烟熏炉
    CampfireCooking,   ///< 营火烹饪
    Stonecutting,      ///< 切石机
    Smithing,          ///< 锻造台
    Transmute,         ///< 物品转化（MC 1.21+，如收纳袋染色）
    Special            ///< 特殊配方（如地图扩展）
};

/**
 * @brief 获取RecipeType的字符串表示
 * @param type 配方类型
 * @return 类型的字符串ID（如 "minecraft:crafting_shaped"）
 */
std::string recipeTypeToString(RecipeType type);

/**
 * @brief 从字符串解析RecipeType
 * @param str 类型的字符串ID
 * @return 配方类型，如果无法识别返回std::nullopt
 */
std::optional<RecipeType> recipeTypeFromString(const std::string& str);

/**
 * @brief 配方接口模板
 *
 * 所有配方的基类，定义配方的基本行为。
 * 模板参数C是容器类型，用于检查配方是否匹配。
 *
 * 常见容器类型：
 * - CraftingInventory: 工作台合成网格
 * - FurnaceInventory: 熔炉输入槽
 * - SmokerInventory: 烟熏炉输入槽
 *
 * 注意事项：
 * - matches()和assemble()不应修改容器内容
 * - assemble()返回的物品堆应该总是相同的结果（对于确定性配方）
 * - 对于有NBT结果的配方，assemble()可能需要复制NBT数据
 */
template <typename C>
class IRecipe {
public:
    virtual ~IRecipe() = default;

    /**
     * @brief 检查配方是否匹配给定容器
     * @param inventory 容器实例
     * @return 如果匹配返回true
     *
     * 注意：此方法不应修改容器的任何状态
     */
    [[nodiscard]] virtual bool matches(const C& inventory) const = 0;

    /**
     * @brief 根据容器内容生成结果物品堆
     * @param inventory 容器实例
     * @return 结果物品堆
     *
     * 注意：
     * - 返回的结果物品堆可能包含容器的NBT数据
     * - 此方法不应修改容器的任何状态
     * - 对于无序配方，结果的NBT可能来自任一输入物品
     */
    [[nodiscard]] virtual ItemStack assemble(const C& inventory) const = 0;

    /**
     * @brief 获取结果物品类型
     * @return 结果物品（数量始终为1）
     *
     * 用于在UI中显示配方结果
     */
    [[nodiscard]] virtual ItemStack getResultItem() const = 0;

    /**
     * @brief 获取配方所需的所有原料
     * @return 原料列表
     *
     * 用于配方书显示和解锁检测
     */
    [[nodiscard]] virtual const std::vector<Ingredient>& getIngredients() const = 0;

    /**
     * @brief 获取配方的分组名称
     * @return 分组名，如果无分组返回空字符串
     *
     * 分组用于在配方书中合并相似的配方
     */
    [[nodiscard]] virtual const std::string& getGroup() const
    {
        static const std::string empty;
        return empty;
    }

    /**
     * @brief 获取配方的宽度（仅对有序配方有意义）
     * @return 宽度，默认为0
     */
    [[nodiscard]] virtual i32 getRecipeWidth() const { return 0; }

    /**
     * @brief 获取配方的高度（仅对有序配方有意义）
     * @return 高度，默认为0
     */
    [[nodiscard]] virtual i32 getRecipeHeight() const { return 0; }

    /**
     * @brief 获取配方的资源位置ID
     * @return 配方ID（如 "minecraft:crafting_table"）
     */
    [[nodiscard]] virtual ResourceLocation getId() const = 0;

    /**
     * @brief 获取配方的类型
     * @return 配方类型
     */
    [[nodiscard]] virtual RecipeType getType() const = 0;

    /**
     * @brief 检查配方是否需要特殊条件才能使用
     * @return 如果配方可以在普通工作台使用返回true
     */
    [[nodiscard]] virtual bool isSpecial() const { return false; }

    /**
     * @brief 获取配方消耗的物品堆（用于减少原料数量）
     * @param slot 槽位索引
     * @return 该槽位应该减少的数量，默认为1
     */
    [[nodiscard]] virtual i32 getIngredientCount(i32 slot) const
    {
        (void)slot;
        return 1;
    }

    /**
     * @brief 检查配方是否可以在给定尺寸的网格中制作
     * @param width 网格宽度
     * @param height 网格高度
     * @return 如果配方适合返回true
     */
    [[nodiscard]] virtual bool canFitIn(i32 width, i32 height) const
    {
        (void)width;
        (void)height;
        return true;
    }

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 合成容器
     * @return 每个槽位的剩余物品堆列表
     *
     * 用于处理合成后的剩余物品，如：
     * - 水桶 -> 空桶（牛奶桶、水桶等）
     * - 玻璃瓶 -> 玻璃瓶（药水合成）
     * - 碗 -> 碗（蘑菇汤）
     */
    [[nodiscard]] virtual std::vector<ItemStack> getRemainingItems(const C& inventory) const = 0;

    /**
     * @brief 检查配方是否为动态配方（每次合成结果可能不同）
     * @return 如果是动态配方返回true
     *
     * 动态配方的结果物品在 assemble() 时动态生成，
     * 如染色配方、修复配方等。
     * 动态配方在配方书中显示为"特殊"配方。
     */
    [[nodiscard]] virtual bool isDynamic() const { return false; }
};

/**
 * @brief 配方工具函数命名空间
 */
namespace RecipeUtils {

/**
 * @brief 获取容器中每个槽位的默认剩余物品
 * @tparam C 容器类型
 * @param inventory 容器
 * @return 每个槽位的剩余物品堆列表
 *
 * 默认实现检查每个槽位的物品是否有容器物品（hasContainerItem()），
 * 如果有则返回容器物品，否则返回空堆。
 *
 * 使用示例：
 * @code
 * std::vector<ItemStack> remaining = RecipeUtils::getDefaultRemainingItems(inventory);
 * @endcode
 */
template <typename C>
std::vector<ItemStack> getDefaultRemainingItems(const C& inventory)
{
    std::vector<ItemStack> remaining(inventory.getContainerSize());
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.hasContainerItem()) {
            remaining[static_cast<size_t>(i)] = stack.getContainerItem();
        }
        // 否则 remaining[i] 保持为空堆
    }
    return remaining;
}

} // namespace RecipeUtils

/**
 * @brief 配方序列化接口
 *
 * 定义配方的序列化/反序列化方法
 * @tparam C 容器类型（如CraftingInventory）
 */
template <typename C>
class IRecipeSerializer {
public:
    virtual ~IRecipeSerializer() = default;

    /**
     * @brief 从JSON解析配方
     * @param id 配方ID
     * @param json JSON数据
     * @return 解析的配方，或错误
     */
    virtual Result<std::unique_ptr<IRecipe<C>>> fromJson(const ResourceLocation& id, const nlohmann::json& json) = 0;

    /**
     * @brief 获取序列化器ID
     * @return 序列化器的资源位置
     */
    virtual ResourceLocation getId() const = 0;
};

} // namespace crafting
} // namespace mc
