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

#include "LayoutEngine.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/algorithms/AnchorLayout.hpp"
#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/algorithms/GridLayout.hpp"
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/core/LayoutResult.hpp"
#include "client/ui/kagero/layout/core/MeasureSpec.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace mc::client::ui::kagero::layout::detail {

/**
 * @brief Grid 布局算法适配器
 */
class GridLayoutAlgorithm final : public ILayoutAlgorithm {
public:
    /**
     * @brief 构造网格适配器
     */
    explicit GridLayoutAlgorithm(GridConfig config = {})
        : m_config(config)
    {}

    /**
     * @brief 更新网格配置
     */
    void setConfig(const GridConfig& config) { m_config = config; }

    /**
     * @brief 计算网格布局
     *
     * @warning 这里会忽略容器的额外约束，由具体网格配置控制排布。
     */
    [[nodiscard]] std::vector<LayoutResult> compute(const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints) override
    {
        (void)containerConstraints;
        GridLayout layout;
        layout.setConfig(m_config);
        return layout.compute(containerBounds, children);
    }

    /**
     * @brief 估算网格布局所需尺寸
     *
     * @note 这里基于子项当前尺寸做保守估算，便于布局引擎预留空间。
     */
    [[nodiscard]] Size measure(const MeasureSpec& widthSpec,
        const MeasureSpec& heightSpec,
        const std::vector<WidgetLayoutAdaptor*>& children) override
    {
        if (children.empty()) {
            return Size(widthSpec.adjust(0), heightSpec.adjust(0));
        }

        const i32 columns = std::max(1, m_config.columns);
        const i32 rows = std::max(
            1, m_config.rows > 0 ? m_config.rows : static_cast<i32>((children.size() + columns - 1) / columns));

        i32 maxChildWidth = 0;
        i32 maxChildHeight = 0;
        for (const auto* child : children) {
            if (!child || !child->isValid()) {
                continue;
            }

            const Size size = child->currentSize();
            maxChildWidth = std::max(maxChildWidth, size.width);
            maxChildHeight = std::max(maxChildHeight, size.height);
        }

        const i32 measuredWidth = columns * std::max(1, maxChildWidth) + std::max(0, columns - 1) * m_config.columnGap;
        const i32 measuredHeight = rows * std::max(1, maxChildHeight) + std::max(0, rows - 1) * m_config.rowGap;
        return Size(widthSpec.adjust(measuredWidth), heightSpec.adjust(measuredHeight));
    }

    /**
     * @brief 获取算法名称
     */
    [[nodiscard]] std::string name() const override { return "grid"; }

private:
    GridConfig m_config;
};

/**
 * @brief Anchor 布局算法适配器
 */
class AnchorLayoutAlgorithm final : public ILayoutAlgorithm {
public:
    /**
     * @brief 计算锚点布局
     *
     * @warning 这里直接复用 AnchorLayout 的现有实现。
     */
    [[nodiscard]] std::vector<LayoutResult> compute(const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints) override
    {
        (void)containerConstraints;
        AnchorLayout layout;
        return layout.compute(containerBounds, children);
    }

    /**
     * @brief 估算锚点布局所需尺寸
     */
    [[nodiscard]] Size measure(const MeasureSpec& widthSpec,
        const MeasureSpec& heightSpec,
        const std::vector<WidgetLayoutAdaptor*>& children) override
    {
        i32 measuredWidth = 0;
        i32 measuredHeight = 0;

        for (const auto* child : children) {
            if (!child || !child->isValid()) {
                continue;
            }

            const Size size = child->currentSize();
            measuredWidth = std::max(measuredWidth, size.width);
            measuredHeight = std::max(measuredHeight, size.height);
        }

        return Size(widthSpec.adjust(measuredWidth), heightSpec.adjust(measuredHeight));
    }

    /**
     * @brief 获取算法名称
     */
    [[nodiscard]] std::string name() const override { return "anchor"; }
};

/**
 * @brief Stack 布局算法适配器
 *
 * Stack 相当于垂直方向的 flex 容器。
 */
class StackLayoutAlgorithm final : public FlexLayoutAlgorithm {
public:
    /**
     * @brief 构造堆叠布局适配器
     */
    StackLayoutAlgorithm()
        : FlexLayoutAlgorithm([]() {
            FlexConfig config;
            config.direction = Direction::Column;
            return config;
        }())
    {}

    /**
     * @brief 计算堆叠布局
     *
     * Stack 作为垂直容器时，需要把子元素拉伸到容器交叉轴宽度，
     * 这样才能符合"堆叠面板占满宽度"的常见 UI 语义。
     */
    [[nodiscard]] std::vector<LayoutResult> compute(const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints) override
    {
        auto results = FlexLayoutAlgorithm::compute(containerBounds, children, containerConstraints);

        const i32 availableCrossSize = std::max(0, containerBounds.width - containerConstraints.padding.horizontal());
        for (size_t i = 0; i < results.size() && i < children.size(); ++i) {
            auto* child = children[i];
            if (child == nullptr || !results[i].isValid()) {
                continue;
            }

            const i32 stretchedWidth = std::max(0, availableCrossSize - child->constraints().margin.horizontal());
            results[i].bounds.width = child->constraints().clampWidth(stretchedWidth);
        }

        return results;
    }

    /**
     * @brief 获取算法名称
     */
    [[nodiscard]] std::string name() const override { return "stack"; }
};

} // namespace mc::client::ui::kagero::layout::detail
