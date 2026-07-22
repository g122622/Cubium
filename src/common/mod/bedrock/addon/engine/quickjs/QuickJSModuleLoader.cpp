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

        // 相对路径解析：基于导入者路径
        std::string resolved;
        if (name.starts_with("./") || name.starts_with("../")) {
            auto lastSlash = base.rfind('/');
            if (lastSlash != std::string::npos) {
                resolved = base.substr(0, lastSlash + 1) + name;
            } else {
                resolved = name;
            }
        } else {
            resolved = name;
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
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. 查找原生C++模块
    auto nativeIt = m_nativeModules.find(moduleName);
    if (nativeIt != m_nativeModules.end()) {
        JSModuleDef* m =
            JS_NewCModule(ctx, moduleName.c_str(), [](JSContext* ctx, JSModuleDef* m) -> int { return 0; });
        if (!m) {
            spdlog::error("[BedrockAddon] Failed to create native module: {}", moduleName);
            return nullptr;
        }

        if (nativeIt->second(ctx, m) < 0) {
            spdlog::error("[BedrockAddon] Failed to initialize native module: {}", moduleName);
            return nullptr;
        }
        return m;
    }

    // 2. 查找路径别名映射
    std::string resolvedName = moduleName;
    auto aliasIt = m_aliases.find(moduleName);
    if (aliasIt != m_aliases.end()) {
        resolvedName = aliasIt->second;
    }

    // 3. 通过源码提供者加载JS模块
    if (m_sourceProvider) {
        std::string source = m_sourceProvider(resolvedName);
        if (!source.empty()) {
            // 编译JS源码为模块
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
