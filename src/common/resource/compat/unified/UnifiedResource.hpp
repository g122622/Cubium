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

#pragma once

#include "../../../core/Types.hpp"
#include "../ResourceMapper.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mc {
namespace resource {
namespace compat {
namespace unified {

/**
 * @brief 资源类型枚举
 */
enum class ResourceType : u8 {
    Texture,    ///< 图像纹理 (PNG)
    Model,      ///< JSON 模型文件
    BlockState, ///< JSON 方块状态定义
    Sound,      ///< OGG 音频文件
    Language,   ///< 语言文件 (JSON)
    Data,       ///< 数据文件（战利品表等）
    Unknown
};

/**
 * @brief 统一资源表示
 *
 * 所有统一资源类型的基类。
 * 所有资源都被规范化为规范格式，
 * 无论源包版本如何。
 */
struct UnifiedResource {
    ResourceLocation location; ///< 规范位置（现代格式）
    std::string originalPath;  ///< 资源包中的原始路径
    PackFormat sourceFormat;   ///< 源包格式版本
    ResourceType type = ResourceType::Unknown;

    UnifiedResource() = default;
    UnifiedResource(const ResourceLocation& loc, PackFormat format, ResourceType t)
        : location(loc)
        , sourceFormat(format)
        , type(t)
    {}

    virtual ~UnifiedResource() = default;
};

/**
 * @brief RGBA 像素数据表示
 */
struct PixelData {
    std::vector<u8> data; ///< RGBA 像素数据（每像素 4 字节）
    u32 width = 0;
    u32 height = 0;

    bool valid() const noexcept { return width > 0 && height > 0 && data.size() == width * height * 4; }

    void clear()
    {
        data.clear();
        width = 0;
        height = 0;
    }
};

/**
 * @brief 统一纹理表示
 *
 * 所有纹理都以规范格式表示:
 * - 路径: textures/block/<name>.png (1.13+ 风格)
 * - 名称: <block>_<variant> (现代命名)
 */
struct UnifiedTexture : public UnifiedResource {
    PixelData pixels; ///< RGBA 像素数据

    UnifiedTexture() { type = ResourceType::Texture; }

    /**
     * @brief 检查纹理是否有有效的像素数据
     */
    bool hasValidPixels() const noexcept { return pixels.valid(); }

    /**
     * @brief 获取指定坐标的像素
     * @param x X 坐标 (0 到 width-1)
     * @param y Y 坐标 (0 到 height-1)
     * @return RGBA 值作为 u32 (0xAABBGGRR)
     */
    u32 getPixel(u32 x, u32 y) const
    {
        if (x >= pixels.width || y >= pixels.height) {
            return 0;
        }
        size_t idx = (y * pixels.width + x) * 4;
        if (idx + 3 >= pixels.data.size()) {
            return 0;
        }
        return static_cast<u32>(pixels.data[idx]) | (static_cast<u32>(pixels.data[idx + 1]) << 8) |
            (static_cast<u32>(pixels.data[idx + 2]) << 16) | (static_cast<u32>(pixels.data[idx + 3]) << 24);
    }
};

} // namespace unified
} // namespace compat
} // namespace resource
} // namespace mc
