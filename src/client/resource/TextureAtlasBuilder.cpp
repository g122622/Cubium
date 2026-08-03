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

#include "TextureAtlasBuilder.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"

// 只在stb_image.h未被包含时定义实现
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#endif
#include <stb_image.h>

// 只在stb_image_write.h未被包含时定义实现
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <stb_image_write.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace mc {

namespace {

[[nodiscard]] std::vector<u8> extractTopLeftFrameRgba(
    const std::vector<u8>& pixels, u32 imageWidth, u32 imageHeight, u32 frameWidth, u32 frameHeight)
{
    if (frameWidth == 0 || frameHeight == 0 || frameWidth > imageWidth || frameHeight > imageHeight ||
        pixels.size() < static_cast<size_t>(imageWidth) * static_cast<size_t>(imageHeight) * 4) {
        return pixels;
    }

    if (frameWidth == imageWidth && frameHeight == imageHeight) {
        return pixels;
    }

    std::vector<u8> framePixels(static_cast<size_t>(frameWidth) * static_cast<size_t>(frameHeight) * 4, 0);

    for (u32 y = 0; y < frameHeight; ++y) {
        const u8* src = pixels.data() + (static_cast<size_t>(y) * imageWidth * 4);
        u8* dst = framePixels.data() + (static_cast<size_t>(y) * frameWidth * 4);
        std::memcpy(dst, src, static_cast<size_t>(frameWidth) * 4);
    }

    return framePixels;
}

} // namespace

void TextureAtlasBuilder::setMaxSize(u32 width, u32 height)
{
    m_maxWidth = width;
    m_maxHeight = height;
}

void TextureAtlasBuilder::setPadding(u32 padding)
{
    m_padding = padding;
}

Result<void> TextureAtlasBuilder::addTexture(IResourcePack& resourcePack, const ResourceLocation& location)
{
    // 检查是否已添加
    if (m_addedLocations.count(location) > 0) {
        return Result<void>::ok();
    }

    // 构建文件路径
    std::string filePath = location.toFilePath(resource::PackType::ClientResources, "png");
    filePath.erase(0, std::string("assets/").size());

    // 读取纹理数据
    auto readResult = resourcePack.readResource(resource::PackType::ClientResources, filePath);
    if (readResult.failed()) {
        return readResult.error();
    }

    auto& data = readResult.value();

    // 解析PNG
    int width, height, channels;
    stbi_uc* pixels = stbi_load_from_memory(data.data(),
        static_cast<int>(data.size()),
        &width,
        &height,
        &channels,
        4); // 强制RGBA

    if (!pixels) {
        return Error(ErrorCode::TextureLoadFailed, std::string("Failed to decode PNG: ") + location.toString());
    }

    // 复制像素数据
    std::vector<u8> pixelData(pixels, pixels + width * height * 4);
    stbi_image_free(pixels);

    // 添加纹理
    addTexture(location, pixelData, static_cast<u32>(width), static_cast<u32>(height));

    return Result<void>::ok();
}

void TextureAtlasBuilder::addTexture(
    const ResourceLocation& location, const std::vector<u8>& pixels, u32 width, u32 height)
{
    addTextureFrame(location, pixels, width, height, width, height);
}

void TextureAtlasBuilder::addTextureFrame(const ResourceLocation& location,
    const std::vector<u8>& pixels,
    u32 width,
    u32 height,
    u32 frameWidth,
    u32 frameHeight)
{
    // 无动画元数据时使用默认构造的 AnimationMetadata
    addTextureFrame(location, pixels, width, height, frameWidth, frameHeight, resource::metadata::AnimationMetadata{});
}

void TextureAtlasBuilder::addTextureFrame(const ResourceLocation& location,
    const std::vector<u8>& pixels,
    u32 width,
    u32 height,
    u32 frameWidth,
    u32 frameHeight,
    const resource::metadata::AnimationMetadata& metadata)
{
    // 检查是否已添加
    if (m_addedLocations.count(location) > 0) {
        return;
    }

    if (width == 0 || height == 0) {
        return;
    }

    const u32 normalizedFrameWidth = (frameWidth == 0) ? width : frameWidth;
    const u32 normalizedFrameHeight = (frameHeight == 0) ? height : frameHeight;

    // 仅当帧尺寸合法且与整图不同才裁剪首帧。
    const bool canUseFrameSize = normalizedFrameWidth <= width && normalizedFrameHeight <= height &&
        (width % normalizedFrameWidth == 0) && (height % normalizedFrameHeight == 0);

    const u32 storedWidth = canUseFrameSize ? normalizedFrameWidth : width;
    const u32 storedHeight = canUseFrameSize ? normalizedFrameHeight : height;

    TextureInfo info;
    info.location = location;
    info.pixels = canUseFrameSize ? extractTopLeftFrameRgba(pixels, width, height, storedWidth, storedHeight) : pixels;
    info.width = storedWidth;
    info.height = storedHeight;

    m_textures.push_back(std::move(info));
    m_addedLocations.insert(location);

    // 如果是动画纹理（帧数 > 1），保存完整帧数据
    if (canUseFrameSize && height > storedHeight) {
        u32 frameCount = height / storedHeight;
        if (frameCount > 1) {
            PendingAnimation anim;
            anim.location = location;
            anim.frameWidth = storedWidth;
            anim.frameHeight = storedHeight;
            anim.metadata = metadata;

            // 提取所有帧
            anim.framePixels.reserve(frameCount);
            for (u32 i = 0; i < frameCount; ++i) {
                std::vector<u8> frameData(static_cast<size_t>(storedWidth) * storedHeight * 4);
                for (u32 row = 0; row < storedHeight; ++row) {
                    const u8* src = pixels.data() + (static_cast<size_t>(i * storedHeight + row) * width * 4);
                    u8* dst = frameData.data() + (static_cast<size_t>(row) * storedWidth * 4);
                    std::memcpy(dst, src, static_cast<size_t>(storedWidth) * 4);
                }
                anim.framePixels.push_back(std::move(frameData));
            }

            m_pendingAnimations.push_back(std::move(anim));
        }
    }
}

Result<AtlasBuildResult> TextureAtlasBuilder::build()
{
    if (m_textures.empty()) {
        AtlasBuildResult empty;
        empty.width = 1;
        empty.height = 1;
        empty.pixels.resize(4, 0);
        return empty;
    }

    // 按面积从大到小排序
    std::vector<size_t> indices(m_textures.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }

    std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
        u32 areaA = m_textures[a].width * m_textures[a].height;
        u32 areaB = m_textures[b].width * m_textures[b].height;
        return areaA > areaB;
    });

    // Skyline打包算法
    std::vector<SkylineNode> skyline;
    skyline.push_back({0, 0, m_maxWidth});

    // 计算需要的图集尺寸
    u32 atlasWidth = 0;
    u32 atlasHeight = 0;

    // 首先确定最小需要的尺寸
    for (const auto& tex : m_textures) {
        atlasWidth = std::max(atlasWidth, tex.width + m_padding * 2);
        atlasHeight = std::max(atlasHeight, tex.height + m_padding * 2);
    }

    // 从最小可行尺寸开始尝试（2 的幂）
    u32 tryWidth = 64;
    u32 tryHeight = 64;
    while (tryWidth < atlasWidth && tryWidth < m_maxWidth)
        tryWidth *= 2;
    while (tryHeight < atlasHeight && tryHeight < m_maxHeight)
        tryHeight *= 2;

    tryWidth = std::min(tryWidth, m_maxWidth);
    tryHeight = std::min(tryHeight, m_maxHeight);

    // 实际使用的尺寸
    u32 actualWidth = tryWidth;
    u32 actualHeight = tryHeight;

    // 存储放置结果
    struct PlacedTexture {
        size_t index;
        u32 x, y;
    };
    std::vector<PlacedTexture> placed;
    placed.reserve(m_textures.size());

    // 尝试打包
    bool success = false;
    while (true) {
        skyline.clear();
        skyline.push_back({0, 0, actualWidth});
        placed.clear();
        success = true;

        for (size_t idx : indices) {
            const auto& tex = m_textures[idx];
            u32 texW = tex.width + m_padding * 2;
            u32 texH = tex.height + m_padding * 2;

            u32 x, y;
            size_t nodeIndex;
            if (_canPlace(skyline, texW, texH, actualWidth, actualHeight, x, y, nodeIndex)) {
                placed.push_back({idx, x + m_padding, y + m_padding});
                _placeTexture(skyline, x, y, texW, texH, nodeIndex);

                // 更新图集高度
                atlasHeight = std::max(atlasHeight, y + texH);
            } else {
                success = false;
                break;
            }
        }

        if (success) break;

        // 尝试增大尺寸
        bool expanded = false;
        if (actualWidth < m_maxWidth) {
            actualWidth = std::min(actualWidth * 2, m_maxWidth);
            expanded = true;
        } else if (actualHeight < m_maxHeight) {
            actualHeight = std::min(actualHeight * 2, m_maxHeight);
            expanded = true;
        }

        if (!expanded) {
            break;
        }
    }

    if (!success) {
        return Error(ErrorCode::TextureAtlasFull, "Texture atlas is full, cannot fit all textures");
    }

    // 计算实际高度 (使用2的幂次)
    u32 finalHeight = 1;
    while (finalHeight < atlasHeight)
        finalHeight *= 2;
    finalHeight = std::min(finalHeight, actualHeight);

    // 创建图集像素缓冲区
    AtlasBuildResult result;
    result.width = actualWidth;
    result.height = finalHeight;
    result.pixels.resize(static_cast<size_t>(actualWidth) * finalHeight * 4, 0);

    // 复制纹理到图集
    for (const auto& p : placed) {
        const auto& tex = m_textures[p.index];

        if (p.x + tex.width > actualWidth || p.y + tex.height > finalHeight) {
            return Error(ErrorCode::InvalidState, "Texture placement exceeds atlas bounds");
        }

        // 复制像素
        for (u32 y = 0; y < tex.height; ++y) {
            const u8* src = tex.pixels.data() + y * tex.width * 4;
            u8* dst = result.pixels.data() + ((p.y + y) * actualWidth + p.x) * 4;
            std::memcpy(dst, src, tex.width * 4);
        }

        // 计算UV坐标
        TextureRegion region;
        region.u0 = static_cast<f32>(p.x) / static_cast<f32>(actualWidth);
        region.v0 = static_cast<f32>(p.y) / static_cast<f32>(finalHeight);
        region.u1 = static_cast<f32>(p.x + tex.width) / static_cast<f32>(actualWidth);
        region.v1 = static_cast<f32>(p.y + tex.height) / static_cast<f32>(finalHeight);

        result.regions[tex.location] = region;
    }

    // 填充动画描述符（设置 atlasX/atlasY 位置）
    for (auto& anim : m_pendingAnimations) {
        auto regionIt = result.regions.find(anim.location);
        if (regionIt != result.regions.end()) {
            const TextureRegion& region = regionIt->second;
            AnimationDescriptor desc;
            desc.location = anim.location;
            desc.atlasX = static_cast<u32>(region.u0 * static_cast<f32>(result.width));
            desc.atlasY = static_cast<u32>(region.v0 * static_cast<f32>(result.height));
            desc.frameWidth = anim.frameWidth;
            desc.frameHeight = anim.frameHeight;
            desc.framePixels = std::move(anim.framePixels);
            desc.metadata = std::move(anim.metadata);
            result.animations.push_back(std::move(desc));
        }
    }

    return result;
}

void TextureAtlasBuilder::clear()
{
    m_textures.clear();
    m_addedLocations.clear();
    m_pendingAnimations.clear();
}

std::vector<ResourceLocation> TextureAtlasBuilder::getTextureLocations() const
{
    std::vector<ResourceLocation> locations;
    locations.reserve(m_textures.size());
    for (const auto& tex : m_textures) {
        locations.push_back(tex.location);
    }
    return locations;
}

bool TextureAtlasBuilder::_canPlace(const std::vector<SkylineNode>& skyline,
    u32 width,
    u32 height,
    u32 maxWidth,
    u32 maxHeight,
    u32& outX,
    u32& outY,
    size_t& outIndex) const
{
    u32 bestY = std::numeric_limits<u32>::max();
    u32 bestX = 0;
    size_t bestIndex = 0;

    for (size_t i = 0; i < skyline.size(); ++i) {
        const auto& node = skyline[i];

        // 检查是否可以放在这个节点
        u32 x = node.x;
        u32 y = node.y;

        // 检查宽度是否足够
        u32 availableWidth = node.width;
        size_t j = i;

        // 检查后续节点是否可以提供足够的宽度
        while (availableWidth < width && j + 1 < skyline.size()) {
            ++j;
            const auto& nextNode = skyline[j];

            // 检查是否有高度差
            if (nextNode.y > node.y) {
                break;
            }

            // 下一个节点是否与当前节点相邻
            if (skyline[j - 1].x + skyline[j - 1].width != nextNode.x) {
                break;
            }

            availableWidth += nextNode.width;
        }

        if (availableWidth < width) {
            continue;
        }

        // 检查是否超出边界
        if (x + width > maxWidth || y + height > maxHeight) {
            continue;
        }

        // 选择最低的位置
        if (y < bestY) {
            bestY = y;
            bestX = x;
            bestIndex = i;
        }
    }

    if (bestY == std::numeric_limits<u32>::max()) {
        return false;
    }

    outX = bestX;
    outY = bestY;
    outIndex = bestIndex;
    return true;
}

void TextureAtlasBuilder::_placeTexture(
    std::vector<SkylineNode>& skyline, u32 x, u32 y, u32 width, u32 height, size_t index)
{
    // 创建新节点
    SkylineNode newNode;
    newNode.x = x;
    newNode.y = y + height;
    newNode.width = width;

    // 插入新节点
    skyline.insert(skyline.begin() + index, newNode);

    // 移除被覆盖的节点
    for (size_t i = index + 1; i < skyline.size();) {
        if (skyline[i].x < x + width) {
            // 这个节点被部分或完全覆盖
            u32 overlapEnd = skyline[i].x + skyline[i].width;
            u32 placeEnd = x + width;

            if (overlapEnd <= placeEnd) {
                // 完全覆盖，移除
                skyline.erase(skyline.begin() + i);
            } else {
                // 部分覆盖，调整
                skyline[i].x = placeEnd;
                skyline[i].width = overlapEnd - placeEnd;
                ++i;
            }
        } else {
            ++i;
        }
    }

    // 合并相邻的相同高度节点
    for (size_t i = 0; i + 1 < skyline.size();) {
        if (skyline[i].y == skyline[i + 1].y && skyline[i].x + skyline[i].width == skyline[i + 1].x) {
            skyline[i].width += skyline[i + 1].width;
            skyline.erase(skyline.begin() + i + 1);
        } else {
            ++i;
        }
    }
}

} // namespace mc
