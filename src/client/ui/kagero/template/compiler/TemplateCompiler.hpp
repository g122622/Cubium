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

#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/template/core/TemplateConfig.hpp"
#include "client/ui/kagero/template/core/TemplateError.hpp"
#include "client/ui/kagero/template/parser/Ast.hpp"
#include "client/ui/kagero/template/parser/Lexer.hpp"
#include "client/ui/kagero/template/parser/Parser.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::kagero::tpl::compiler {

// 引入core命名空间类型
using core::SourceLocation;
using core::TemplateConfig;
using core::TemplateErrorCollector;
using core::TemplateErrorInfo;
using core::TemplateErrorType;

// 引入parser命名空间类型
using parser::Lexer;
using parser::Parser;
using parser::Token;

// 引入ast命名空间类型
using ast::DocumentNode;
using ast::ElementNode;
using ast::Node;

// 前向声明
class CompiledTemplate;

/**
 * @brief 绑定计划
 *
 * 存储编译时确定的绑定信息，用于运行时快速执行绑定
 */
struct BindingPlan {
    std::string widgetPath;     ///< Widget路径 (如 "screen.grid.slot")
    std::string statePath;      ///< 状态路径 (如 "player.inventory.main[0].item")
    std::string attributeName;  ///< 属性名 (如 "text", "visible")
    bool isLoopBinding = false; ///< 是否是循环内绑定
    std::string loopVarName;    ///< 循环变量名

    BindingPlan() = default;
    BindingPlan(std::string widget, std::string state, std::string attr, bool loop = false, std::string loopVar = "")
        : widgetPath(std::move(widget))
        , statePath(std::move(state))
        , attributeName(std::move(attr))
        , isLoopBinding(loop)
        , loopVarName(std::move(loopVar))
    {}
};

/**
 * @brief 事件计划
 *
 * 存储编译时确定的事件处理器信息
 */
struct EventPlan {
    std::string widgetPath;   ///< Widget路径
    std::string eventName;    ///< 事件名 (如 "click", "hover")
    std::string callbackName; ///< 回调名 (如 "onStartGame")

    EventPlan() = default;
    EventPlan(std::string widget, std::string event, std::string callback)
        : widgetPath(std::move(widget))
        , eventName(std::move(event))
        , callbackName(std::move(callback))
    {}
};

/**
 * @brief 循环计划
 *
 * 存储循环渲染的信息
 */
struct LoopPlan {
    std::string parentPath;                ///< 父Widget路径
    std::string collectionPath;            ///< 集合路径
    std::string itemVarName;               ///< 循环变量名
    std::vector<BindingPlan> itemBindings; ///< 子项绑定

    LoopPlan() = default;
    LoopPlan(std::string parent, std::string collection, std::string item)
        : parentPath(std::move(parent))
        , collectionPath(std::move(collection))
        , itemVarName(std::move(item))
    {}
};

/**
 * @brief 编译后的模板
 *
 * 包含预编译的绑定计划和事件处理器，
 * 可以快速实例化为Widget树。
 */
class CompiledTemplate {
public:
    /**
     * @brief 构造函数
     */
    CompiledTemplate();

    /**
     * @brief 析构函数
     */
    ~CompiledTemplate();

    // 禁止拷贝
    CompiledTemplate(const CompiledTemplate&) = delete;
    CompiledTemplate& operator=(const CompiledTemplate&) = delete;

    // 允许移动
    CompiledTemplate(CompiledTemplate&&) noexcept;
    CompiledTemplate& operator=(CompiledTemplate&&) noexcept;

    /**
     * @brief 设置AST根节点
     */
    void setAstRoot(std::unique_ptr<ast::DocumentNode> root);

    /**
     * @brief 获取AST根节点
     */
    [[nodiscard]] const ast::DocumentNode* astRoot() const { return m_astRoot.get(); }
    [[nodiscard]] ast::DocumentNode* astRoot() { return m_astRoot.get(); }

    // ========== 绑定计划 ==========

    /**
     * @brief 添加绑定计划
     */
    void addBindingPlan(BindingPlan plan);

    /**
     * @brief 获取所有绑定计划
     */
    [[nodiscard]] const std::vector<BindingPlan>& bindingPlans() const { return m_bindingPlans; }
    [[nodiscard]] std::vector<BindingPlan>& bindingPlans() { return m_bindingPlans; }

    // ========== 事件计划 ==========

    /**
     * @brief 添加事件计划
     */
    void addEventPlan(EventPlan plan);

    /**
     * @brief 获取所有事件计划
     */
    [[nodiscard]] const std::vector<EventPlan>& eventPlans() const { return m_eventPlans; }
    [[nodiscard]] std::vector<EventPlan>& eventPlans() { return m_eventPlans; }

    // ========== 循环计划 ==========

    /**
     * @brief 添加循环计划
     */
    void addLoopPlan(LoopPlan plan);

    /**
     * @brief 获取所有循环计划
     */
    [[nodiscard]] const std::vector<LoopPlan>& loopPlans() const { return m_loopPlans; }
    [[nodiscard]] std::vector<LoopPlan>& loopPlans() { return m_loopPlans; }

    // ========== 状态路径 ==========

    /**
     * @brief 添加需要监听的状态路径
     */
    void addWatchedPath(const std::string& path);

    /**
     * @brief 获取所有需要监听的状态路径
     */
    [[nodiscard]] const std::set<std::string>& watchedPaths() const { return m_watchedPaths; }

    // ========== 回调注册 ==========

    /**
     * @brief 添加注册的回调名
     */
    void addRegisteredCallback(const std::string& name);

    /**
     * @brief 获取所有注册的回调名
     */
    [[nodiscard]] const std::set<std::string>& registeredCallbacks() const { return m_registeredCallbacks; }

    // ========== 元数据 ==========

    /**
     * @brief 设置模板源路径
     */
    void setSourcePath(const std::string& path) { m_sourcePath = path; }

    /**
     * @brief 获取模板源路径
     */
    [[nodiscard]] const std::string& sourcePath() const { return m_sourcePath; }

    /**
     * @brief 设置编译时间
     */
    void setCompileTime(u64 time) { m_compileTime = time; }

    /**
     * @brief 获取编译时间（毫秒）
     */
    [[nodiscard]] u64 compileTime() const { return m_compileTime; }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return m_astRoot != nullptr; }

    /**
     * @brief 获取错误信息
     */
    [[nodiscard]] const std::vector<TemplateErrorInfo>& errors() const { return m_errors; }
    [[nodiscard]] std::vector<TemplateErrorInfo>& errors() { return m_errors; }

    /**
     * @brief 添加错误
     */
    void addError(TemplateErrorInfo error);

    /**
     * @brief 检查是否有错误
     */
    [[nodiscard]] bool hasErrors() const { return !m_errors.empty(); }

    // ========== 调试 ==========

    /**
     * @brief 导出编译结果为字符串（调试用）
     */
    [[nodiscard]] std::string debugDump() const;

private:
    std::unique_ptr<ast::DocumentNode> m_astRoot;
    std::vector<BindingPlan> m_bindingPlans;
    std::vector<EventPlan> m_eventPlans;
    std::vector<LoopPlan> m_loopPlans;
    std::set<std::string> m_watchedPaths;
    std::set<std::string> m_registeredCallbacks;
    std::vector<TemplateErrorInfo> m_errors;
    std::string m_sourcePath;
    u64 m_compileTime = 0;
};

/**
 * @brief 模板编译器
 *
 * 将模板源码编译为可执行的CompiledTemplate。
 * 支持严格模式验证、缓存和增量更新。
 *
 * 使用示例：
 * @code
 * TemplateCompiler compiler;
 * compiler.setStrictMode(true);
 *
 * auto compiled = compiler.compile(templateSource);
 * if (compiled->hasErrors()) {
 *     // 处理错误
 * }
 *
 * // 使用编译结果
 * auto widget = compiled->instantiate(ctx);
 * @endcode
 */
class TemplateCompiler {
public:
    /**
     * @brief 构造函数
     */
    TemplateCompiler();

    /**
     * @brief 构造函数（带配置）
     */
    explicit TemplateCompiler(const TemplateConfig& config);

    /**
     * @brief 编译模板源码
     *
     * @param source 模板源码字符串
     * @param sourcePath 源文件路径（用于错误报告）
     * @return 编译结果
     */
    [[nodiscard]] std::unique_ptr<CompiledTemplate> compile(
        const std::string& source, const std::string& sourcePath = "");

    /**
     * @brief 从文件编译
     *
     * @param filePath 文件路径
     * @return 编译结果
     */
    [[nodiscard]] std::unique_ptr<CompiledTemplate> compileFile(const std::string& filePath);

    /**
     * @brief 编译已解析的AST
     *
     * @param document AST文档节点
     * @param sourcePath 源文件路径
     * @return 编译结果
     */
    [[nodiscard]] std::unique_ptr<CompiledTemplate> compileAst(
        std::unique_ptr<ast::DocumentNode> document, const std::string& sourcePath = "");

    // ========== 配置 ==========

    /**
     * @brief 设置严格模式
     *
     * 严格模式下禁止所有动态特性
     */
    void setStrictMode(bool enabled) { m_config.strictMode = enabled; }

    /**
     * @brief 检查是否启用严格模式
     */
    [[nodiscard]] bool isStrictMode() const { return m_config.strictMode; }

    /**
     * @brief 设置配置
     */
    void setConfig(const TemplateConfig& config) { m_config = config; }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const TemplateConfig& config() const { return m_config; }

    // ========== 错误处理 ==========

    /**
     * @brief 获取最后的编译错误
     */
    [[nodiscard]] const std::vector<TemplateErrorInfo>& lastErrors() const { return m_lastErrors; }

    /**
     * @brief 检查最后一次编译是否有错误
     */
    [[nodiscard]] bool hasErrors() const { return !m_lastErrors.empty(); }

private:
    // ========== 编译阶段 ==========

    /**
     * @brief 词法分析
     */
    bool _tokenize(const std::string& source, const std::string& sourcePath);

    /**
     * @brief 语法分析
     * TODO: 当前为空实现，仅返回true，实际的解析逻辑在compile()中完成
     */
    bool _parse(const std::string& sourcePath);

    /**
     * @brief 语义分析和验证
     */
    bool _validate(ast::DocumentNode* document);

    /**
     * @brief 生成绑定计划
     */
    void _generateBindingPlans(ast::DocumentNode* document, CompiledTemplate* result);

    /**
     * @brief 递归生成绑定计划
     */
    void _generateBindingPlansRecursive(const ast::Node* node, const std::string& parentPath, CompiledTemplate* result);

    /**
     * @brief 生成事件计划
     */
    void _generateEventPlans(ast::DocumentNode* document, CompiledTemplate* result);

    /**
     * @brief 递归生成事件计划
     */
    void _generateEventPlansRecursive(const ast::Node* node, const std::string& parentPath, CompiledTemplate* result);

    /**
     * @brief 收集状态路径
     */
    void _collectWatchedPaths(ast::DocumentNode* document, CompiledTemplate* result);

    /**
     * @brief 收集回调名称
     */
    void _collectCallbacks(ast::DocumentNode* document, CompiledTemplate* result);

    // ========== 验证方法 ==========

    /**
     * @brief 验证AST节点
     */
    bool _validateNode(const ast::Node* node, TemplateErrorCollector& collector);

    /**
     * @brief 验证元素节点
     */
    bool _validateElement(const ast::ElementNode* element, TemplateErrorCollector& collector);

    /**
     * @brief 验证属性
     * TODO: element参数暂未使用，后续属性验证可能需要元素上下文
     */
    bool _validateAttribute(
        const ast::Attribute& attr, const ast::ElementNode* element, TemplateErrorCollector& collector);

    /**
     * @brief 检查是否包含内联脚本/表达式
     */
    bool _containsInlineScript(const std::string& value) const;

    /**
     * @brief 检查是否包含禁止的模式
     */
    bool _containsForbiddenPattern(const std::string& value) const;

    // ========== 工具方法 ==========

    /**
     * @brief 生成Widget路径
     */
    static std::string _generateWidgetPath(const ast::ElementNode* element, const std::string& parentPath);

    /**
     * @brief 从AST节点提取绑定信息
     * TODO: 此方法当前未被调用，待循环绑定功能完善后启用
     */
    static void _extractBindings(const ast::ElementNode* element,
        const std::string& widgetPath,
        std::vector<BindingPlan>& plans,
        std::vector<LoopPlan>& loopPlans);

private:
    TemplateConfig m_config;
    // TODO: m_tokens 当前未被使用，词法分析结果直接从Lexer获取，后续缓存优化时可启用
    std::vector<Token> m_tokens;
    std::unique_ptr<ast::DocumentNode> m_parsedAst;
    std::vector<TemplateErrorInfo> m_lastErrors;
};

} // namespace mc::client::ui::kagero::tpl::compiler
