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
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <vector>

namespace mc::client::renderer::entity::effect::fire {

/**
 * @brief 火焰纹理解码结果
 *
 * 由 loadFireTextureData 返回，包含 fire_0 和 fire_1 的全部动画帧，
 * 已按 [fire_0 全部帧][fire_1 全部帧] 纵向拼接为单条 RGBA 像素缓冲区。
 * 该函数为纯 CPU 操作，不依赖 Vulkan，便于单元测试。
 */
struct FireTextureData {
    /// 纵向拼接后的 RGBA 像素数据
    /// （frameWidth * (frameHeight * frameCount) * 4 字节）
    std::vector<u8> pixels;
    /// 单帧宽度（像素）
    u32 frameWidth = 0;
    /// 单帧高度（像素）
    u32 frameHeight = 0;
    /// 总帧数（fire0FrameCount + fire1FrameCount）
    u32 frameCount = 0;
    /// fire_0 的帧数
    u32 fire0FrameCount = 0;
    /// fire_1 的帧数
    u32 fire1FrameCount = 0;
    /// fire_0 的动画元数据（从 fire_0.png.mcmeta 解析）
    resource::metadata::AnimationMetadata fire0Metadata;
    /// fire_1 的动画元数据（从 fire_1.png.mcmeta 解析）
    resource::metadata::AnimationMetadata fire1Metadata;
};

/**
 * @brief 从资源包解码火焰纹理（纯 CPU，不依赖 Vulkan）
 *
 * 依次查找 fire_0.png、fire_1.png。每个 PNG 可能是动画条带
 * （例如 16x512 = 32 帧 16x16）。若存在同名 .png.mcmeta 文件，
 * 则按 mcmeta 配置提取全部帧和动画序列；否则按"帧高 = 图像宽度"
 * 启发式地从条带中提取所有帧（默认 frametime=1、顺序播放）。
 * 两张纹理的所有帧按 [fire_0 全部帧][fire_1 全部帧] 纵向拼接返回。
 *
 * 资源包列表按优先级从低到高排列（索引 0 优先级最低）；
 * 本函数逆序遍历以让高优先级包先命中。
 *
 * 若 fire_1.png 缺失，则复制 fire_0 的全部帧和元数据作为 fire_1，
 * 保持纹理布局一致。若两张纹理都缺失，则返回程序化生成的占位纹理
 * （16x16 × 2 帧，自下而上红→黄渐变）。
 *
 * @param resourcePacks 资源包列表（按优先级从低到高）
 * @return 解码结果
 */
[[nodiscard]] FireTextureData loadFireTextureData(const std::vector<IResourcePack*>& resourcePacks);

} // namespace mc::client::renderer::entity::effect::fire
