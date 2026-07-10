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

#include "client/sound/SoundHandler.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <set>

using namespace mc::trace;

namespace mc::client::sound {

SoundHandler::SoundHandler(PackRepository& resourcePacks)
    : m_resourcePacks(resourcePacks)
{}

Result<void> SoundHandler::reload()
{
    // 清空现有数据
    clear();
    m_errorCount = 0;
    m_warningCount = 0;

    // 获取启用的资源包（按优先级降序）
    auto packs = m_resourcePacks.getEnabledPacks();

    SoundLoadProgress progress;
    progress.totalPacks = packs.size();

    spdlog::info("[SoundHandler] Starting reload: {} packs to process", packs.size());

    // 已知的命名空间列表（优先加载 minecraft）
    // 大多数资源包只使用 minecraft 命名空间
    static const std::vector<std::string> knownNamespaces = {
        "minecraft"
        // 未来可以添加其他常用命名空间
        // 例如：如果检测到 mod 资源包，动态添加
    };

    // 收集已处理的命名空间，避免重复
    std::set<std::string> processedNamespaces;

    // 遍历资源包（从低优先级到高优先级）
    // 这样高优先级的资源包可以覆盖低优先级的
    for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
        const auto& pack = *it;
        progress.currentPack++;
        progress.currentPackName = pack->name();
        _notifyProgress(progress);

        // 快速路径：直接尝试加载已知命名空间的 sounds.json
        for (const auto& namespace_ : knownNamespaces) {
            // 加载 sounds.json
            auto result = _loadSoundsJson(*pack, namespace_);
            if (result.success()) {
                progress.loadedEvents += result.value();
                _notifyProgress(progress);
                processedNamespaces.insert(namespace_);
            }
        }
    }

    // 如果需要发现自定义命名空间（当已知命名空间没有找到声音时）
    // 使用更高效的方式：只列出顶层目录，而不是递归遍历
    if (progress.loadedEvents == 0) {
        spdlog::info("[SoundHandler] No sounds found in known namespaces, scanning for custom namespaces...");

        for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
            const auto& pack = *it;

            // 尝试直接检查 assets/<namespace>/sounds.json 是否存在
            // 使用 listResources 获取 assets 下的直接子目录
            auto namespacesResult = m_resourcePacks.getResourceNamespaces();
            if (!namespacesResult.success()) {
                continue;
            }

            std::set<std::string> foundNamespaces;
            for (const auto& namespace_ : namespacesResult.value()) {
                if (namespace_.empty() || processedNamespaces.count(namespace_) > 0) {
                    continue;
                }

                foundNamespaces.insert(namespace_);
            }

            // 加载新发现的命名空间
            for (const auto& namespace_ : foundNamespaces) {
                {
                    MC_TRACE_SCOPED_EVENT(
                        TraceEvents.Client.Sound, "SoundHandler_LoadSoundsJson", "phase", "load_sounds_json");
                    auto result = _loadSoundsJson(*pack, namespace_);
                    if (result.success()) {
                        progress.loadedEvents += result.value();
                        _notifyProgress(progress);
                        processedNamespaces.insert(namespace_);
                    }
                }
            }
        }
    }

    progress.totalEvents = m_registry.getSoundEventCount();
    _notifyProgress(progress);

    spdlog::info("SoundHandler: Loaded {} sound events ({} errors, {} warnings)",
        m_registry.getSoundEventCount(),
        m_errorCount,
        m_warningCount);

    return {}; // Result<void> 默认构造表示成功
}

void SoundHandler::clear()
{
    m_registry.clear();
    m_errorCount = 0;
    m_warningCount = 0;
}

const SoundEventDefinition* SoundHandler::getSoundEvent(const ResourceLocation& id) const
{
    return m_registry.getSoundEvent(id);
}

bool SoundHandler::hasSoundEvent(const ResourceLocation& id) const
{
    return m_registry.hasSoundEvent(id);
}

const SoundDefinition* SoundHandler::getRandomSound(const ResourceLocation& id, mc::math::Random& rng) const
{
    const auto* eventDef = m_registry.getSoundEvent(id);
    if (!eventDef) {
        return nullptr;
    }
    return eventDef->selectSound(rng);
}

std::vector<ResourceLocation> SoundHandler::getAllSoundEventIds() const
{
    return m_registry.getAllSoundEventIds();
}

size_t SoundHandler::getSoundEventCount() const
{
    return m_registry.getSoundEventCount();
}

std::vector<ResourceLocation> SoundHandler::getPreloadSounds() const
{
    return m_registry.getPreloadSounds();
}

Result<size_t> SoundHandler::_loadSoundsJson(const IResourcePack& pack, std::string_view namespace_)
{
    // 构建 sounds.json 路径
    std::string jsonPath = std::string(namespace_) + "/sounds.json";

    // 检查文件是否存在
    if (!pack.hasResource(resource::PackType::ClientResources, jsonPath)) {
        // 不是错误，只是该命名空间没有声音定义
        return 0;
    }

    // 读取文件内容
    auto contentResult = pack.readTextResource(resource::PackType::ClientResources, jsonPath);
    if (!contentResult.success()) {
        m_warningCount++;
        spdlog::warn("SoundHandler: Failed to read {}: {}", jsonPath, contentResult.error().message());
        return 0;
    }

    // 解析 JSON
    return _parseSoundsJson(contentResult.value(), namespace_);
}

Result<size_t> SoundHandler::_parseSoundsJson(std::string_view content, std::string_view namespace_)
{
    // 解析 JSON
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(content);
    }
    catch (const nlohmann::json::parse_error& e) {
        m_errorCount++;
        return Error(ErrorCode::ResourceParseError, "Failed to parse sounds.json: " + std::string(e.what()));
    }

    if (!json.is_object()) {
        m_errorCount++;
        return Error(ErrorCode::ResourceParseError, "sounds.json must be an object");
    }

    size_t count = 0;

    // 遍历所有声音事件
    for (auto it = json.begin(); it != json.end(); ++it) {
        const std::string& eventId = it.key();
        const auto& eventJson = it.value();

        // 构建完整的声音事件ID
        ResourceLocation location;
        if (eventId.find(':') != std::string::npos) {
            // 已有命名空间
            location = ResourceLocation::parse(eventId);
        } else {
            // 使用当前命名空间
            location = ResourceLocation(std::string(namespace_), eventId);
        }

        // 解析声音事件定义
        auto result = SoundEventDefinition::parse(location.toString(), eventJson, namespace_);

        if (result.success()) {
            auto& def = result.value();
            def.location = location;
            m_registry.registerSoundEvent(std::move(def));
            count++;
        } else {
            m_warningCount++;
            spdlog::warn("SoundHandler: Failed to parse sound event '{}': {}", eventId, result.error().message());
        }
    }

    return count;
}

void SoundHandler::_notifyProgress(const SoundLoadProgress& progress)
{
    if (m_progressCallback) {
        m_progressCallback(progress);
    }
}

} // namespace mc::client::sound
