#pragma once

#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/binding/IModuleBindingFactory.hpp"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

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
    ~QuickJSEngine() override;

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
    [[nodiscard]] std::unique_ptr<IScriptContext> createContext(
        const ModuleDescriptor& descriptor,
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
