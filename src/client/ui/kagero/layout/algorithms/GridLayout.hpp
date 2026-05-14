#pragma once

#include "../core/LayoutResult.hpp"
#include "../integration/WidgetLayoutAdaptor.hpp"
#include <vector>

namespace mc::client::ui::kagero::layout {

/**
 * @brief 网格布局配置
 */
struct GridConfig {
    i32 columns = 1;
    i32 rows = 0;
    i32 columnGap = 0;
    i32 rowGap = 0;
    bool autoPlacement = true;
};

/**
 * @brief Grid布局算法
 */
class GridLayout {
public:
    void setColumns(i32 columns);
    [[nodiscard]] i32 columns() const;

    void setRows(i32 rows);
    [[nodiscard]] i32 rows() const;

    void setColumnGap(i32 gap);
    [[nodiscard]] i32 columnGap() const;

    void setRowGap(i32 gap);
    [[nodiscard]] i32 rowGap() const;

    void setConfig(const GridConfig& config);
    [[nodiscard]] const GridConfig& config() const;

    [[nodiscard]] std::vector<LayoutResult> compute(
        const Rect& containerBounds, const std::vector<WidgetLayoutAdaptor*>& children);

private:
    [[nodiscard]] i32 resolveRows(i32 childCount) const;

    GridConfig m_config;
};

} // namespace mc::client::ui::kagero::layout
