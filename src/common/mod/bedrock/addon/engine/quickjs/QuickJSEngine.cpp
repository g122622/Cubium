#include "common/mod/bedrock/addon/engine/quickjs/QuickJSEngine.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSContext.hpp"
#include "common/mod/bedrock/addon/engine/quickjs/QuickJSRuntime.hpp"

#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

QuickJSEngine::QuickJSEngine() = default;

QuickJSEngine::~QuickJSEngine()
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

    ContextConfig config;
    auto context = m_runtime->createContext(config);
    if (!context) {
        spdlog::error("[BedrockAddon] Failed to create QuickJS context");
        return nullptr;
    }

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
