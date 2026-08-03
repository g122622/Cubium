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

/**
 * @file Template.hpp
 * @brief Kagero模板系统入口
 *
 * Kagero（阳炎）是Minecraft Reborn项目的现代化UI模板引擎，
 * 采用声明式模板、响应式状态管理和组件化架构。
 *
 * 核心特性：
 * - 声明式XML样式模板语法
 * - 类型安全的绑定系统
 * - 零内联脚本（严格模式）
 * - 编译时验证
 * - 增量更新
 *
 * 使用示例：
 * @code
 * // 1. 编译模板
 * TemplateCompiler compiler;
 * auto compiled = compiler.compile(R"(
 *     <screen id="main">
 *         <text id="title" bind:text="player.name"/>
 *         <button id="startBtn" on:click="onStart"/>
 *     </screen>
 * )");
 *
 * // 2. 创建绑定上下文
 * StateStore store;
 * EventBus eventBus;
 * BindingContext ctx(store, eventBus);
 *
 * // 暴露状态
 * std::string playerName = "Steve";
 * ctx.expose("player.name", &playerName);
 *
 * // 暴露回调
 * ctx.exposeCallback("onStart", [](Widget* w, const Event& e) {
 *     // 处理点击
 * });
 *
 * // 3. 实例化模板
 * TemplateInstance instance(compiled.get(), ctx);
 * auto root = instance.instantiate();
 *
 * // 4. 更新绑定
 * playerName = "Alex";
 * ctx.notifyChange("player.name");
 * instance.updateBindings();
 * @endcode
 */

#pragma once

// 核心配置和错误
#include "core/TemplateConfig.hpp"
#include "core/TemplateError.hpp"

// 模板系统入口函数
#include "TemplateSystem.hpp"

// AST和解析器
#include "parser/Ast.hpp"
#include "parser/AstVisitor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

// 绑定上下文
#include "binder/BindingContext.hpp"

// 编译器
#include "compiler/TemplateCompiler.hpp"

// 运行时
#include "runtime/TemplateInstance.hpp"

// 内置组件和事件
#include "bindings/BuiltinEvents.hpp"
#include "bindings/BuiltinWidgets.hpp"
#include "common/core/Types.hpp"

/**
 * @brief Kagero模板系统命名空间
 *
 * 命名空间组织：
 * - core: 核心配置和错误处理
 * - parser: 词法分析和语法分析
 * - ast: 抽象语法树
 * - binder: 绑定上下文和值系统
 * - compiler: 模板编译器
 * - runtime: 运行时实例和更新调度
 * - bindings: 内置Widget和事件注册
 */
namespace mc::client::ui::kagero::tpl {

/**
 * @brief 模板系统版本信息
 */
struct TemplateVersion {
    static constexpr u32 Major = 1;
    static constexpr u32 Minor = 0;
    static constexpr u32 Patch = 0;
    static constexpr const char* VersionString = "1.0.0";
};

} // namespace mc::client::ui::kagero::tpl
