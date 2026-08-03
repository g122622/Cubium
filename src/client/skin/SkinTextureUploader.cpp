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

#include "SkinTextureUploader.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include <array>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::client::skin {

using ::mc::skin::getDefaultSkinVariantForUUID;

u32 SkinTextureUploader::loadDefaultSkins(
    const std::array<std::vector<u8>, ::mc::skin::DEFAULT_SKIN_COUNT>& skinDataByIndex,
    const std::array<ResourceLocation, ::mc::skin::DEFAULT_SKIN_COUNT>& locations)
{
    if (m_textureAtlas == nullptr) {
        spdlog::warn("SkinTextureUploader: atlas not injected, skip default skins");
        return 0;
    }

    u32 uploaded = 0;
    for (size_t i = 0; i < ::mc::skin::DEFAULT_SKIN_COUNT; ++i) {
        const auto& pixels = skinDataByIndex[i];
        if (pixels.empty()) {
            spdlog::warn("SkinTextureUploader: empty pixel data for default skin variant {}", i);
            continue;
        }

        const auto* region = m_textureAtlas->injectRegion(locations[i], 64, 64, pixels.data());
        if (region == nullptr) {
            spdlog::warn(
                "SkinTextureUploader: failed to inject default skin variant {} ({})", i, locations[i].toString());
            continue;
        }

        m_defaultRegions[i] = region;
        ++uploaded;
    }

    spdlog::info("SkinTextureUploader: uploaded {}/{} default skin variants", uploaded, ::mc::skin::DEFAULT_SKIN_COUNT);
    return uploaded;
}

const TextureRegion* SkinTextureUploader::findRegion(const ResourceLocation& location) const
{
    if (m_textureAtlas == nullptr) {
        return nullptr;
    }
    return m_textureAtlas->getRegion(location);
}

const TextureRegion* SkinTextureUploader::getDefaultRegion(const std::array<u8, 16>& uuid) const
{
    const ::mc::skin::DefaultSkinVariant& variant = getDefaultSkinVariantForUUID(uuid);
    return m_defaultRegions[variant.index];
}

const TextureRegion* SkinTextureUploader::getOrCreateRegion(
    const std::array<u8, 16>& uuid, const std::vector<u8>& rgbaPixels) const
{
    if (m_textureAtlas == nullptr) {
        return nullptr;
    }

    const ResourceLocation location = _uuidToLocation(uuid);

    // 命中缓存
    {
        std::lock_guard<std::mutex> lock(m_customMutex);
        auto it = m_customRegions.find(location.toString());
        if (it != m_customRegions.end() && it->second != nullptr) {
            return it->second;
        }
    }

    if (rgbaPixels.empty()) {
        return nullptr;
    }

    const auto* region = m_textureAtlas->injectRegion(location, 64, 64, rgbaPixels.data());
    if (region == nullptr) {
        spdlog::warn("SkinTextureUploader: dynamic region exhausted, fallback to default skin");
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_customMutex);
    m_customRegions[location.toString()] = region;
    return region;
}

void SkinTextureUploader::removeRegion(const std::array<u8, 16>& uuid)
{
    if (m_textureAtlas == nullptr) {
        return;
    }

    const ResourceLocation location = _uuidToLocation(uuid);

    {
        std::lock_guard<std::mutex> lock(m_customMutex);
        auto it = m_customRegions.find(location.toString());
        if (it == m_customRegions.end()) {
            return; // 非自定义皮肤（默认皮肤不移除）
        }
        m_customRegions.erase(it);
    }

    m_textureAtlas->removeDynamicRegion(location);
}

void SkinTextureUploader::clear()
{
    std::lock_guard<std::mutex> lock(m_customMutex);
    m_customRegions.clear();
    m_defaultRegions.fill(nullptr);
}

ResourceLocation SkinTextureUploader::_uuidToLocation(const std::array<u8, 16>& uuid)
{
    // player_skin:<uuid-hex-no-dashes> 命名约定
    const std::string hex = ::mc::skin::GameProfile(uuid, "").uuidToStringNoDashes();
    return ResourceLocation("minecraft:player_skin:" + hex);
}

} // namespace mc::client::skin
