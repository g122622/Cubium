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

#include "DestroyStageTextures.hpp"
#include "ResourceManager.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/math/random/Random.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc {
namespace client {
namespace renderer {

// ============================================================================
// 静态成员
// ============================================================================

DestroyStageTextures& DestroyStageTextures::instance()
{
    static DestroyStageTextures instance;
    return instance;
}

// ============================================================================
// 初始化
// ============================================================================

bool DestroyStageTextures::initialize()
{
    return initialize(nullptr);
}

bool DestroyStageTextures::initialize(ResourceManager* resourceManager)
{
    if (m_initialized) {
        return true;
    }

    spdlog::info("DestroyStageTextures: Initializing...");

    // 尝试从资源包加载，失败则使用程序生成
    for (size_t i = 0; i < STAGE_COUNT; ++i) {
        bool loaded = false;

        // 尝试从 ResourceManager 加载
        if (resourceManager) {
            loaded = _loadTextureFromResourcePack(resourceManager, i, m_textures[i]);
        } else {
            spdlog::warn(
                "DestroyStageTextures: No ResourceManager provided, skipping resource pack loading for stage {}", i);
        }

        // 如果加载失败，使用程序生成
        if (!loaded) {
            spdlog::warn("DestroyStageTextures: Using generated texture for stage {}", i);
            _generateDefaultTexture(i, m_textures[i]);
        }
    }

    // 构建纹理图集
    _buildAtlas();

    m_initialized = true;
    spdlog::info("DestroyStageTextures: Initialized successfully");
    return true;
}

void DestroyStageTextures::cleanup()
{
    for (auto& texture : m_textures) {
        texture.clear();
        texture.shrink_to_fit();
    }
    m_atlasData.clear();
    m_atlasData.shrink_to_fit();
    m_initialized = false;
}

// ============================================================================
// 纹理访问
// ============================================================================

const u8* DestroyStageTextures::getTextureData(size_t stage) const
{
    if (stage >= STAGE_COUNT) {
        return nullptr;
    }
    return m_textures[stage].data();
}

bool DestroyStageTextures::getTextureUV(size_t stage, f32& u0, f32& v0, f32& u1, f32& v1) const
{
    if (stage >= STAGE_COUNT) {
        return false;
    }

    // 图集布局：2行5列
    // 阶段0-4在第一行，阶段5-9在第二行
    u32 col = static_cast<u32>(stage % ATLAS_COLS);
    u32 row = static_cast<u32>(stage / ATLAS_COLS);

    f32 cellWidth = 1.0f / static_cast<f32>(ATLAS_COLS);
    f32 cellHeight = 1.0f / static_cast<f32>(ATLAS_ROWS);

    u0 = static_cast<f32>(col) * cellWidth;
    v0 = static_cast<f32>(row) * cellHeight;
    u1 = u0 + cellWidth;
    v1 = v0 + cellHeight;

    return true;
}

// ============================================================================
// 私有方法
// ============================================================================

void DestroyStageTextures::_generateDefaultTexture(size_t stage, std::vector<u8>& data)
{
    // 16x16 RGBA 纹理
    constexpr size_t pixelCount = TEXTURE_SIZE * TEXTURE_SIZE;
    data.resize(pixelCount * 4, 0);

    // 使用确定性随机种子，基于阶段号
    math::Random rng(static_cast<u32>(stage * 12345 + 67890));

    // 破坏强度随阶段增加
    // 阶段0：几乎无裂纹
    // 阶段9：几乎完全破碎
    f32 intensity = static_cast<f32>(stage) / static_cast<f32>(STAGE_COUNT - 1);

    for (u32 y = 0; y < TEXTURE_SIZE; ++y) {
        for (u32 x = 0; x < TEXTURE_SIZE; ++x) {
            size_t idx = (y * TEXTURE_SIZE + x) * 4;

            // 基础噪声
            f32 noise = rng.nextFloat();

            // 创建裂纹图案
            // 使用多个噪声层叠加
            f32 crack = 0.0f;

            // 水平裂纹
            if (y >= 4 && y <= 5 && stage >= 2) {
                crack += 0.3f;
            }
            if (y >= 10 && y <= 11 && stage >= 4) {
                crack += 0.3f;
            }

            // 垂直裂纹
            if (x >= 7 && x <= 8 && stage >= 3) {
                crack += 0.3f;
            }
            if (x >= 3 && x <= 4 && stage >= 5) {
                crack += 0.3f;
            }

            // 对角裂纹
            if (std::abs(static_cast<i32>(x) - static_cast<i32>(y)) <= 1 && stage >= 6) {
                crack += 0.25f;
            }
            if (std::abs(static_cast<i32>(x) + static_cast<i32>(y) - static_cast<i32>(TEXTURE_SIZE - 1)) <= 1 &&
                stage >= 7) {
                crack += 0.25f;
            }

            // 随机裂纹点
            if (noise < intensity * 0.4f) {
                crack += 0.4f;
            }

            // 边缘破损
            if ((x == 0 || x == TEXTURE_SIZE - 1 || y == 0 || y == TEXTURE_SIZE - 1) && stage >= 3) {
                crack += 0.3f * intensity;
            }

            // 计算最终alpha值
            // 破坏纹理使用alpha通道控制可见度
            // 高alpha = 更明显的裂纹
            f32 alpha = std::min(1.0f, crack * intensity * 1.5f);

            // RGBA
            data[idx + 0] = 0;                               // R - 黑色裂纹
            data[idx + 1] = 0;                               // G
            data[idx + 2] = 0;                               // B
            data[idx + 3] = static_cast<u8>(alpha * 255.0f); // A
        }
    }
}

bool DestroyStageTextures::_loadTextureFromResourcePack(
    ResourceManager* resourceManager, size_t stage, std::vector<u8>& data)
{
    // 构建资源位置
    // 现代 MC 1.13+ 路径: textures/block/destroy_stage_X
    // 旧版 MC 1.12 路径: textures/blocks/destroy_stage_X
    // ResourceLocation 会自动添加 .png 后缀

    std::vector<u8> rawData;
    u32 srcWidth = 0, srcHeight = 0;

    // 尝试现代路径
    ResourceLocation modernLoc("minecraft", fmt::format("textures/block/destroy_stage_{}", stage));
    auto result = resourceManager->loadTextureRGBA(modernLoc);

    if (result.success()) {
        auto& decoded = result.value();
        if (decoded.width > 0 && decoded.height > 0 && !decoded.pixels.empty()) {
            rawData = std::move(decoded.pixels);
            srcWidth = decoded.width;
            srcHeight = decoded.height;
        }
    }

    if (rawData.empty()) {
        // 尝试旧版路径
        ResourceLocation legacyLoc("minecraft", fmt::format("textures/blocks/destroy_stage_{}", stage));
        result = resourceManager->loadTextureRGBA(legacyLoc);

        if (result.success()) {
            auto& decoded = result.value();
            if (decoded.width > 0 && decoded.height > 0 && !decoded.pixels.empty()) {
                rawData = std::move(decoded.pixels);
                srcWidth = decoded.width;
                srcHeight = decoded.height;
            }
        }
    }

    if (rawData.empty()) {
        return false;
    }

    // MC 原版破坏纹理为灰度图，高亮度=无裂纹区域，低亮度=裂纹区域。
    // 着色器期望格式：RGB=(0,0,0), Alpha=裂纹强度（高alpha=强裂纹=变暗明显）。
    // 转换：crackIntensity = 255 - luminance（反转亮度映射到裂纹强度）

    // 处理纹理：缩放并转换格式
    data.resize(TEXTURE_SIZE * TEXTURE_SIZE * 4);

    f32 xRatio = static_cast<f32>(srcWidth) / TEXTURE_SIZE;
    f32 yRatio = static_cast<f32>(srcHeight) / TEXTURE_SIZE;

    for (u32 y = 0; y < TEXTURE_SIZE; ++y) {
        for (u32 x = 0; x < TEXTURE_SIZE; ++x) {
            u32 srcX = static_cast<u32>(x * xRatio);
            u32 srcY = static_cast<u32>(y * yRatio);
            srcX = std::min(srcX, srcWidth - 1);
            srcY = std::min(srcY, srcHeight - 1);

            size_t srcIdx = (srcY * srcWidth + srcX) * 4;
            size_t dstIdx = (y * TEXTURE_SIZE + x) * 4;

            u8 srcR = rawData[srcIdx + 0];
            u8 srcG = rawData[srcIdx + 1];
            u8 srcB = rawData[srcIdx + 2];

            // 计算亮度，反转映射到裂纹强度
            u8 luminance = static_cast<u8>((static_cast<u32>(srcR) + srcG + srcB) / 3);
            u8 crackIntensity = static_cast<u8>(255 - luminance);

            // 转换为着色器期望格式：RGB=(0,0,0), Alpha=裂纹强度
            data[dstIdx + 0] = 0;              // R
            data[dstIdx + 1] = 0;              // G
            data[dstIdx + 2] = 0;              // B
            data[dstIdx + 3] = crackIntensity; // A = 裂纹强度
        }
    }

    return true;
}

void DestroyStageTextures::_buildAtlas()
{
    // 图集尺寸：5列2行，每格16x16
    constexpr u32 atlasWidth = TEXTURE_SIZE * ATLAS_COLS;
    constexpr u32 atlasHeight = TEXTURE_SIZE * ATLAS_ROWS;

    m_atlasData.resize(atlasWidth * atlasHeight * 4, 0);

    for (size_t stage = 0; stage < STAGE_COUNT; ++stage) {
        u32 col = static_cast<u32>(stage % ATLAS_COLS);
        u32 row = static_cast<u32>(stage / ATLAS_COLS);

        u32 destX = col * TEXTURE_SIZE;
        u32 destY = row * TEXTURE_SIZE;

        const auto& srcData = m_textures[stage];

        for (u32 y = 0; y < TEXTURE_SIZE; ++y) {
            for (u32 x = 0; x < TEXTURE_SIZE; ++x) {
                size_t srcIdx = (y * TEXTURE_SIZE + x) * 4;
                size_t dstIdx = ((destY + y) * atlasWidth + destX + x) * 4;

                m_atlasData[dstIdx + 0] = srcData[srcIdx + 0];
                m_atlasData[dstIdx + 1] = srcData[srcIdx + 1];
                m_atlasData[dstIdx + 2] = srcData[srcIdx + 2];
                m_atlasData[dstIdx + 3] = srcData[srcIdx + 3];
            }
        }
    }
}

} // namespace renderer
} // namespace client
} // namespace mc
