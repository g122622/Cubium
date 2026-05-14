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

#include "SkinManager.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/parser/SkinMetadataParser.hpp"
#include <spdlog/spdlog.h>

namespace mc::skin {

SkinManager::SkinManager(const std::string& cacheDir)
    : m_cacheDir(cacheDir)
    , m_cache(std::make_unique<SkinCache>(cacheDir))
    , m_defaultSkinProvider(std::make_unique<DefaultSkinProvider>())
{}

SkinManager::~SkinManager()
{
    shutdown();
}

Result<void> SkinManager::initialize()
{
    if (m_initialized.load()) {
        return {};
    }

    // 初始化缓存
    auto cacheResult = m_cache->initialize();
    if (!cacheResult.success()) {
        spdlog::warn("SkinManager: Failed to initialize cache: {}", cacheResult.error().toString());
        // 缓存初始化失败不是致命错误，继续
    }

    // 初始化默认皮肤提供者
    auto defaultResult = m_defaultSkinProvider->initialize();
    if (!defaultResult.success()) {
        spdlog::warn("SkinManager: Failed to initialize default skin provider: {}", defaultResult.error().toString());
        // 默认皮肤初始化失败也不是致命错误
    }

    m_initialized.store(true);
    spdlog::info("SkinManager initialized with cache dir: {}", m_cacheDir);
    return {};
}

void SkinManager::shutdown()
{
    if (!m_initialized.load()) {
        return;
    }

    m_shuttingDown.store(true);

    // 清理玩家信息
    {
        std::lock_guard<std::mutex> lock(m_playerInfosMutex);
        m_playerInfos.clear();
    }

    // 关闭缓存
    m_cache->shutdown();

    m_initialized.store(false);
    m_shuttingDown.store(false);
    spdlog::info("SkinManager shutdown");
}

std::shared_ptr<PlayerSkinInfo> SkinManager::getOrCreatePlayerInfo(const GameProfile& profile)
{
    std::string key = uuidToKey(profile.uuid());

    std::shared_ptr<PlayerSkinInfo> info;
    bool wasNew = false;

    {
        std::lock_guard<std::mutex> lock(m_playerInfosMutex);
        auto it = m_playerInfos.find(key);
        if (it != m_playerInfos.end()) {
            return it->second;
        }
        info = std::make_shared<PlayerSkinInfo>(profile);
        m_playerInfos.emplace(key, info);
        wasNew = true;
    }

    if (wasNew) {
        if (profile.hasTextures()) {
            loadSkin(profile);
        } else {
            useDefaultSkin(info);
        }
    }

    return info;
}

std::shared_ptr<PlayerSkinInfo> SkinManager::getPlayerInfo(const std::array<u8, 16>& uuid) const
{
    std::string key = uuidToKey(uuid);
    std::lock_guard<std::mutex> lock(m_playerInfosMutex);
    auto it = m_playerInfos.find(key);
    return it != m_playerInfos.end() ? it->second : nullptr;
}

std::shared_ptr<PlayerSkinInfo> SkinManager::getPlayerInfo(const std::string& uuidStr) const
{
    std::lock_guard<std::mutex> lock(m_playerInfosMutex);
    auto it = m_playerInfos.find(uuidStr);
    return it != m_playerInfos.end() ? it->second : nullptr;
}

void SkinManager::removePlayerInfo(const std::array<u8, 16>& uuid)
{
    std::string key = uuidToKey(uuid);
    std::lock_guard<std::mutex> lock(m_playerInfosMutex);
    m_playerInfos.erase(key);
    spdlog::debug("SkinManager: Removed player info for {}", key);
}

void SkinManager::clearAllPlayerInfos()
{
    std::lock_guard<std::mutex> lock(m_playerInfosMutex);
    m_playerInfos.clear();
    spdlog::info("SkinManager: Cleared all player infos");
}

size_t SkinManager::playerCount() const
{
    std::lock_guard<std::mutex> lock(m_playerInfosMutex);
    return m_playerInfos.size();
}

void SkinManager::loadSkin(const GameProfile& profile, const SkinLoadCallbacks& callbacks)
{
    if (m_shuttingDown.load()) {
        return;
    }

    auto info = getPlayerInfo(profile.uuid());
    if (!info) {
        info = getOrCreatePlayerInfo(profile);
    }

    // 检查是否已在加载
    if (info->isLoading()) {
        spdlog::debug("SkinManager: Skin already loading for {}", profile.name());
        return;
    }

    // 标记为加载中
    info->setLoadState(SkinLoadState::Loading);

    // 从 textures 属性加载
    loadFromTextures(profile, info, callbacks);
}

bool SkinManager::isSkinLoaded(const std::array<u8, 16>& uuid) const
{
    auto info = getPlayerInfo(uuid);
    return info && info->isLoaded();
}

ResourceLocation SkinManager::getDefaultSkin(const std::array<u8, 16>& uuid) const
{
    return m_defaultSkinProvider->getDefaultSkin(uuid);
}

SkinType SkinManager::getDefaultSkinType(const std::array<u8, 16>& uuid) const
{
    return m_defaultSkinProvider->getDefaultSkinType(uuid);
}

bool SkinManager::loadFromCache(const SkinTextures& textures, std::shared_ptr<PlayerSkinInfo> info)
{
    // 尝试从缓存加载皮肤
    if (textures.hasSkin() && textures.skinHash().has_value()) {
        const std::string& hash = *textures.skinHash();
        if (m_cache->hasSkin(hash)) {
            auto location = m_cache->generateSkinLocation(hash);
            info->setSkinLocation(location);
            info->setSkinType(textures.skinType());
            info->setLoadState(SkinLoadState::Loaded);
            spdlog::debug("SkinManager: Loaded skin from cache for hash {}", hash);
            return true;
        }
    }

    // 尝试从缓存加载披风
    if (textures.hasCape() && textures.capeHash().has_value()) {
        const std::string& hash = *textures.capeHash();
        if (m_cache->hasCape(hash)) {
            auto location = m_cache->generateCapeLocation(hash);
            info->setCapeLocation(location);
        }
    }

    return false;
}

void SkinManager::loadFromTextures(
    const GameProfile& profile, std::shared_ptr<PlayerSkinInfo> info, const SkinLoadCallbacks& callbacks)
{
    const GameProfileProperty* texturesProp = profile.getTexturesProperty();
    if (!texturesProp) {
        useDefaultSkin(info);
        if (callbacks.onSkinFailed) {
            callbacks.onSkinFailed(profile.uuid(), "No textures property");
        }
        return;
    }

    // 解析 textures 属性
    auto parseResult = SkinMetadataParser::parse(*texturesProp);
    if (!parseResult.success()) {
        spdlog::warn(
            "SkinManager: Failed to parse textures for {}: {}", profile.name(), parseResult.error().toString());
        useDefaultSkin(info);
        if (callbacks.onSkinFailed) {
            callbacks.onSkinFailed(profile.uuid(), parseResult.error().toString());
        }
        return;
    }

    SkinTextures textures = parseResult.value();

    // 设置皮肤类型
    if (textures.skinType() == SkinType::Default && textures.skinUrl().has_value()) {
        // 如果 URL 没有指定类型，使用默认
        textures.setSkinType(getDefaultSkinType(profile.uuid()));
    }

    // 尝试从缓存加载
    if (loadFromCache(textures, info)) {
        if (callbacks.onSkinLoaded) {
            callbacks.onSkinLoaded(profile.uuid());
        }
        return;
    }

    // TODO: 实现异步下载
    // 当前简化实现：使用默认皮肤
    spdlog::info("SkinManager: Skin not in cache, would download: {}", textures.skinUrl().value_or("(none)"));

    // 暂时使用默认皮肤
    useDefaultSkin(info);

    if (callbacks.onSkinLoaded) {
        callbacks.onSkinLoaded(profile.uuid());
    }
}

void SkinManager::useDefaultSkin(std::shared_ptr<PlayerSkinInfo> info)
{
    ResourceLocation defaultSkin = getDefaultSkin(info->uuid());
    info->setSkinLocation(defaultSkin);
    info->setSkinType(getDefaultSkinType(info->uuid()));
    info->setLoadState(SkinLoadState::UsingDefault);

    spdlog::debug("SkinManager: Using default skin for {}", info->profile().uuidToString());
}

std::string SkinManager::uuidToKey(const std::array<u8, 16>& uuid)
{
    return GameProfile(uuid, "").uuidToStringNoDashes();
}

} // namespace mc::skin
