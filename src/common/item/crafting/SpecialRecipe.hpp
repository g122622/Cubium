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
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/crafting/RecipeManager.hpp"
#include <string>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 特殊配方基类
 *
 * 特殊配方是动态配方，结果物品在 assemble() 时根据输入动态生成。
 * 这些配方不会出现在配方书中，也不需要从 JSON 数据包加载。
 *
 * 特殊配方包括：
 * - RepairItemRecipe - 物品修复
 * - ArmorDyeRecipe - 盔甲染色
 * - BookCloningRecipe - 书复制
 * - MapCloningRecipe - 地图复制
 * - BannerDuplicateRecipe - 旗帜复制
 * - 等等
 *
 * 参考: net.minecraft.item.crafting.SpecialRecipe
 */
class SpecialRecipe : public CraftingRecipe {
public:
    /**
     * @brief 构造函数
     * @param id 配方ID
     */
    explicit SpecialRecipe(const ResourceLocation& id)
        : m_id(id)
    {}

    ~SpecialRecipe() override = default;

    // ========== IRecipe 接口实现 ==========

    /**
     * @brief 获取配方的序列化器 ID
     * @return 数据包 JSON 里 `type` 字段的取值（如 `minecraft:crafting_special_mapextending`）
     *
     * 与 {@link getType} 不同：`getType()` 回答「属于哪一类配方」（特殊配方统一是
     * `RecipeType::Special`），而配方表本身由序列化器决定——原版的配方注册表就挂在
     * `RecipeSerializer` 上，不同特殊配方的 `type` 互不相同，配方书、标签、
     * 网络同步都要用到它。装载器（`RecipeSerializers::parseSpecialRecipe`）会写入
     * 数据包里的 `type`；未经装载器直接构造的实例保留该 `type` 的缺省值。
     */
    [[nodiscard]] const std::string& getSerializerId() const
    {
        return m_serializerId;
    }

    /**
     * @brief 设置序列化器 ID（仅供配方装载器使用）
     */
    void setSerializerId(std::string serializerId)
    {
        m_serializerId = std::move(serializerId);
    }

    /**
     * @brief 获取配方ID
     */
    [[nodiscard]] ResourceLocation getId() const override
    {
        return m_id;
    }

    /**
     * @brief 获取配方类型
     * @return RecipeType::Special
     */
    [[nodiscard]] RecipeType getType() const override
    {
        return RecipeType::Special;
    }

    /**
     * @brief 检查配方是否为特殊配方
     * @return true
     */
    [[nodiscard]] bool isSpecial() const override
    {
        return true;
    }

    /**
     * @brief 检查配方是否为动态配方
     * @return true（特殊配方总是动态的）
     *
     * 动态配方不会出现在配方书中。
     */
    [[nodiscard]] bool isDynamic() const override
    {
        return true;
    }

    /**
     * @brief 获取结果物品
     * @return ItemStack::EMPTY（动态配方的结果在 assemble() 中生成）
     */
    [[nodiscard]] ItemStack getResultItem() const override
    {
        return ItemStack::EMPTY;
    }

    /**
     * @brief 获取原料列表
     * @return 空列表（特殊配方没有固定的原料列表）
     */
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override
    {
        static const std::vector<Ingredient> empty;
        return empty;
    }

    /**
     * @brief 获取配方分组
     * @return 空字符串（特殊配方不使用分组）
     */
    [[nodiscard]] const std::string& getGroup() const override
    {
        static const std::string empty;
        return empty;
    }

    /**
     * @brief 检查配方是否可以在给定尺寸的网格中制作
     * @param width 网格宽度
     * @param height 网格高度
     * @return 默认返回 true（特殊配方通常可以在任何尺寸的网格中制作）
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override
    {
        (void)width;
        (void)height;
        return true;
    }

protected:
    ResourceLocation m_id;
    std::string m_serializerId = "minecraft:crafting_special_unknown";
};

} // namespace crafting
} // namespace mc
