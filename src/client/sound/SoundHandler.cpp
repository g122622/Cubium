#include "client/sound/SoundHandler.hpp"
#include "common/util/assert/AssertAll.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::sound {

SoundHandler::SoundHandler(ResourcePackList& resourcePacks)
    : m_resourcePacks(resourcePacks)
{
}

Result<void> SoundHandler::reload() {
    // 清空现有数据
    clear();
    m_errorCount = 0;
    m_warningCount = 0;

    // 获取启用的资源包（按优先级降序）
    auto packs = m_resourcePacks.getEnabledPacks();

    SoundLoadProgress progress;
    progress.totalPacks = packs.size();

    // 遍历资源包（从低优先级到高优先级）
    // 这样高优先级的资源包可以覆盖低优先级的
    for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
        const auto& pack = *it;
        progress.currentPack++;
        progress.currentPackName = pack->name();
        notifyProgress(progress);

        // 获取所有命名空间
        // 通常只需要 minecraft 命名空间，但也支持自定义命名空间
        auto namespacesResult = m_resourcePacks.listResources("assets", "");
        if (!namespacesResult.success()) {
            continue;
        }

        // 从每个命名空间加载 sounds.json
        for (const auto& nsPath : namespacesResult.value()) {
            // 提取命名空间名称
            // 格式: assets/<namespace>/...
            String namespace_;
            if (nsPath.size() > 7 && nsPath.substr(0, 7) == "assets/") {
                auto slashPos = nsPath.find('/', 7);
                if (slashPos != String::npos) {
                    namespace_ = nsPath.substr(7, slashPos - 7);
                } else {
                    namespace_ = nsPath.substr(7);
                }
            } else {
                continue;
            }

            if (namespace_.empty()) {
                continue;
            }

            // 加载 sounds.json
            auto result = loadSoundsJson(*pack, namespace_);
            if (result.success()) {
                progress.loadedEvents += result.value();
                notifyProgress(progress);
            }
        }
    }

    progress.totalEvents = m_registry.getSoundEventCount();
    notifyProgress(progress);

    spdlog::info("SoundHandler: Loaded {} sound events ({} errors, {} warnings)",
                 m_registry.getSoundEventCount(), m_errorCount, m_warningCount);

    return {};  // Result<void> 默认构造表示成功
}

void SoundHandler::clear() {
    m_registry.clear();
    m_errorCount = 0;
    m_warningCount = 0;
}

const SoundEventDefinition* SoundHandler::getSoundEvent(
    const ResourceLocation& id
) const {
    return m_registry.getSoundEvent(id);
}

bool SoundHandler::hasSoundEvent(const ResourceLocation& id) const {
    return m_registry.hasSoundEvent(id);
}

const SoundDefinition* SoundHandler::getRandomSound(
    const ResourceLocation& id,
    mc::math::Random& rng
) const {
    const auto* eventDef = m_registry.getSoundEvent(id);
    if (!eventDef) {
        return nullptr;
    }
    return eventDef->selectSound(rng);
}

std::vector<ResourceLocation> SoundHandler::getAllSoundEventIds() const {
    return m_registry.getAllSoundEventIds();
}

size_t SoundHandler::getSoundEventCount() const {
    return m_registry.getSoundEventCount();
}

std::vector<ResourceLocation> SoundHandler::getPreloadSounds() const {
    return m_registry.getPreloadSounds();
}

Result<size_t> SoundHandler::loadSoundsJson(
    const IResourcePack& pack,
    StringView namespace_
) {
    // 构建 sounds.json 路径
    String jsonPath = "assets/" + String(namespace_) + "/sounds.json";

    // 检查文件是否存在
    if (!pack.hasResource(jsonPath)) {
        // 不是错误，只是该命名空间没有声音定义
        return 0;
    }

    // 读取文件内容
    auto contentResult = pack.readTextResource(jsonPath);
    if (!contentResult.success()) {
        m_warningCount++;
        spdlog::warn("SoundHandler: Failed to read {}: {}",
                     jsonPath, contentResult.error().message());
        return 0;
    }

    // 解析 JSON
    return parseSoundsJson(contentResult.value(), namespace_);
}

Result<size_t> SoundHandler::parseSoundsJson(
    StringView content,
    StringView namespace_
) {
    // 解析 JSON
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(content);
    } catch (const nlohmann::json::parse_error& e) {
        m_errorCount++;
        return Error(ErrorCode::ResourceParseError,
                     "Failed to parse sounds.json: " + String(e.what()));
    }

    if (!json.is_object()) {
        m_errorCount++;
        return Error(ErrorCode::ResourceParseError,
                     "sounds.json must be an object");
    }

    size_t count = 0;

    // 遍历所有声音事件
    for (auto it = json.begin(); it != json.end(); ++it) {
        const String& eventId = it.key();
        const auto& eventJson = it.value();

        // 构建完整的声音事件ID
        ResourceLocation location;
        if (eventId.find(':') != String::npos) {
            // 已有命名空间
            location = ResourceLocation::parse(eventId);
        } else {
            // 使用当前命名空间
            location = ResourceLocation(String(namespace_), eventId);
        }

        // 解析声音事件定义
        auto result = SoundEventDefinition::parse(
            location.toString(),
            eventJson,
            namespace_
        );

        if (result.success()) {
            auto& def = result.value();
            def.location = location;
            m_registry.registerSoundEvent(std::move(def));
            count++;
        } else {
            m_warningCount++;
            spdlog::warn("SoundHandler: Failed to parse sound event '{}': {}",
                         eventId, result.error().message());
        }
    }

    return count;
}

void SoundHandler::notifyProgress(const SoundLoadProgress& progress) {
    if (m_progressCallback) {
        m_progressCallback(progress);
    }
}

} // namespace mc::client::sound
