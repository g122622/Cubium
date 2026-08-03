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
#include "client/renderer/MeshTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/manager/SkinManager.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"
#include <cstring>
#include <spdlog/spdlog.h>

// stb_image is already implemented elsewhere (TextureAtlasBuilder.cpp)
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::skin {

using ::mc::skin::DefaultSkinVariant;
using ::mc::skin::getDefaultSkinVariants;

namespace {

/**
 * @brief 将 PNG 字节解码为 64x64 RGBA 像素
 *
 * 处理 64x32 旧版皮肤格式（扩展为 64x64）与标准 64x64 两种。其它尺寸返回空。
 */
std::vector<u8> decodeSkinPng(const std::vector<u8>& pngData)
{
    if (pngData.empty()) {
        return {};
    }

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
        return {};
    }

    std::vector<u8> processedPixels;

    if (width == 64 && height == 32) {
        // 旧版 Java 皮肤格式，扩展到 64x64
        processedPixels.assign(static_cast<size_t>(64 * 64 * 4), 0);
        for (int y = 0; y < 32; ++y) {
            const size_t srcOffset = static_cast<size_t>(y * 64 * 4);
            const size_t dstOffset = static_cast<size_t>(y * 64 * 4);
            std::memcpy(processedPixels.data() + dstOffset, pixels + srcOffset, static_cast<size_t>(64 * 4));
        }
        // 复制上半部分作为兼容外层
        for (int y = 0; y < 32; ++y) {
            const size_t srcOffset = static_cast<size_t>(y * 64 * 4);
            const size_t dstOffset = static_cast<size_t>((y + 32) * 64 * 4);
            std::memcpy(processedPixels.data() + dstOffset, pixels + srcOffset, static_cast<size_t>(64 * 4));
        }
    } else if (width == 64 && height == 64) {
        processedPixels.resize(static_cast<size_t>(width * height * 4));
        std::memcpy(processedPixels.data(), pixels, processedPixels.size());
    } else {
        stbi_image_free(pixels);
        spdlog::warn("ClientSkinManager: invalid skin dimensions {}x{}", width, height);
        return {};
    }

    stbi_image_free(pixels);
    return processedPixels;
}

} // anonymous namespace

ClientSkinManager::ClientSkinManager()
    : m_skinManager(std::make_unique<::mc::skin::SkinManager>(""))
{}

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
    (void)device;
    (void)physicalDevice;
    (void)commandPool;
    (void)graphicsQueue;

    if (m_initialized) {
        return {};
    }

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

    // 上传 18 种默认皮肤到渲染器图集动态区域
    _uploadDefaultSkins();

    m_initialized = true;
    spdlog::info("ClientSkinManager initialized");
    return {};
}

void ClientSkinManager::shutdown()
{
    const bool wasInitialized = m_initialized;

    m_uploader.clear();

    if (m_skinManager) {
        m_skinManager->shutdown();
    }

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

    auto info = m_skinManager->getOrCreatePlayerInfo(profile);
    if (!info) {
        return Error(ErrorCode::InvalidArgument, "Failed to create player info");
    }

    // 自定义皮肤像素就绪后由 getSkinRegionForEntity 懒上传；
    // 此处仅返回皮肤 ResourceLocation（默认 or 自定义）。
    return info->getSkinLocation();
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

void ClientSkinManager::removePlayerInfo(const std::array<u8, 16>& uuid)
{
    if (!m_initialized) {
        return;
    }

    // 联动释放图集动态区域（修复旧实现 removePlayerInfo 不同步 region 的 bug）
    m_uploader.removeRegion(uuid);
    m_skinManager->removePlayerInfo(uuid);
}

const TextureRegion* ClientSkinManager::getSkinRegionForEntity(EntityInstanceId entityId) const
{
    if (!m_initialized || m_identityRegistry == nullptr) {
        return nullptr;
    }

    const std::array<u8, 16>* uuid = m_identityRegistry->uuidOf(entityId);
    if (uuid == nullptr) {
        // 非玩家实体或 UUID 未注册 → 回退默认实体纹理路径
        return nullptr;
    }

    // 尝试自定义皮肤：若 PlayerSkinInfo 有 skinHash 且缓存命中，懒上传
    auto info = m_skinManager->getPlayerInfo(*uuid);
    if (info && info->textures().hasSkin() && info->textures().skinHash().has_value()) {
        const std::string& hash = *info->textures().skinHash();
        if (m_skinManager->cache().hasSkin(hash)) {
            // 仅在缓存文件存在时尝试解码上传（异步加载未完成时跳过，回退默认）
            auto readResult = m_skinManager->cache().readSkin(hash);
            if (readResult.success()) {
                std::vector<u8> rgbaPixels = decodeSkinPng(readResult.value());
                if (!rgbaPixels.empty()) {
                    const TextureRegion* region = m_uploader.getOrCreateRegion(*uuid, rgbaPixels);
                    if (region != nullptr) {
                        return region;
                    }
                }
            }
        }
    }

    // 回退默认皮肤区域（loadDefaultSkins 时已注入）
    return m_uploader.getDefaultRegion(*uuid);
}

void ClientSkinManager::_uploadDefaultSkins()
{
    const auto& variants = getDefaultSkinVariants();
    const auto& provider = m_skinManager->defaultSkinProvider();

    std::array<std::vector<u8>, ::mc::skin::DEFAULT_SKIN_COUNT> skinDataByIndex;
    std::array<ResourceLocation, ::mc::skin::DEFAULT_SKIN_COUNT> locations;

    for (size_t i = 0; i < ::mc::skin::DEFAULT_SKIN_COUNT; ++i) {
        skinDataByIndex[i] = provider.getSkinData(i);
        locations[i] = variants[i].textureLocation();
    }

    m_uploader.loadDefaultSkins(skinDataByIndex, locations);
}

} // namespace mc::client::skin
