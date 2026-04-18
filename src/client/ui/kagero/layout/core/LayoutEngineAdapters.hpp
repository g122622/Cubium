#pragma once

#include "../algorithms/AnchorLayout.hpp"
#include "../algorithms/GridLayout.hpp"
#include "../algorithms/FlexLayout.hpp"
#include "LayoutEngine.hpp"
#include <algorithm>

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
    {
    }

    /**
     * @brief 更新网格配置
     */
    void setConfig(const GridConfig& config)
    {
        m_config = config;
    }

    /**
     * @brief 计算网格布局
     *
     * @warning 这里会忽略容器的额外约束，由具体网格配置控制排布。
     */
    [[nodiscard]] std::vector<LayoutResult> compute(
        const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints
    ) override
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
    [[nodiscard]] Size measure(
        const MeasureSpec& widthSpec,
        const MeasureSpec& heightSpec,
        const std::vector<WidgetLayoutAdaptor*>& children
    ) override
    {
        if (children.empty()) {
            return Size(widthSpec.adjust(0), heightSpec.adjust(0));
        }

        const i32 columns = std::max(1, m_config.columns);
        const i32 rows = std::max(1, m_config.rows > 0
            ? m_config.rows
            : static_cast<i32>((children.size() + columns - 1) / columns));

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

        const i32 measuredWidth = columns * std::max(1, maxChildWidth)
            + std::max(0, columns - 1) * m_config.columnGap;
        const i32 measuredHeight = rows * std::max(1, maxChildHeight)
            + std::max(0, rows - 1) * m_config.rowGap;
        return Size(widthSpec.adjust(measuredWidth), heightSpec.adjust(measuredHeight));
    }

    /**
     * @brief 获取算法名称
     */
    [[nodiscard]] String name() const override
    {
        return "grid";
    }

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
    [[nodiscard]] std::vector<LayoutResult> compute(
        const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints
    ) override
    {
        (void)containerConstraints;
        AnchorLayout layout;
        return layout.compute(containerBounds, children);
    }

    /**
     * @brief 估算锚点布局所需尺寸
     */
    [[nodiscard]] Size measure(
        const MeasureSpec& widthSpec,
        const MeasureSpec& heightSpec,
        const std::vector<WidgetLayoutAdaptor*>& children
    ) override
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
    [[nodiscard]] String name() const override
    {
        return "anchor";
    }
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
    {
    }

    /**
     * @brief 获取算法名称
     */
    [[nodiscard]] String name() const override
    {
        return "stack";
    }
};

} // namespace mc::client::ui::kagero::layout::detail
