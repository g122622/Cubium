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

#include "common/core/Types.hpp"

namespace mc::client::renderer::api {

// Forward declaration
class ITexture;

/**
 * @brief 纹理区域 (UV坐标)
 *
 * 表示纹理图集中一个子区域的 UV 坐标。
 * UV 坐标范围 [0, 1]，其中 (u0, v0) 是左上角，(u1, v1) 是右下角。
 */
struct TextureRegion {
    f64 u0 = 0.0f, v0 = 0.0f; // 左上角
    f64 u1 = 1.0f, v1 = 1.0f; // 右下角

    TextureRegion() = default;
    TextureRegion(f64 u0_, f64 v0_, f64 u1_, f64 v1_)
        : u0(u0_)
        , v0(v0_)
        , u1(u1_)
        , v1(v1_)
    {}

    /**
     * @brief 获取区域宽度 (UV单位)
     */
    [[nodiscard]] f64 width() const { return u1 - u0; }

    /**
     * @brief 获取区域高度 (UV单位)
     */
    [[nodiscard]] f64 height() const { return v1 - v0; }

    /**
     * @brief 创建默认区域 (整个纹理)
     */
    static TextureRegion full() { return TextureRegion(0.0f, 0.0f, 1.0f, 1.0f); }

    bool operator==(const TextureRegion& other) const
    {
        return u0 == other.u0 && v0 == other.v0 && u1 == other.u1 && v1 == other.v1;
    }

    bool operator!=(const TextureRegion& other) const { return !(*this == other); }
};

/**
 * @brief 纹理图集接口
 *
 * 管理多个小纹理合并成一个大纹理。
 * 用于方块纹理、字体纹理等场景。
 */
class ITextureAtlas {
public:
    virtual ~ITextureAtlas() = default;

    /**
     * @brief 获取纹理图集宽度
     */
    [[nodiscard]] virtual u32 width() const = 0;

    /**
     * @brief 获取纹理图集高度
     */
    [[nodiscard]] virtual u32 height() const = 0;

    /**
     * @brief 获取瓦片大小
     */
    [[nodiscard]] virtual u32 tileSize() const = 0;

    /**
     * @brief 获取每行瓦片数
     */
    [[nodiscard]] virtual u32 tilesPerRow() const = 0;

    /**
     * @brief 获取瓦片的纹理区域 (以格子坐标)
     * @param tileX X 方向格子索引
     * @param tileY Y 方向格子索引
     * @return 纹理区域
     */
    [[nodiscard]] virtual TextureRegion getRegion(u32 tileX, u32 tileY) const = 0;

    /**
     * @brief 获取瓦片的纹理区域 (以线性索引)
     * @param tileIndex 线性索引
     * @return 纹理区域
     */
    [[nodiscard]] virtual TextureRegion getRegion(u32 tileIndex) const = 0;

    /**
     * @brief 获取底层纹理
     */
    [[nodiscard]] virtual ITexture* texture() = 0;
    [[nodiscard]] virtual const ITexture* texture() const = 0;

    /**
     * @brief 检查纹理图集是否有效
     */
    [[nodiscard]] virtual bool isValid() const = 0;
};

} // namespace mc::client::renderer::api
