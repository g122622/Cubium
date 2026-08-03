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

#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <limits>
#include <optional>

namespace mc::client::ui::kagero::layout {

/**
 * @brief 对齐方式
 *
 * 用于 flex 布局中子元素在交叉轴上的对齐。
 */
enum class Align : u8 {
    Start,   ///< 起始对齐
    Center,  ///< 居中对齐
    End,     ///< 结束对齐
    Stretch, ///< 拉伸填充
    Baseline ///< 基线对齐（仅用于文本）
};

/**
 * @brief 主轴方向
 *
 * 用于 flex 布局的主轴方向。
 */
enum class Direction : u8 {
    Row,          ///< 水平方向，从左到右
    RowReverse,   ///< 水平方向，从右到左
    Column,       ///< 垂直方向，从上到下
    ColumnReverse ///< 垂直方向，从下到上
};

/**
 * @brief 主轴对齐方式
 *
 * 用于 flex 布局中子元素在主轴上的对齐。
 */
enum class JustifyContent : u8 {
    Start,        ///< 起始对齐
    Center,       ///< 居中对齐
    End,          ///< 结束对齐
    SpaceBetween, ///< 两端对齐，元素之间间距相等
    SpaceAround,  ///< 元素两侧间距相等
    SpaceEvenly   ///< 所有间距相等
};

/**
 * @brief 换行方式
 *
 * 用于 flex 布局的换行行为。
 */
enum class Wrap : u8 {
    NoWrap,     ///< 不换行
    Wrap,       ///< 换行
    WrapReverse ///< 反向换行
};

/**
 * @brief 弹性参数
 *
 * 用于 flex 布局中子元素的弹性尺寸调整。
 *
 * 使用示例：
 * @code
 * FlexItem item;
 * item.grow = 1.0f;     // 占据剩余空间
 * item.shrink = 0.0f;   // 不缩小
 * item.basis = 100;     // 初始尺寸 100px
 * item.alignSelf = Align::Center;  // 居中对齐
 * @endcode
 */
struct FlexItem {
    f32 grow = 0.0f;                                 ///< 增长因子（分配剩余空间的比例）
    f32 shrink = 1.0f;                               ///< 缩小因子（空间不足时缩小的比例）
    i32 basis = -1;                                  ///< 基准尺寸（-1 表示使用内容尺寸）
    Align alignSelf = Align::Stretch;                ///< 单独的对齐方式（覆盖容器的 align-items）
    i32 minWidth = 0;                                ///< 最小宽度
    i32 maxWidth = std::numeric_limits<i32>::max();  ///< 最大宽度
    i32 minHeight = 0;                               ///< 最小高度
    i32 maxHeight = std::numeric_limits<i32>::max(); ///< 最大高度

    /**
     * @brief 检查是否有增长能力
     */
    [[nodiscard]] bool canGrow() const { return grow > 0.0f; }

    /**
     * @brief 检查是否有缩小能力
     */
    [[nodiscard]] bool canShrink() const { return shrink > 0.0f; }

    /**
     * @brief 检查是否有最小/最大宽度限制
     */
    [[nodiscard]] bool hasWidthConstraints() const
    {
        return minWidth > 0 || maxWidth < std::numeric_limits<i32>::max();
    }

    /**
     * @brief 检查是否有最小/最大高度限制
     */
    [[nodiscard]] bool hasHeightConstraints() const
    {
        return minHeight > 0 || maxHeight < std::numeric_limits<i32>::max();
    }

    /**
     * @brief 将宽度限制在约束范围内
     */
    [[nodiscard]] i32 clampWidth(i32 width) const { return std::max(minWidth, std::min(maxWidth, width)); }

    /**
     * @brief 将高度限制在约束范围内
     */
    [[nodiscard]] i32 clampHeight(i32 height) const { return std::max(minHeight, std::min(maxHeight, height)); }
};

/**
 * @brief 网格项参数
 *
 * 用于 grid 布局中子元素的位置和跨越。
 * 默认 column = -1 和 row = -1 表示需要自动放置。
 */
struct GridItem {
    i32 column = -1;                  ///< 起始列（0-based，-1 表示自动放置）
    i32 row = -1;                     ///< 起始行（0-based，-1 表示自动放置）
    i32 columnSpan = 1;               ///< 跨越列数
    i32 rowSpan = 1;                  ///< 跨越行数
    Align alignSelf = Align::Stretch; ///< 垂直对齐

    /**
     * @brief 检查是否跨越单个单元格
     */
    [[nodiscard]] bool isSingleCell() const { return columnSpan == 1 && rowSpan == 1; }

    /**
     * @brief 获取结束列（不含）
     */
    [[nodiscard]] i32 columnEnd() const { return column + columnSpan; }

    /**
     * @brief 获取结束行（不含）
     */
    [[nodiscard]] i32 rowEnd() const { return row + rowSpan; }
};

/**
 * @brief 锚点约束
 *
 * 用于 anchor 布局中子元素相对于容器边缘的定位。
 */
struct AnchorConstraints {
    // 使用 std::optional 表示"不约束"
    // 当值有定义时，表示距离该边缘的距离

    std::optional<i32> left;   ///< 距离左边缘的距离
    std::optional<i32> top;    ///< 距离上边缘的距离
    std::optional<i32> right;  ///< 距离右边缘的距离
    std::optional<i32> bottom; ///< 距离下边缘的距离

    // 用于百分比定位
    std::optional<f32> leftPercent; ///< 左边百分比定位
    std::optional<f32> topPercent;  ///< 上边百分比定位

    // 偏移量
    i32 offsetX = 0; ///< X轴偏移
    i32 offsetY = 0; ///< Y轴偏移

    // 居中定位
    bool centerX = false; ///< 水平居中
    bool centerY = false; ///< 垂直居中

    /**
     * @brief 检查是否是纯左上角定位
     */
    [[nodiscard]] bool isTopLeft() const
    {
        return left.has_value() && top.has_value() && !right.has_value() && !bottom.has_value();
    }

    /**
     * @brief 检查是否是拉伸定位（同时约束两个相对边）
     */
    [[nodiscard]] bool isStretchHorizontal() const { return left.has_value() && right.has_value(); }

    [[nodiscard]] bool isStretchVertical() const { return top.has_value() && bottom.has_value(); }

    /**
     * @brief 检查是否使用百分比定位
     */
    [[nodiscard]] bool hasPercentPosition() const { return leftPercent.has_value() || topPercent.has_value(); }
};

/**
 * @brief 布局约束
 *
 * 子元素向父容器声明的布局需求和约束。
 * 包含尺寸偏好、边距、弹性参数等。
 *
 * 使用示例：
 * @code
 * LayoutConstraints constraints;
 * constraints.preferredWidth = 200;
 * constraints.preferredHeight = 100;
 * constraints.margin = Margin(10, 5);  // 上下10px，左右5px
 * constraints.flex.grow = 1.0f;        // 可增长
 * constraints.minWidth = 100;
 * constraints.maxWidth = 400;
 *
 * // 根据父容器规格计算实际尺寸
 * MeasureSpec widthSpec = constraints.resolveWidth(parentWidthSpec);
 * @endcode
 */
struct LayoutConstraints {
    // ==================== 尺寸约束 ====================

    i32 minWidth = 0;                                ///< 最小宽度
    i32 minHeight = 0;                               ///< 最小高度
    i32 preferredWidth = -1;                         ///< 期望宽度（-1 表示无偏好，使用内容尺寸）
    i32 preferredHeight = -1;                        ///< 期望高度
    i32 maxWidth = std::numeric_limits<i32>::max();  ///< 最大宽度
    i32 maxHeight = std::numeric_limits<i32>::max(); ///< 最大高度

    // ==================== 边距与内边距 ====================

    Margin margin;   ///< 外边距
    Padding padding; ///< 内边距

    // ==================== 弹性参数 ====================

    FlexItem flex; ///< Flex布局参数

    // ==================== 网格参数 ====================

    GridItem grid; ///< Grid布局参数

    // ==================== 锚点参数 ====================

    AnchorConstraints anchor; ///< Anchor布局参数

    // ==================== 通用参数 ====================

    Align alignSelf = Align::Stretch; ///< 单独的对齐方式
    bool enabled = true;              ///< 是否参与布局
    f32 aspectRatio = 0.0f;           ///< 宽高比（0 表示不约束）

    // ==================== 工厂方法 ====================

    /**
     * @brief 创建固定尺寸约束
     */
    [[nodiscard]] static LayoutConstraints fixed(i32 width, i32 height)
    {
        LayoutConstraints c;
        c.preferredWidth = width;
        c.preferredHeight = height;
        c.maxWidth = width;
        c.maxHeight = height;
        c.minWidth = width;
        c.minHeight = height;
        return c;
    }

    /**
     * @brief 创建弹性尺寸约束（可增长）
     */
    [[nodiscard]] static LayoutConstraints flexible(i32 minWidth, i32 minHeight)
    {
        LayoutConstraints c;
        c.minWidth = minWidth;
        c.minHeight = minHeight;
        c.flex.grow = 1.0f;
        return c;
    }

    /**
     * @brief 创建内容尺寸约束
     */
    [[nodiscard]] static LayoutConstraints wrapContent()
    {
        return LayoutConstraints{}; // 默认值即内容尺寸
    }

    /**
     * @brief 创建填充父容器的约束
     */
    [[nodiscard]] static LayoutConstraints fillParent()
    {
        LayoutConstraints c;
        c.flex.grow = 1.0f;
        c.alignSelf = Align::Stretch;
        return c;
    }

    // ==================== 解析方法 ====================

    /**
     * @brief 根据父容器规格解析实际可用宽度
     *
     * @param parentSpec 父容器的宽度规格
     * @return 解析后的宽度测量规格
     */
    [[nodiscard]] MeasureSpec resolveWidth(const MeasureSpec& parentSpec) const
    {
        // 首先计算去掉边距后的可用空间
        i32 availableWidth = parentSpec.size - margin.horizontal();
        if (availableWidth < 0) availableWidth = 0;

        switch (parentSpec.mode) {
            case MeasureMode::Exactly:
                // 父容器指定精确尺寸，子元素必须使用该尺寸
                // 但仍需受 min/max 约束限制
                return MeasureSpec::MakeExactly(clampWidth(availableWidth));

            case MeasureMode::AtMost:
                // 父容器指定最大尺寸
                if (preferredWidth >= 0) {
                    i32 resolved = std::max(minWidth, std::min(maxWidth, preferredWidth));
                    return MeasureSpec::MakeAtMost(std::min(resolved, availableWidth));
                }
                return MeasureSpec::MakeAtMost(availableWidth);

            case MeasureMode::Unspecified:
            default:
                // 无限制
                if (preferredWidth >= 0) {
                    i32 resolved = std::max(minWidth, std::min(maxWidth, preferredWidth));
                    return MeasureSpec::MakeAtMost(resolved);
                }
                return MeasureSpec::MakeUnspecified();
        }
    }

    /**
     * @brief 根据父容器规格解析实际可用高度
     */
    [[nodiscard]] MeasureSpec resolveHeight(const MeasureSpec& parentSpec) const
    {
        i32 availableHeight = parentSpec.size - margin.vertical();
        if (availableHeight < 0) availableHeight = 0;

        switch (parentSpec.mode) {
            case MeasureMode::Exactly:
                // 父容器指定精确尺寸，子元素必须使用该尺寸
                // 但仍需受 min/max 约束限制
                return MeasureSpec::MakeExactly(clampHeight(availableHeight));

            case MeasureMode::AtMost:
                if (preferredHeight >= 0) {
                    i32 resolved = std::max(minHeight, std::min(maxHeight, preferredHeight));
                    return MeasureSpec::MakeAtMost(std::min(resolved, availableHeight));
                }
                return MeasureSpec::MakeAtMost(availableHeight);

            case MeasureMode::Unspecified:
            default:
                if (preferredHeight >= 0) {
                    i32 resolved = std::max(minHeight, std::min(maxHeight, preferredHeight));
                    return MeasureSpec::MakeAtMost(resolved);
                }
                return MeasureSpec::MakeUnspecified();
        }
    }

    // ==================== 辅助方法 ====================

    /**
     * @brief 检查是否有固定的期望尺寸
     */
    [[nodiscard]] bool hasPreferredSize() const { return preferredWidth >= 0 && preferredHeight >= 0; }

    /**
     * @brief 检查是否有尺寸下限约束
     */
    [[nodiscard]] bool hasMinConstraints() const { return minWidth > 0 || minHeight > 0; }

    /**
     * @brief 检查是否有尺寸上限约束
     */
    [[nodiscard]] bool hasMaxConstraints() const
    {
        return maxWidth < std::numeric_limits<i32>::max() || maxHeight < std::numeric_limits<i32>::max();
    }

    /**
     * @brief 将宽度限制在约束范围内
     */
    [[nodiscard]] i32 clampWidth(i32 width, i32 available = std::numeric_limits<i32>::max()) const
    {
        i32 result = std::max(minWidth, std::min(maxWidth, width));
        return std::min(result, available);
    }

    /**
     * @brief 将高度限制在约束范围内
     */
    [[nodiscard]] i32 clampHeight(i32 height, i32 available = std::numeric_limits<i32>::max()) const
    {
        i32 result = std::max(minHeight, std::min(maxHeight, height));
        return std::min(result, available);
    }

    /**
     * @brief 获取包含边距的总宽度
     */
    [[nodiscard]] i32 totalWidth(i32 contentWidth) const { return contentWidth + margin.horizontal(); }

    /**
     * @brief 获取包含边距的总高度
     */
    [[nodiscard]] i32 totalHeight(i32 contentHeight) const { return contentHeight + margin.vertical(); }

    /**
     * @brief 检查是否参与布局
     */
    [[nodiscard]] bool isLayoutEnabled() const { return enabled; }
};

} // namespace mc::client::ui::kagero::layout
