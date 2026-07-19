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
#include "common/resource/metadata/AnimationMetadata.hpp"
#include <optional>
#include <vector>

namespace mc::client::resource::atlas {

/**
 * @brief 解码后的精灵内容
 *
 * 对齐原版 SpriteContents。SpriteLoader::resolve 产出此结构，
 * 喂给 TextureAtlasBuilder 打包。
 *
 * - single/directory source：从 PNG 解码，可能含 .mcmeta 动画元数据
 * - unstitch source：从大图切片得到，无 mcmeta
 * - paletted_permutations source：调色板映射生成，无 mcmeta
 */
struct SpriteContents {
    std::vector<u8> pixels;                                             ///< RGBA8 像素数据（动画纹理含竖排多帧）
    u32 width = 0;                                                      ///< 纹理宽度
    u32 height = 0;                                                     ///< 纹理高度
    std::optional<mc::resource::metadata::AnimationMetadata> animation; ///< 动画元数据（来自 .mcmeta）
};

} // namespace mc::client::resource::atlas
