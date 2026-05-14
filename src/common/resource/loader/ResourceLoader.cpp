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

#include "ResourceLoader.hpp"
#include "../FolderResourcePack.hpp"
#include <set>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>

namespace mc {
namespace resource {
namespace loader {

ResourceLoader::ResourceLoader() = default;
ResourceLoader::~ResourceLoader() = default;

Result<void> ResourceLoader::addResourcePack(ResourcePackPtr pack)
{
    if (!pack) {
        return Error(ErrorCode::InvalidArgument, "资源包为空");
    }

    // 初始化包
    auto result = pack->initialize();
    if (result.failed()) {
        return result.error();
    }

    // 检测格式
    compat::PackFormat format = detectFormat(*pack);

    return addResourcePack(std::move(pack), format);
}

Result<void> ResourceLoader::addResourcePack(ResourcePackPtr pack, compat::PackFormat format)
{
    if (!pack) {
        return Error(ErrorCode::InvalidArgument, "资源包为空");
    }

    // 创建适当的映射器
    auto mapper = compat::ResourceMapper::create(format);

    spdlog::info("[ResourceLoader] 添加包 '{}'，格式 {}", pack->name(), compat::packFormatToString(format));

    m_packs.push_back({std::move(pack), format, std::move(mapper)});
    return Result<void>::ok();
}

void ResourceLoader::clearResourcePacks()
{
    m_packs.clear();
    m_stats.reset();
}

compat::PackFormat ResourceLoader::detectFormat(const IResourcePack& pack)
{
    // 尝试读取 pack.mcmeta
    auto result = pack.readTextResource("pack.mcmeta");
    if (result.failed()) {
        // 如果没有元数据，默认使用现代格式
        spdlog::debug("[ResourceLoader] 未找到 pack.mcmeta，默认使用 1.13+ 格式");
        return compat::PackFormat::V1_13_to_1_14;
    }

    try {
        auto json = nlohmann::json::parse(result.value());
        if (json.contains("pack") && json["pack"].contains("pack_format")) {
            i32 format = json["pack"]["pack_format"].get<i32>();
            return compat::detectPackFormat(format);
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("[ResourceLoader] 解析 pack.mcmeta 失败: {}", e.what());
    }

    // 默认使用现代格式
    return compat::PackFormat::V1_13_to_1_14;
}

std::vector<compat::unified::UnifiedTexture> ResourceLoader::loadTextures()
{
    std::vector<compat::unified::UnifiedTexture> textures;
    std::set<std::string> loadedLocations; // 跟踪已加载的位置以避免重复

    m_stats.texturesLoaded = 0;
    m_stats.texturesFailed = 0;

    // 逆序遍历包（后添加的包优先级更高）
    for (auto it = m_packs.rbegin(); it != m_packs.rend(); ++it) {
        const auto& ctx = *it;

        // 列出纹理文件
        auto listResult = ctx.pack->listResources("textures/block", ".png");
        if (listResult.success()) {
            for (const auto& file : listResult.value()) {
                // 转换为统一位置
                std::string unifiedPath = ctx.mapper->toUnifiedTexturePath(file);

                // 跳过已加载的
                if (loadedLocations.count(unifiedPath)) {
                    continue;
                }

                // 加载纹理
                auto texResult = loadTexture(ResourceLocation(unifiedPath));
                if (texResult.success()) {
                    textures.push_back(std::move(texResult.value()));
                    loadedLocations.insert(unifiedPath);
                    m_stats.texturesLoaded++;

                    if (m_textureCallback) {
                        m_textureCallback(unifiedPath, true);
                    }
                } else {
                    m_stats.texturesFailed++;

                    if (m_textureCallback) {
                        m_textureCallback(unifiedPath, false);
                    }
                }
            }
        }

        // 也列出 textures/items/ 中的物品纹理
        listResult = ctx.pack->listResources("textures/item", ".png");
        if (listResult.success()) {
            for (const auto& file : listResult.value()) {
                std::string unifiedPath = ctx.mapper->toUnifiedTexturePath(file);

                if (loadedLocations.count(unifiedPath)) {
                    continue;
                }

                auto texResult = loadTexture(ResourceLocation(unifiedPath));
                if (texResult.success()) {
                    textures.push_back(std::move(texResult.value()));
                    loadedLocations.insert(unifiedPath);
                    m_stats.texturesLoaded++;

                    if (m_textureCallback) {
                        m_textureCallback(unifiedPath, true);
                    }
                } else {
                    m_stats.texturesFailed++;

                    if (m_textureCallback) {
                        m_textureCallback(unifiedPath, false);
                    }
                }
            }
        }

        // 对于 1.12 包，还要检查 textures/blocks/ 和 textures/items/
        if (compat::usesOldTexturePaths(ctx.format)) {
            listResult = ctx.pack->listResources("textures/blocks", ".png");
            if (listResult.success()) {
                for (const auto& file : listResult.value()) {
                    std::string unifiedPath = ctx.mapper->toUnifiedTexturePath(file);

                    if (loadedLocations.count(unifiedPath)) {
                        continue;
                    }

                    auto texResult = loadTexture(ResourceLocation(unifiedPath));
                    if (texResult.success()) {
                        textures.push_back(std::move(texResult.value()));
                        loadedLocations.insert(unifiedPath);
                        m_stats.texturesLoaded++;

                        if (m_textureCallback) {
                            m_textureCallback(unifiedPath, true);
                        }
                    }
                }
            }
        }
    }

    spdlog::info("[ResourceLoader] 加载 {} 个纹理（{} 个失败）", m_stats.texturesLoaded, m_stats.texturesFailed);

    return textures;
}

Result<compat::unified::UnifiedTexture> ResourceLoader::loadTexture(const ResourceLocation& location)
{
    std::string unifiedPath = location.path();

    // 逆序尝试每个包
    for (auto it = m_packs.rbegin(); it != m_packs.rend(); ++it) {
        const auto& ctx = *it;

        // 获取路径变体
        auto variants = ctx.mapper->getTexturePathVariants(unifiedPath);

        for (const auto& variant : variants) {
            std::string filePath = ResourceLocation(location.namespace_(), variant).toFilePath("png");

            if (ctx.pack->hasResource(filePath)) {
                auto pixelsResult = readTexturePixels(*ctx.pack, filePath);
                if (pixelsResult.success()) {
                    compat::unified::UnifiedTexture texture;
                    texture.location = location;
                    texture.originalPath = variant;
                    texture.sourceFormat = ctx.format;
                    texture.type = compat::unified::ResourceType::Texture;
                    texture.pixels = std::move(pixelsResult.value());

                    return texture;
                }
            }
        }
    }

    return Error(ErrorCode::ResourceNotFound, "未找到纹理: " + location.toString());
}

Result<compat::unified::PixelData> ResourceLoader::readTexturePixels(const IResourcePack& pack, const std::string& path)
{
    auto readResult = pack.readResource(path);
    if (readResult.failed()) {
        return readResult.error();
    }

    const auto& data = readResult.value();
    int width, height, channels;
    stbi_uc* pixels =
        stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 4); // 强制 RGBA

    if (!pixels) {
        return Error(ErrorCode::TextureLoadFailed, "解码纹理失败: " + path);
    }

    compat::unified::PixelData pixelData;
    pixelData.width = static_cast<u32>(width);
    pixelData.height = static_cast<u32>(height);
    pixelData.data.resize(width * height * 4);
    std::memcpy(pixelData.data.data(), pixels, pixelData.data.size());

    stbi_image_free(pixels);
    return pixelData;
}

std::vector<compat::unified::UnifiedModel> ResourceLoader::loadModels()
{
    std::vector<compat::unified::UnifiedModel> models;
    std::set<std::string> loadedLocations;

    m_stats.modelsLoaded = 0;
    m_stats.modelsFailed = 0;

    // TODO: 实现模型加载
    // 这将解析 JSON 模型文件并创建 UnifiedModel 对象

    spdlog::info("[ResourceLoader] 加载 {} 个模型（{} 个失败）", m_stats.modelsLoaded, m_stats.modelsFailed);

    return models;
}

Result<compat::unified::UnifiedModel> ResourceLoader::loadModel(const ResourceLocation& location)
{
    // TODO: 实现模型加载
    (void)location;
    return Error(ErrorCode::Unsupported, "模型加载尚未实现");
}

std::vector<compat::unified::UnifiedBlockState> ResourceLoader::loadBlockStates()
{
    std::vector<compat::unified::UnifiedBlockState> blockStates;
    std::set<std::string> loadedLocations;

    m_stats.blockStatesLoaded = 0;
    m_stats.blockStatesFailed = 0;

    // TODO: 实现方块状态加载
    // 这将解析 JSON 方块状态文件并创建 UnifiedBlockState 对象

    spdlog::info(
        "[ResourceLoader] 加载 {} 个方块状态（{} 个失败）", m_stats.blockStatesLoaded, m_stats.blockStatesFailed);

    return blockStates;
}

Result<compat::unified::UnifiedBlockState> ResourceLoader::loadBlockState(const ResourceLocation& location)
{
    // TODO: 实现方块状态加载
    (void)location;
    return Error(ErrorCode::Unsupported, "方块状态加载尚未实现");
}

std::pair<const IResourcePack*, std::string> ResourceLoader::findTexture(const std::string& unifiedPath)
{
    for (auto it = m_packs.rbegin(); it != m_packs.rend(); ++it) {
        const auto& ctx = *it;

        auto variants = ctx.mapper->getTexturePathVariants(unifiedPath);
        for (const auto& variant : variants) {
            std::string filePath = ResourceLocation(variant).toFilePath("png");
            if (ctx.pack->hasResource(filePath)) {
                return {ctx.pack.get(), filePath};
            }
        }
    }
    return {nullptr, ""};
}

} // namespace loader
} // namespace resource
} // namespace mc
