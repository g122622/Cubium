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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/loader/FileSkinLoader.hpp"
#include "common/skin/loader/HttpSkinLoader.hpp"
#include "common/skin/loader/SkinLoader.hpp"
#include "common/skin/manager/DefaultSkinProvider.hpp"
#include "common/skin/manager/SkinCache.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"
#include "common/skin/parser/SkinMetadataParser.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::skin {

SkinManager::SkinManager(const std::string& cacheDir)
    : m_cacheDir(cacheDir)
    , m_cache(std::make_unique<SkinCache>(cacheDir))
    , m_defaultSkinProvider(std::make_unique<DefaultSkinProvider>())
    , m_fileLoader(std::make_unique<FileSkinLoader>())
    , m_httpLoader(std::make_unique<HttpSkinLoader>())
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

    // 初始化加载器
    auto fileResult = m_fileLoader->initialize();
    if (!fileResult.success()) {
        spdlog::warn("SkinManager: Failed to initialize FileSkinLoader: {}", fileResult.error().toString());
    }
    auto httpResult = m_httpLoader->initialize();
    if (!httpResult.success()) {
        spdlog::warn("SkinManager: Failed to initialize HttpSkinLoader: {}", httpResult.error().toString());
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

    // 关闭加载器（等待所有在途异步任务完成）
    m_httpLoader->shutdown();
    m_fileLoader->shutdown();

    // 关闭缓存
    m_cache->shutdown();

    m_initialized.store(false);
    m_shuttingDown.store(false);
    spdlog::info("SkinManager shutdown");
}

std::shared_ptr<PlayerSkinInfo> SkinManager::getOrCreatePlayerInfo(const GameProfile& profile)
{
    std::string key = _uuidToKey(profile.uuid());

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
            _useDefaultSkin(info);
        }
    }

    return info;
}

std::shared_ptr<PlayerSkinInfo> SkinManager::getPlayerInfo(const std::array<u8, 16>& uuid) const
{
    std::string key = _uuidToKey(uuid);
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
    std::string key = _uuidToKey(uuid);
    std::lock_guard<std::mutex> lock(m_playerInfosMutex);
    m_playerInfos.erase(key);
    spdlog::info("SkinManager: Removed player info for {}", key);
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
        return;
    }

    // 标记为加载中
    info->setLoadState(SkinLoadState::Loading);

    // 从 textures 属性加载
    _loadFromTextures(profile, info, callbacks);
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

bool SkinManager::_loadFromCache(const SkinTextures& textures, std::shared_ptr<PlayerSkinInfo> info)
{
    // 尝试从缓存加载皮肤
    if (textures.hasSkin() && textures.skinHash().has_value()) {
        const std::string& hash = *textures.skinHash();
        if (m_cache->hasSkin(hash)) {
            auto location = m_cache->generateSkinLocation(hash);
            info->setSkinLocation(location);
            info->setSkinType(textures.skinType());
            info->setLoadState(SkinLoadState::Loaded);
            spdlog::info("SkinManager: Loaded skin from cache for hash {}", hash);
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

void SkinManager::_loadFromTextures(
    const GameProfile& profile, std::shared_ptr<PlayerSkinInfo> info, const SkinLoadCallbacks& callbacks)
{
    const GameProfileProperty* texturesProp = profile.getTexturesProperty();
    if (!texturesProp) {
        _useDefaultSkin(info);
        if (callbacks.onSkinFailed) {
            callbacks.onSkinFailed(profile.uuid(), "No textures property");
        }
        return;
    }

    // 验证 textures 属性签名状态
    // 对应 MC Java 版 SkinManager 中对 SignatureState 的处理：
    // - SIGNED: 皮肤来源可信，正常使用
    // - UNSIGNED: 无签名（离线模式或会话服务未提供签名），允许使用
    // - INVALID: 签名验证失败，记录警告日志，但仍允许使用（降级处理）
    SignatureState sigState = SkinMetadataParser::getSignatureState(*texturesProp);
    info->setSignatureState(sigState);
    if (sigState == SignatureState::Invalid) {
        spdlog::warn(
            "SkinManager: Profile contained invalid signature for textures property (profile: {})", profile.name());
    }

    // 解析 textures 属性
    auto parseResult = SkinMetadataParser::parse(*texturesProp);
    if (!parseResult.success()) {
        spdlog::warn(
            "SkinManager: Failed to parse textures for {}: {}", profile.name(), parseResult.error().toString());
        _useDefaultSkin(info);
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
    if (_loadFromCache(textures, info)) {
        if (callbacks.onSkinLoaded) {
            callbacks.onSkinLoaded(profile.uuid());
        }
        return;
    }

    // 缓存未命中：异步下载皮肤
    // 皮肤 URL 形如 http://textures.minecraft.net/texture/<hash>，由 HttpSkinLoader 处理
    // 若 URL 不存在或加载失败，回退到默认皮肤
    if (textures.skinUrl().has_value()) {
        const std::string& skinUrl = *textures.skinUrl();
        std::string skinHash = textures.skinHash().value_or("");

        // 捕获回调所需的上下文（裸指针，保证对象生命周期由 SkinManager 自身管理）
        auto infoPtr = info;
        auto cache = m_cache.get();
        auto defaultProvider = m_defaultSkinProvider.get();
        auto uuid = profile.uuid();
        auto skinType = textures.skinType();
        auto onLoaded = callbacks.onSkinLoaded;
        auto onFailed = callbacks.onSkinFailed;
        auto httpLoader = m_httpLoader.get();
        auto shuttingDown = &m_shuttingDown;

        auto onSkinDownloaded =
            [infoPtr, cache, defaultProvider, uuid, skinType, skinHash, onLoaded, onFailed, shuttingDown](
                Result<SkinLoadResult> result) {
                if (result.success()) {
                    // 移动出结果避免多次访问
                    SkinLoadResult loadResult = std::move(result.value());
                    std::string effectiveHash = skinHash.empty() ? loadResult.hash : skinHash;

                    // 保存到缓存
                    auto saveResult = cache->saveSkin(effectiveHash, loadResult.pngData);
                    if (saveResult.success()) {
                        auto location = cache->generateSkinLocation(effectiveHash);
                        infoPtr->setSkinLocation(location);
                        infoPtr->setSkinType(skinType);
                        infoPtr->setLoadState(SkinLoadState::Loaded);
                        spdlog::info("SkinManager: Downloaded skin for UUID, saved to cache (hash: {})", effectiveHash);
                        if (onLoaded) {
                            onLoaded(uuid);
                        }
                        return;
                    }
                    spdlog::warn(
                        "SkinManager: Failed to save downloaded skin to cache: {}", saveResult.error().toString());
                } else {
                    spdlog::warn("SkinManager: Failed to download skin: {}", result.error().toString());
                }

                // 失败回退到默认皮肤
                if (!shuttingDown->load()) {
                    ResourceLocation defaultSkin = defaultProvider->getDefaultSkin(uuid);
                    infoPtr->setSkinLocation(defaultSkin);
                    infoPtr->setSkinType(defaultProvider->getDefaultSkinType(uuid));
                    infoPtr->setLoadState(SkinLoadState::UsingDefault);
                }
                if (onFailed) {
                    onFailed(uuid, result.success() ? "save failed" : result.error().toString());
                }
            };

        // 异步下载：HttpSkinLoader 会通过注入的线程池异步执行
        // 若未注入线程池，loadAsync 降级为同步执行后立即回调
        httpLoader->loadAsync(skinUrl, std::move(onSkinDownloaded));
        return;
    }

    // 无皮肤 URL，使用默认皮肤
    _useDefaultSkin(info);

    if (callbacks.onSkinLoaded) {
        callbacks.onSkinLoaded(profile.uuid());
    }
}

void SkinManager::_useDefaultSkin(std::shared_ptr<PlayerSkinInfo> info)
{
    ResourceLocation defaultSkin = getDefaultSkin(info->uuid());
    info->setSkinLocation(defaultSkin);
    info->setSkinType(getDefaultSkinType(info->uuid()));
    info->setLoadState(SkinLoadState::UsingDefault);
}

std::string SkinManager::_uuidToKey(const std::array<u8, 16>& uuid)
{
    return GameProfile(uuid, "").uuidToStringNoDashes();
}

} // namespace mc::skin
