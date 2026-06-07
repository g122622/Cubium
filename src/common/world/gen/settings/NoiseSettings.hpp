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

#include "ScalingSettings.hpp"
#include "SlideSettings.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 噪声地形生成设置
 *
 * 用于配置地形噪声生成参数。
 *
 * 使用方法：
 * @code
 * NoiseSettings settings = NoiseSettings::overworld();
 * // 使用 settings 生成地形
 * @endcode
 */
struct NoiseSettings {
    // === 基本尺寸 ===
    i32 minY = 0;                         ///< 噪声最低 Y 坐标（主世界 -64，下界/末地 0）
    i32 height = world::MAX_BUILD_HEIGHT; ///< 噪声高度
    i32 sizeHorizontal = 1;               ///< 水平大小
    i32 sizeVertical = 2;                 ///< 垂直大小

    // === 缩放设置 ===
    ScalingSettings scaling;

    // === 滑动设置 ===
    SlideSettings topSlide{-10, 3, 0};    ///< 顶部滑动
    SlideSettings bottomSlide{-30, 0, 0}; ///< 底部滑动

    // === 密度参数 ===
    f32 densityFactor = 1.0f;      ///< 密度因子
    f32 densityOffset = -0.46875f; ///< 密度偏移

    // === 噪声选项 ===
    bool simplexSurfaceNoise = true; ///< 使用 Simplex 地表噪声
    bool randomDensityOffset = true; ///< 随机密度偏移
    bool isAmplified = false;        ///< 放大化地形
    bool aquifersEnabled = true;     ///< 是否启用含水层（MC 1.18+ 水下洞穴和地下水）

    // === 噪声尺寸计算 ===
    [[nodiscard]] i32 noiseSizeX() const { return world::CHUNK_WIDTH / (sizeHorizontal * 4); }

    [[nodiscard]] i32 noiseSizeY() const { return height / (sizeVertical * 4); }

    [[nodiscard]] i32 noiseSizeZ() const { return world::CHUNK_WIDTH / (sizeHorizontal * 4); }

    [[nodiscard]] i32 verticalNoiseGranularity() const { return sizeVertical * 4; }

    [[nodiscard]] i32 horizontalNoiseGranularity() const { return sizeHorizontal * 4; }

    // === 预设 ===

    /**
     * @brief 主世界设置
     */
    static NoiseSettings overworld()
    {
        NoiseSettings settings;
        settings.minY = world::MIN_BUILD_HEIGHT;
        settings.height = world::CHUNK_HEIGHT;
        settings.sizeHorizontal = 1;
        settings.sizeVertical = 2;
        settings.densityFactor = 1.0f;
        settings.densityOffset = -0.46875f;
        settings.topSlide = SlideSettings{-10, 3, 0};
        settings.bottomSlide = SlideSettings{-30, 0, 0};
        settings.simplexSurfaceNoise = true;
        settings.randomDensityOffset = true;
        return settings;
    }

    /**
     * @brief 放大化主世界设置
     */
    static NoiseSettings amplified()
    {
        NoiseSettings settings = overworld();
        settings.isAmplified = true;
        settings.densityFactor = 2.0f;
        return settings;
    }

    /**
     * @brief 下界设置
     */
    static NoiseSettings nether()
    {
        NoiseSettings settings;
        settings.minY = 0;
        settings.height = 128;
        settings.sizeHorizontal = 1;
        settings.sizeVertical = 2;
        settings.densityFactor = 0.0f;
        settings.densityOffset = 0.019921875f;
        settings.topSlide = SlideSettings{120, 3, 0};
        settings.bottomSlide = SlideSettings{320, 4, -1};
        settings.simplexSurfaceNoise = false;
        settings.randomDensityOffset = false;
        settings.aquifersEnabled = false;
        return settings;
    }

    /**
     * @brief 末地设置
     */
    static NoiseSettings end()
    {
        NoiseSettings settings;
        settings.minY = 0;
        settings.height = 128;
        settings.sizeHorizontal = 2;
        settings.sizeVertical = 1;
        settings.densityFactor = 0.0f;
        settings.densityOffset = 0.0f;
        settings.simplexSurfaceNoise = false;
        settings.randomDensityOffset = false;
        settings.aquifersEnabled = false;
        return settings;
    }
};

} // namespace mc
