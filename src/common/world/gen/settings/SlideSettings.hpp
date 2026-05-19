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

namespace mc {

/**
 * @brief 滑动设置（用于地形边界平滑）
 *
 * 参考 MC NoiseSettings.Slide
 * 用于在顶部和底部创建平滑的地形边界
 *
 * @note 这使得地形在接近世界顶部和底部时逐渐变得平坦
 */
struct SlideSettings {
    i32 target = 0; ///< 目标值
    i32 size = 0;   ///< 大小（影响范围）
    i32 offset = 0; ///< 偏移

    SlideSettings() = default;
    SlideSettings(i32 target_, i32 size_, i32 offset_)
        : target(target_)
        , size(size_)
        , offset(offset_)
    {}
};

} // namespace mc
