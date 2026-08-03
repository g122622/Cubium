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

#include "GuiTextureLoader.hpp"
#include "GuiSprite.hpp"
#include "GuiSpriteAtlas.hpp"
#include "GuiSpriteParser.hpp"
#include "GuiSpriteRegistry.hpp"
#include "GuiTextureAtlas.hpp"
#include "client/resource/TextureAtlasBuilder.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <stb_image.h>

namespace mc::client::renderer::trident::gui {

GuiTextureLoader::GuiTextureLoader() = default;
GuiTextureLoader::~GuiTextureLoader() = default;

void GuiTextureLoader::addResourcePack(std::shared_ptr<IResourcePack> resourcePack)
{
    if (resourcePack) {
        m_resourcePacks.push_back(std::move(resourcePack));
    }
}

void GuiTextureLoader::clearResourcePacks()
{
    m_resourcePacks.clear();
}

// ==================== GuiSpriteAtlas 重载 ====================

Result<void> GuiTextureLoader::loadGuiTexture(GuiSpriteAtlas& atlas, const std::string& location)
{

    spdlog::info(
        "[GuiTextureLoader] loadGuiTexture: location='{}', resourcePacks={}", location, m_resourcePacks.size());

    std::vector<u8> textureData;
    auto result = _findTexture(location, textureData);
    if (result.failed()) {
        spdlog::warn("[GuiTextureLoader] findTexture failed for '{}': {}", location, result.error().toString());
        return result;
    }

    spdlog::info("[GuiTextureLoader] Found texture '{}', size={} bytes", location, textureData.size());

    // 解码PNG
    i32 width, height;
    std::vector<u8> pixels;
    auto decodeResult = decodePng(textureData, width, height, pixels);
    if (decodeResult.failed()) {
        spdlog::warn("[GuiTextureLoader] decodePng failed: {}", decodeResult.error().toString());
        return decodeResult;
    }

    spdlog::info("[GuiTextureLoader] Decoded PNG: {}x{}, pixels={} bytes", width, height, pixels.size());

    // 设置图集尺寸并上传纹理
    atlas.setAtlasSize(width, height);
    auto uploadResult = atlas.loadTextureFromMemory(pixels, width, height);
    if (uploadResult.failed()) {
        spdlog::warn("[GuiTextureLoader] loadTextureFromMemory failed: {}", uploadResult.error().toString());
        return uploadResult;
    }

    spdlog::info(
        "[GuiTextureLoader] Texture uploaded successfully, atlas size: {}x{}", atlas.atlasWidth(), atlas.atlasHeight());
    return {};
}

Result<void> GuiTextureLoader::loadGuiTexture(
    GuiSpriteAtlas& atlas, const std::string& location, i32 atlasWidth, i32 atlasHeight)
{

    std::vector<u8> textureData;
    auto result = _findTexture(location, textureData);
    if (result.failed()) {
        return result;
    }

    // 解码PNG
    i32 width, height;
    std::vector<u8> pixels;
    auto decodeResult = decodePng(textureData, width, height, pixels);
    if (decodeResult.failed()) {
        return decodeResult;
    }

    // 使用指定的图集尺寸
    atlas.setAtlasSize(atlasWidth, atlasHeight);
    return atlas.loadTextureFromMemory(pixels, width, height);
}

Result<void> GuiTextureLoader::loadAllToSpriteAtlas(GuiSpriteAtlas& atlas, const std::string& textureLocation)
{

    // 尝试加载纹理
    auto loadResult = loadGuiTexture(atlas, textureLocation);
    if (loadResult.failed()) {
        // 纹理加载失败，使用默认纹理
        auto defaultResult = atlas.loadDefaultTextures();
        if (defaultResult.failed()) {
            return defaultResult;
        }
    }

    return {};
}

// ==================== GuiTextureAtlas 重载（传统）====================

Result<void> GuiTextureLoader::loadGuiTexture(GuiTextureAtlas& atlas, const std::string& location)
{

    std::vector<u8> textureData;
    auto result = _findTexture(location, textureData);
    if (result.failed()) {
        return result;
    }

    // 解码PNG
    i32 width, height;
    std::vector<u8> pixels;
    auto decodeResult = decodePng(textureData, width, height, pixels);
    if (decodeResult.failed()) {
        return decodeResult;
    }

    // 更新图集尺寸
    atlas.setAtlasSize(width, height);

    // TODO: 将纹理数据上传到GuiTextureAtlas
    // 这需要扩展GuiTextureAtlas以支持动态纹理上传

    return {};
}

Result<void> GuiTextureLoader::loadSpritesFromJson(GuiTextureAtlas& atlas, const std::string& jsonPath)
{

    if (m_resourcePacks.empty()) {
        return Error(ErrorCode::NotFound, "No resource packs available");
    }

    // 构建资源路径
    std::string resourcePath = _buildResourcePath(jsonPath);

    // 按优先级搜索资源包
    for (auto it = m_resourcePacks.rbegin(); it != m_resourcePacks.rend(); ++it) {
        auto& pack = *it;

        if (!pack->hasResource(resource::PackType::ClientResources, resourcePath)) {
            continue;
        }

        auto readResult = pack->readResource(resource::PackType::ClientResources, resourcePath);
        if (readResult.failed()) {
            continue;
        }

        std::string jsonContent(readResult.value().begin(), readResult.value().end());
        auto parseResult = GuiSpriteParser::parse(jsonContent, atlas.atlasWidth(), atlas.atlasHeight());

        if (parseResult.success()) {
            const auto& definition = parseResult.value();

            // 注册精灵
            for (const auto& [id, sprite] : definition.sprites) {
                atlas.registerSprite(sprite);
            }

            return {};
        }
    }

    return Error(ErrorCode::NotFound, std::string("Sprite definition not found: ") + jsonPath);
}

Result<u32> GuiTextureLoader::buildSpriteAtlas(GuiSpriteAtlas& atlas, const std::vector<SpriteMapping>& spriteMappings)
{
    if (m_resourcePacks.empty()) {
        spdlog::warn("[GuiTextureLoader] No resource packs available for buildSpriteAtlas");
        return Error(ErrorCode::NotFound, "No resource packs available");
    }

    if (spriteMappings.empty()) {
        spdlog::warn("[GuiTextureLoader] Empty sprite mappings for buildSpriteAtlas");
        return Error(ErrorCode::InvalidArgument, "Empty sprite mappings");
    }

    // 使用 TextureAtlasBuilder 将独立精灵拼合成图集
    TextureAtlasBuilder builder;
    u32 loadedCount = 0;

    for (const auto& [spriteId, location] : spriteMappings) {
        // 将 ResourceLocation 转换为资源包内的文件路径
        std::string pngPath = location.toFilePath(resource::PackType::ClientResources, "png");
        // toFilePath 产生 "assets/<namespace>/<path>.png"，需要去掉 "assets/" 前缀
        const std::string assetsPrefix = "assets/";
        if (pngPath.size() > assetsPrefix.size() && pngPath.substr(0, assetsPrefix.size()) == assetsPrefix) {
            pngPath.erase(0, assetsPrefix.size());
        }

        // 按资源包优先级搜索（后添加的优先）
        bool found = false;
        for (auto it = m_resourcePacks.rbegin(); it != m_resourcePacks.rend() && !found; ++it) {
            auto& pack = *it;
            if (!pack) continue;

            if (!pack->hasResource(resource::PackType::ClientResources, pngPath)) {
                continue;
            }

            auto readResult = pack->readResource(resource::PackType::ClientResources, pngPath);
            if (readResult.failed()) {
                spdlog::warn(
                    "[GuiTextureLoader] Failed to read sprite '{}': {}", spriteId, readResult.error().toString());
                continue;
            }

            // 解码PNG
            i32 width, height;
            std::vector<u8> pixels;
            auto decodeResult = decodePng(readResult.value(), width, height, pixels);
            if (decodeResult.failed()) {
                spdlog::warn(
                    "[GuiTextureLoader] Failed to decode sprite '{}': {}", spriteId, decodeResult.error().toString());
                continue;
            }

            // 添加到图集构建器（精灵ID作为key用于后续查找）
            ResourceLocation spriteKey(location.namespace_(), location.path());
            builder.addTexture(spriteKey, pixels, static_cast<u32>(width), static_cast<u32>(height));
            found = true;
            loadedCount++;
        }

        if (!found) {
            spdlog::debug("[GuiTextureLoader] Sprite '{}' not found in any resource pack, skipping", spriteId);
        }
    }

    if (loadedCount == 0) {
        spdlog::warn("[GuiTextureLoader] No sprites loaded from mapping, falling back to monolithic atlas");
        return Error(ErrorCode::NotFound, "No sprites loaded from mapping");
    }

    // 构建图集
    auto buildResult = builder.build();
    if (buildResult.failed()) {
        spdlog::error("[GuiTextureLoader] Failed to build sprite atlas: {}", buildResult.error().toString());
        return buildResult.error();
    }

    auto& atlasData = buildResult.value();

    // 上传图集纹理到 GuiSpriteAtlas
    auto uploadResult = atlas.loadTextureFromMemory(
        atlasData.pixels, static_cast<i32>(atlasData.width), static_cast<i32>(atlasData.height));
    if (uploadResult.failed()) {
        spdlog::error("[GuiTextureLoader] Failed to upload sprite atlas: {}", uploadResult.error().toString());
        return uploadResult.error();
    }

    // 根据拼合结果注册精灵：从 AtlasBuildResult.regions 查找每个精灵的UV坐标
    u32 registeredCount = 0;
    for (const auto& [spriteId, location] : spriteMappings) {
        // 使用与 addTexture 相同的 key 来查找 region
        ResourceLocation spriteKey(location.namespace_(), location.path());
        auto regionIt = atlasData.regions.find(spriteKey);
        if (regionIt == atlasData.regions.end()) {
            continue;
        }

        const auto& region = regionIt->second;
        // 从UV坐标反算像素尺寸：width = (u1 - u0) * atlasWidth, height = (v1 - v0) * atlasHeight
        i32 spriteWidth = static_cast<i32>(std::round((region.u1 - region.u0) * atlasData.width));
        i32 spriteHeight = static_cast<i32>(std::round((region.v1 - region.v0) * atlasData.height));

        GuiSprite sprite;
        sprite.id = spriteId;
        sprite.u0 = region.u0;
        sprite.v0 = region.v0;
        sprite.u1 = region.u1;
        sprite.v1 = region.v1;
        sprite.width = spriteWidth;
        sprite.height = spriteHeight;
        atlas.registerSprite(sprite);
        registeredCount++;
    }

    spdlog::info("[GuiTextureLoader] Built sprite atlas: {}x{}, loaded {} sprites, registered {}",
        atlasData.width,
        atlasData.height,
        loadedCount,
        registeredCount);

    return registeredCount;
}

Result<void> GuiTextureLoader::loadDefaultTextures(GuiTextureAtlas& atlas)
{
    // 注册默认精灵（作为后备）
    GuiSpriteRegistry::registerAllDefaults(atlas);
    return {};
}

Result<void> GuiTextureLoader::loadAll(GuiTextureAtlas& atlas)
{
    // 尝试加载widgets.png
    bool hasWidgets = false;
    std::vector<u8> widgetsData;
    auto widgetsResult = _findTexture("minecraft:textures/gui/widgets.png", widgetsData);
    if (widgetsResult.success()) {
        i32 width, height;
        std::vector<u8> pixels;
        if (decodePng(widgetsData, width, height, pixels).success()) {
            // TODO: 将解码后的像素数据上传到GuiTextureAtlas，当前仅设置了图集尺寸
            atlas.setAtlasSize(width, height);
            hasWidgets = true;
        }
    }

    // 尝试加载icons.png
    std::vector<u8> iconsData;
    auto iconsResult = _findTexture("minecraft:textures/gui/icons.png", iconsData);
    if (iconsResult.success()) {
        i32 width, height;
        std::vector<u8> pixels;
        if (decodePng(iconsData, width, height, pixels).success()) {
            // TODO: 将解码后的像素数据上传到GuiTextureAtlas，当前仅设置了图集尺寸
            // 如果widgets.png还没设置尺寸，使用icons.png的尺寸
            if (!hasWidgets) {
                atlas.setAtlasSize(width, height);
            }
        }
    }

    // 尝试从JSON加载精灵定义
    // widgets精灵定义
    auto widgetsJsonResult = loadSpritesFromJson(atlas, "minecraft:gui/sprites/widgets.json");
    if (widgetsJsonResult.failed()) {
        // JSON不存在，使用硬编码的默认精灵
        GuiSpriteRegistry::registerWidgetsSprites(atlas);
    }

    // icons精灵定义
    auto iconsJsonResult = loadSpritesFromJson(atlas, "minecraft:gui/sprites/icons.json");
    if (iconsJsonResult.failed()) {
        // JSON不存在，使用硬编码的默认精灵
        GuiSpriteRegistry::registerIconsSprites(atlas);
    }

    // 容器精灵（通常从硬编码注册）
    GuiSpriteRegistry::registerContainerSprites(atlas);

    return {};
}

// ==================== 工具方法 ====================

Result<void> GuiTextureLoader::decodePng(
    const std::vector<u8>& data, i32& outWidth, i32& outHeight, std::vector<u8>& outPixels)
{

    i32 width, height, channels;
    u8* pixels = stbi_load_from_memory(data.data(),
        static_cast<i32>(data.size()),
        &width,
        &height,
        &channels,
        4 // 强制RGBA
    );

    if (!pixels) {
        return Error(ErrorCode::TextureLoadFailed, std::string("Failed to decode PNG: ") + stbi_failure_reason());
    }

    outWidth = width;
    outHeight = height;
    outPixels.assign(pixels, pixels + static_cast<size_t>(width * height * 4));

    stbi_image_free(pixels);
    return {};
}

Result<void> GuiTextureLoader::loadPngFromFile(
    const std::string& filePath, i32& outWidth, i32& outHeight, std::vector<u8>& outPixels)
{

    i32 width, height, channels;
    u8* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 4);

    if (!pixels) {
        return Error(ErrorCode::TextureLoadFailed,
            std::string("Failed to load PNG from file: ") + filePath + " - " + stbi_failure_reason());
    }

    outWidth = width;
    outHeight = height;
    outPixels.assign(pixels, pixels + static_cast<size_t>(width * height * 4));

    stbi_image_free(pixels);
    return {};
}

Result<void> GuiTextureLoader::_findTexture(const std::string& location, std::vector<u8>& outData)
{

    if (m_resourcePacks.empty()) {
        spdlog::warn("[GuiTextureLoader] No resource packs available");
        return Error(ErrorCode::NotFound, "No resource packs available");
    }

    std::string resourcePath = _buildResourcePath(location);
    spdlog::info("[GuiTextureLoader] Looking for texture: location='{}', resourcePath='{}'", location, resourcePath);

    // 按优先级搜索资源包（后添加的优先）
    for (auto it = m_resourcePacks.rbegin(); it != m_resourcePacks.rend(); ++it) {
        auto& pack = *it;
        const std::string& packName = pack->name();

        spdlog::info("[GuiTextureLoader] Checking pack '{}'", packName);

        if (!pack->hasResource(resource::PackType::ClientResources, resourcePath)) {
            spdlog::info("[GuiTextureLoader] Pack '{}' does not have resource '{}'", packName, resourcePath);
            continue;
        }

        spdlog::info("[GuiTextureLoader] Pack '{}' has resource '{}'", packName, resourcePath);

        auto readResult = pack->readResource(resource::PackType::ClientResources, resourcePath);
        if (readResult.success()) {
            outData = std::move(readResult.value());
            spdlog::info("[GuiTextureLoader] Successfully read {} bytes from pack '{}'", outData.size(), packName);
            return {};
        } else {
            spdlog::warn(
                "[GuiTextureLoader] Failed to read from pack '{}': {}", packName, readResult.error().toString());
        }
    }

    spdlog::warn("[GuiTextureLoader] Texture not found in any resource pack: {}", location);
    return Error(ErrorCode::NotFound, std::string("Texture not found: ") + location);
}

std::string GuiTextureLoader::_buildResourcePath(const std::string& location)
{
    // 转换 minecraft:textures/gui/widgets.png -> assets/minecraft/textures/gui/widgets.png
    auto colonPos = location.find(':');
    if (colonPos != std::string::npos) {
        std::string namespace_ = location.substr(0, colonPos);
        std::string path = location.substr(colonPos + 1);
        return namespace_ + "/" + path;
    }
    // 默认命名空间
    return "minecraft/" + location;
}

} // namespace mc::client::renderer::trident::gui
