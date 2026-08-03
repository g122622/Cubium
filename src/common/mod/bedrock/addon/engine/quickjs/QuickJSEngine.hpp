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

#include "common/mod/bedrock/addon/binding/IModuleBindingFactory.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

class QuickJSContext;
class QuickJSRuntime;

/**
 * @brief QuickJS引擎实现
 *
 * 使用QuickJS-NG引擎封装，实现IScriptEngine接口。
 * 支持未来替换为V8等其他JS引擎。
 */
class QuickJSEngine : public IScriptEngine {
public:
    QuickJSEngine();
    ~QuickJSEngine() noexcept override;

    // 禁止拷贝
    QuickJSEngine(const QuickJSEngine&) = delete;
    QuickJSEngine& operator=(const QuickJSEngine&) = delete;

    // IScriptEngine接口实现
    [[nodiscard]] bool initialize() override;
    void shutdown() override;
    [[nodiscard]] IScriptRuntime& runtime() override;
    [[nodiscard]] const IScriptRuntime& runtime() const;
    void addModuleFactory(std::unique_ptr<IModuleBindingFactory> factory) override;
    [[nodiscard]] IModuleBindingFactory* findModuleFactory(const std::string& name) const override;
    [[nodiscard]] std::unique_ptr<IScriptContext> createContext(const ModuleDescriptor& descriptor,
        const std::vector<ModuleDependency>& dependencies,
        IDependencyLoader& loader,
        IScriptPrinter& printer) override;
    [[nodiscard]] bool isInitialized() const override;

private:
    std::unique_ptr<QuickJSRuntime> m_runtime;
    std::vector<std::unique_ptr<IModuleBindingFactory>> m_moduleFactories;
    std::unordered_map<std::string, IModuleBindingFactory*> m_factoryByName;
    bool m_initialized = false;
};

} // namespace mc::mod::bedrock::addon
