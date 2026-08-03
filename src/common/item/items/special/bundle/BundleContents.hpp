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
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// Forward declarations
class Player;
class Slot;

namespace item::items {

/**
 * @brief 收纳袋内容物数据组件
 *
 * 对应 MC 1.21.11 的 net.minecraft.world.item.component.BundleContents。
 * 由于本项目尚未引入 DataComponents 系统，本类作为 BundleItem 的内部辅助类型，
 * 通过 ItemStack NBT（tag.custom_data.BundleContents）序列化/反序列化。
 *
 * 权重系统：
 * - 每个物品堆的权重 = 64 / maxStackSize（整数算术，避免浮点误差）
 * - 例如：石头（maxStackSize=64）权重为 1，剑（maxStackSize=1）权重为 64
 * - 收纳袋总权重上限为 64（对应 MC 的 Fraction.ONE）
 * - 收纳袋嵌套权重：1 + 内袋权重（对应 MC 的 1/16 + inner.weight）
 *   为避免浮点与分数运算，本项目使用整数权重，内袋基础权重固定为 4（= 64/16）
 *
 * 不可变性：
 * - BundleContents 为不可变值类型，修改操作通过 Mutable 完成
 * - EMPTY 为静态空实例
 *
 * 参考: net.minecraft.world.item.component.BundleContents
 */
class BundleContents {
public:
    /// 权重上限（对应 MC 的 Fraction.ONE = 1.0）
    static constexpr i64 MAX_WEIGHT = 64;

    /// 收纳袋嵌套权重（对应 MC 的 Fraction.getFraction(1, 16) = 1/16，整数算术为 64/16 = 4）
    static constexpr i64 BUNDLE_IN_BUNDLE_WEIGHT = 4;

    /// 无选中项常量（对应 MC 的 NO_SELECTED_ITEM_INDEX = -1）
    static constexpr i32 NO_SELECTED_ITEM = -1;

    /// 空内容物单例
    static const BundleContents EMPTY;

    /**
     * @brief 默认构造（创建空内容物）
     */
    BundleContents() = default;

    /**
     * @brief 从物品列表构造（计算权重，selectedItem 设为 -1）
     * @param items 物品列表（按插入顺序，最新在前）
     */
    explicit BundleContents(std::vector<ItemStack> items);

    /**
     * @brief 完整构造函数（用于反序列化）
     * @param items 物品列表
     * @param weight 总权重
     * @param selectedItem 选中项索引
     */
    BundleContents(std::vector<ItemStack> items, i64 weight, i32 selectedItem);

    // ========== 查询 ==========

    /**
     * @brief 内容物数量
     */
    [[nodiscard]] Size size() const noexcept { return static_cast<Size>(m_items.size()); }

    /**
     * @brief 是否为空
     */
    [[nodiscard]] bool isEmpty() const noexcept { return m_items.empty(); }

    /**
     * @brief 总权重
     */
    [[nodiscard]] i64 weight() const noexcept { return m_weight; }

    /**
     * @brief 选中项索引（-1 表示无）
     */
    [[nodiscard]] i32 selectedItem() const noexcept { return m_selectedItem; }

    /**
     * @brief 是否有选中项
     */
    [[nodiscard]] bool hasSelectedItem() const noexcept { return m_selectedItem != NO_SELECTED_ITEM; }

    /**
     * @brief 获取指定索引的物品（不拷贝，仅用于只读访问）
     * @note 索引越界行为未定义
     */
    [[nodiscard]] const ItemStack& getItemUnsafe(i32 index) const { return m_items[index]; }

    /**
     * @brief 获取物品列表（常量引用）
     */
    [[nodiscard]] const std::vector<ItemStack>& items() const noexcept { return m_items; }

    /**
     * @brief 获取物品拷贝列表
     */
    [[nodiscard]] std::vector<ItemStack> itemsCopy() const;

    /**
     * @brief 获取要显示的物品数量
     *
     * MC 1.21.11: 大于 12 项时显示 11 项，否则按 4 列对齐。
     */
    [[nodiscard]] i32 numberOfItemsToShow() const;

    // ========== 静态工具 ==========

    /**
     * @brief 计算物品堆在收纳袋中的权重
     *
     * - 若是收纳袋：BUNDLE_IN_BUNDLE_WEIGHT + 内袋权重
     * - 否则：64 / maxStackSize（向上取整避免 0）
     *
     * @param stack 物品堆
     * @return 权重（整数）
     */
    [[nodiscard]] static i64 getWeight(const ItemStack& stack);

    /**
     * @brief 检查物品是否可以放入收纳袋
     *
     * 物品必须非空，且其 Item::canFitInsideContainerItems() 返回 true。
     */
    [[nodiscard]] static bool canItemBeInBundle(const ItemStack& stack);

    /**
     * @brief 从物品堆读取 BundleContents
     *
     * 从 ItemStack NBT 的 tag.BundleContents 字段反序列化。
     * 不存在时返回 EMPTY。
     *
     * @param stack 物品堆
     * @return 内容物
     */
    [[nodiscard]] static BundleContents fromItemStack(const ItemStack& stack);

    // ========== 序列化 ==========

    /**
     * @brief 序列化为 JSON（用于 ItemStack NBT）
     * @return JSON 对象，结构为 {"items": [...], "weight": int, "selected": int}
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 JSON 反序列化
     */
    [[nodiscard]] static BundleContents fromJson(const nlohmann::json& json);

    // ========== 比较 ==========

    bool operator==(const BundleContents& other) const;
    bool operator!=(const BundleContents& other) const { return !(*this == other); }

    // ========== Mutable 内部类 ==========
    class Mutable;

private:
    std::vector<ItemStack> m_items;
    i64 m_weight = 0;
    i32 m_selectedItem = NO_SELECTED_ITEM;

    /**
     * @brief 计算物品列表的总权重
     */
    [[nodiscard]] static i64 computeContentWeight(const std::vector<ItemStack>& items);
};

/**
 * @brief 收纳袋内容物的可变构建器
 *
 * 对应 MC 1.21.11 的 BundleContents.Mutable。所有修改操作通过此类完成，
 * 完成后调用 toImmutable() 生成不可变 BundleContents。
 *
 * 参考: net.minecraft.world.item.component.BundleContents.Mutable
 */
class BundleContents::Mutable {
public:
    /**
     * @brief 从已有内容物创建 Mutable
     */
    explicit Mutable(BundleContents contents);

    /**
     * @brief 清空所有物品
     */
    Mutable& clearItems();

    /**
     * @brief 尝试插入物品
     *
     * - 若物品不可入袋，返回 0
     * - 否则尽量插入（受权重上限约束），返回实际插入数量
     * - 插入后从源堆 shrink 对应数量
     *
     * @param stack 源物品堆（会被修改）
     * @return 实际插入数量
     */
    i32 tryInsert(ItemStack& stack);

    /**
     * @brief 从槽位尝试转移物品到收纳袋
     *
     * 调用 Slot::safeTake 取出物品后调用 tryInsert。
     *
     * @param slot 源槽位
     * @param player 玩家（用于 safeTake 的回调）
     * @return 实际插入数量
     */
    i32 tryTransfer(Slot& slot, Player& player);

    /**
     * @brief 切换选中项
     *
     * 若当前选中项 != 指定索引且指定索引在有效范围内，则设为指定索引；
     * 否则清除选中项（设为 -1）。
     *
     * @param index 要切换的索引
     */
    void toggleSelectedItem(i32 index);

    /**
     * @brief 取出一个物品堆
     *
     * - 若为空，返回空 optional
     * - 优先取出选中项（若选中项越界则取第 0 个）
     * - 取出后清除选中项
     *
     * @return 取出的物品堆拷贝
     */
    std::optional<ItemStack> removeOne();

    /**
     * @brief 当前权重
     */
    [[nodiscard]] i64 weight() const noexcept { return m_weight; }

    /**
     * @brief 当前选中项
     */
    [[nodiscard]] i32 selectedItem() const noexcept { return m_selectedItem; }

    /**
     * @brief 转换为不可变 BundleContents
     */
    [[nodiscard]] BundleContents toImmutable() const;

private:
    std::vector<ItemStack> m_items;
    i64 m_weight = 0;
    i32 m_selectedItem = NO_SELECTED_ITEM;

    /**
     * @brief 查找可堆叠物品的索引
     * @return 找到的索引，未找到返回 -1
     */
    [[nodiscard]] i32 findStackIndex(const ItemStack& stack) const;

    /**
     * @brief 计算可添加的最大数量
     */
    [[nodiscard]] i32 getMaxAmountToAdd(const ItemStack& stack) const;
};

} // namespace item::items
} // namespace mc
