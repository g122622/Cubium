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

#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"
#include "common/mod/bedrock/addon/pack/AddonModule.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/PackDependencyResolver.hpp"

#include <algorithm>
#include <filesystem>

#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

Result<void> BehaviorPackList::scanDirectory(const std::string& path)
{
    std::filesystem::path dirPath(path);

    if (!std::filesystem::exists(dirPath)) {
        return Error(ErrorCode::FileNotFound, "Directory not found: " + path);
    }

    if (!std::filesystem::is_directory(dirPath)) {
        return Error(ErrorCode::InvalidArgument, "Path is not a directory: " + path);
    }

    std::lock_guard<std::shared_mutex> lock(m_mutex);

    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::filesystem::path manifestPath = entry.path() / "manifest.json";
        if (!std::filesystem::exists(manifestPath)) {
            // 跳过没有manifest.json的目录
            continue;
        }

        auto result = AddonManifest::loadFromFile(manifestPath.string());
        if (result.failed()) {
            spdlog::warn(
                "[BedrockAddon] Failed to load manifest from {}: {}", manifestPath.string(), result.error().message());
            continue;
        }

        auto pack = std::make_unique<BehaviorPack>(entry.path().string(), std::move(result.value()));
        spdlog::info("[BedrockAddon] Loaded behavior pack: {} ({})", pack->name(), pack->uuid());
        m_packs.push_back(std::move(pack));
    }

    return Result<void>::ok();
}

Result<void> BehaviorPackList::addPack(const std::string& path, bool enabled, i32 priority)
{
    std::filesystem::path packPath(path);

    if (!std::filesystem::exists(packPath)) {
        return Error(ErrorCode::FileNotFound, "Pack directory not found: " + path);
    }

    std::filesystem::path manifestPath = packPath / "manifest.json";
    if (!std::filesystem::exists(manifestPath)) {
        return Error(ErrorCode::FileNotFound, "manifest.json not found in: " + path);
    }

    auto result = AddonManifest::loadFromFile(manifestPath.string());
    if (result.failed()) {
        return Error(result.error().code(), "Failed to load manifest: " + result.error().message());
    }

    std::lock_guard<std::shared_mutex> lock(m_mutex);

    auto pack = std::make_unique<BehaviorPack>(path, std::move(result.value()));
    pack->setEnabled(enabled);
    pack->setPriority(priority);

    spdlog::info("[BedrockAddon] Added behavior pack: {} ({})", pack->name(), pack->uuid());
    m_packs.push_back(std::move(pack));

    return Result<void>::ok();
}

void BehaviorPackList::removePack(const std::string& uuid)
{
    std::lock_guard<std::shared_mutex> lock(m_mutex);

    auto it = std::remove_if(m_packs.begin(), m_packs.end(), [&uuid](const std::unique_ptr<BehaviorPack>& pack) {
        return pack->uuid() == uuid;
    });

    if (it != m_packs.end()) {
        spdlog::info("[BedrockAddon] Removed behavior pack with UUID: {}", uuid);
        m_packs.erase(it, m_packs.end());
    }
}

std::vector<BehaviorPack*> BehaviorPackList::getEnabledPacks()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::vector<BehaviorPack*> enabled;
    for (const auto& pack : m_packs) {
        if (pack->isEnabled()) {
            enabled.push_back(pack.get());
        }
    }

    // 按优先级排序（优先级高的先加载）
    std::sort(enabled.begin(), enabled.end(), [](const BehaviorPack* a, const BehaviorPack* b) {
        return a->priority() > b->priority();
    });

    return enabled;
}

std::vector<const BehaviorPack*> BehaviorPackList::getAllPacks() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::vector<const BehaviorPack*> packs;
    packs.reserve(m_packs.size());
    for (const auto& pack : m_packs) {
        packs.push_back(pack.get());
    }
    return packs;
}

BehaviorPack* BehaviorPackList::getPackByUuid(const std::string& uuid)
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    for (const auto& pack : m_packs) {
        if (pack->uuid() == uuid) {
            return pack.get();
        }
    }
    return nullptr;
}

const BehaviorPack* BehaviorPackList::getPackByUuid(const std::string& uuid) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    for (const auto& pack : m_packs) {
        if (pack->uuid() == uuid) {
            return pack.get();
        }
    }
    return nullptr;
}

Result<void> BehaviorPackList::resolveDependencies()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    auto result = PackDependencyResolver::resolve(m_packs);
    if (!result.success) {
        return Error(ErrorCode::InvalidData, result.toString());
    }

    return Result<void>::ok();
}

std::vector<AddonModule> BehaviorPackList::getScriptModules() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::vector<AddonModule> modules;
    for (const auto& pack : m_packs) {
        if (pack->isEnabled()) {
            auto packModules = pack->manifest().getScriptModules();
            modules.insert(modules.end(), packModules.begin(), packModules.end());
        }
    }
    return modules;
}

size_t BehaviorPackList::size() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_packs.size();
}

bool BehaviorPackList::empty() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_packs.empty();
}

void BehaviorPackList::clear()
{
    std::lock_guard<std::shared_mutex> lock(m_mutex);
    m_packs.clear();
}

} // namespace mc::mod::bedrock::addon
