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

#include "MeasureSpec.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"
#ifdef KAGERO_LAYOUT_DEBUG
#include <algorithm>
#include <string>
#endif

namespace mc::client::ui::kagero::layout {

/**
 * @brief 布局计算结果
 *
 * 包含Widget在父容器中的最终位置和尺寸，
 * 以及重布局/重渲染标志。
 *
 * 布局流程：
 * 1. LayoutEngine 计算每个 Widget 的 LayoutResult
 * 2. WidgetLayoutAdaptor 将 LayoutResult 应用到 Widget
 * 3. Widget 根据 bounds 更新渲染状态
 */
struct LayoutResult {
    Rect bounds;                ///< 最终位置+尺寸（相对于父容器）
    bool needsRepaint = true;   ///< 是否需要重渲染
    bool needsRelayout = false; ///< 是否需要重新布局（子元素变化时）

#ifdef KAGERO_LAYOUT_DEBUG
    std::string algorithmUsed; ///< 使用的布局算法（调试）
    f64 computeTimeMs = 0.0;   ///< 计算耗时（调试）
    i32 depth = 0;             ///< 布局深度（调试）
#endif

    LayoutResult() = default;

    /**
     * @brief 构造布局结果
     * @param x X坐标
     * @param y Y坐标
     * @param w 宽度
     * @param h 高度
     */
    LayoutResult(i32 x, i32 y, i32 w, i32 h)
        : bounds(x, y, w, h)
    {}

    /**
     * @brief 从Rect构造
     */
    explicit LayoutResult(const Rect& rect)
        : bounds(rect)
    {}

    /**
     * @brief 检查布局结果是否有效
     */
    [[nodiscard]] bool isValid() const { return bounds.width >= 0 && bounds.height >= 0; }

    /**
     * @brief 获取中心X坐标
     */
    [[nodiscard]] i32 centerX() const { return bounds.centerX(); }

    /**
     * @brief 获取中心Y坐标
     */
    [[nodiscard]] i32 centerY() const { return bounds.centerY(); }

    /**
     * @brief 获取右边界
     */
    [[nodiscard]] i32 right() const { return bounds.right(); }

    /**
     * @brief 获取下边界
     */
    [[nodiscard]] i32 bottom() const { return bounds.bottom(); }

    /**
     * @brief 检查点是否在布局区域内
     */
    [[nodiscard]] bool contains(i32 x, i32 y) const { return bounds.contains(x, y); }

    /**
     * @brief 合并两个布局结果（取并集）
     */
    [[nodiscard]] LayoutResult merged(const LayoutResult& other) const
    {
        if (!isValid()) return other;
        if (!other.isValid()) return *this;

        i32 newX = std::min(bounds.x, other.bounds.x);
        i32 newY = std::min(bounds.y, other.bounds.y);
        i32 newRight = std::max(right(), other.right());
        i32 newBottom = std::max(bottom(), other.bottom());

        LayoutResult result(newX, newY, newRight - newX, newBottom - newY);
        result.needsRepaint = needsRepaint || other.needsRepaint;
        return result;
    }
};

/**
 * @brief 布局上下文
 *
 * 在布局计算过程中传递的信息。
 * 包含父容器的约束、可用空间等。
 */
class LayoutContext {
public:
    LayoutContext() = default;

    /**
     * @brief 构造布局上下文
     * @param availableWidth 可用宽度
     * @param availableHeight 可用高度
     */
    LayoutContext(i32 availableWidth, i32 availableHeight)
        : m_availableWidth(availableWidth)
        , m_availableHeight(availableHeight)
    {}

    /**
     * @brief 获取可用宽度
     */
    [[nodiscard]] i32 availableWidth() const { return m_availableWidth; }

    /**
     * @brief 获取可用高度
     */
    [[nodiscard]] i32 availableHeight() const { return m_availableHeight; }

    /**
     * @brief 获取宽度测量规格
     */
    [[nodiscard]] MeasureSpec widthSpec() const { return m_widthSpec; }

    /**
     * @brief 获取高度测量规格
     */
    [[nodiscard]] MeasureSpec heightSpec() const { return m_heightSpec; }

    /**
     * @brief 设置可用宽度
     */
    void setAvailableWidth(i32 width) { m_availableWidth = width; }

    /**
     * @brief 设置可用高度
     */
    void setAvailableHeight(i32 height) { m_availableHeight = height; }

    /**
     * @brief 设置宽度测量规格
     */
    void setWidthSpec(const MeasureSpec& spec) { m_widthSpec = spec; }

    /**
     * @brief 设置高度测量规格
     */
    void setHeightSpec(const MeasureSpec& spec) { m_heightSpec = spec; }

    /**
     * @brief 获取布局深度
     */
    [[nodiscard]] i32 depth() const { return m_depth; }

    /**
     * @brief 设置布局深度
     */
    void setDepth(i32 depth) { m_depth = depth; }

    /**
     * @brief 创建子上下文
     *
     * 用于递归布局子元素时创建新的上下文。
     *
     * @param width 子元素可用宽度
     * @param height 子元素可用高度
     * @return 子布局上下文
     */
    [[nodiscard]] LayoutContext createChildContext(i32 width, i32 height) const
    {
        LayoutContext child(width, height);
        child.setDepth(m_depth + 1);
        return child;
    }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return m_availableWidth >= 0 && m_availableHeight >= 0; }

private:
    i32 m_availableWidth = 0;
    i32 m_availableHeight = 0;
    MeasureSpec m_widthSpec;
    MeasureSpec m_heightSpec;
    i32 m_depth = 0;
};

/**
 * @brief 布局引擎统计信息
 *
 * 用于性能分析和调试。
 */
struct LayoutStats {
    i32 totalWidgets = 0;      ///< 总Widget数量
    i32 relayoutedWidgets = 0; ///< 重新布局的Widget数量
    f64 totalTimeMs = 0.0;     ///< 总布局时间（毫秒）
    i32 maxDepth = 0;          ///< 最大布局深度
    i32 measureCount = 0;      ///< 测量次数
    i32 layoutCount = 0;       ///< 布局次数

    /**
     * @brief 重置统计信息
     */
    void reset()
    {
        totalWidgets = 0;
        relayoutedWidgets = 0;
        totalTimeMs = 0.0;
        maxDepth = 0;
        measureCount = 0;
        layoutCount = 0;
    }
};

} // namespace mc::client::ui::kagero::layout
