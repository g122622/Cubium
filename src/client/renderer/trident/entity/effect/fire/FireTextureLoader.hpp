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
#include "common/resource/pack/IResourcePack.hpp"
#include <vector>

namespace mc::client::renderer::entity::effect::fire {

/**
 * @brief 火焰纹理解码结果
 *
 * 由 loadFireTextureData 返回，包含已按帧纵向拼接的 RGBA 像素数据。
 * 该函数为纯 CPU 操作，不依赖 Vulkan，便于单元测试。
 */
struct FireTextureData {
    /// 纵向拼接后的 RGBA 像素数据（frameWidth * (frameHeight * frameCount) * 4 字节）
    std::vector<u8> pixels;
    /// 单帧宽度（像素）
    u32 frameWidth = 0;
    /// 单帧高度（像素）
    u32 frameHeight = 0;
    /// 已加载的帧数
    u32 frameCount = 0;
};

/**
 * @brief 从资源包解码火焰纹理（纯 CPU，不依赖 Vulkan）
 *
 * 依次查找 fire_0.png、fire_1.png。每个 PNG 可能是动画条带
 * （例如 16x512 = 32 帧 16x16），仅取其第一帧。
 * 两帧按纵向拼接返回。
 *
 * 资源包列表按优先级从低到高排列（索引 0 优先级最低）；
 * 本函数逆序遍历以让高优先级包先命中。
 *
 * 若所有资源包均未提供火焰纹理，则返回程序化生成的占位纹理
 * （16x16 × FIRE_FRAME_COUNT，自下而上红→黄渐变）。
 *
 * @param resourcePacks 资源包列表（按优先级从低到高）
 * @return 解码结果
 */
[[nodiscard]] FireTextureData loadFireTextureData(const std::vector<IResourcePack*>& resourcePacks);

} // namespace mc::client::renderer::entity::effect::fire
