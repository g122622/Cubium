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
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace mc {

class Item;
class ItemStack;

/**
 * @brief 创造模式物品组（标签页）
 *
 * 定义创造模式物品栏的分类。
 *
 * 用法示例:
 * @code
 * // 在物品注册时设置物品组
 * auto& sword = ItemRegistry::instance().registerItem<SwordItem>(
 *     ResourceLocation("minecraft:diamond_sword"),
 *     ItemProperties().maxDamage(1561).group(&ItemGroups::COMBAT)
 * );
 * @endcode
 */
class ItemGroup {
public:
    /**
     * @brief 物品组类型枚举
     *
     * 定义了所有原版物品组的顺序。
     */
    enum class Type : i32 {
        BuildingBlocks = 0, ///< 建筑方块
        Decorations = 1,    ///< 装饰方块
        Redstone = 2,       ///< 红石
        Transportation = 3, ///< 交通工具
        Misc = 4,           ///< 杂项
        Food = 5,           ///< 食物
        Tools = 6,          ///< 工具
        Combat = 7,         ///< 战斗
        Brewing = 8,        ///< 酿造
        Hotbar = 9,         ///< 快捷栏（保存的热门物品）
        Inventory = 10,     ///< 背包（生存模式物品栏）
        Search = 11         ///< 搜索
    };

    /**
     * @brief 构造物品组
     * @param type 物品组类型
     * @param id 物品组ID（如"building_blocks"）
     */
    ItemGroup(Type type, std::string id);

    /**
     * @brief 获取物品组类型
     */
    [[nodiscard]] Type getType() const { return m_type; }

    /**
     * @brief 获取物品组ID
     */
    [[nodiscard]] const std::string& getId() const { return m_id; }

    /**
     * @brief 获取物品组图标
     *
     * 返回显示在物品栏标签上的图标物品。
     * 默认返回第一个填充的物品。
     *
     * @return 图标物品堆
     */
    [[nodiscard]] ItemStack getIconItem() const;

    /**
     * @brief 填充物品列表
     *
     * 将所有属于此物品组的物品添加到列表中。
     *
     * @param items 物品列表
     */
    void fill(std::vector<ItemStack>& items) const;

    /**
     * @brief 设置填充函数
     * @param fillFunc 填充函数
     */
    void setFillFunction(std::function<void(std::vector<ItemStack>&)> fillFunc) { m_fillFunc = std::move(fillFunc); }

    /**
     * @brief 设置图标物品
     * @param item 图标物品
     */
    void setIconItem(const Item* item);

    /**
     * @brief 是否在第一列
     */
    [[nodiscard]] bool isFirstColumn() const { return static_cast<i32>(m_type) <= 5; }

    /**
     * @brief 获取标签页索引
     */
    [[nodiscard]] i32 getTabIndex() const { return static_cast<i32>(m_type); }

private:
    Type m_type;
    std::string m_id;
    const Item* m_iconItem = nullptr;
    std::function<void(std::vector<ItemStack>&)> m_fillFunc;
};

} // namespace mc
