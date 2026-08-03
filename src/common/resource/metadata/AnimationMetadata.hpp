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
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::resource::metadata {

/**
 * @brief 动画帧数据
 *
 * 表示动画纹理中的单个帧。
 */
struct AnimationFrame {
    /**
     * @brief 帧索引
     *
     * 在动画纹理图中的帧索引（从上到下）。
     * 例如，一个16x64的纹理，帧高16，则共有4帧，索引0-3。
     */
    i32 index = 0;

    /**
     * @brief 帧持续时间（游戏tick）
     *
     * 该帧显示的tick数。-1表示使用默认frametime。
     * 在MC中，1秒 = 20 tick，所以frametime=1表示每帧显示0.05秒。
     */
    i32 time = -1;

    /**
     * @brief 默认构造函数
     */
    AnimationFrame() noexcept = default;

    /**
     * @brief 构造指定索引和时间的帧
     * @param frameIndex 帧索引
     * @param frameTime 帧时间（-1使用默认值）
     */
    AnimationFrame(i32 frameIndex, i32 frameTime) noexcept
        : index(frameIndex)
        , time(frameTime)
    {}
};

/**
 * @brief 动画元数据
 *
 * 从.mcmeta文件解析的动画配置信息。
 *
 * mcmeta文件格式示例：
 * @code
 * {
 *   "animation": {
 *     "frametime": 5,
 *     "width": 16,
 *     "height": 16,
 *     "interpolate": true,
 *     "frames": [
 *       0,
 *       {"index": 1, "time": 10},
 *       2,
 *       3
 *     ]
 *   }
 * }
 * @endcode
 */
struct AnimationMetadata {
    /**
     * @brief 默认帧时间（游戏tick）
     *
     * 每帧的默认持续时间。当frames数组中的帧未指定time时使用此值。
     * 默认值为1，即每帧显示1 tick（0.05秒）。
     */
    i32 frametime = 1;

    /**
     * @brief 帧宽度（像素）
     *
     * 单帧的宽度。-1表示自动检测（使用图像宽度）。
     */
    i32 width = -1;

    /**
     * @brief 帧高度（像素）
     *
     * 单帧的高度。-1表示自动检测。
     * 对于动画纹理，通常图像是垂直排列的多帧，
     * 图像高度 = 帧高度 × 帧数。
     */
    i32 height = -1;

    /**
     * @brief 是否启用帧间插值
     *
     * 如果为true，在帧切换时会进行颜色插值，
     * 产生平滑的过渡效果。默认为false。
     *
     * @note 插值会增加GPU计算开销
     */
    bool interpolate = false;

    /**
     * @brief 自定义帧序列
     *
     * 如果为空，则使用默认序列：0, 1, 2, ..., frameCount-1。
     * 可以自定义帧顺序和每帧的持续时间，例如：
     * - {0, 1, 2, 3} - 按顺序播放
     * - {0, 1, 2, 1, 0} - 摆动播放
     * - {0, 0, 1, 1, 2, 2} - 每帧显示两次
     */
    std::vector<AnimationFrame> frames;

    /**
     * @brief 默认构造函数
     */
    AnimationMetadata() = default;

    /**
     * @brief 检查是否为有效动画
     * @return 如果有有效的帧配置返回true
     */
    [[nodiscard]] bool isValidAnimation() const noexcept { return frametime > 0 && width > 0 && height > 0; }

    /**
     * @brief 获取总帧数
     * @return 如果有自定义帧序列返回序列长度，否则返回0表示需要从图像计算
     */
    [[nodiscard]] Size getFrameCount() const noexcept { return frames.size(); }

    /**
     * @brief 获取指定位置的帧索引
     * @param position 动画播放位置（0, 1, 2, ...）
     * @return 帧索引
     */
    [[nodiscard]] i32 getFrameIndex(Size position) const noexcept
    {
        if (frames.empty()) {
            return static_cast<i32>(position);
        }
        return frames[position % frames.size()].index;
    }

    /**
     * @brief 获取指定位置的帧时间
     * @param position 动画播放位置（0, 1, 2, ...）
     * @return 帧时间（tick），如果帧未指定时间则返回默认frametime
     */
    [[nodiscard]] i32 getFrameTime(Size position) const noexcept
    {
        if (frames.empty()) {
            return frametime;
        }
        const auto& frame = frames[position % frames.size()];
        return frame.time >= 0 ? frame.time : frametime;
    }

    /**
     * @brief 从JSON解析动画元数据
     * @param json MC元数据JSON对象
     * @return 解析后的AnimationMetadata
     */
    static AnimationMetadata fromJson(const nlohmann::json& json);

    /**
     * @brief 从二进制mcmeta数据解析
     * @param mcmetaData mcmeta文件内容
     * @param imageWidth 图像宽度（用于自动检测帧尺寸）
     * @param imageHeight 图像高度（用于计算帧数）
     * @return 解析后的AnimationMetadata，如果解析失败返回空帧序列
     */
    static AnimationMetadata fromMcmeta(const std::vector<u8>& mcmetaData, u32 imageWidth, u32 imageHeight);
};

} // namespace mc::resource::metadata

namespace mc::resource {
using AnimationMetadata = metadata::AnimationMetadata;
} // namespace mc::resource
