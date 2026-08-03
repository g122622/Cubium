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

#include "client/resource/atlas/Sources.hpp"

#include "client/resource/atlas/AtlasSource.hpp"
#include "client/resource/atlas/SpriteContents.hpp"
#include "client/resource/atlas/SpriteLoader.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <stb_image.h>

namespace mc::client::resource::atlas {

namespace {

/// 从 listResources 返回的相对路径（如 "minecraft/textures/block/stone.png"）
/// 提取 namespace 与去掉 "textures/<source>/" 前缀和 ".png" 后缀的 sprite 相对名。
/// 返回 false 表示路径不符合预期格式。
bool _extractDirectorySprite(
    std::string_view relPath, std::string_view sourceDir, std::string& outNs, std::string& outRelative)
{
    // relPath 形如 "<namespace>/textures/<source>/<sub...>.png"
    auto firstSlash = relPath.find('/');
    if (firstSlash == std::string_view::npos) {
        return false;
    }
    outNs = std::string(relPath.substr(0, firstSlash));
    std::string_view rest = relPath.substr(firstSlash + 1);

    // rest 形如 "textures/<source>/<sub...>.png"，剥掉 "textures/" 前缀
    constexpr std::string_view texturesPrefix = "textures/";
    if (rest.size() <= texturesPrefix.size() || rest.compare(0, texturesPrefix.size(), texturesPrefix) != 0) {
        return false;
    }
    rest = rest.substr(texturesPrefix.size());

    // 剥掉 "<source>/" 前缀
    if (rest.size() <= sourceDir.size() || rest.compare(0, sourceDir.size(), sourceDir) != 0 ||
        rest[sourceDir.size()] != '/') {
        return false;
    }
    rest = rest.substr(sourceDir.size() + 1);

    // 剥掉 ".png" 后缀
    constexpr std::string_view pngExt = ".png";
    if (rest.size() <= pngExt.size() || rest.compare(rest.size() - pngExt.size(), pngExt.size(), pngExt) != 0) {
        return false;
    }
    outRelative = std::string(rest.substr(0, rest.size() - pngExt.size()));
    return true;
}

/// 解码 PNG 到 RGBA8 像素（unstitch/paletted 共用）
struct DecodedImage {
    std::vector<u8> pixels;
    u32 width = 0;
    u32 height = 0;
};

[[nodiscard]] Result<DecodedImage> _decodePng(IResourcePack& pack, const ResourceLocation& loc)
{
    // loc 为 sprite 名风格（如 minecraft:colormap/pumpkin），文件路径 = <ns>/textures/<path>.png
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
        return Error(ErrorCode::TextureLoadFailed, "Failed to decode image: " + loc.toString());
    }

    DecodedImage img;
    img.width = static_cast<u32>(width);
    img.height = static_cast<u32>(height);
    img.pixels.assign(pixels, pixels + (static_cast<size_t>(width) * static_cast<size_t>(height) * 4));
    stbi_image_free(pixels);
    return img;
}

/// 从大图裁剪子区域像素（RGBA8）
[[nodiscard]] std::vector<u8> _cropRegion(const std::vector<u8>& src, u32 srcW, u32 srcH, u32 x, u32 y, u32 w, u32 h)
{
    // 裁剪到合法范围
    x = std::min(x, srcW);
    y = std::min(y, srcH);
    w = std::min(w, srcW - x);
    h = std::min(h, srcH - y);

    std::vector<u8> dst(static_cast<size_t>(w) * h * 4, 0);
    for (u32 row = 0; row < h; ++row) {
        const size_t srcOffset = (static_cast<size_t>(y + row) * srcW + x) * 4;
        const size_t dstOffset = static_cast<size_t>(row) * w * 4;
        std::memcpy(dst.data() + dstOffset, src.data() + srcOffset, static_cast<size_t>(w) * 4);
    }
    return dst;
}

} // namespace

// ============================================================================
// SingleFileSource
// ============================================================================
Result<void> SingleFileSource::run(IResourcePack& pack, SpriteSourceOutput& output) const
{
    // m_resource 为 sprite 名风格（如 minecraft:block/stone），文件路径 = <ns>/textures/<path>.png
    const std::string filePath = m_resource.namespace_() + "/textures/" + m_resource.path() + ".png";
    if (!pack.hasResource(mc::resource::PackType::ClientResources, filePath)) {
        spdlog::warn("atlas single source: missing sprite resource {}", m_resource.toString());
        return Result<void>::ok();
    }
    output.add(m_sprite, SpriteLoader::fromTextureResource(m_resource));
    return Result<void>::ok();
}

std::string SingleFileSource::describe() const
{
    return "single(resource=" + m_resource.toString() + ")";
}

// ============================================================================
// DirectoryListerSource
// ============================================================================
Result<void> DirectoryListerSource::run(IResourcePack& pack, SpriteSourceOutput& output) const
{
    // Cubium 的 listResources 以 <ns>/<dir> 为目录参数（对齐 BlockStateLoader 范式），
    // 故需逐命名空间枚举 textures/<source>/**/*.png。
    auto nsResult = pack.getResourceNamespaces(mc::resource::PackType::ClientResources);
    if (nsResult.failed()) {
        return Result<void>::ok();
    }

    for (const auto& ns : nsResult.value()) {
        const std::string dir = ns + "/textures/" + m_source;
        auto listResult = pack.listResources(mc::resource::PackType::ClientResources, dir, "png");
        if (listResult.failed()) {
            // 目录不存在不算错误
            continue;
        }
        for (const auto& relPath : listResult.value()) {
            std::string pathNs;
            std::string relative;
            if (!_extractDirectorySprite(relPath, m_source, pathNs, relative)) {
                continue;
            }
            // sprite 名 = prefix + relative（如 block/stone）
            const std::string spritePath = m_prefix + relative;
            const ResourceLocation spriteLoc(pathNs, spritePath);
            // 纹理资源位置为 sprite 名风格（如 block/stone），SpriteLoader 解码时补 textures/ 前缀
            const ResourceLocation texLoc(pathNs, m_source + "/" + relative);
            output.add(spriteLoc, SpriteLoader::fromTextureResource(texLoc));
        }
    }
    return Result<void>::ok();
}

std::string DirectoryListerSource::describe() const
{
    return "directory(source=" + m_source + ", prefix=" + m_prefix + ")";
}

// ============================================================================
// FilterSource
// ============================================================================
Result<void> FilterSource::run(IResourcePack& /*pack*/, SpriteSourceOutput& output) const
{
    output.removeAll(m_pattern);
    return Result<void>::ok();
}

std::string FilterSource::describe() const
{
    return "filter";
}

// ============================================================================
// UnstitcherSource
// ============================================================================
Result<void> UnstitcherSource::run(IResourcePack& pack, SpriteSourceOutput& output) const
{
    // 解码大图（一次解码供所有 region 共享）
    auto imgResult = _decodePng(pack, m_resource);
    if (imgResult.failed()) {
        spdlog::warn("atlas unstitch source: failed to decode source image {}: {}",
            m_resource.toString(),
            imgResult.error().message());
        return Result<void>::ok();
    }
    const auto& img = imgResult.value();

    for (const auto& region : m_regions) {
        // 实际像素 = floor(region.x * imgW / divisorX)
        const double unitX = (m_divisorX > 0.0) ? (static_cast<double>(img.width) / m_divisorX) : 1.0;
        const double unitY = (m_divisorY > 0.0) ? (static_cast<double>(img.height) / m_divisorY) : 1.0;

        const u32 px = static_cast<u32>(std::floor(region.x * unitX));
        const u32 py = static_cast<u32>(std::floor(region.y * unitY));
        const u32 pw = static_cast<u32>(std::floor(region.width * unitX));
        const u32 ph = static_cast<u32>(std::floor(region.height * unitY));

        if (pw == 0 || ph == 0) {
            continue;
        }

        SpriteContents contents;
        contents.width = pw;
        contents.height = ph;
        contents.pixels = _cropRegion(img.pixels, img.width, img.height, px, py, pw, ph);
        output.add(region.sprite, SpriteLoader::fromPredecoded(std::move(contents)));
    }
    return Result<void>::ok();
}

std::string UnstitcherSource::describe() const
{
    return "unstitch(resource=" + m_resource.toString() + ", regions=" + std::to_string(m_regions.size()) + ")";
}

// ============================================================================
// PalettedPermutationsSource
// ============================================================================
Result<void> PalettedPermutationsSource::run(IResourcePack& pack, SpriteSourceOutput& output) const
{
    // 解码 palette_key 像素
    auto keyResult = _decodePng(pack, m_paletteKey);
    if (keyResult.failed()) {
        spdlog::warn("atlas paletted_permutations: failed to decode palette_key {}: {}",
            m_paletteKey.toString(),
            keyResult.error().message());
        return Result<void>::ok();
    }
    const auto& keyImg = keyResult.value();

    // 对每个 permutation 解码调色板像素，构造颜色映射
    // 映射：paletteKey[i] 的 RGB -> permPalette[i] 的 RGB（按位置对应）
    struct PaletteMapping {
        std::string name;
        std::vector<u8> palettePixels; // 与 keyImg 等尺寸
    };
    std::vector<PaletteMapping> mappings;
    mappings.reserve(m_permutations.size());
    for (const auto& [name, paletteLoc] : m_permutations) {
        auto permResult = _decodePng(pack, paletteLoc);
        if (permResult.failed()) {
            spdlog::warn("atlas paletted_permutations: failed to decode permutation '{}' palette {}: {}",
                name,
                paletteLoc.toString(),
                permResult.error().message());
            continue;
        }
        const auto& permImg = permResult.value();
        if (permImg.width != keyImg.width || permImg.height != keyImg.height) {
            // 调色板必须等长（等尺寸），否则跳过该 permutation
            spdlog::warn(
                "atlas paletted_permutations: permutation '{}' palette size {}x{} != palette_key {}x{}, skipping",
                name,
                permImg.width,
                permImg.height,
                keyImg.width,
                keyImg.height);
            continue;
        }
        mappings.push_back({name, std::move(permImg.pixels)});
    }

    // 对每个基础纹理 × 每个 permutation 生成衍生 sprite
    for (const auto& baseTex : m_textures) {
        auto baseResult = _decodePng(pack, baseTex);
        if (baseResult.failed()) {
            spdlog::warn("atlas paletted_permutations: failed to decode base texture {}: {}",
                baseTex.toString(),
                baseResult.error().message());
            continue;
        }
        const auto& baseImg = baseResult.value();

        for (const auto& mapping : mappings) {
            // 衍生 sprite 名 = baseTex.path() + separator + permutationName
            const std::string spritePath = baseTex.path() + m_separator + mapping.name;
            const ResourceLocation spriteLoc(baseTex.namespace_(), spritePath);

            // 应用调色板映射：对 baseImg 每像素，按 RGB 在 paletteKey 查找位置 i，
            // 取 permPalette[i] 的 RGB，保留输入 alpha
            SpriteContents contents;
            contents.width = baseImg.width;
            contents.height = baseImg.height;
            contents.pixels.resize(baseImg.pixels.size(), 0);

            const size_t pixelCount = static_cast<size_t>(keyImg.width) * keyImg.height;
            for (size_t i = 0; i < baseImg.pixels.size() / 4; ++i) {
                const size_t baseIdx = i * 4;
                const u8 br = baseImg.pixels[baseIdx + 0];
                const u8 bg = baseImg.pixels[baseIdx + 1];
                const u8 bb = baseImg.pixels[baseIdx + 2];
                const u8 ba = baseImg.pixels[baseIdx + 3];

                // 在 paletteKey 中查找相同 RGB 的像素位置
                u8 outR = br, outG = bg, outB = bb;
                for (size_t k = 0; k < pixelCount; ++k) {
                    const size_t keyIdx = k * 4;
                    // 忽略 alpha=0 的透明槽位（原版行为）
                    if (keyImg.pixels[keyIdx + 3] == 0) {
                        continue;
                    }
                    if (keyImg.pixels[keyIdx + 0] == br && keyImg.pixels[keyIdx + 1] == bg &&
                        keyImg.pixels[keyIdx + 2] == bb) {
                        // 取目标调色板的 RGB，保留输入 alpha（原版 ARGB.color(alpha*targetAlpha/255, target)）
                        outR = mapping.palettePixels[keyIdx + 0];
                        outG = mapping.palettePixels[keyIdx + 1];
                        outB = mapping.palettePixels[keyIdx + 2];
                        break;
                    }
                }
                contents.pixels[baseIdx + 0] = outR;
                contents.pixels[baseIdx + 1] = outG;
                contents.pixels[baseIdx + 2] = outB;
                contents.pixels[baseIdx + 3] = ba;
            }

            output.add(spriteLoc, SpriteLoader::fromPredecoded(std::move(contents)));
        }
    }
    return Result<void>::ok();
}

std::string PalettedPermutationsSource::describe() const
{
    return "paletted_permutations(textures=" + std::to_string(m_textures.size()) +
        ", permutations=" + std::to_string(m_permutations.size()) + ")";
}

} // namespace mc::client::resource::atlas
