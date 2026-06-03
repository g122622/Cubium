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
 */

#pragma once

#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

class IScriptContext;

/**
 * @brief 模块绑定工厂接口
 *
 * 每个JS模块（如 @minecraft/server）对应一个绑定工厂，
 * 负责在脚本上下文中注册C++ API绑定。
 *
 * 实现示例：MinecraftModuleFactory 为 @minecraft/server 模块
 * 注册World、Entity、Block等类的绑定。
 */
class IModuleBindingFactory {
public:
    virtual ~IModuleBindingFactory() = default;

    /**
     * @brief 获取模块名
     *
     * @return 模块名，如 "@minecraft/server"
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief 获取模块UUID
     *
     * @return 模块UUID，用于模块版本解析
     */
    [[nodiscard]] virtual std::string uuid() const = 0;

    /**
     * @brief 获取支持的版本列表
     */
    [[nodiscard]] virtual std::vector<ModuleVersion> supportedVersions() const = 0;

    /**
     * @brief 获取指定版本的依赖
     *
     * @param version 模块版本
     * @return 依赖列表
     */
    [[nodiscard]] virtual std::vector<ModuleDependency> dependencies(const ModuleVersion& version) const = 0;

    /**
     * @brief 在脚本上下文中注册模块绑定
     *
     * 此方法将C++类、函数、枚举等注册为JS可访问的对象。
     *
     * @param context 目标脚本上下文
     * @return 注册是否成功
     */
    virtual bool registerBindings(IScriptContext& context) = 0;

    /**
     * @brief 检查模块是否有别名
     *
     * @param alias 别名
     * @return 是否有此别名
     */
    [[nodiscard]] virtual bool hasAlias(const std::string& alias) const { return alias == name(); }
};

} // namespace mc::mod::bedrock::addon
