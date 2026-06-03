#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>

struct JSContext;
struct JSModuleDef;
struct JSRuntime;

namespace mc::mod::bedrock::addon {

/**
 * @brief QuickJS模块加载器
 *
 * 处理ES6模块的路径规范化和源码加载。
 * 当JS代码使用import语句时，QuickJS通过此加载器解析和加载模块。
 *
 * 使用方式：
 * 1. 在JS_SetModuleLoaderFunc中注册静态回调
 * 2. 通过setModuleSourceProvider注册源码提供者
 * 3. JS引擎遇到import时自动调用moduleNormalize和moduleLoader
 */
class QuickJSModuleLoader {
public:
    /**
     * @brief 模块源码提供者回调类型
     *
     * @param moduleName 模块名（规范化后）
     * @return 模块源码，模块不存在时返回空字符串
     */
    using ModuleSourceProvider = std::function<std::string(const std::string& moduleName)>;

    /**
     * @brief 模块路径规范化回调（QuickJS C API签名）
     *
     * 将相对模块路径解析为绝对路径。
     * 当module_normalize为NULL时，QuickJS使用默认的文件路径规范化。
     *
     * @param ctx JS上下文
     * @param module_base_name 导入者的基础路径
     * @param module_name 被导入的模块名
     * @param opaque 用户数据指针（QuickJSModuleLoader实例）
     * @return 规范化后的模块名（需用js_malloc分配），失败返回NULL
     */
    static char* moduleNormalize(JSContext* ctx, const char* module_base_name,
                                 const char* module_name, void* opaque);

    /**
     * @brief 模块加载回调（QuickJS C API签名）
     *
     * 根据模块名加载源码并编译为JSModuleDef。
     *
     * @param ctx JS上下文
     * @param module_name 规范化后的模块名
     * @param opaque 用户数据指针（QuickJSModuleLoader实例）
     * @return 编译后的模块定义，失败返回NULL
     */
    static JSModuleDef* moduleLoader(JSContext* ctx, const char* module_name, void* opaque);

    /**
     * @brief 注册原生C++模块
     *
     * 注册一个C++模块，当JS代码import此模块名时，
     * 将使用注册的初始化函数创建模块导出。
     *
     * @param name 模块名，如 "@minecraft/server"
     * @param initFunc 模块初始化函数，接收JSContext和JSModuleDef，
     *                 返回0表示成功，-1表示失败
     * @return 是否注册成功
     */
    bool registerNativeModule(const std::string& name,
                              std::function<int(JSContext*, JSModuleDef*)> initFunc);

    /**
     * @brief 注册模块源码提供者
     *
     * @param provider 源码提供者回调
     */
    void setModuleSourceProvider(ModuleSourceProvider provider);

    /**
     * @brief 添加模块路径映射
     *
     * 将模块名映射到文件路径，用于解析import语句。
     *
     * @param alias 模块别名，如 "my-module"
     * @param path 实际路径，如 "./scripts/my-module.js"
     */
    void addModuleAlias(const std::string& alias, const std::string& path);

private:
    /**
     * @brief 内部模块加载实现
     *
     * 先查找原生C++模块，再查找源码提供者，最后查找路径映射。
     */
    JSModuleDef* loadModule(JSContext* ctx, const std::string& moduleName);

    std::unordered_map<std::string, std::function<int(JSContext*, JSModuleDef*)>> m_nativeModules;
    ModuleSourceProvider m_sourceProvider;
    std::unordered_map<std::string, std::string> m_aliases;
    std::mutex m_mutex;
};

} // namespace mc::mod::bedrock::addon
