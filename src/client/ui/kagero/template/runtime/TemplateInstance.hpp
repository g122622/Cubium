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

#include "../../widget/IWidgetContainer.hpp"
#include "../../widget/Widget.hpp"
#include "../binder/BindingContext.hpp"
#include "../compiler/TemplateCompiler.hpp"
#include "UpdateScheduler.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc::client::ui::kagero::tpl::runtime {

// 前向声明已移至 UpdateScheduler.hpp

/**
 * @brief Widget工厂函数类型
 */
using WidgetFactory = std::function<std::unique_ptr<widget::Widget>(
    const std::string& tagName, const std::string& id, const std::map<std::string, std::string>& attrs)>;

/**
 * @brief 属性设置器函数类型
 */
using AttributeSetter =
    std::function<void(widget::Widget* widget, const std::string& attrName, const binder::Value& value)>;

/**
 * @brief 事件绑定器函数类型
 */
using EventBinder = std::function<void(widget::Widget* widget,
    const std::string& eventName,
    const std::string& callbackName,
    binder::BindingContext& ctx)>;

/**
 * @brief 模板实例
 *
 * 运行时的模板实例，拥有编译后的模板和运行时状态。
 * 负责将模板实例化为Widget树，并管理绑定和事件。
 *
 * 注意：TemplateInstance 拥有 CompiledTemplate 的所有权，
 * 确保 Widget 树和绑定的生命周期安全。
 *
 * 使用示例：
 * @code
 * // 编译模板
 * TemplateCompiler compiler;
 * auto compiled = compiler.compile(source);
 *
 * // 创建实例（转移所有权）
 * BindingContext ctx(store, bus);
 * TemplateInstance instance(std::move(compiled), ctx);
 *
 * // 实例化Widget树（所有权保留在 TemplateInstance 中）
 * instance.instantiate();
 *
 * // 获取根Widget
 * widget::Widget* root = instance.rootWidget();
 *
 * // 更新绑定
 * instance.updateBindings();
 * @endcode
 */
class TemplateInstance {
public:
    /**
     * @brief 构造函数（转移 CompiledTemplate 所有权）
     * @param compiled 编译后的模板（转移所有权）
     * @param ctx 绑定上下文
     */
    TemplateInstance(std::unique_ptr<compiler::CompiledTemplate> compiled, binder::BindingContext& ctx);

    /**
     * @brief 构造函数（兼容旧 API，不推荐）
     * @param compiled 编译后的模板裸指针
     * @param ctx 绑定上下文
     * @deprecated 使用 unique_ptr 版本以确保生命周期安全
     */
    [[deprecated("Use unique_ptr version for safe lifetime management")]]
    TemplateInstance(const compiler::CompiledTemplate* compiled, binder::BindingContext& ctx);

    /**
     * @brief 析构函数
     */
    ~TemplateInstance();

    // 禁止拷贝
    TemplateInstance(const TemplateInstance&) = delete;
    TemplateInstance& operator=(const TemplateInstance&) = delete;

    // 允许移动
    TemplateInstance(TemplateInstance&&) noexcept;
    TemplateInstance& operator=(TemplateInstance&&) noexcept;

    // ========== Widget工厂注册 ==========

    /**
     * @brief 注册Widget工厂
     *
     * @param tagName 标签名
     * @param factory 工厂函数
     */
    void registerWidgetFactory(const std::string& tagName, WidgetFactory factory);

    /**
     * @brief 注册默认Widget工厂
     *
     * 为内置Widget类型注册工厂
     */
    void registerDefaultFactories();

    /**
     * @brief 设置默认Widget工厂
     *
     * 当没有找到特定工厂时使用
     */
    void setDefaultFactory(WidgetFactory factory);

    // ========== 属性设置器注册 ==========

    /**
     * @brief 注册属性设置器
     *
     * @param attrName 属性名
     * @param setter 设置函数
     */
    void registerAttributeSetter(const std::string& attrName, AttributeSetter setter);

    /**
     * @brief 注册默认属性设置器
     */
    void registerDefaultAttributeSetters();

    // ========== 事件绑定器注册 ==========

    /**
     * @brief 注册事件绑定器
     *
     * @param eventName 事件名
     * @param binder 绑定函数
     */
    void registerEventBinder(const std::string& eventName, EventBinder binder);

    /**
     * @brief 注册默认事件绑定器
     */
    void registerDefaultEventBinders();

    // ========== 实例化 ==========

    /**
     * @brief 实例化Widget树
     *
     * Widget 树所有权保留在 TemplateInstance 中。
     * 使用 rootWidget() 获取根 Widget 指针。
     *
     * @return 是否成功
     */
    bool instantiate();

    /**
     * @brief 实例化到指定容器
     *
     * @param container 目标容器
     * @return 是否成功
     */
    bool instantiateInto(widget::IWidgetContainer* container);

    /**
     * @brief 检查是否已实例化
     */
    [[nodiscard]] bool isInstantiated() const { return m_rootWidget != nullptr; }

    /**
     * @brief 获取根Widget
     *
     * Widget 树所有权保留在 TemplateInstance 中。
     * TemplateInstance 销毁时 Widget 树也会销毁。
     */
    [[nodiscard]] widget::Widget* rootWidget() { return m_rootWidget.get(); }
    [[nodiscard]] const widget::Widget* rootWidget() const { return m_rootWidget.get(); }

    // ========== 绑定管理 ==========

    /**
     * @brief 更新所有绑定
     *
     * 从绑定上下文读取最新值并更新Widget
     */
    void updateBindings();

    /**
     * @brief 更新指定路径的绑定
     *
     * 从绑定上下文读取最新值并更新Widget
     *
     * @param path 状态路径
     * @return 是否更新成功（无对应绑定计划时返回 false）
     */
    [[nodiscard]] bool updateBinding(const std::string& path);

    /**
     * @brief 设置绑定上下文
     */
    void setBindingContext(binder::BindingContext& ctx) { m_context = &ctx; }

    /**
     * @brief 获取绑定上下文
     */
    [[nodiscard]] binder::BindingContext* bindingContext() { return m_context; }
    [[nodiscard]] const binder::BindingContext* bindingContext() const { return m_context; }

    // ========== 状态更新 ==========

    /**
     * @brief 通知状态变更
     *
     * 将路径入队到内部 UpdateScheduler，由 tick() / flush() 统一调度执行。
     * 若调度器禁用延迟更新，则等价于立即调用 updateBinding(path)。
     *
     * @param path 变更的状态路径
     */
    void notifyStateChange(const std::string& path);

    /**
     * @brief 刷新所有绑定
     *
     * 立即同步执行所有待处理任务，并全量刷新绑定。
     */
    void refresh();

    /**
     * @brief 每帧推进调度器
     *
     * 由外部（如 TemplateScreen::tick）每帧调用，传入当前毫秒时间戳。
     * 内部驱动 UpdateScheduler::tick(currentMs) 执行到期任务。
     *
     * @param currentMs 当前毫秒时间戳（通常来自 TimeUtils::getCurrentTimeMs()）
     * @return 本次 tick 执行的任务数量
     */
    u32 tick(u64 currentMs);

    /**
     * @brief 立即执行所有待处理任务（无视延迟）
     *
     * 用于屏幕关闭、强制刷新等需要立即同步的场景。
     *
     * @return 执行的任务数量
     */
    u32 flushPending();

    // ========== Widget查找 ==========

    /**
     * @brief 通过ID查找Widget
     */
    [[nodiscard]] widget::Widget* findWidgetById(const std::string& id);

    /**
     * @brief 通过路径查找Widget
     */
    [[nodiscard]] widget::Widget* findWidgetByPath(const std::string& path);

    // ========== 调试 ==========

    /**
     * @brief 获取实例统计信息
     */
    [[nodiscard]] std::string debugInfo() const;

    // ========== 调度器配置 ==========

    /**
     * @brief 获取内部调度器（用于配置批量延迟、延迟更新等参数）
     *
     * 调度器由 TemplateInstance 内部持有，外部可通过此方法配置：
     * - setBatchDelay(ms)：批量更新延迟
     * - setMaxBatchSize(n)：最大批量大小
     * - setDeferredUpdate(true/false)：是否启用延迟更新
     */
    [[nodiscard]] UpdateScheduler& scheduler() { return m_scheduler; }
    [[nodiscard]] const UpdateScheduler& scheduler() const { return m_scheduler; }

private:
    // ========== 实例化辅助方法 ==========

    /**
     * @brief 实例化节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> _instantiateNode(
        const ast::Node* node, widget::Widget* parent = nullptr);

    /**
     * @brief 实例化元素节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> _instantiateElement(
        const ast::ElementNode* element, widget::Widget* parent = nullptr);

    /**
     * @brief 实例化文本节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> _instantiateText(
        const ast::TextNode* textNode, widget::Widget* parent = nullptr);

    /**
     * @brief 创建Widget
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> _createWidget(
        const std::string& tagName, const std::string& id, const std::map<std::string, std::string>& attrs);

    /**
     * @brief 应用静态属性
     */
    void _applyStaticAttributes(widget::Widget* widget, const std::vector<ast::Attribute>& attrs);

    /**
     * @brief 应用绑定属性
     */
    void _applyBindingAttributes(
        widget::Widget* widget, const std::vector<ast::Attribute>& attrs, const std::string& widgetPath);

    /**
     * @brief 应用事件绑定
     */
    void _applyEventBindings(
        widget::Widget* widget, const std::vector<ast::Attribute>& attrs, const std::string& widgetPath);

    /**
     * @brief 解析静态属性值
     */
    [[nodiscard]] binder::Value _parseStaticValue(const ast::Attribute& attr) const;

    /**
     * @brief 从属性创建Widget路径
     */
    [[nodiscard]] std::string _buildWidgetPath(
        const ast::ElementNode* element, const std::string& parentPath = "") const;

    /**
     * @brief 注册Widget到路径映射
     */
    void _registerWidgetPath(const std::string& path, widget::Widget* widget);

    /**
     * @brief 注册Widget ID映射
     */
    void _registerWidgetId(const std::string& id, widget::Widget* widget);

    // ========== 循环渲染辅助方法 ==========

    /**
     * @brief 实例化循环子元素
     *
     * 为集合中每个元素创建子元素的副本
     *
     * @param element 循环元素模板
     * @param parent 父Widget
     * @param collectionPath 集合路径
     * @param itemVarName 循环变量名
     * @param indexVarName 索引变量名（可选）
     */
    void _instantiateLoopChildren(const ast::ElementNode* element,
        widget::Widget* parent,
        const std::string& collectionPath,
        const std::string& itemVarName,
        const std::string& indexVarName = "");

    /**
     * @brief 解析集合
     *
     * @param path 集合路径
     * @return 值数组
     */
    [[nodiscard]] std::vector<binder::Value> _resolveCollection(const std::string& path) const;

    /**
     * @brief 检查条件是否满足
     *
     * @param condition 条件信息
     * @return 条件是否满足
     */
    [[nodiscard]] bool _evaluateCondition(const ast::ConditionInfo& condition) const;

    /**
     * @brief 重新绑定调度器回调
     *
     * 调度器回调捕获 this 指针，移动构造/赋值后需重新绑定。
     * 也在 instantiate() 完成后初次绑定。
     */
    void _rebindSchedulerCallback();

private:
    // Owned compiled template
    std::unique_ptr<compiler::CompiledTemplate> m_ownedCompiled;

    // Raw pointer for fast access (points to m_ownedCompiled or external)
    const compiler::CompiledTemplate* m_compiled;
    binder::BindingContext* m_context;

    std::unique_ptr<widget::Widget> m_rootWidget;

    // Widget映射
    std::unordered_map<std::string, widget::Widget*> m_widgetById;
    std::unordered_map<std::string, widget::Widget*> m_widgetByPath;

    // 工厂和设置器
    std::unordered_map<std::string, WidgetFactory> m_widgetFactories;
    std::unordered_map<std::string, AttributeSetter> m_attributeSetters;
    std::unordered_map<std::string, EventBinder> m_eventBinders;
    WidgetFactory m_defaultFactory;

    // 订阅ID
    std::vector<u64> m_subscriptionIds;

    // 更新调度器（管理增量更新的批量延迟和优先级调度）
    UpdateScheduler m_scheduler;
};

} // namespace mc::client::ui::kagero::tpl::runtime
