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

#include "DefaultSkinProvider.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <cstring>
#include <spdlog/spdlog.h>

// stb_image 用于解码 PNG 纹理（STB_IMAGE_IMPLEMENTATION 已在 TextureAtlasBuilder.cpp 中定义）
#include <stb_image.h>

namespace mc::skin {

// 皮肤尺寸常量
constexpr size_t SKIN_WIDTH = 64;
constexpr size_t SKIN_HEIGHT = 64;
constexpr size_t SKIN_CHANNELS = 4; // RGBA
constexpr size_t SKIN_DATA_SIZE = SKIN_WIDTH * SKIN_HEIGHT * SKIN_CHANNELS;

// 资源包路径前缀，toFilePath(PackType::ClientResources) 返回 "assets/..."，
// 而 IResourcePack::readResource 期望相对于 PackType 根目录的路径（不含 "assets/" 前缀）
constexpr const char* CLIENT_RESOURCE_PREFIX = "assets/";

Result<void> DefaultSkinProvider::initialize()
{
    if (m_initialized) {
        return {};
    }

    auto result = _loadBuiltinSkins();
    if (!result.success()) {
        spdlog::warn(
            "DefaultSkinProvider: Failed to load builtin skins, using fallback: {}", result.error().toString());
        // 不返回错误，使用 fallback 数据
    }

    m_initialized = true;
    spdlog::info("DefaultSkinProvider initialized with {} default skins", DEFAULT_SKIN_COUNT);
    return {};
}

Result<void> DefaultSkinProvider::_loadBuiltinSkins()
{
    const auto& variants = getDefaultSkinVariants();

    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        const auto& variant = variants[i];

        // 先填充零像素占位数据，作为单变体加载失败时的兜底
        m_skinData[i].assign(SKIN_DATA_SIZE, 0);

        if (m_resourcePacks.empty()) {
            spdlog::warn(
                "DefaultSkinProvider: No resource pack set, variant {} ({}) uses zero-pixel fallback", i, variant.name);
            continue;
        }

        auto pixels = _loadSkinFromResourcePack(variant);
        if (pixels.empty()) {
            spdlog::warn(
                "DefaultSkinProvider: Failed to load skin variant {} ({}), using zero-pixel fallback", i, variant.name);
            continue;
        }

        m_skinData[i] = std::move(pixels);
        spdlog::info("DefaultSkinProvider: Loaded skin variant {} ({})", i, variant.name);
    }

    return {};
}

std::vector<u8> DefaultSkinProvider::_loadSkinFromResourcePack(const DefaultSkinVariant& variant) const
{
    if (m_resourcePacks.empty()) {
        spdlog::warn("DefaultSkinProvider: No resource pack available for variant {}", variant.name);
        return {};
    }

    // 将 ResourceLocation 转换为相对于 ClientResources 根目录的路径
    // toFilePath 返回 "assets/minecraft/textures/entity/player/{slim|wide}/{name}.png"
    // readResource 期望路径不含 "assets/" 前缀
    std::string resourcePath = variant.textureLocation().toFilePath(resource::PackType::ClientResources);
    if (resourcePath.rfind(CLIENT_RESOURCE_PREFIX, 0) == 0) {
        resourcePath.erase(0, std::string(CLIENT_RESOURCE_PREFIX).size());
    }

    // 按资源包优先级反向遍历（后添加的优先），与 ResourceManager 的纹理加载惯例一致
    std::vector<u8> pngData;
    for (auto packIt = m_resourcePacks.rbegin(); packIt != m_resourcePacks.rend(); ++packIt) {
        IResourcePack* pack = *packIt;
        if (pack == nullptr) {
            continue;
        }
        if (!pack->hasResource(resource::PackType::ClientResources, resourcePath)) {
            continue;
        }
        auto readResult = pack->readResource(resource::PackType::ClientResources, resourcePath);
        if (!readResult.success() || readResult.value().empty()) {
            continue;
        }
        pngData = std::move(readResult.value());
        break;
    }

    if (pngData.empty()) {
        spdlog::warn(
            "DefaultSkinProvider: Skin '{}' not found in any resource pack", variant.textureLocation().toString());
        return {};
    }

    const auto& textureLocation = variant.textureLocation();

    // 使用 stbi_load_from_memory 解码 PNG，强制 RGBA 4 通道输出
    int width = 0;
    int height = 0;
    int channels = 0;
    u8* pixels = stbi_load_from_memory(
        pngData.data(), static_cast<int>(pngData.size()), &width, &height, &channels, static_cast<int>(SKIN_CHANNELS));

    if (!pixels) {
        spdlog::warn("DefaultSkinProvider: stb_image failed to decode skin '{}'", textureLocation.toString());
        return {};
    }

    // 验证皮肤尺寸：MC 1.21.1 默认皮肤均为 64x64，旧版 64x32 也允许（转换为 64x64）
    if (width != static_cast<int>(SKIN_WIDTH)) {
        spdlog::warn("DefaultSkinProvider: Invalid skin width for '{}' (expected {}, got {})",
            textureLocation.toString(),
            SKIN_WIDTH,
            width);
        stbi_image_free(pixels);
        return {};
    }

    if (height != static_cast<int>(SKIN_HEIGHT) && height != 32) {
        spdlog::warn("DefaultSkinProvider: Invalid skin height for '{}' (expected {} or 32, got {})",
            textureLocation.toString(),
            SKIN_HEIGHT,
            height);
        stbi_image_free(pixels);
        return {};
    }

    std::vector<u8> result;

    if (height == 32) {
        // 旧版 64x32 皮肤：将上半部分复制到 64x64，下半部分保留为零（透明）
        result.assign(SKIN_DATA_SIZE, 0);
        const size_t copySize = static_cast<size_t>(32) * SKIN_WIDTH * SKIN_CHANNELS;
        std::memcpy(result.data(), pixels, copySize);
        spdlog::info("DefaultSkinProvider: Converted legacy 64x32 skin '{}' to 64x64", textureLocation.toString());
    } else {
        // 64x64 皮肤：直接复制全部像素
        result.resize(SKIN_DATA_SIZE);
        std::memcpy(result.data(), pixels, SKIN_DATA_SIZE);
    }

    stbi_image_free(pixels);
    return result;
}

ResourceLocation DefaultSkinProvider::getDefaultSkin(const std::array<u8, 16>& uuid) const noexcept
{
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(uuid);
    return variant.textureLocation();
}

SkinType DefaultSkinProvider::getDefaultSkinType(const std::array<u8, 16>& uuid) const noexcept
{
    const DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(uuid);
    return variant.skinType;
}

ResourceLocation DefaultSkinProvider::getSkinLocation(size_t variantIndex) const noexcept
{
    if (variantIndex >= DEFAULT_SKIN_COUNT) {
        variantIndex = 6; // 回退到 slim/steve（规范默认皮肤）
    }
    const auto& variants = getDefaultSkinVariants();
    return variants[variantIndex].textureLocation();
}

ResourceLocation DefaultSkinProvider::getCanonicalDefaultSkinLocation() const noexcept
{
    return getCanonicalDefaultSkin().textureLocation();
}

const std::vector<u8>& DefaultSkinProvider::getSkinData(size_t variantIndex) const noexcept
{
    if (variantIndex >= DEFAULT_SKIN_COUNT) {
        variantIndex = 6; // 回退到 slim/steve
    }
    return m_skinData[variantIndex];
}

bool DefaultSkinProvider::isDefaultSkin(const ResourceLocation& location) const noexcept
{
    const auto& variants = getDefaultSkinVariants();
    for (size_t i = 0; i < DEFAULT_SKIN_COUNT; ++i) {
        if (location == variants[i].textureLocation()) {
            return true;
        }
    }
    return false;
}

} // namespace mc::skin
