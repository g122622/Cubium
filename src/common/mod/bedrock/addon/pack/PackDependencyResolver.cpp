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

#include "common/mod/bedrock/addon/pack/PackDependencyResolver.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"

#include <unordered_map>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

std::string PackDependencyResolver::ResolveResult::toString() const
{
    if (success) {
        return "All dependencies resolved successfully";
    }

    std::string result;
    if (!missingDependencies.empty()) {
        result += "Missing dependencies: ";
        for (size_t i = 0; i < missingDependencies.size(); ++i) {
            if (i > 0) {
                result += ", ";
            }
            result += missingDependencies[i];
        }
    }
    if (!versionMismatches.empty()) {
        if (!result.empty()) {
            result += "; ";
        }
        result += "Version mismatches: ";
        for (size_t i = 0; i < versionMismatches.size(); ++i) {
            if (i > 0) {
                result += ", ";
            }
            result += versionMismatches[i];
        }
    }
    return result;
}

PackDependencyResolver::ResolveResult PackDependencyResolver::resolve(
    const std::vector<std::unique_ptr<BehaviorPack>>& packs)
{

    ResolveResult result;
    result.success = true;

    // 构建UUID到包的映射
    std::unordered_map<std::string, const BehaviorPack*> packByUuid;
    for (const auto& pack : packs) {
        packByUuid[pack->uuid()] = pack.get();
    }

    // 检查每个包的依赖
    for (const auto& pack : packs) {
        const auto& manifest = pack->manifest();

        for (const auto& dep : manifest.dependencies) {
            auto it = packByUuid.find(dep.uuid);
            if (it == packByUuid.end()) {
                // 依赖缺失
                result.missingDependencies.push_back(pack->name() + " -> " + dep.uuid + " (not found)");
                result.success = false;
                spdlog::warn("[BedrockAddon] Missing dependency: {} requires UUID {}", pack->name(), dep.uuid);
                continue;
            }

            // 检查版本兼容性
            const auto* depPack = it->second;
            const auto& depPackVersion = depPack->manifest().header.version;

            if (!depPackVersion.isCompatibleWith(dep.version)) {
                result.versionMismatches.push_back(pack->name() + " requires " + dep.uuid + " v" +
                    dep.version.toString() + " but found v" + depPackVersion.toString());
                result.success = false;
                spdlog::warn("[BedrockAddon] Version mismatch: {} requires {} v{} but found v{}",
                    pack->name(),
                    dep.uuid,
                    dep.version.toString(),
                    depPackVersion.toString());
            }
        }
    }

    if (result.success) {
        spdlog::info("[BedrockAddon] All dependencies resolved successfully");
    }

    return result;
}

} // namespace mc::mod::bedrock::addon
