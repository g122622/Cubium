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
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <vector>

namespace mc::client::ui::kagero::layout {

/**
 * @brief Flex布局配置
 *
 * 定义弹性容器的布局参数。
 *
 * 使用示例：
 * @code
 * FlexConfig config;
 * config.direction = Direction::Row;
 * config.justifyContent = JustifyContent::Center;
 * config.alignItems = Align::Center;
 * config.gap = 8;
 * config.wrap = Wrap::Wrap;
 * @endcode
 */
struct FlexConfig {
    Direction direction = Direction::Row;                  ///< 主轴方向
    JustifyContent justifyContent = JustifyContent::Start; ///< 主轴对齐
    Align alignItems = Align::Stretch;                     ///< 交叉轴对齐（默认拉伸）
    Wrap wrap = Wrap::NoWrap;                              ///< 换行方式
    i32 gap = 0;                                           ///< 子元素间距

    /**
     * @brief 检查是否是水平方向
     */
    [[nodiscard]] bool isHorizontal() const
    {
        return direction == Direction::Row || direction == Direction::RowReverse;
    }

    /**
     * @brief 检查是否是垂直方向
     */
    [[nodiscard]] bool isVertical() const
    {
        return direction == Direction::Column || direction == Direction::ColumnReverse;
    }

    /**
     * @brief 检查是否需要换行
     */
    [[nodiscard]] bool shouldWrap() const { return wrap != Wrap::NoWrap; }

    /**
     * @brief 检查是否是反向排列
     */
    [[nodiscard]] bool isReverse() const
    {
        return direction == Direction::RowReverse || direction == Direction::ColumnReverse;
    }
};

/**
 * @brief Flex布局行信息
 *
 * 在换行模式下，存储每一行的布局信息。
 */
struct FlexLine {
    std::vector<size_t> indices;             ///< 子元素在原始数组中的索引
    std::vector<WidgetLayoutAdaptor*> items; ///< 该行的子元素
    i32 mainSize = 0;                        ///< 主轴尺寸
    i32 crossSize = 0;                       ///< 交叉轴尺寸
    i32 offsetX = 0;                         ///< 主轴偏移
    i32 offsetY = 0;                         ///< 交叉轴偏移
    f32 totalGrow = 0.0f;                    ///< 总增长因子
    f32 totalShrink = 0.0f;                  ///< 总缩小因子

    /**
     * @brief 添加子元素到行
     */
    void addItem(size_t idx, WidgetLayoutAdaptor* item, i32 itemMainSize)
    {
        indices.push_back(idx);
        items.push_back(item);
        mainSize += itemMainSize;
        totalGrow += item->flexItem().grow;
        totalShrink += item->flexItem().shrink;
    }

    /**
     * @brief 获取子元素数量
     */
    [[nodiscard]] size_t itemCount() const { return items.size(); }
};

/**
 * @brief Flex布局算法
 *
 * 实现类似CSS Flexbox的布局算法。
 * 支持主轴方向、对齐方式、换行等特性。
 *
 * 算法流程：
 * 1. 收集并测量所有子元素
 * 2. 计算主轴和交叉轴尺寸
 * 3. 分配剩余空间（grow）或缩小空间（shrink）
 * 4. 应用对齐方式
 * 5. 设置子元素位置
 *
 * 使用示例：
 * @code
 * FlexLayout layout;
 * layout.setDirection(Direction::Row);
 * layout.setJustifyContent(JustifyContent::SpaceBetween);
 * layout.setAlignItems(Align::Center);
 * layout.setGap(10);
 *
 * std::vector<LayoutResult> results = layout.compute(containerBounds, children, containerConstraints);
 * @endcode
 */
class FlexLayout {
public:
    FlexLayout() = default;
    ~FlexLayout() = default;

    // ==================== 配置 ====================

    /**
     * @brief 设置布局方向
     */
    void setDirection(Direction direction) { m_config.direction = direction; }
    [[nodiscard]] Direction direction() const { return m_config.direction; }

    /**
     * @brief 设置主轴对齐
     */
    void setJustifyContent(JustifyContent justify) { m_config.justifyContent = justify; }
    [[nodiscard]] JustifyContent justifyContent() const { return m_config.justifyContent; }

    /**
     * @brief 设置交叉轴对齐
     */
    void setAlignItems(Align align) { m_config.alignItems = align; }
    [[nodiscard]] Align alignItems() const { return m_config.alignItems; }

    /**
     * @brief 设置换行方式
     */
    void setWrap(Wrap wrap) { m_config.wrap = wrap; }
    [[nodiscard]] Wrap wrap() const { return m_config.wrap; }

    /**
     * @brief 设置间距
     */
    void setGap(i32 gap) { m_config.gap = gap; }
    [[nodiscard]] i32 gap() const { return m_config.gap; }

    /**
     * @brief 设置完整配置
     */
    void setConfig(const FlexConfig& config) { m_config = config; }
    [[nodiscard]] const FlexConfig& config() const { return m_config; }

    // ==================== 布局计算 ====================

    /**
     * @brief 计算布局
     *
     * 核心布局算法入口。
     *
     * @param containerBounds 容器边界
     * @param children 子元素适配器列表
     * @param containerConstraints 容器约束（可选）
     * @return 每个子元素的布局结果
     */
    [[nodiscard]] std::vector<LayoutResult> compute(const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints = LayoutConstraints{});

    /**
     * @brief 测量容器尺寸
     *
     * 计算容器在给定约束下的自然尺寸。
     *
     * @param widthSpec 宽度测量规格
     * @param heightSpec 高度测量规格
     * @param children 子元素适配器列表
     * @return 容器的测量尺寸
     */
    [[nodiscard]] Size measure(
        const MeasureSpec& widthSpec, const MeasureSpec& heightSpec, const std::vector<WidgetLayoutAdaptor*>& children);

private:
    // ==================== 内部辅助方法 ====================

    /**
     * @brief 测量所有子元素
     */
    void _measureChildren(const std::vector<WidgetLayoutAdaptor*>& children, i32 mainAxisSize, bool isHorizontal);

    /**
     * @brief 收集行（换行模式下）
     */
    void _collectLines(const std::vector<WidgetLayoutAdaptor*>& children, i32 mainAxisSize, bool isHorizontal);

    /**
     * @brief 计算单行布局
     *
     * TODO: 当前布局逻辑已整合到compute()中，此方法暂未使用，待重构时启用
     */
    void _layoutLine(FlexLine& line, i32 mainAxisSize, i32 crossAxisSize, i32 lineOffset, bool isHorizontal);

    /**
     * @brief 应用主轴对齐
     */
    void _applyJustifyContent(FlexLine& line, i32 mainAxisSize, bool isHorizontal);

    /**
     * @brief 应用交叉轴对齐
     */
    void _applyAlignItems(FlexLine& line, i32 crossAxisSize, bool isHorizontal);

    /**
     * @brief 分配剩余空间（grow）
     */
    void _distributeFreeSpace(FlexLine& line, i32 freeSpace, bool isHorizontal);

    /**
     * @brief 缩小空间（shrink）
     */
    void _shrinkSpace(FlexLine& line, i32 overflow, bool isHorizontal);

    /**
     * @brief 设置子元素位置
     *
     * TODO: 当前布局逻辑已整合到compute()中，此方法暂未使用，待重构时启用
     */
    void _positionChildren(const Rect& containerBounds, bool isHorizontal);

    /**
     * @brief 计算子元素的主轴尺寸
     *
     * TODO: 此方法暂未被调用，待后续完善布局算法时启用
     */
    [[nodiscard]] i32 _calculateMainAxisSize(
        WidgetLayoutAdaptor* child, const MeasureSpec& mainSpec, bool isHorizontal);

    /**
     * @brief 计算子元素的交叉轴尺寸
     *
     * TODO: 此方法暂未被调用，待后续完善布局算法时启用
     */
    [[nodiscard]] i32 _calculateCrossAxisSize(
        WidgetLayoutAdaptor* child, const MeasureSpec& crossSpec, bool isHorizontal, i32 baseline = 0);

    // ================= 成员变量 =================

    FlexConfig m_config;
    std::vector<FlexLine> m_lines;
    std::vector<LayoutResult> m_results;
    std::vector<Size> m_measuredSizes;

    // 临时布局状态
    i32 m_totalMainSize = 0;
    i32 m_maxCrossSize = 0;
};

/**
 * @brief 创建默认Flex布局配置
 */
[[nodiscard]] inline FlexConfig defaultFlexConfig()
{
    return FlexConfig{};
}

/**
 * @brief 创建水平居中Flex布局配置
 */
[[nodiscard]] inline FlexConfig centerRowFlexConfig()
{
    FlexConfig config;
    config.direction = Direction::Row;
    config.justifyContent = JustifyContent::Center;
    config.alignItems = Align::Center;
    return config;
}

/**
 * @brief 创建垂直居中Flex布局配置
 */
[[nodiscard]] inline FlexConfig centerColumnFlexConfig()
{
    FlexConfig config;
    config.direction = Direction::Column;
    config.justifyContent = JustifyContent::Center;
    config.alignItems = Align::Center;
    return config;
}

/**
 * @brief 创建两端对齐Flex布局配置
 */
[[nodiscard]] inline FlexConfig spaceBetweenFlexConfig()
{
    FlexConfig config;
    config.direction = Direction::Row;
    config.justifyContent = JustifyContent::SpaceBetween;
    config.alignItems = Align::Center;
    return config;
}

} // namespace mc::client::ui::kagero::layout
