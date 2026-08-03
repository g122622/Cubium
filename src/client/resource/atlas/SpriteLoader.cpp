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
 */

#include "client/resource/atlas/SpriteLoader.hpp"
#include "client/resource/atlas/SpriteContents.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <stb_image.h>

namespace mc::client::resource::atlas {

SpriteLoader SpriteLoader::fromTextureResource(ResourceLocation textureLocation)
{
    return SpriteLoader(TextureResource{std::move(textureLocation)});
}

SpriteLoader SpriteLoader::fromPredecoded(SpriteContents contents)
{
    return SpriteLoader(Predecoded{std::make_shared<SpriteContents>(std::move(contents))});
}

bool SpriteLoader::isPredecoded() const
{
    return std::holds_alternative<Predecoded>(m_state);
}

Result<SpriteContents> SpriteLoader::_decodeFromPack(IResourcePack& pack, const ResourceLocation& loc)
{
    // loc 为 sprite 名风格（如 minecraft:block/stone，不含 textures/ 前缀）。
    // 对齐原版 FileToIdConverter("textures",".png")：sprite id -> <ns>/textures/<path>.png
    const std::string filePath = loc.namespace_() + "/textures/" + loc.path() + ".png";

    const auto readResult = pack.readResource(mc::resource::PackType::ClientResources, filePath);
    if (readResult.failed()) {
        return readResult.error();
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(
        readResult.value().data(), static_cast<int>(readResult.value().size()), &width, &height, &channels, 4);

    if (pixels == nullptr || width <= 0 || height <= 0) {
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }
        return Error(ErrorCode::TextureLoadFailed, "Failed to decode texture: " + loc.toString());
    }

    SpriteContents contents;
    contents.width = static_cast<u32>(width);
    contents.height = static_cast<u32>(height);
    contents.pixels.assign(pixels, pixels + (static_cast<size_t>(width) * static_cast<size_t>(height) * 4));
    stbi_image_free(pixels);

    // 读取同名 .mcmeta 动画元数据（若存在）
    const std::string mcmetaPath = filePath + ".mcmeta";
    if (pack.hasResource(mc::resource::PackType::ClientResources, mcmetaPath)) {
        const auto mcmetaResult = pack.readResource(mc::resource::PackType::ClientResources, mcmetaPath);
        if (mcmetaResult.success()) {
            auto metadata = mc::resource::metadata::AnimationMetadata::fromMcmeta(
                mcmetaResult.value(), contents.width, contents.height);
            if (metadata.width > 0 && metadata.height > 0) {
                contents.animation = std::move(metadata);
            }
        }
    }

    return contents;
}

Result<SpriteContents> SpriteLoader::resolve(const std::vector<ResourcePackPtr>& packs) const
{
    if (const auto* predecoded = std::get_if<Predecoded>(&m_state)) {
        // unstitch/paletted 产出的像素直接返回（拷贝一份，因 builder 会消费）
        return *predecoded->contents;
    }

    const auto& tex = std::get<TextureResource>(m_state);
    // 按资源包优先级查找（后添加优先），与现有图集构建一致
    for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
        if (!*it) {
            continue;
        }
        auto result = _decodeFromPack(**it, tex.textureLocation);
        if (result.success()) {
            return result;
        }
    }
    return Error(ErrorCode::ResourceNotFound, "Texture not found in any pack: " + tex.textureLocation.toString());
}

} // namespace mc::client::resource::atlas
