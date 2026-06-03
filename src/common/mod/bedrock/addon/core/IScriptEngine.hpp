#pragma once

#include "common/mod/bedrock/addon/core/Capabilities.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/core/ScriptData.hpp"
#include "common/mod/bedrock/addon/core/ScriptException.hpp"
#include "common/mod/bedrock/addon/core/ScriptResult.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

class IScriptRuntime;
class IModuleBindingFactory;
class IScriptContext;

/**
 * @brief 脚本日志打印接口
 *
 * 脚本引擎通过此接口输出日志信息
 */
class IScriptPrinter {
public:
    virtual ~IScriptPrinter() = default;
    virtual void onInfo(const std::string& message) = 0;
    virtual void onWarn(const std::string& message) = 0;
    virtual void onError(const std::string& message) = 0;
    virtual void onException(const ScriptException& exception) = 0;
};

/**
 * @brief 依赖加载器接口
 *
 * 脚本引擎通过此接口加载模块源码
 */
class IDependencyLoader {
public:
    virtual ~IDependencyLoader() = default;

    /**
     * @brief 加载指定模块的脚本源码
     *
     * @param moduleName 模块名，如 "@minecraft/server"
     * @return 加载的脚本数据，模块不存在时返回std::nullopt
     */
    [[nodiscard]] virtual std::optional<ScriptData> loadScript(const std::string& moduleName) = 0;
};

/**
 * @brief 脚本引擎抽象接口
 *
 * 管理脚本运行时和模块绑定工厂。
 * 每个服务器实例创建一个IScriptEngine。
 *
 * @note 引擎负责创建IScriptRuntime和管理模块绑定。
 *       具体的插件运行在IScriptContext中，由IScriptRuntime创建。
 */
class IScriptEngine {
public:
    virtual ~IScriptEngine() = default;

    /**
     * @brief 初始化脚本引擎
     *
     * 创建运行时、注册内置模块。
     * @return 初始化是否成功
     */
    [[nodiscard]] virtual bool initialize() = 0;

    /**
     * @brief 关闭脚本引擎
     *
     * 销毁所有上下文、释放运行时资源。
     */
    virtual void shutdown() = 0;

    /**
     * @brief 获取底层运行时
     */
    [[nodiscard]] virtual IScriptRuntime& runtime() = 0;

    /**
     * @brief 添加模块绑定工厂
     *
     * 在初始化时调用，注册如@minecraft/server等模块的绑定工厂。
     */
    virtual void addModuleFactory(std::unique_ptr<IModuleBindingFactory> factory) = 0;

    /**
     * @brief 查找模块绑定工厂
     *
     * @param name 模块名
     * @return 工厂指针，未找到返回nullptr
     */
    [[nodiscard]] virtual IModuleBindingFactory* findModuleFactory(const std::string& name) const = 0;

    /**
     * @brief 为插件创建脚本上下文
     *
     * @param descriptor 插件的模块描述符
     * @param dependencies 模块依赖列表
     * @param loader 依赖加载器（用于加载模块源码）
     * @param printer 日志打印机
     * @return 新创建的脚本上下文
     */
    [[nodiscard]] virtual std::unique_ptr<IScriptContext> createContext(const ModuleDescriptor& descriptor,
        const std::vector<ModuleDependency>& dependencies,
        IDependencyLoader& loader,
        IScriptPrinter& printer) = 0;

    /**
     * @brief 检查引擎是否已初始化
     */
    [[nodiscard]] virtual bool isInitialized() const = 0;
};

/**
 * @brief 创建默认脚本引擎实例
 *
 * 工厂函数，返回当前配置的引擎实现（如QuickJS）。
 * 调用方仅依赖IScriptEngine接口，无需了解具体引擎类型。
 *
 * @return 脚本引擎实例
 */
[[nodiscard]] std::unique_ptr<IScriptEngine> createScriptEngine();

} // namespace mc::mod::bedrock::addon
