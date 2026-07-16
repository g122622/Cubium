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

#include "common/world/gen/density/DensityFunctionLoader.hpp"

#include "common/resource/PackType.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/world/gen/density/DensityFunction.hpp"
#include "common/world/gen/density/DensityFunctionRegistry.hpp"
#include "common/world/gen/density/DensityFunctionTypeRegistry.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace mc {
namespace world {
namespace gen {
namespace density {

namespace {

using json = nlohmann::json;

/// 解析会话状态：阶段 A 建的 name→json 表 + 阶段 B 的 memo/visiting。
/// 整个 loadFromDataPackRepository 调用期间存活，递归共享。
struct ResolverState {
    /// 阶段 A：name(RL 字符串) → 原始 JSON（未解析）
    std::unordered_map<std::string, json> rawMap;

    /// 阶段 B：name → 已解析 shared_ptr（memo + 共享子图）
    std::unordered_map<std::string, std::shared_ptr<DensityFunction>> resolved;

    /// 阶段 B：正在解析中的 name（循环引用检测）
    std::unordered_set<std::string> visiting;
};

/// 全局解析会话指针：仅在一次 load 调用期间非空，供工厂的 resolveInline 回调访问。
/// 递归是单线程的（加载在主线程同步进行），无需并发保护。
ResolverState* g_state = nullptr;

/// 前向声明：阶段 B 核心递归（见下方定义）
[[nodiscard]] Result<std::shared_ptr<DensityFunction>> resolveHolderByName(const std::string& name);

/// 从资源路径推导 ResourceLocation（路径格式 <ns>/worldgen/density_function/<path>.json）
ResourceLocation locationFromResourcePath(const std::string& ns, const std::string& directory, const std::string& path)
{
    std::string name = path.substr(directory.length() + 1); // +1 跳过 '/'
    if (name.size() >= 5 && name.substr(name.size() - 5) == ".json") {
        name = name.substr(0, name.size() - 5);
    }
    return ResourceLocation(ns, name);
}

/// 解析一个 JSON 元素为独立 unique_ptr（内联，不入 memo 不共享）。
/// 三态：裸数字→Constant，裸字符串→递归 resolveHolderByName，对象→TypeRegistry::create。
Result<std::unique_ptr<DensityFunction>> resolveElement(const json& element)
{
    if (element.is_number()) {
        return factory::constant(element.get<f64>());
    }
    if (element.is_string()) {
        // Holder 引用：递归解析同名 density_function（带 memo + 循环检测）
        const std::string ref = element.get<std::string>();
        auto holder = resolveHolderByName(ref);
        if (holder.failed()) {
            return holder.error();
        }
        // SharedHolder 让多个引用共享同一 shared_ptr 子图
        return factory::sharedHolder(holder.value());
    }
    if (element.is_object()) {
        if (!element.contains("type") || !element["type"].is_string()) {
            return Error(ErrorCode::InvalidData, "density_function object missing 'type' string field");
        }
        const std::string type = element["type"].get<std::string>();
        ResolveContext ctx;
        ctx.resolveInline = [](const json& e) { return resolveElement(e); };
        return DensityFunctionTypeRegistry::instance().create(type, element, ctx);
    }
    return Error(ErrorCode::InvalidData, "density_function JSON element must be number, string, or object");
}

/// 阶段 B 核心：按 name 递归解析具名 density_function，返回 shared_ptr（memo + 共享子图）。
/// - memo 命中 → 返回已有 shared_ptr
/// - visiting 命中 → 循环引用
/// - 字符串/对象三态分发，子字段字符串引用递归 resolveHolderByName
Result<std::shared_ptr<DensityFunction>> resolveHolderByName(const std::string& name)
{
    if (g_state == nullptr) {
        return Error(ErrorCode::InvalidState, "density_function resolver not active");
    }

    // memo 命中
    auto memoIt = g_state->resolved.find(name);
    if (memoIt != g_state->resolved.end()) {
        return memoIt->second;
    }

    // 循环引用检测
    if (g_state->visiting.contains(name)) {
        return Error(
            ErrorCode::InvalidData, "circular density_function reference: '" + name + "' (chain includes itself)");
    }

    // 阶段 A 表中找不到该 name
    auto rawIt = g_state->rawMap.find(name);
    if (rawIt == g_state->rawMap.end()) {
        return Error(ErrorCode::NotFound, "referenced density_function not found: '" + name + "'");
    }

    g_state->visiting.insert(name);

    auto parsed = resolveElement(rawIt->second);
    g_state->visiting.erase(name);

    if (parsed.failed()) {
        return parsed.error();
    }

    // 存入 memo（shared_ptr），后续任何引用同一 name 都复用此子图
    std::shared_ptr<DensityFunction> shared(parsed.value().release());
    g_state->resolved.emplace(name, shared);
    return shared;
}

} // namespace

Result<std::unique_ptr<DensityFunction>> DensityFunctionLoader::resolveInline(const json& element)
{
    return resolveElement(element);
}

Result<size_t> DensityFunctionLoader::loadFromDataPackRepository(const resource::DataPackRepository& dataPackList)
{
    // 先清空注册表与数据驱动标记，再由数据包注入
    DensityFunctionRegistry::instance().clear();
    DensityFunctionRegistry::instance().markLoadedFromDatapack(false);
    DensityFunctionTypeRegistry::instance().clear();
    initializeBuiltinDensityFunctionTypes();

    ResolverState state;
    g_state = &state;
    size_t loadedCount = 0;

    // ========== 阶段 A：全量建 name→json 表 ==========
    auto namespacesResult = dataPackList.getResourceNamespaces();
    if (!namespacesResult.success()) {
        g_state = nullptr;
        return loadedCount;
    }

    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/worldgen/density_function";
        auto listResult = dataPackList.listResources(directory, ".json");
        if (!listResult.success()) {
            continue;
        }
        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = locationFromResourcePath(namespace_, directory, resourcePath);
            auto readResult = dataPackList.readTextResource(resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read density_function: {}", resourcePath);
                continue;
            }
            json jsonObj;
            try {
                jsonObj = json::parse(readResult.value());
            }
            catch (const json::parse_error& e) {
                spdlog::warn("Failed to parse density_function {}: {}", location.toString(), e.what());
                continue;
            }
            state.rawMap[location.toString()] = std::move(jsonObj);
        }
    }

    // ========== 阶段 B：逐 name 递归解析 + 注册 ==========
    for (const auto& [name, jsonObj] : state.rawMap) {
        auto result = resolveHolderByName(name);
        if (result.failed()) {
            spdlog::warn("Failed to resolve density_function '{}': {}", name, result.error().message());
            continue;
        }
        DensityFunctionRegistry::instance().registerFunction(ResourceLocation::parse(name), result.value());
        ++loadedCount;
    }

    g_state = nullptr;

    DensityFunctionRegistry::instance().markLoadedFromDatapack(true);
    spdlog::info("Loaded {} density functions from datapacks", loadedCount);
    return loadedCount;
}

Result<size_t> DensityFunctionLoader::loadFromResourcePack(const resource::IResourcePack& pack)
{
    ResolverState state;
    g_state = &state;
    size_t loadedCount = 0;

    auto namespacesResult = pack.getResourceNamespaces(resource::PackType::ServerData);
    if (!namespacesResult.success()) {
        g_state = nullptr;
        return loadedCount;
    }

    // 阶段 A
    for (const auto& namespace_ : namespacesResult.value()) {
        std::string directory = namespace_ + "/worldgen/density_function";
        auto listResult = pack.listResources(resource::PackType::ServerData, directory, ".json");
        if (!listResult.success()) {
            continue;
        }
        for (const auto& resourcePath : listResult.value()) {
            ResourceLocation location = locationFromResourcePath(namespace_, directory, resourcePath);
            auto readResult = pack.readTextResource(resource::PackType::ServerData, resourcePath);
            if (!readResult.success()) {
                spdlog::warn("Failed to read density_function: {}", resourcePath);
                continue;
            }
            json jsonObj;
            try {
                jsonObj = json::parse(readResult.value());
            }
            catch (const json::parse_error& e) {
                spdlog::warn("Failed to parse density_function {}: {}", location.toString(), e.what());
                continue;
            }
            state.rawMap[location.toString()] = std::move(jsonObj);
        }
    }

    // 阶段 B
    for (const auto& [name, jsonObj] : state.rawMap) {
        auto result = resolveHolderByName(name);
        if (result.failed()) {
            spdlog::warn("Failed to resolve density_function '{}': {}", name, result.error().message());
            continue;
        }
        DensityFunctionRegistry::instance().registerFunction(ResourceLocation::parse(name), result.value());
        ++loadedCount;
    }

    g_state = nullptr;
    return loadedCount;
}

} // namespace density
} // namespace gen
} // namespace world
} // namespace mc
