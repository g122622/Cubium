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
#include "client/ui/kagero/widget/IWidgetContainer.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::layout {

/**
 * @brief Widget布局适配器
 *
 * 让任意Widget支持布局系统。提供：
 * - 获取/设置布局约束
 * - 测量子元素尺寸
 * - 应用布局结果
 * - 脏标记管理
 *
 * 使用示例：
 * @code
 * // 创建适配器
 * auto adaptor = std::make_unique<WidgetLayoutAdaptor>(widget);
 *
 * // 设置约束
 * adaptor->constraints().flex.grow = 1.0f;
 * adaptor->constraints().minWidth = 100;
 *
 * // 测量
 * Size measured = adaptor->measure(widthSpec, heightSpec);
 *
 * // 应用布局
 * LayoutResult result(10, 10, 200, 50);
 * adaptor->applyLayout(result);
 * @endcode
 */
class WidgetLayoutAdaptor {
public:
    /**
     * @brief 测量函数类型
     *
     * 用于自定义测量逻辑。当Widget需要特殊测量时设置。
     */
    using MeasureFunc = std::function<Size(WidgetLayoutAdaptor*, const MeasureSpec&, const MeasureSpec&)>;

    /**
     * @brief 构造函数
     * @param widget 关联的Widget
     */
    explicit WidgetLayoutAdaptor(widget::Widget* widget);

    /**
     * @brief 析构函数
     */
    ~WidgetLayoutAdaptor() = default;

    // 禁止拷贝
    WidgetLayoutAdaptor(const WidgetLayoutAdaptor&) = delete;
    WidgetLayoutAdaptor& operator=(const WidgetLayoutAdaptor&) = delete;

    // 允许移动
    WidgetLayoutAdaptor(WidgetLayoutAdaptor&&) noexcept = default;
    WidgetLayoutAdaptor& operator=(WidgetLayoutAdaptor&&) noexcept = default;

    // ==================== Widget访问 ====================

    /**
     * @brief 获取关联的Widget
     */
    [[nodiscard]] widget::Widget* getWidget() const { return m_widget; }

    /**
     * @brief 获取Widget ID
     */
    [[nodiscard]] const std::string& id() const;

    /**
     * @brief 检查Widget是否有效
     */
    [[nodiscard]] bool isValid() const { return m_widget != nullptr; }

    // ==================== 约束访问 ====================

    /**
     * @brief 获取布局约束（可修改）
     */
    LayoutConstraints& constraints() { return m_constraints; }

    /**
     * @brief 获取布局约束（只读）
     */
    [[nodiscard]] const LayoutConstraints& constraints() const { return m_constraints; }

    /**
     * @brief 设置布局约束
     */
    void setConstraints(const LayoutConstraints& constraints) { m_constraints = constraints; }

    /**
     * @brief 获取Flex参数（快捷访问）
     */
    FlexItem& flexItem() { return m_constraints.flex; }
    [[nodiscard]] const FlexItem& flexItem() const { return m_constraints.flex; }

    /**
     * @brief 获取Grid参数（快捷访问）
     */
    GridItem& gridItem() { return m_constraints.grid; }
    [[nodiscard]] const GridItem& gridItem() const { return m_constraints.grid; }

    // ==================== 尺寸信息 ====================

    /**
     * @brief 获取当前尺寸
     */
    [[nodiscard]] Size currentSize() const;

    /**
     * @brief 获取当前边界
     */
    [[nodiscard]] Rect currentBounds() const;

    /**
     * @brief 获取边距
     */
    [[nodiscard]] Margin margin() const;

    /**
     * @brief 获取内边距
     */
    [[nodiscard]] Padding padding() const;

    // ==================== 测量接口 ====================

    /**
     * @brief 测量Widget在给定规格下的期望尺寸
     *
     * @param widthSpec 宽度测量规格
     * @param heightSpec 高度测量规格
     * @return 测量的尺寸
     *
     * 测量规则：
     * 1. 如果设置了自定义测量函数，使用自定义函数
     * 2. 如果Widget是容器，测量所有子元素并计算总尺寸
     * 3. 如果Widget有preferredSize，使用preferredSize
     * 4. 否则使用当前Widget尺寸
     */
    [[nodiscard]] Size measure(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec);

    /**
     * @brief 设置自定义测量函数
     */
    void setMeasureFunc(MeasureFunc func) { m_measureFunc = std::move(func); }

    /**
     * @brief 清除自定义测量函数
     */
    void clearMeasureFunc() { m_measureFunc = nullptr; }

    /**
     * @brief 获取上次测量的尺寸
     */
    [[nodiscard]] const Size& lastMeasuredSize() const { return m_lastMeasuredSize; }

    // ==================== 布局接口 ====================

    /**
     * @brief 应用布局结果
     *
     * 将布局计算的结果应用到Widget，更新其位置和尺寸。
     *
     * @param result 布局结果
     */
    void applyLayout(const LayoutResult& result);

    /**
     * @brief 应用布局（简化版）
     *
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    void applyLayout(i32 x, i32 y, i32 width, i32 height);

    // ==================== 脏标记管理 ====================

    /**
     * @brief 检查是否需要重新布局
     */
    [[nodiscard]] bool isLayoutDirty() const { return m_layoutDirty; }

    /**
     * @brief 检查是否需要重新渲染
     */
    [[nodiscard]] bool isRenderDirty() const { return m_renderDirty; }

    /**
     * @brief 标记需要重新布局
     *
     * 会同时标记所有祖先节点需要重新布局。
     */
    void requestLayout();

    /**
     * @brief 标记需要重新渲染
     */
    void requestRender() { m_renderDirty = true; }

    /**
     * @brief 清除脏标记
     */
    void clearLayoutDirty() { m_layoutDirty = false; }
    void clearRenderDirty() { m_renderDirty = false; }

    // ==================== 子元素管理 ====================

    /**
     * @brief 获取子元素适配器列表
     *
     * 如果Widget是容器，返回所有子元素的适配器。
     */
    [[nodiscard]] std::vector<WidgetLayoutAdaptor*> getChildren();

    /**
     * @brief 获取子元素数量
     */
    [[nodiscard]] size_t childCount() const;

    /**
     * @brief 检查是否是容器
     */
    [[nodiscard]] bool isContainer() const;

    /**
     * @brief 获取基线偏移量
     *
     * 用于Flex布局的Baseline对齐。返回元素基线相对于顶部的偏移像素数。
     * 默认实现返回元素高度的一半作为近似基线。
     * 文本类Widget应返回字体的基线偏移以实现精确对齐。
     */
    [[nodiscard]] i32 getBaseline() const;

    // ==================== 布局深度 ====================

    /**
     * @brief 获取布局深度
     */
    [[nodiscard]] i32 depth() const { return m_depth; }

    /**
     * @brief 设置布局深度
     */
    void setDepth(i32 depth) { m_depth = depth; }

private:
    /**
     * @brief 默认测量实现
     */
    [[nodiscard]] Size _measureDefault(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec);

    /**
     * @brief 测量容器子元素
     */
    [[nodiscard]] Size _measureContainer(const MeasureSpec& widthSpec, const MeasureSpec& heightSpec);

    /**
     * @brief 向上传播布局请求
     */
    void _propagateLayoutRequest();

    widget::Widget* m_widget = nullptr;
    LayoutConstraints m_constraints;
    MeasureFunc m_measureFunc;
    std::vector<std::unique_ptr<WidgetLayoutAdaptor>> m_childAdaptorsCache;
    bool m_childrenCacheDirty = true;

    Size m_lastMeasuredSize;
    MeasureSpec m_lastWidthSpec;
    MeasureSpec m_lastHeightSpec;
    bool m_cacheValid = false;

    // 约束缓存，用于检测约束变化使测量缓存失效
    i32 m_lastPreferredWidth = -1;
    i32 m_lastPreferredHeight = -1;
    i32 m_lastMinWidth = 0;
    i32 m_lastMinHeight = 0;

    bool m_layoutDirty = true;
    bool m_renderDirty = true;
    i32 m_depth = 0;
};

/**
 * @brief 容器布局适配器基类
 *
 * 为容器Widget提供额外的布局功能。
 * 实现IWidgetContainer的Widget可以使用此适配器。
 */
class ContainerLayoutAdaptor : public WidgetLayoutAdaptor {
public:
    /**
     * @brief 构造函数
     * @param widget 关联的容器Widget
     * @param container 容器接口
     */
    ContainerLayoutAdaptor(widget::Widget* widget, widget::IWidgetContainer* container);

    /**
     * @brief 获取容器接口
     */
    [[nodiscard]] widget::IWidgetContainer* container() const { return m_container; }

    /**
     * @brief 获取所有子元素的适配器
     *
     * 注意：返回的是临时创建的适配器，仅用于当前布局周期。
     */
    [[nodiscard]] std::vector<WidgetLayoutAdaptor> getChildAdaptors();

private:
    widget::IWidgetContainer* m_container = nullptr;
};

} // namespace mc::client::ui::kagero::layout
