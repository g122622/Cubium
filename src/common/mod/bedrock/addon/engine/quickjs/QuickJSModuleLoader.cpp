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

#include "common/mod/bedrock/addon/engine/quickjs/QuickJSModuleLoader.hpp"

#include "common/mod/bedrock/addon/engine/quickjs/QuickJSBindingContext.hpp"

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <quickjs.h>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

char* QuickJSModuleLoader::moduleNormalize(
    JSContext* ctx, const char* module_base_name, const char* module_name, void* opaque)
{
    if (module_name == nullptr) {
        JS_ThrowReferenceError(ctx, "Module name is missing");
        return nullptr;
    }
    std::string normalized(module_name);
    if (!normalized.starts_with('@')) {
        const std::filesystem::path name(normalized);
        const std::filesystem::path base(module_base_name != nullptr ? module_base_name : "");
        auto path = (name.is_absolute() ? name : base.parent_path() / name).lexically_normal();
        if (!path.has_extension()) {
            path += ".js";
        }
        normalized = path.generic_string();
    }
    auto* result = static_cast<char*>(js_malloc(ctx, normalized.size() + 1));
    if (result != nullptr) {
        std::memcpy(result, normalized.c_str(), normalized.size() + 1);
    }
    return result;
}

JSModuleDef* QuickJSModuleLoader::moduleLoader(JSContext* ctx, const char* module_name, void* opaque)
{
    auto* bindings = static_cast<QuickJSBindingContext*>(JS_GetContextOpaque(ctx));
    if (bindings == nullptr || bindings->moduleLoader() == nullptr) {
        JS_ThrowReferenceError(ctx, "Module loader is not configured for this context");
        return nullptr;
    }
    return bindings->moduleLoader()->_loadModule(ctx, module_name != nullptr ? module_name : "");
}

bool QuickJSModuleLoader::registerNativeModule(
    const std::string& name, std::function<i32(JSContext*, JSModuleDef*)> initFunc)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nativeModules[name] = std::move(initFunc);
    spdlog::info("[BedrockAddon] Registered native module: {}", name);
    return true;
}

void QuickJSModuleLoader::setModuleSourceProvider(ModuleSourceProvider provider)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sourceProvider = std::move(provider);
}

void QuickJSModuleLoader::addModuleAlias(const std::string& alias, const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aliases[alias] = path;
}

JSModuleDef* QuickJSModuleLoader::_loadModule(JSContext* ctx, const std::string& moduleName)
{
    // 锁仅保护成员查找（m_nativeModules/m_aliases/m_sourceProvider 的快照）。
    // 关键：JS_Eval(COMPILE_ONLY) 与 provider 调用必须在锁外执行——编译模块时 QuickJS 会递归
    // 触发 import → moduleLoader → _loadModule，若持锁递归同线程二次加锁 std::mutex 即死锁
    // （resource deadlock would occur）。故锁内只拷贝出 provider/别名结果，锁外编译。
    ModuleSourceProvider provider;
    std::string resolvedName = moduleName;
    bool isNative = false;
    std::function<i32(JSContext*, JSModuleDef*)> nativeInit;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 1. 查找原生C++模块
        auto nativeIt = m_nativeModules.find(moduleName);
        if (nativeIt != m_nativeModules.end()) {
            isNative = true;
            nativeInit = nativeIt->second;
        } else {
            // 2. 查找路径别名映射
            auto aliasIt = m_aliases.find(moduleName);
            if (aliasIt != m_aliases.end()) {
                resolvedName = aliasIt->second;
            }
            // 3. 拷贝源码提供者（std::function 拷贝安全）
            provider = m_sourceProvider;
        }
    }

    if (isNative) {
        JSModuleDef* m =
            JS_NewCModule(ctx, moduleName.c_str(), [](JSContext* ctx, JSModuleDef* m) -> i32 { return 0; });
        if (!m) {
            spdlog::error("[BedrockAddon] Failed to create native module: {}", moduleName);
            return nullptr;
        }

        if (nativeInit(ctx, m) < 0) {
            spdlog::error("[BedrockAddon] Failed to initialize native module: {}", moduleName);
            return nullptr;
        }
        return m;
    }

    // 通过源码提供者加载JS模块（锁外，允许递归 import 触发 _loadModule）
    if (provider) {
        std::string source = provider(resolvedName);
        if (!source.empty()) {
            // 编译JS源码为模块（COMPILE_ONLY：编译期递归解析 import，运行期实例化由 JS_Eval 调用方驱动）
            JSValue func = JS_Eval(ctx,
                source.c_str(),
                source.size(),
                resolvedName.c_str(),
                JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
            if (JS_IsException(func)) {
                JS_FreeValue(ctx, func);
                spdlog::error("[BedrockAddon] Failed to compile module: {}", resolvedName);
                return nullptr;
            }

            JSModuleDef* m = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func));
            return m;
        }
    }

    // 模块未找到
    JS_ThrowReferenceError(ctx, "Module not found: %s", moduleName.c_str());
    spdlog::error("[BedrockAddon] Module not found: {}", moduleName);
    return nullptr;
}

} // namespace mc::mod::bedrock::addon
