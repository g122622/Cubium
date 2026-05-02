#pragma once

#include "../compiler/TemplateCompiler.hpp"
#include "../binder/BindingContext.hpp"
#include "UpdateScheduler.hpp"
#include "../../widget/Widget.hpp"
#include "../../widget/IWidgetContainer.hpp"
#include <memory>
#include <unordered_map>
#include <functional>

namespace mc::client::ui::kagero::tpl::runtime {

// 前向声明已移至 UpdateScheduler.hpp

/**
 * @brief Widget工厂函数类型
 */
using WidgetFactory = std::function<std::unique_ptr<widget::Widget>(
    const String& tagName, const String& id, const std::map<String, String>& attrs)>;

/**
 * @brief 属性设置器函数类型
 */
using AttributeSetter = std::function<void(widget::Widget* widget,
    const String& attrName, const binder::Value& value)>;

/**
 * @brief 事件绑定器函数类型
 */
using EventBinder = std::function<void(widget::Widget* widget,
    const String& eventName, const String& callbackName, binder::BindingContext& ctx)>;

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
    TemplateInstance(std::unique_ptr<compiler::CompiledTemplate> compiled,
                     binder::BindingContext& ctx);

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
    void registerWidgetFactory(const String& tagName, WidgetFactory factory);

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
    void registerAttributeSetter(const String& attrName, AttributeSetter setter);

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
    void registerEventBinder(const String& eventName, EventBinder binder);

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
     * @param path 状态路径
     */
    void updateBinding(const String& path);

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
     * @param path 变更的状态路径
     */
    void notifyStateChange(const String& path);

    /**
     * @brief 刷新所有绑定
     */
    void refresh();

    // ========== Widget查找 ==========

    /**
     * @brief 通过ID查找Widget
     */
    [[nodiscard]] widget::Widget* findWidgetById(const String& id);

    /**
     * @brief 通过路径查找Widget
     */
    [[nodiscard]] widget::Widget* findWidgetByPath(const String& path);

    // ========== 调试 ==========

    /**
     * @brief 获取实例统计信息
     */
    [[nodiscard]] String debugInfo() const;

private:
    // ========== 实例化辅助方法 ==========

    /**
     * @brief 实例化节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> instantiateNode(const ast::Node* node,
        widget::Widget* parent = nullptr);

    /**
     * @brief 实例化元素节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> instantiateElement(const ast::ElementNode* element,
        widget::Widget* parent = nullptr);

    /**
     * @brief 实例化文本节点
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> instantiateText(const ast::TextNode* textNode,
        widget::Widget* parent = nullptr);

    /**
     * @brief 创建Widget
     */
    [[nodiscard]] std::unique_ptr<widget::Widget> createWidget(
        const String& tagName, const String& id,
        const std::map<String, String>& attrs);

    /**
     * @brief 应用静态属性
     */
    void applyStaticAttributes(widget::Widget* widget,
        const std::vector<ast::Attribute>& attrs);

    /**
     * @brief 应用绑定属性
     */
    void applyBindingAttributes(widget::Widget* widget,
        const std::vector<ast::Attribute>& attrs,
        const String& widgetPath);

    /**
     * @brief 应用事件绑定
     */
    void applyEventBindings(widget::Widget* widget,
        const std::vector<ast::Attribute>& attrs,
        const String& widgetPath);

    /**
     * @brief 解析静态属性值
     */
    [[nodiscard]] binder::Value parseStaticValue(const ast::Attribute& attr) const;

    /**
     * @brief 从属性创建Widget路径
     */
    [[nodiscard]] String buildWidgetPath(const ast::ElementNode* element,
        const String& parentPath = "") const;

    /**
     * @brief 注册Widget到路径映射
     */
    void registerWidgetPath(const String& path, widget::Widget* widget);

    /**
     * * @brief 注册Widget ID映射
     */
    void registerWidgetId(const String& id, widget::Widget* widget);

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
    void instantiateLoopChildren(const ast::ElementNode* element,
                                  widget::Widget* parent,
                                  const String& collectionPath,
                                  const String& itemVarName,
                                  const String& indexVarName = "");

    /**
     * @brief 解析集合
     *
     * @param path 集合路径
     * @return 值数组
     */
    [[nodiscard]] std::vector<binder::Value> resolveCollection(const String& path) const;

    /**
     * @brief 检查条件是否满足
     *
     * @param condition 条件信息
     * @return 条件是否满足
     */
    [[nodiscard]] bool evaluateCondition(const ast::ConditionInfo& condition) const;

private:
    // Owned compiled template
    std::unique_ptr<compiler::CompiledTemplate> m_ownedCompiled;

    // Raw pointer for fast access (points to m_ownedCompiled or external)
    const compiler::CompiledTemplate* m_compiled;
    binder::BindingContext* m_context;

    std::unique_ptr<widget::Widget> m_rootWidget;

    // Widget映射
    std::unordered_map<String, widget::Widget*> m_widgetById;
    std::unordered_map<String, widget::Widget*> m_widgetByPath;

    // 工厂和设置器
    std::unordered_map<String, WidgetFactory> m_widgetFactories;
    std::unordered_map<String, AttributeSetter> m_attributeSetters;
    std::unordered_map<String, EventBinder> m_eventBinders;
    WidgetFactory m_defaultFactory;

    // 订阅ID
    std::vector<u64> m_subscriptionIds;
};

} // namespace mc::client::ui::kagero::tpl::runtime
