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
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/interactive/DecoratedPotPattern.hpp"
#include <array>
#include <memory>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class Player;
class Item;

namespace blockentity {

/**
 * @brief 饰纹陶罐四面图案数据
 *
 * 存储饰纹陶罐四个面（后、左、右、前）的图案信息。
 * 每个面可以是 Blank（砖块面，默认）或某种陶片图案。
 *
 * 图案顺序：[back, left, right, front]
 * - back:  陶罐背面（与 FACING 方向相反）
 * - left:  陶罐左面
 * - right: 陶罐右面
 * - front: 陶罐正面（与 FACING 方向一致）
 */
class PotDecorations {
public:
    /// 空图案（全部为砖块面）
    static const PotDecorations EMPTY;

    /**
     * @brief 默认构造函数（全部为空白/砖块面）
     */
    PotDecorations();

    /**
     * @brief 指定四面图案的构造函数
     * @param back  背面图案
     * @param left  左面图案
     * @param right 右面图案
     * @param front 正面图案
     */
    PotDecorations(
        DecoratedPotPattern back, DecoratedPotPattern left, DecoratedPotPattern right, DecoratedPotPattern front);

    /**
     * @brief 从图案列表构造
     * @param patterns 图案列表，顺序为 [back, left, right, front]
     *         不足4个时用 Blank 填充，超过4个时截断
     */
    explicit PotDecorations(const std::vector<DecoratedPotPattern>& patterns);

    // ========== 访问器 ==========

    [[nodiscard]] DecoratedPotPattern back() const noexcept { return m_patterns[0]; }
    [[nodiscard]] DecoratedPotPattern left() const noexcept { return m_patterns[1]; }
    [[nodiscard]] DecoratedPotPattern right() const noexcept { return m_patterns[2]; }
    [[nodiscard]] DecoratedPotPattern front() const noexcept { return m_patterns[3]; }

    /**
     * @brief 获取有序图案列表
     * @return [back, left, right, front] 的图案列表
     */
    [[nodiscard]] const std::array<DecoratedPotPattern, 4>& ordered() const noexcept { return m_patterns; }

    /**
     * @brief 检查是否为空图案（全部为砖块面）
     */
    [[nodiscard]] bool isEmpty() const noexcept;

    /**
     * @brief 比较运算符
     */
    bool operator==(const PotDecorations& other) const noexcept;
    bool operator!=(const PotDecorations& other) const noexcept { return !(*this == other); }

    // ========== 序列化 ==========

    /**
     * @brief 从 JSON 加载图案数据
     * @param sherdsArray 包含4个陶片物品ID的 JSON 数组
     * @return 加载的 PotDecorations
     */
    static PotDecorations fromJson(const nlohmann::json& sherdsArray);

    /**
     * @brief 保存图案数据到 JSON
     * @return 包含4个陶片物品ID的 JSON 数组
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 从 NBT 加载图案数据
     * @param sherdsTag 包含4个陶片物品ID字符串的 NBT 列表标签
     * @return 加载的 PotDecorations
     */
    static PotDecorations fromNBT(const nbt::tags::list_tag& sherdsTag);

    /**
     * @brief 保存图案数据到 NBT
     * @return 包含4个陶片物品ID字符串的 NBT 列表标签
     */
    [[nodiscard]] nbt::tags::string_list_tag toNBT() const;

private:
    std::array<DecoratedPotPattern, 4> m_patterns; ///< [back, left, right, front]
};

/**
 * @brief 饰纹陶罐方块实体
 *
 * 饰纹陶罐可以存储一个物品，并记录四面的陶片图案。
 *
 * 特点：
 * - 1格物品容器（类似唱片机）
 * - 四面图案（PotDecorations）
 * - 支持红石比较器信号输出
 * - 摇晃动画（放入物品时正摇，空手交互时负摇）
 * - 可被战利品表填充（自然生成的陶罐）
 */
class DecoratedPotBlockEntity : public ContainerBlockEntity {
public:
    // ========== 摇晃动画样式 ==========

    /**
     * @brief 摇晃动画样式
     */
    enum class WobbleStyle : u8 {
        Positive = 0, ///< 正摇（放入物品时），持续7tick
        Negative = 1, ///< 负摇（空手交互时），持续10tick
    };

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit DecoratedPotBlockEntity(const BlockPos& pos);

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return 1; }

    // ========== 图案管理 ==========

    /**
     * @brief 获取四面图案
     */
    [[nodiscard]] const PotDecorations& getDecorations() const noexcept { return m_decorations; }

    /**
     * @brief 设置四面图案
     * @param decorations 图案数据
     */
    void setDecorations(const PotDecorations& decorations);

    // ========== 物品存取 ==========

    /**
     * @brief 获取陶罐中的物品
     */
    [[nodiscard]] ItemStack getItem() const;

    /**
     * @brief 设置陶罐中的物品
     * @param stack 物品堆
     */
    void setItem(const ItemStack& stack);

    /**
     * @brief 检查陶罐是否有物品
     */
    [[nodiscard]] bool hasItem() const;

    // ========== 摇晃动画 ==========

    /**
     * @brief 触发摇晃动画
     * @param style 摇晃样式
     */
    void wobble(WobbleStyle style);

    /**
     * @brief 获取摇晃动画开始时间
     * @return 游戏tick，0表示无动画
     */
    [[nodiscard]] i64 wobbleStartedAtTick() const noexcept { return m_wobbleStartedAtTick; }

    /**
     * @brief 获取最近一次摇晃样式
     */
    [[nodiscard]] const WobbleStyle& lastWobbleStyle() const noexcept { return m_lastWobbleStyle; }

    /**
     * @brief 检查是否正在摇晃
     * @param currentTick 当前游戏tick
     */
    [[nodiscard]] bool isWobbling(i64 currentTick) const;

    // ========== 红石比较器 ==========

    /**
     * @brief 获取红石比较器信号强度
     * @return 信号强度（0-15），根据罐内物品数量计算
     */
    [[nodiscard]] i32 getComparatorSignal() const;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::tags::compound_tag& tag) override;
    void saveToNBT(nbt::tags::compound_tag& tag) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /**
     * @brief 获取朝向
     *
     * 从方块状态获取 HORIZONTAL_FACING 属性。
     * @return 朝向方向，如果无法获取则返回 North
     */
    [[nodiscard]] Direction getDirection() const;

    // ========== 方块事件 ==========

    /**
     * @brief 处理客户端方块事件（覆盖 BlockEntity::triggerEvent）
     *
     * @param id 事件ID（1=摇晃动画）
     * @param type 事件类型（0=Positive, 1=Negative）
     * @return 是否处理成功
     */
    [[nodiscard]] bool triggerEvent(i32 id, i32 type) override;

private:
    SimpleInventory m_inventory;                           ///< 1格物品存储
    PotDecorations m_decorations;                          ///< 四面图案
    i64 m_wobbleStartedAtTick = 0;                         ///< 摇晃动画开始tick
    WobbleStyle m_lastWobbleStyle = WobbleStyle::Positive; ///< 最近摇晃样式
};

/**
 * @brief 从 Item 指针获取对应的 DecoratedPotPattern
 *
 * 陶片物品映射到对应图案，砖块映射到 Blank，未知物品映射到 Blank。
 * 在 DecoratedPotRecipe 合成配方中，将输入的陶片/砖块物品转换为对应的图案。
 *
 * @param item 物品指针
 * @return 对应的图案，如果物品不是陶片或砖块则返回 Blank
 */
[[nodiscard]] DecoratedPotPattern getPatternFromItem(const Item* item);

/**
 * @brief 从 DecoratedPotPattern 获取对应的 Item 指针
 *
 * Blank 图案返回砖块物品，其他图案返回对应的陶片物品。
 * 在合成结果展示中，将图案反向映射为对应的陶片物品用于配方展示。
 *
 * @param pattern 图案类型
 * @return 对应的物品指针，如果找不到则返回 nullptr
 */
[[nodiscard]] const Item* getItemFromPattern(DecoratedPotPattern pattern);

/**
 * @brief 创建带有图案数据的饰纹陶罐物品
 *
 * 创建一个 minecraft:decorated_pot 物品，在 NBT 中存储四面图案信息。
 *
 * @param decorations 图案数据
 * @return 创建的物品堆
 */
[[nodiscard]] ItemStack createDecoratedPotItem(const PotDecorations& decorations);

} // namespace blockentity
} // namespace mc
