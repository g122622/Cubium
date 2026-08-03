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

#include "ITexture.hpp"
#include "TextureRegion.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <map>
#include <vector>

namespace mc::client::renderer::api {

/**
 * @brief 纹理图集构建结果
 */
struct AtlasBuildResult {
    std::vector<u8> pixelData; // 图集像素数据
    u32 width = 0;             // 图集宽度
    u32 height = 0;            // 图集高度
    u32 tileSize = 16;         // 瓦片大小

    // 纹理位置到UV区域的映射
    std::map<ResourceLocation, TextureRegion> regions;
};

/**
 * @brief 纹理图集接口 (扩展版)
 *
 * 扩展了 ITextureAtlas，添加了构建和更新功能。
 */
class ITextureAtlasBuilder {
public:
    virtual ~ITextureAtlasBuilder() = default;

    /**
     * @brief 添加纹理到图集
     * @param location 纹理位置
     * @param data 像素数据 (RGBA)
     * @param width 纹理宽度
     * @param height 纹理高度
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> addTexture(
        const ResourceLocation& location, const u8* data, u32 width, u32 height) = 0;

    /**
     * @brief 构建图集
     * @return 构建结果
     */
    [[nodiscard]] virtual Result<AtlasBuildResult> build() = 0;

    /**
     * @brief 清空所有已添加的纹理
     */
    virtual void clear() = 0;

    /**
     * @brief 设置瓦片大小
     */
    virtual void setTileSize(u32 tileSize) = 0;

    /**
     * @brief 获取已添加纹理数量
     */
    [[nodiscard]] virtual u32 textureCount() const = 0;
};

} // namespace mc::client::renderer::api
