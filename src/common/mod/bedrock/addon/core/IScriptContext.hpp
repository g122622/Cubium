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

#include "common/mod/bedrock/addon/core/Privilege.hpp"
#include "common/mod/bedrock/addon/core/ScriptException.hpp"
#include "common/mod/bedrock/addon/core/ScriptResult.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本上下文抽象接口
 *
 * 每个脚本插件运行在自己的上下文中，实现隔离。
 * 上下文包含独立的JS执行环境和模块作用域。
 *
 * @note 上下文由IScriptRuntime创建，销毁时由IScriptRuntime回收。
 */
class IScriptContext {
public:
    virtual ~IScriptContext() = default;

    /**
     * @brief 执行一段脚本代码
     *
     * @param source 脚本源码
     * @param filename 文件名（用于错误报告）
     * @param flags 执行标志
     * @return 执行结果
     */
    [[nodiscard]] virtual ScriptResult evaluate(
        const std::string& source, const std::string& filename, EvalFlags flags = EvalFlags::None) = 0;

    /**
     * @brief 作为ES6模块执行脚本
     *
     * @param source 模块源码
     * @param filename 文件名
     * @return 执行结果
     */
    [[nodiscard]] virtual ScriptResult evaluateModule(const std::string& source, const std::string& filename) = 0;

    /**
     * @brief 调用全局函数
     *
     * @param name 函数名
     * @param args 参数列表
     * @return 执行结果
     */
    [[nodiscard]] virtual ScriptResult callFunction(const std::string& name, const std::vector<ScriptValue>& args) = 0;

    /**
     * @brief 导入模块
     *
     * @param moduleName 模块名
     * @return 模块导出对象
     */
    [[nodiscard]] virtual ScriptResult importModule(const std::string& moduleName) = 0;

    /**
     * @brief 检查上下文是否有效
     *
     * 上下文在销毁或发生致命错误后变为无效。
     */
    [[nodiscard]] virtual bool isValid() const = 0;

    /**
     * @brief 获取原生上下文句柄
     *
     * @return QuickJS的JSContext*指针，或其他引擎的原生句柄
     *
     * @warning 仅用于引擎实现层，modules/层不应使用此方法。
     *          modules/层应使用bindingContext()获取引擎无关的绑定接口。
     */
    [[nodiscard]] virtual void* nativeHandle() = 0;

    /**
     * @brief 获取脚本绑定上下文
     *
     * 返回引擎无关的绑定接口，modules/层应通过此接口
     * 注册类、方法、属性、创建对象等，而不直接使用QuickJS API。
     *
     * @return 绑定上下文引用
     */
    [[nodiscard]] virtual IScriptBindingContext& bindingContext() = 0;

    /**
     * @brief 注册一个全局回调函数
     *
     * @param name 全局函数名
     * @param callback 回调函数
     * @return 注册是否成功
     */
    virtual bool registerGlobalFunction(
        const std::string& name, std::function<ScriptValue(const std::vector<ScriptValue>&)> callback) = 0;

    /**
     * @brief 设置全局变量
     */
    virtual bool setGlobalVariable(const std::string& name, const ScriptValue& value) = 0;

    /**
     * @brief 获取全局变量
     */
    [[nodiscard]] virtual std::optional<ScriptValue> getGlobalVariable(const std::string& name) const = 0;

    /**
     * @brief 获取上下文的模块描述符
     */
    [[nodiscard]] virtual const std::string& moduleName() const = 0;
};

} // namespace mc::mod::bedrock::addon
