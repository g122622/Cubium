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

#include <cstddef>
#include <cstring>
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
    // 对于原生模块（以@开头的），直接返回模块名
    if (module_name && module_name[0] == '@') {
        auto* rt = JS_GetRuntime(ctx);
        size_t len = strlen(module_name);
        char* result = static_cast<char*>(js_malloc(ctx, len + 1));
        if (result) {
            std::memcpy(result, module_name, len + 1);
        }
        return result;
    }

    // 对于相对路径导入，拼接基础路径和模块路径
    if (module_base_name && module_name) {
        std::string base(module_base_name);
        std::string name(module_name);

        // 如果模块名已经是绝对路径或原生模块，直接返回
        if (name.starts_with('@') || name.starts_with('/') || name.starts_with("file://")) {
            auto* rt = JS_GetRuntime(ctx);
            char* result = static_cast<char*>(js_malloc(ctx, name.size() + 1));
            if (result) {
                std::memcpy(result, name.c_str(), name.size() + 1);
            }
            return result;
        }

        // 路径解析：基于导入者（base）的目录拼接。./、../ 与 bare specifier（如 "Utilities.js"）
        // 均按相对 base 处理——基岩行为包常用 bare specifier 简写"同目录模块"（如 ChallengeTests.js
        // 里 `import { Utilities } from "Utilities.js"`），不按 ES 规范报错而按同目录解析。
        // base 目录分隔符可能是 '/'（filePath 拼接）或 '\'（Windows pack path），取最后出现的任一。
        std::string resolved;
        auto lastSep = base.find_last_of("/\\");
        if (name.starts_with("./") || name.starts_with("../")) {
            if (lastSep != std::string::npos) {
                resolved = base.substr(0, lastSep + 1) + name;
            } else {
                resolved = name;
            }
        } else {
            // bare specifier：基于 base 目录解析（同目录），保留 name 中的相对前缀已在上分支处理。
            if (lastSep != std::string::npos) {
                resolved = base.substr(0, lastSep + 1) + name;
            } else {
                resolved = name;
            }
        }

        char* result = static_cast<char*>(js_malloc(ctx, resolved.size() + 1));
        if (result) {
            std::memcpy(result, resolved.c_str(), resolved.size() + 1);
        }
        return result;
    }

    // 默认行为：直接复制模块名
    size_t len = module_name ? strlen(module_name) : 0;
    char* result = static_cast<char*>(js_malloc(ctx, len + 1));
    if (result) {
        if (module_name) {
            std::memcpy(result, module_name, len + 1);
        } else {
            result[0] = '\0';
        }
    }
    return result;
}

JSModuleDef* QuickJSModuleLoader::moduleLoader(JSContext* ctx, const char* module_name, void* opaque)
{
    if (!opaque) {
        JS_ThrowReferenceError(ctx, "Module loader not configured");
        return nullptr;
    }

    auto* loader = static_cast<QuickJSModuleLoader*>(opaque);
    std::string name(module_name ? module_name : "");
    return loader->_loadModule(ctx, name);
}

bool QuickJSModuleLoader::registerNativeModule(
    const std::string& name, std::function<int(JSContext*, JSModuleDef*)> initFunc)
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
    std::function<int(JSContext*, JSModuleDef*)> nativeInit;
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
            JS_NewCModule(ctx, moduleName.c_str(), [](JSContext* ctx, JSModuleDef* m) -> int { return 0; });
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
