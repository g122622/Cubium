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

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <vector>

namespace mc::client::resource::atlas {

/**
 * @brief missingno 兜底纹理工具
 *
 * 对齐原版 SpriteLoader.list 末尾追加 MissingTextureAtlasSprite。
 * 每个图集末尾追加 minecraft:missingno sprite（紫黑棋盘格），
 * 查询 miss 时返回该 sprite 的 region 而非 nullptr。
 */
struct MissingNo {
    /// missingno 的 sprite 资源位置
    static const ResourceLocation& spriteLocation();

    /// 生成 16×16 紫黑棋盘格 RGBA8 像素数据
    [[nodiscard]] static std::vector<u8> generatePixels();
};

} // namespace mc::client::resource::atlas
