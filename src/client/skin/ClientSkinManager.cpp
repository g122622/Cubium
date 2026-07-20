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

#include "ClientSkinManager.hpp"
#include "common/skin/core/GameProfile.hpp"
#include <cstring> // for std::memcpy
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

// stb_image is already implemented elsewhere (TextureAtlasBuilder.cpp)
#include <stb_image.h>

namespace mc::client::skin {

using ::mc::skin::DefaultSkinVariant;
using ::mc::skin::getDefaultSkinVariantForUUID;
using ::mc::skin::getDefaultSkinVariants;

ClientSkinManager::ClientSkinManager()
    : m_skinManager(std::make_unique<::mc::skin::SkinManager>(""))
    , m_textureAtlas(std::make_unique<renderer::entity::pipeline::EntityTextureAtlas>())
{
    m_defaultSkinRegions.fill(nullptr);
}

ClientSkinManager::~ClientSkinManager()
{
    shutdown();
}

Result<void> ClientSkinManager::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const std::string& cacheDir)
{
    if (m_initialized) {
        return {};
    }

    m_device = device;

    // 初始化底层皮肤管理器
    // 用正确的 cacheDir 重建底层 SkinManager（构造函数用的是空 cacheDir）。
    // 重建会丢弃此前 setResourcePacks/setWorkerPool 注入到旧对象的状态，
    // 因此必须把缓存的列表重新下发给新对象，再调用 initialize()。
    m_skinManager = std::make_unique<::mc::skin::SkinManager>(cacheDir);
    if (!m_resourcePacks.empty()) {
        m_skinManager->setResourcePacks(m_resourcePacks);
    }
    if (m_workerPool) {
        m_skinManager->setWorkerPool(m_workerPool);
    }
    auto skinResult = m_skinManager->initialize();
    if (!skinResult.success()) {
        return skinResult.error();
    }

    // 初始化纹理图集
    auto atlasResult = m_textureAtlas->initialize(device,
        physicalDevice,
        commandPool,
        graphicsQueue,
        256, // 最大纹理数
        64   // 纹理尺寸（64x64）
    );
    if (!atlasResult.success()) {
        return atlasResult.error();
    }

    // 加载默认皮肤
    auto defaultResult = _loadDefaultSkins();
    if (!defaultResult.success()) {
        spdlog::warn("ClientSkinManager: Failed to load default skins: {}", defaultResult.error().toString());
        // 默认皮肤加载失败不是致命错误
    }

    // 构建图集
    auto buildResult = m_textureAtlas->build();
    if (!buildResult.success()) {
        return buildResult.error();
    }

    m_initialized = true;
    spdlog::info("ClientSkinManager initialized");
    return {};
}

void ClientSkinManager::shutdown()
{
    const bool wasInitialized = m_initialized;

    if (m_textureAtlas) {
        m_textureAtlas->destroy();
    }

    {
        std::lock_guard<std::mutex> lock(m_regionMutex);
        m_skinRegions.clear();
        m_capeRegions.clear();
        m_elytraRegions.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        m_pendingSkins.clear();
    }

    if (m_skinManager) {
        m_skinManager->shutdown();
    }

    m_defaultSkinRegions.fill(nullptr);
    m_device = VK_NULL_HANDLE;
    m_initialized = false;

    if (wasInitialized) {
        spdlog::info("ClientSkinManager shutdown");
    }
}

Result<ResourceLocation> ClientSkinManager::registerPlayerSkin(const ::mc::skin::GameProfile& profile)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "ClientSkinManager not initialized");
    }

    // 获取或创建玩家信息
    auto info = m_skinManager->getOrCreatePlayerInfo(profile);
    if (!info) {
        return Error(ErrorCode::InvalidArgument, "Failed to create player info");
    }

    std::string key = _uuidToKey(profile.uuid());

    // 检查是否已有纹理区域
    {
        std::lock_guard<std::mutex> lock(m_regionMutex);
        auto it = m_skinRegions.find(key);
        if (it != m_skinRegions.end() && it->second) {
            // 已有纹理区域，检查图集是否有效
            if (m_textureAtlas->getRegion(info->getSkinLocation())) {
                return info->getSkinLocation();
            }
        }
    }

    // 获取皮肤位置
    ResourceLocation location = info->getSkinLocation();

    // 检查是否为默认皮肤
    if (m_skinManager->defaultSkinProvider().isDefaultSkin(location)) {
        // 使用默认皮肤的纹理区域
        const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(profile.uuid());
        const TextureRegion* region = m_defaultSkinRegions[variant.index];
        if (region) {
            return variant.textureLocation();
        }
        // 如果该默认皮肤区域未加载，返回其 ResourceLocation
        return variant.textureLocation();
    }

    // 尝试从缓存加载皮肤 PNG 数据并上传到图集
    const auto& textures = info->textures();
    if (textures.hasSkin() && textures.skinHash().has_value()) {
        const std::string& hash = *textures.skinHash();

        // 检查缓存中是否有皮肤文件
        if (m_skinManager->cache().hasSkin(hash)) {
            // 读取缓存的皮肤文件
            auto cachePathOpt = m_skinManager->cache().getSkinPath(hash);
            if (cachePathOpt.has_value() && std::filesystem::exists(*cachePathOpt)) {
                // 从文件加载 PNG 数据
                std::ifstream file(*cachePathOpt, std::ios::binary);
                if (file.is_open()) {
                    std::vector<u8> pngData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    file.close();

                    // 上传到图集
                    auto uploadResult = _uploadSkinToAtlas(pngData, location);
                    if (uploadResult.success()) {
                        // 标记需要重建图集
                        // 注意：调用者应在合适时机调用 rebuildAtlas()

                        // 更新纹理区域映射
                        {
                            std::lock_guard<std::mutex> lock(m_regionMutex);
                            m_skinRegions[key] = nullptr; // 将在重建后更新
                        }

                        spdlog::info("ClientSkinManager: Uploaded cached skin for {} to atlas", profile.name());
                        return location;
                    } else {
                        spdlog::warn("ClientSkinManager: Failed to upload skin for {}: {}",
                            profile.name(),
                            uploadResult.error().toString());
                    }
                }
            }
        }
    }

    // 没有缓存，使用 UUID 哈希选择的默认皮肤
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(profile.uuid());
    return variant.textureLocation();
}

const TextureRegion* ClientSkinManager::getSkinRegion(const std::array<u8, 16>& uuid) const
{
    std::string key = _uuidToKey(uuid);

    {
        std::lock_guard<std::mutex> lock(m_regionMutex);
        auto it = m_skinRegions.find(key);
        if (it != m_skinRegions.end() && it->second) {
            return it->second;
        }
    }

    // 返回默认皮肤
    return getDefaultSkinRegion(uuid);
}

const TextureRegion* ClientSkinManager::getDefaultSkinRegion(const std::array<u8, 16>& uuid) const
{
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(uuid);
    return m_defaultSkinRegions[variant.index];
}

const TextureRegion* ClientSkinManager::getCapeRegion(const std::array<u8, 16>& uuid) const
{
    std::string key = _uuidToKey(uuid);

    std::lock_guard<std::mutex> lock(m_regionMutex);
    auto it = m_capeRegions.find(key);
    return it != m_capeRegions.end() ? it->second : nullptr;
}

const TextureRegion* ClientSkinManager::getElytraRegion(const std::array<u8, 16>& uuid) const
{
    std::string key = _uuidToKey(uuid);

    std::lock_guard<std::mutex> lock(m_regionMutex);
    auto it = m_elytraRegions.find(key);
    return it != m_elytraRegions.end() ? it->second : nullptr;
}

::mc::skin::SkinType ClientSkinManager::getSkinType(const std::array<u8, 16>& uuid) const
{
    auto info = m_skinManager->getPlayerInfo(uuid);
    if (info) {
        return info->getSkinType();
    }
    return m_skinManager->getDefaultSkinType(uuid);
}

std::shared_ptr<::mc::skin::PlayerSkinInfo> ClientSkinManager::getPlayerInfo(const std::array<u8, 16>& uuid) const
{
    return m_skinManager->getPlayerInfo(uuid);
}

Result<void> ClientSkinManager::rebuildAtlas()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "ClientSkinManager not initialized");
    }

    // 检查是否需要重建
    if (!m_textureAtlas->needsRebuild()) {
        return {};
    }

    // 重建纹理图集
    auto buildResult = m_textureAtlas->rebuild();
    if (!buildResult.success()) {
        return buildResult.error();
    }

    // 更新 18 个默认皮肤纹理区域引用
    const auto& variants = getDefaultSkinVariants();
    for (size_t i = 0; i < ::mc::skin::DEFAULT_SKIN_COUNT; ++i) {
        m_defaultSkinRegions[i] = m_textureAtlas->getRegion(variants[i].textureLocation());
    }

    // 更新所有已注册玩家的纹理区域引用
    {
        std::lock_guard<std::mutex> lock(m_regionMutex);
        for (auto& [uuidKey, region] : m_skinRegions) {
            // 尝试获取更新后的区域
            auto uuid = ::mc::skin::GameProfile::parseUUID(uuidKey);
            auto info = m_skinManager->getPlayerInfo(uuid);
            if (info && info->isLoaded()) {
                const auto* newRegion = m_textureAtlas->getRegion(info->getSkinLocation());
                if (newRegion) {
                    region = newRegion;
                }
            }
        }

        // 更新披风和鞘翅区域
        for (auto& [uuidKey, region] : m_capeRegions) {
            auto uuid = ::mc::skin::GameProfile::parseUUID(uuidKey);
            auto info = m_skinManager->getPlayerInfo(uuid);
            if (info) {
                auto capeLoc = info->getCapeLocation();
                if (capeLoc) {
                    const auto* newRegion = m_textureAtlas->getRegion(*capeLoc);
                    if (newRegion) {
                        region = newRegion;
                    }
                }
            }
        }

        for (auto& [uuidKey, region] : m_elytraRegions) {
            auto uuid = ::mc::skin::GameProfile::parseUUID(uuidKey);
            auto info = m_skinManager->getPlayerInfo(uuid);
            if (info) {
                auto elytraLoc = info->getElytraLocation();
                if (elytraLoc) {
                    const auto* newRegion = m_textureAtlas->getRegion(*elytraLoc);
                    if (newRegion) {
                        region = newRegion;
                    }
                }
            }
        }
    }

    spdlog::info("ClientSkinManager: Rebuilt texture atlas successfully");
    return {};
}

Result<void> ClientSkinManager::_loadDefaultSkins()
{
    // 加载 MC 1.21.1 的 18 种默认皮肤（9 slim + 9 wide）
    const auto& variants = getDefaultSkinVariants();
    const auto& provider = m_skinManager->defaultSkinProvider();

    for (size_t i = 0; i < ::mc::skin::DEFAULT_SKIN_COUNT; ++i) {
        const auto& variant = variants[i];
        const auto& skinData = provider.getSkinData(i);

        if (!skinData.empty()) {
            ResourceLocation location = variant.textureLocation();
            auto result = m_textureAtlas->addTextureFromPixels(skinData,
                64, // 皮肤是 64x64
                64,
                location);
            if (!result.success()) {
                spdlog::warn("ClientSkinManager: Failed to add default skin {}: {}",
                    location.toString(),
                    result.error().toString());
            }
        } else {
            spdlog::warn("ClientSkinManager: No skin data for default skin variant {} ({})", i, variant.name);
        }
    }

    return {};
}

Result<ResourceLocation> ClientSkinManager::_uploadSkinToAtlas(
    const std::vector<u8>& pngData, const ResourceLocation& preferredLocation)
{

    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "ClientSkinManager not initialized");
    }

    if (pngData.empty()) {
        return Error(ErrorCode::InvalidData, "Empty PNG data");
    }

    // stb_image 使用 int 参数，保持 int 类型以匹配其 API
    int width = 0;
    int height = 0;
    int channels = 0;
    u8* pixels = stbi_load_from_memory(pngData.data(),
        static_cast<int>(pngData.size()),
        &width,
        &height,
        &channels,
        4); // 强制 RGBA

    if (pixels == nullptr) {
        return Error(ErrorCode::InvalidData, "Failed to decode PNG data");
    }

    // 确保是有效的皮肤尺寸 (64x64 或 64x32)
    std::vector<u8> processedPixels;
    u32 finalWidth = 0;
    u32 finalHeight = 0;

    if (width == 64 && height == 32) {
        // 旧版 Java 皮肤格式，需要扩展到 64x64
        finalWidth = 64;
        finalHeight = 64;
        processedPixels.assign(static_cast<size_t>(64 * 64 * 4), 0);

        // 复制上半部分
        for (int y = 0; y < 32; ++y) {
            const size_t srcOffset = static_cast<size_t>(y * 64 * 4);
            const size_t dstOffset = static_cast<size_t>(y * 64 * 4);
            std::memcpy(processedPixels.data() + dstOffset, pixels + srcOffset, static_cast<size_t>(64 * 4));
        }

        // 复制下半部分作为兼容层（旧皮肤的外层）
        for (int y = 0; y < 32; ++y) {
            const size_t srcOffset = static_cast<size_t>(y * 64 * 4);
            const size_t dstOffset = static_cast<size_t>((y + 32) * 64 * 4);
            std::memcpy(processedPixels.data() + dstOffset, pixels + srcOffset, static_cast<size_t>(64 * 4));
        }
    } else if (width == 64 && height == 64) {
        // 标准皮肤格式
        finalWidth = 64;
        finalHeight = 64;
        processedPixels.resize(static_cast<size_t>(width * height * 4));
        std::memcpy(processedPixels.data(), pixels, processedPixels.size());
    } else {
        stbi_image_free(pixels);
        return Error(
            ErrorCode::InvalidData, "Invalid skin dimensions: " + std::to_string(width) + "x" + std::to_string(height));
    }

    stbi_image_free(pixels);

    // 添加到纹理图集
    auto result = m_textureAtlas->addTextureFromPixels(processedPixels, finalWidth, finalHeight, preferredLocation);

    if (!result.success()) {
        return result.error();
    }

    spdlog::info(
        "ClientSkinManager: Uploaded skin to atlas: {} ({}x{})", preferredLocation.toString(), finalWidth, finalHeight);

    return preferredLocation;
}

std::string ClientSkinManager::_uuidToKey(const std::array<u8, 16>& uuid)
{
    return ::mc::skin::GameProfile(uuid, "").uuidToStringNoDashes();
}

} // namespace mc::client::skin
