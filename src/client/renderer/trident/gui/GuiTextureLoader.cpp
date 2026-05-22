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
#include "GuiSpriteAtlas.hpp"
#include "GuiSpriteParser.hpp"
#include "GuiSpriteRegistry.hpp"
#include "GuiTextureAtlas.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <algorithm>
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
    auto result = findTexture(location, textureData);
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
    auto result = findTexture(location, textureData);
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
    auto result = findTexture(location, textureData);
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
    std::string resourcePath = buildResourcePath(jsonPath);

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
    auto widgetsResult = findTexture("minecraft:textures/gui/widgets.png", widgetsData);
    if (widgetsResult.success()) {
        i32 width, height;
        std::vector<u8> pixels;
        if (decodePng(widgetsData, width, height, pixels).success()) {
            atlas.setAtlasSize(width, height);
            hasWidgets = true;
        }
    }

    // 尝试加载icons.png
    std::vector<u8> iconsData;
    auto iconsResult = findTexture("minecraft:textures/gui/icons.png", iconsData);
    if (iconsResult.success()) {
        i32 width, height;
        std::vector<u8> pixels;
        if (decodePng(iconsData, width, height, pixels).success()) {
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

    int width, height, channels;
    u8* pixels = stbi_load_from_memory(data.data(),
        static_cast<int>(data.size()),
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

    int width, height, channels;
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

Result<void> GuiTextureLoader::findTexture(const std::string& location, std::vector<u8>& outData)
{

    if (m_resourcePacks.empty()) {
        spdlog::warn("[GuiTextureLoader] No resource packs available");
        return Error(ErrorCode::NotFound, "No resource packs available");
    }

    std::string resourcePath = buildResourcePath(location);
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

std::string GuiTextureLoader::buildResourcePath(const std::string& location)
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
