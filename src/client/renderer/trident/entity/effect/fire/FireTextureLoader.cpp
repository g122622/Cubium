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

#include "FireTextureLoader.hpp"

#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/assert/AssertAll.hpp"

// STB 图像加载（仅 PNG）
#define STBI_ONLY_PNG
#include <string>
#include <vector>
#include <stb_image.h>

namespace mc::client::renderer::entity::effect::fire {

namespace {

// 火焰纹理路径（readResource 仅自动补 assets/ 前缀，命名空间 minecraft/ 需显式给出）
const char* FIRE_TEXTURE_PATHS[] = {"minecraft/textures/block/fire_0.png", "minecraft/textures/block/fire_1.png"};

/**
 * @brief 单张火焰纹理的解码结果
 *
 * 从一张 PNG（可能是动画条带）解码得到的所有帧数据，
 * 配合 mcmeta 解析出的动画元数据。
 */
struct SingleFireTexture {
    /// 所有帧纵向拼接后的 RGBA 像素（frameWidth * (frameHeight * frameCount) * 4 字节）
    std::vector<u8> pixels;
    u32 frameWidth = 0;
    u32 frameHeight = 0;
    u32 frameCount = 0;
    resource::metadata::AnimationMetadata metadata;
};

/**
 * @brief 从资源包读取 .png.mcmeta 动画元数据
 *
 * 若 mcmeta 不存在或解析失败，返回默认元数据（frametime=1、顺序播放）。
 *
 * @param pack 资源包
 * @param pngPath 对应的 PNG 路径（不含 .mcmeta 后缀）
 * @param imageWidth PNG 图像宽度
 * @param imageHeight PNG 图像高度
 * @return 动画元数据
 */
resource::metadata::AnimationMetadata readMcmeta(
    const IResourcePack* pack, const std::string& pngPath, u32 imageWidth, u32 imageHeight)
{
    const std::string mcmetaPath = pngPath + ".mcmeta";
    if (!pack->hasResource(resource::PackType::ClientResources, mcmetaPath)) {
        return resource::metadata::AnimationMetadata();
    }
    auto mcmetaResult = pack->readResource(resource::PackType::ClientResources, mcmetaPath);
    if (!mcmetaResult.success() || mcmetaResult.value().empty()) {
        return resource::metadata::AnimationMetadata();
    }
    return resource::metadata::AnimationMetadata::fromMcmeta(mcmetaResult.value(), imageWidth, imageHeight);
}

/**
 * @brief 从动画条带提取所有帧
 *
 * 将 sourceWidth × sourceHeight 的动画条带拆分为
 * (sourceHeight / frameHeight) 个 frameWidth × frameHeight 的帧，
 * 纵向拼接后返回。
 *
 * @param pixels STB 解码后的 RGBA 像素
 * @param sourceWidth 源图像宽度
 * @param sourceHeight 源图像高度
 * @param frameWidth 单帧宽度
 * @param frameHeight 单帧高度
 * @return 纵向拼接的所有帧像素
 */
std::vector<u8> extractAllFrames(const u8* pixels, u32 sourceWidth, u32 sourceHeight, u32 frameWidth, u32 frameHeight)
{
    MC_ASSERT_RELEASE(sourceWidth > 0 && sourceHeight > 0);
    MC_ASSERT_RELEASE(frameWidth > 0 && frameHeight > 0);
    MC_ASSERT_RELEASE(sourceWidth >= frameWidth && sourceHeight >= frameHeight);
    MC_ASSERT_RELEASE((sourceWidth % frameWidth) == 0 && (sourceHeight % frameHeight) == 0);

    const u32 framesPerRow = sourceWidth / frameWidth;
    const u32 framesPerCol = sourceHeight / frameHeight;
    const u32 totalFrames = framesPerRow * framesPerCol;

    std::vector<u8> result;
    result.reserve(static_cast<size_t>(frameWidth) * frameHeight * totalFrames * 4);

    // MC 动画纹理约定：帧从上到下、从左到右排列。
    // 实际原版 fire 纹理为单列条带（framesPerRow=1），但此处兼容多列布局。
    for (u32 frameIdx = 0; frameIdx < totalFrames; ++frameIdx) {
        const u32 frameCol = frameIdx % framesPerRow;
        const u32 frameRow = frameIdx / framesPerRow;

        const u32 frameOriginX = frameCol * frameWidth;
        const u32 frameOriginY = frameRow * frameHeight;

        for (u32 row = 0; row < frameHeight; ++row) {
            const u8* srcRow = pixels + static_cast<size_t>(frameOriginY + row) * sourceWidth * 4 + frameOriginX * 4;
            result.insert(result.end(), srcRow, srcRow + frameWidth * 4);
        }
    }
    return result;
}

/**
 * @brief 从资源包解码单张火焰纹理（含 mcmeta 动画元数据）
 *
 * @param resourcePacks 资源包列表（按优先级从低到高）
 * @param path PNG 路径
 * @return 解码结果，若未命中则 frameCount=0
 */
SingleFireTexture loadSingleFireTexture(const std::vector<IResourcePack*>& resourcePacks, const char* path)
{
    SingleFireTexture result;
    for (auto it = resourcePacks.rbegin(); it != resourcePacks.rend(); ++it) {
        auto* pack = *it;
        if (pack == nullptr) {
            continue;
        }

        auto readResult = pack->readResource(resource::PackType::ClientResources, path);
        if (!readResult.success() || readResult.value().empty()) {
            continue;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        u8* pixels = stbi_load_from_memory(
            readResult.value().data(), static_cast<int>(readResult.value().size()), &width, &height, &channels, 4);
        if (pixels == nullptr) {
            spdlog::warn("FireTextureLoader: Failed to decode {}", path);
            continue;
        }

        const u32 sourceWidth = static_cast<u32>(width);
        const u32 sourceHeight = static_cast<u32>(height);
        if (sourceWidth == 0 || sourceHeight == 0) {
            stbi_image_free(pixels);
            spdlog::warn("FireTextureLoader: Decoded {} with zero dimension ({}x{})", path, sourceWidth, sourceHeight);
            continue;
        }

        // 读取 mcmeta 动画元数据
        result.metadata = readMcmeta(pack, path, sourceWidth, sourceHeight);

        // 确定单帧尺寸
        u32 frameWidth = sourceWidth;
        u32 frameHeight = sourceWidth; // 默认正方形帧
        if (result.metadata.isValidAnimation()) {
            frameWidth = static_cast<u32>(result.metadata.width);
            frameHeight = static_cast<u32>(result.metadata.height);
        } else if (sourceHeight >= sourceWidth) {
            // 无 mcmeta 但为竖向条带，启发式：帧高 = 帧宽
            frameHeight = sourceWidth;
        } else {
            // 单帧图像
            frameHeight = sourceHeight;
        }

        // 验证帧尺寸能整除图像尺寸
        if (sourceWidth < frameWidth || sourceHeight < frameHeight || (sourceWidth % frameWidth) != 0 ||
            (sourceHeight % frameHeight) != 0) {
            spdlog::warn("FireTextureLoader: {} frame size {}x{} does not divide image {}x{}, using single frame",
                path,
                frameWidth,
                frameHeight,
                sourceWidth,
                sourceHeight);
            frameWidth = sourceWidth;
            frameHeight = sourceHeight;
        }

        result.frameWidth = frameWidth;
        result.frameHeight = frameHeight;
        result.frameCount = (sourceHeight / frameHeight) * (sourceWidth / frameWidth);

        // 提取所有帧
        result.pixels = extractAllFrames(pixels, sourceWidth, sourceHeight, frameWidth, frameHeight);

        stbi_image_free(pixels);
        spdlog::info("FireTextureLoader: Loaded {} ({}x{}, frame {}x{}, {} frames)",
            path,
            sourceWidth,
            sourceHeight,
            frameWidth,
            frameHeight,
            result.frameCount);
        return result; // 高优先级命中
    }

    spdlog::info("FireTextureLoader: {} not found in any resource pack", path);
    return result;
}

/**
 * @brief 生成程序化占位火焰纹理
 *
 * 当资源包中不存在 fire_0/fire_1 时使用。生成 16x16 × 2 帧
 * 的自下而上红→黄渐变纹理。
 *
 * @return 占位纹理数据
 */
SingleFireTexture makeProceduralFireTexture()
{
    SingleFireTexture result;
    result.frameWidth = 16;
    result.frameHeight = 16;
    result.frameCount = 1;
    result.pixels.resize(static_cast<size_t>(result.frameWidth) * result.frameHeight * 4);

    for (u32 y = 0; y < result.frameHeight; ++y) {
        for (u32 x = 0; x < result.frameWidth; ++x) {
            const size_t idx = (static_cast<size_t>(y) * result.frameWidth + x) * 4;
            const f32 t = static_cast<f32>(y) / static_cast<f32>(result.frameHeight);
            f32 intensity = 1.0f - t;
            const f32 variation = static_cast<f32>(x % 16) / 16.0f * 0.3f;
            intensity = std::min(1.0f, intensity + variation);

            result.pixels[idx + 0] = static_cast<u8>(255 * intensity);                     // R
            result.pixels[idx + 1] = static_cast<u8>(128 * intensity * (1.0f - t * 0.5f)); // G
            result.pixels[idx + 2] = static_cast<u8>(32 * intensity);                      // B
            result.pixels[idx + 3] = static_cast<u8>(255 * intensity);                     // A
        }
    }
    return result;
}

} // namespace

FireTextureData loadFireTextureData(const std::vector<IResourcePack*>& resourcePacks)
{
    FireTextureData result;

    // 分别加载 fire_0 和 fire_1
    SingleFireTexture fire0 = loadSingleFireTexture(resourcePacks, FIRE_TEXTURE_PATHS[0]);
    SingleFireTexture fire1 = loadSingleFireTexture(resourcePacks, FIRE_TEXTURE_PATHS[1]);

    // 两张纹理都缺失时使用程序化占位
    if (fire0.frameCount == 0 && fire1.frameCount == 0) {
        spdlog::info("FireTextureLoader: Creating procedural fire texture");
        fire0 = makeProceduralFireTexture();
        // 复制 fire_0 作为 fire_1（保持布局一致）
        fire1 = fire0;
    } else if (fire0.frameCount == 0) {
        // fire_0 缺失，复制 fire_1
        fire0 = fire1;
    } else if (fire1.frameCount == 0) {
        // fire_1 缺失，复制 fire_0
        fire1 = fire0;
    }

    // 帧尺寸必须一致（原版两张纹理帧尺寸相同）
    MC_ASSERT_RELEASE(fire0.frameWidth == fire1.frameWidth && fire0.frameHeight == fire1.frameHeight);

    result.frameWidth = fire0.frameWidth;
    result.frameHeight = fire0.frameHeight;
    result.fire0FrameCount = fire0.frameCount;
    result.fire1FrameCount = fire1.frameCount;
    result.frameCount = fire0.frameCount + fire1.frameCount;
    result.fire0Metadata = fire0.metadata;
    result.fire1Metadata = fire1.metadata;

    // 纵向拼接 [fire_0 全部帧][fire_1 全部帧]
    result.pixels.reserve(static_cast<size_t>(result.frameWidth) * result.frameHeight * result.frameCount * 4);
    result.pixels.insert(result.pixels.end(), fire0.pixels.begin(), fire0.pixels.end());
    result.pixels.insert(result.pixels.end(), fire1.pixels.begin(), fire1.pixels.end());

    return result;
}

} // namespace mc::client::renderer::entity::effect::fire
