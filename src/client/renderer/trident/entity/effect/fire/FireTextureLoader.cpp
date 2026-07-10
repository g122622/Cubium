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
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>

// STB 图像加载（仅 PNG）
#define STBI_ONLY_PNG
#include <stb_image.h>

namespace mc::client::renderer::entity::effect::fire {

// 火焰动画帧数（fire_0 + fire_1）
namespace {
constexpr u32 FIRE_FRAME_COUNT = 2;

// 火焰纹理路径（readResource 仅自动补 assets/ 前缀，命名空间 minecraft/ 需显式给出）
const char* FIRE_TEXTURE_PATHS[] = {"minecraft/textures/block/fire_0.png", "minecraft/textures/block/fire_1.png"};
} // namespace

FireTextureData loadFireTextureData(const std::vector<IResourcePack*>& resourcePacks)
{
    FireTextureData result;
    // 资源包按优先级从低到高排列，高优先级（索引大）应当覆盖低优先级。
    // 逆序遍历让高优先级包先命中。
    for (const char* path : FIRE_TEXTURE_PATHS) {
        bool loaded = false;
        for (auto it = resourcePacks.rbegin(); it != resourcePacks.rend(); ++it) {
            auto* pack = *it;
            if (!pack) continue;

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

            // fire_0.png / fire_1.png 原版为 16x16 单帧，但资源包可能提供 16x512 动画条带。
            // 启发式：以宽度为单帧边长；若 height >= width 则视为竖向动画条带，取首帧。
            // TODO: 完整的 .mcmeta 动画处理（按帧时间戳循环播放、frametime/interpolate）未实现，
            //       当前仅取动画条带首帧作为静态纹理，后续应读取同名 .png.mcmeta 动画元数据
            //       并将其上传为数组纹理或时间轴驱动更新。
            const u32 sourceWidth = static_cast<u32>(width);
            const u32 sourceHeight = static_cast<u32>(height);
            if (sourceWidth == 0 || sourceHeight == 0) {
                stbi_image_free(pixels);
                spdlog::warn(
                    "FireTextureLoader: Decoded {} with zero dimension ({}x{})", path, sourceWidth, sourceHeight);
                continue;
            }

            const u32 frameWidth = sourceWidth;
            const u32 frameHeight = (sourceHeight >= sourceWidth) ? sourceWidth : sourceHeight;
            if (sourceHeight < frameHeight) {
                stbi_image_free(pixels);
                spdlog::warn(
                    "FireTextureLoader: {} too small to extract a frame ({}x{})", path, sourceWidth, sourceHeight);
                continue;
            }

            if (result.frameWidth == 0) {
                result.frameWidth = frameWidth;
                result.frameHeight = frameHeight;
            } else if (result.frameWidth != frameWidth || result.frameHeight != frameHeight) {
                // 帧尺寸不一致，跳过该帧（保持已加载帧的尺寸）
                stbi_image_free(pixels);
                spdlog::warn("FireTextureLoader: {} frame size {}x{} mismatches expected {}x{}, skipped",
                    path,
                    frameWidth,
                    frameHeight,
                    result.frameWidth,
                    result.frameHeight);
                continue;
            }

            // 追加一帧到拼接缓冲区末尾
            const size_t frameByteSize = static_cast<size_t>(frameWidth) * frameHeight * 4;
            const size_t oldSize = result.pixels.size();
            result.pixels.resize(oldSize + frameByteSize);

            // 从源像素逐行复制首帧（源可能含多帧，行步长为 sourceWidth*4）
            for (u32 row = 0; row < frameHeight; ++row) {
                const u8* srcRow = pixels + static_cast<size_t>(row) * sourceWidth * 4;
                u8* dstRow = result.pixels.data() + oldSize + static_cast<size_t>(row) * frameWidth * 4;
                std::memcpy(dstRow, srcRow, static_cast<size_t>(frameWidth) * 4);
            }

            stbi_image_free(pixels);
            ++result.frameCount;
            loaded = true;
            spdlog::info("FireTextureLoader: Loaded fire texture {} ({}x{}, frame {}x{})",
                path,
                sourceWidth,
                sourceHeight,
                frameWidth,
                frameHeight);
            break; // 高优先级命中，不再查找低优先级包
        }
        if (!loaded) {
            spdlog::debug("FireTextureLoader: {} not found in any resource pack", path);
        }
    }

    // 未找到任何纹理文件时生成程序化占位纹理
    if (result.frameCount == 0) {
        spdlog::info("FireTextureLoader: Creating procedural fire texture");
        result.frameWidth = 16;
        result.frameHeight = 16;
        result.frameCount = FIRE_FRAME_COUNT;
        result.pixels.resize(static_cast<size_t>(result.frameWidth) * result.frameHeight * result.frameCount * 4);

        for (u32 frame = 0; frame < result.frameCount; ++frame) {
            for (u32 y = 0; y < result.frameHeight; ++y) {
                for (u32 x = 0; x < result.frameWidth; ++x) {
                    const size_t idx = (static_cast<size_t>(frame) * result.frameWidth * result.frameHeight +
                                           static_cast<size_t>(y) * result.frameWidth + x) *
                        4;

                    // 火焰颜色渐变（底部红色到顶部黄色）
                    const f32 t = static_cast<f32>(y) / static_cast<f32>(result.frameHeight);
                    f32 intensity = 1.0f - t; // 底部更亮

                    const f32 variation = static_cast<f32>((x + frame * 8) % 16) / 16.0f * 0.3f;
                    intensity = std::min(1.0f, intensity + variation);

                    result.pixels[idx + 0] = static_cast<u8>(255 * intensity);                     // R
                    result.pixels[idx + 1] = static_cast<u8>(128 * intensity * (1.0f - t * 0.5f)); // G
                    result.pixels[idx + 2] = static_cast<u8>(32 * intensity);                      // B
                    result.pixels[idx + 3] = static_cast<u8>(255 * intensity);                     // A
                }
            }
        }
    }

    // 若仅找到一帧，复制首帧作为第二帧，保持纹理布局一致（两帧纵向拼接）
    if (result.frameCount == 1) {
        const size_t frameByteSize = static_cast<size_t>(result.frameWidth) * result.frameHeight * 4;
        const size_t oldSize = result.pixels.size();
        result.pixels.resize(oldSize + frameByteSize);
        std::memcpy(result.pixels.data() + oldSize, result.pixels.data(), frameByteSize);
        result.frameCount = FIRE_FRAME_COUNT;
    }

    return result;
}

} // namespace mc::client::renderer::entity::effect::fire
