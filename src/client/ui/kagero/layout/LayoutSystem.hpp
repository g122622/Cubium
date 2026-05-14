#pragma once

/**
 * @file LayoutSystem.hpp
 * @brief Kagero布局系统统一入口
 *
 * 包含所有布局相关的核心组件：
 * - MeasureSpec: 测量规格
 * - LayoutConstraints: 布局约束
 * - LayoutResult: 布局结果
 * - LayoutEngine: 布局引擎
 * - FlexLayout: 弹性布局算法
 * - WidgetLayoutAdaptor: Widget适配器
 *
 * 使用示例：
 * @code
 * #include "layout/LayoutSystem.hpp"
 *
 * using namespace mc::client::ui::kagero::layout;
 *
 * // 创建Flex布局配置
 * FlexConfig config;
 * config.direction = Direction::Row;
 * config.justifyContent = JustifyContent::Center;
 * config.alignItems = Align::Center;
 * config.gap = 10;
 *
 * // 使用布局引擎
 * auto& engine = LayoutEngine::instance();
 * engine.layoutFlex(containerAdaptor, Rect(0, 0, 800, 600), config);
 *
 * // 或使用FlexLayout直接计算
 * FlexLayout layout;
 * layout.setConfig(config);
 * auto results = layout.compute(containerBounds, children, constraints);
 * @endcode
 */

// 核心组件
#include "core/LayoutEngine.hpp"
#include "core/LayoutResult.hpp"
#include "core/MeasureSpec.hpp"

// 约束系统
#include "constraints/LayoutConstraints.hpp"

// 布局算法
#include "algorithms/FlexLayout.hpp"

// Widget集成
#include "integration/WidgetLayoutAdaptor.hpp"

namespace mc::client::ui::kagero::layout {

/**
 * @brief 初始化布局系统
 *
 * 注册所有内置布局算法。
 * 通常在程序启动时调用一次。
 */
inline void initLayoutSystem()
{
    // 布局引擎在首次访问时自动初始化
    (void)LayoutEngine::instance();
}

} // namespace mc::client::ui::kagero::layout
