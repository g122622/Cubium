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

#include "common/mod/bedrock/addon/engine/quickjs/QuickJSEngine.hpp"
#include "common/mod/bedrock/addon/binding/IModuleBindingFactory.hpp"
#include "common/mod/bedrock/addon/core/Capabilities.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSContext.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSRuntime.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

QuickJSEngine::QuickJSEngine() = default;

QuickJSEngine::~QuickJSEngine() noexcept
{
    if (m_initialized) {
        shutdown();
    }
}

bool QuickJSEngine::initialize()
{
    if (m_initialized) {
        spdlog::warn("[BedrockAddon] QuickJSEngine already initialized");
        return true;
    }

    spdlog::info("[BedrockAddon] Initializing QuickJS engine");

    m_runtime = std::make_unique<QuickJSRuntime>();
    if (!m_runtime->initialize()) {
        spdlog::error("[BedrockAddon] Failed to initialize QuickJS runtime");
        m_runtime.reset();
        return false;
    }

    m_initialized = true;
    spdlog::info("[BedrockAddon] QuickJS engine initialized successfully");
    return true;
}

void QuickJSEngine::shutdown()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("[BedrockAddon] Shutting down QuickJS engine");

    // 上下文由各自的插件管理器销毁，这里只销毁运行时
    m_runtime.reset();
    m_moduleFactories.clear();
    m_factoryByName.clear();
    m_initialized = false;

    spdlog::info("[BedrockAddon] QuickJS engine shut down");
}

IScriptRuntime& QuickJSEngine::runtime()
{
    return *m_runtime;
}

const IScriptRuntime& QuickJSEngine::runtime() const
{
    return *m_runtime;
}

void QuickJSEngine::addModuleFactory(std::unique_ptr<IModuleBindingFactory> factory)
{
    if (!factory) {
        return;
    }

    std::string name = factory->name();
    spdlog::info("[BedrockAddon] Adding module factory: {}", name);

    m_factoryByName[name] = factory.get();
    m_moduleFactories.push_back(std::move(factory));
}

IModuleBindingFactory* QuickJSEngine::findModuleFactory(const std::string& name) const
{
    auto it = m_factoryByName.find(name);
    if (it != m_factoryByName.end()) {
        return it->second;
    }
    return nullptr;
}

std::unique_ptr<IScriptContext> QuickJSEngine::createContext(const ModuleDescriptor& descriptor,
    const std::vector<ModuleDependency>& dependencies,
    IDependencyLoader& loader,
    IScriptPrinter& printer)
{
    if (!m_initialized || !m_runtime) {
        spdlog::error("[BedrockAddon] Cannot create context: engine not initialized");
        return nullptr;
    }

    spdlog::info("[BedrockAddon] Creating script context for module: {}", descriptor.name);

    ContextConfig config{
        Capabilities{},                  // 无特殊能力
        std::nullopt,                    // 无特殊权限
        DEFAULT_CONTEXT_MEMORY_BYTES,    // 64MB 内存限制
        DEFAULT_CONTEXT_STACK_SIZE_BYTES // 4MB 栈限制
    };
    auto context = m_runtime->createContext(config);
    if (!context) {
        spdlog::error("[BedrockAddon] Failed to create QuickJS context");
        return nullptr;
    }

    // 注入模块源码提供者：相对路径 import（如 "./Utilities"）经 moduleLoader 回调调此 provider 取源码。
    // loader（ScriptPluginSource）由 ScriptPluginManager 持有，生命周期长于本上下文，引用安全。
    auto* qjsContext = static_cast<QuickJSContext*>(context.get());
    qjsContext->setModuleSourceProvider([&loader](const std::string& moduleName) -> std::string {
        auto data = loader.loadScript(moduleName);
        return data.has_value() ? data->source : std::string{};
    });

    // 注册所有模块绑定到上下文
    for (const auto& dep : dependencies) {
        auto* factory = findModuleFactory(dep.name);
        if (factory) {
            spdlog::info("[BedrockAddon] Registering module bindings: {}", dep.name);
            if (!factory->registerBindings(*context)) {
                spdlog::error("[BedrockAddon] Failed to register bindings for module: {}", dep.name);
                return nullptr;
            }
        } else {
            spdlog::warn("[BedrockAddon] Module dependency not found: {} (required by {})", dep.name, descriptor.name);
        }
    }

    // 也注册当前模块自身的绑定
    auto* selfFactory = findModuleFactory(descriptor.name);
    if (selfFactory) {
        if (!selfFactory->registerBindings(*context)) {
            spdlog::error("[BedrockAddon] Failed to register self bindings for module: {}", descriptor.name);
            return nullptr;
        }
    }

    return context;
}

bool QuickJSEngine::isInitialized() const
{
    return m_initialized;
}

std::unique_ptr<IScriptEngine> createScriptEngine()
{
    return std::make_unique<QuickJSEngine>();
}

} // namespace mc::mod::bedrock::addon
