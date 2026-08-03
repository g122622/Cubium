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

#include "LayoutResult.hpp"
#include "MeasureSpec.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/algorithms/FlexLayout.hpp"
#include "client/ui/kagero/layout/algorithms/GridLayout.hpp"
#include "client/ui/kagero/layout/constraints/LayoutConstraints.hpp"
#include "client/ui/kagero/layout/integration/WidgetLayoutAdaptor.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mc::client::ui::kagero::layout {

/**
 * @brief 布局算法接口
 *
 * 所有布局算法的基类。支持扩展自定义布局。
 */
class ILayoutAlgorithm {
public:
    virtual ~ILayoutAlgorithm() = default;

    /**
     * @brief 计算布局
     *
     * @param containerBounds 容器边界
     * @param children 子元素列表
     * @param containerConstraints 容器约束
     * @return 子元素的布局结果
     */
    [[nodiscard]] virtual std::vector<LayoutResult> compute(const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints) = 0;

    /**
     * @brief 测量容器尺寸
     *
     * @param widthSpec 宽度测量规格
     * @param heightSpec 高度测量规格
     * @param children 子元素列表
     * @return 容器的测量尺寸
     */
    [[nodiscard]] virtual Size measure(const MeasureSpec& widthSpec,
        const MeasureSpec& heightSpec,
        const std::vector<WidgetLayoutAdaptor*>& children) = 0;

    /**
     * @brief 获取算法名称
     */
    [[nodiscard]] virtual std::string name() const = 0;
};

/**
 * @brief 布局类型枚举
 */
enum class LayoutType : u8 {
    None,   ///< 无布局（绝对定位）
    Flex,   ///< 弹性布局
    Grid,   ///< 网格布局
    Anchor, ///< 锚点布局
    Stack   ///< 堆叠布局
};

/**
 * @brief 布局引擎
 *
 * 中央布局管理器，负责：
 * - 管理布局算法
 * - 执行布局计算
 * - 增量布局优化
 * - 性能统计
 *
 * 使用示例：
 * @code
 * auto& engine = LayoutEngine::instance();
 *
 * // 执行完整布局
 * engine.layout(rootAdaptor, Rect(0, 0, 800, 600));
 *
 * // 执行增量布局
 * engine.layoutDirty(rootAdaptor);
 *
 * // 获取统计信息
 * auto stats = engine.getLastStats();
 * @endcode
 */
class LayoutEngine {
public:
    /**
     * @brief 获取单例实例
     */
    static LayoutEngine& instance();

    // 禁止拷贝和移动
    LayoutEngine(const LayoutEngine&) = delete;
    LayoutEngine& operator=(const LayoutEngine&) = delete;

    // ==================== 算法管理 ====================

    /**
     * @brief 注册布局算法
     *
     * @param name 算法名称
     * @param algorithm 算法实例
     */
    void registerAlgorithm(const std::string& name, std::unique_ptr<ILayoutAlgorithm> algorithm);

    /**
     * @brief 获取布局算法
     *
     * @param name 算法名称
     * @return 算法指针，如果不存在返回nullptr
     */
    [[nodiscard]] ILayoutAlgorithm* getAlgorithm(const std::string& name) const;

    /**
     * @brief 检查算法是否存在
     */
    [[nodiscard]] bool hasAlgorithm(const std::string& name) const;

    // ==================== 布局执行 ====================

    /**
     * @brief 执行完整布局计算
     *
     * 从根节点开始递归计算所有子元素的布局。
     *
     * @param root 根节点适配器
     * @param availableSpace 可用空间
     */
    void layout(WidgetLayoutAdaptor* root, const Rect& availableSpace);

    /**
     * @brief 执行增量布局
     *
     * 只重新布局标记为dirty的子树。
     *
     * @param root 根节点适配器
     */
    void layoutDirty(WidgetLayoutAdaptor* root);

    /**
     * @brief 使用指定算法布局
     *
     * @param algorithmName 算法名称
     * @param container 容器适配器
     * @param availableSpace 可用空间
     */
    void layoutWith(const std::string& algorithmName, WidgetLayoutAdaptor* container, const Rect& availableSpace);

    // ==================== Flex布局便捷方法 ====================

    /**
     * @brief 使用Flex布局
     *
     * @param container 容器适配器
     * @param availableSpace 可用空间
     * @param config Flex配置
     */
    void layoutFlex(WidgetLayoutAdaptor* container, const Rect& availableSpace, const FlexConfig& config);

    // ==================== Grid布局便捷方法 ====================

    /**
     * @brief 使用Grid布局
     *
     * @param container 容器适配器
     * @param availableSpace 可用空间
     * @param config Grid配置
     */
    void layoutGrid(WidgetLayoutAdaptor* container, const Rect& availableSpace, const GridConfig& config);

    // ==================== 统计信息 ====================

    /**
     * @brief 获取上次布局统计
     */
    [[nodiscard]] const LayoutStats& getLastStats() const { return m_stats; }

    /**
     * @brief 重置统计信息
     */
    void resetStats() { m_stats.reset(); }

    // ==================== 调试选项 ====================

    /**
     * @brief 启用/禁用调试可视化
     */
    void setDebugVisualize(bool enabled) { m_debugVisualize = enabled; }
    [[nodiscard]] bool isDebugVisualize() const { return m_debugVisualize; }

    /**
     * @brief 启用/禁用性能追踪
     */
    void setProfiling(bool enabled) { m_profiling = enabled; }
    [[nodiscard]] bool isProfiling() const { return m_profiling; }

private:
    LayoutEngine();
    ~LayoutEngine() = default;

    /**
     * @brief 递归布局单个节点
     */
    LayoutResult _layoutNode(
        WidgetLayoutAdaptor* node, const MeasureSpec& widthSpec, const MeasureSpec& heightSpec, i32 depth);

    /**
     * @brief 收集dirty节点
     */
    void _collectDirtyNodes(WidgetLayoutAdaptor* node, std::vector<WidgetLayoutAdaptor*>& out);

    /**
     * @brief 选择布局算法
     */
    [[nodiscard]] ILayoutAlgorithm* _selectAlgorithm(LayoutType type, const std::string& name);

    std::map<std::string, std::unique_ptr<ILayoutAlgorithm>> m_algorithms;
    LayoutStats m_stats;
    bool m_debugVisualize = false;
    bool m_profiling = false;

    // 内置算法
    std::unique_ptr<FlexLayout> m_flexLayout;
    std::unique_ptr<GridLayout> m_gridLayout;
};

/**
 * @brief Flex布局算法包装器
 *
 * 将FlexLayout包装为ILayoutAlgorithm接口。
 */
class FlexLayoutAlgorithm : public ILayoutAlgorithm {
public:
    explicit FlexLayoutAlgorithm(const FlexConfig& config)
        : m_config(config)
    {}

    void setConfig(const FlexConfig& config) { m_config = config; }
    [[nodiscard]] const FlexConfig& config() const { return m_config; }

    [[nodiscard]] std::vector<LayoutResult> compute(const Rect& containerBounds,
        const std::vector<WidgetLayoutAdaptor*>& children,
        const LayoutConstraints& containerConstraints) override
    {
        FlexLayout layout;
        layout.setConfig(m_config);
        return layout.compute(containerBounds, children, containerConstraints);
    }

    [[nodiscard]] Size measure(const MeasureSpec& widthSpec,
        const MeasureSpec& heightSpec,
        const std::vector<WidgetLayoutAdaptor*>& children) override
    {
        FlexLayout layout;
        layout.setConfig(m_config);
        return layout.measure(widthSpec, heightSpec, children);
    }

    [[nodiscard]] std::string name() const override { return "flex"; }

private:
    FlexConfig m_config;
};

} // namespace mc::client::ui::kagero::layout
