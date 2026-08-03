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
#include "util/color/DyeColor.hpp"
#include <array>

namespace mc {
namespace block {

/**
 * @brief 信标光束颜色提供者接口
 *
 * 实现此接口的方块可以为信标光束提供颜色。
 * 染色玻璃等透明方块实现此接口来修改信标光束的颜色。
 */
class IBeaconBeamColorProvider {
public:
    virtual ~IBeaconBeamColorProvider() = default;

    /**
     * @brief 获取此方块提供的信标光束颜色
     *
     * @return 染料颜色枚举值
     */
    [[nodiscard]] virtual DyeColor getBeaconColor() const = 0;
};

/**
 * @brief 信标光束颜色工具类
 *
 * 提供染料颜色到 RGB float 数组的转换。
 */
struct BeaconColors {
    /**
     * @brief 获取染料颜色对应的 RGB float 数组
     *
     * 返回值范围 [0.0, 1.0]，用于信标光束渲染。
     *
     * @param color 染料颜色
     * @return RGB float 数组 {r, g, b}
     */
    [[nodiscard]] static std::array<f32, 3> getColorComponents(DyeColor color)
    {
        switch (color) {
            case DyeColor::White:
                return {0.9764706f, 0.9764706f, 0.9764706f}; // #FAFAFA
            case DyeColor::Orange:
                return {0.9764706f, 0.5019608f, 0.1137255f}; // #F9801D
            case DyeColor::Magenta:
                return {0.7803922f, 0.3058824f, 0.7411765f}; // #C74EBD
            case DyeColor::LightBlue:
                return {0.2274510f, 0.7019608f, 0.8549020f}; // #3AB3DA
            case DyeColor::Yellow:
                return {0.9960784f, 0.8470588f, 0.2392157f}; // #FED83D
            case DyeColor::Lime:
                return {0.5019608f, 0.7803922f, 0.1176471f}; // #80C71F
            case DyeColor::Pink:
                return {0.9529412f, 0.5450980f, 0.6666667f}; // #F38BAA
            case DyeColor::Gray:
                return {0.2784314f, 0.3098039f, 0.3215686f}; // #474F52
            case DyeColor::LightGray:
                return {0.6156863f, 0.6156863f, 0.5921569f}; // #9D9D97
            case DyeColor::Cyan:
                return {0.0862745f, 0.6117647f, 0.6117647f}; // #169C9C
            case DyeColor::Purple:
                return {0.5372549f, 0.1960784f, 0.7215686f}; // #8932B8
            case DyeColor::Blue:
                return {0.2352941f, 0.2666667f, 0.6666667f}; // #3C44AA
            case DyeColor::Brown:
                return {0.5098039f, 0.3294118f, 0.1960784f}; // #835432
            case DyeColor::Green:
                return {0.3686275f, 0.4862745f, 0.0862745f}; // #5E7C16
            case DyeColor::Red:
                return {0.6901961f, 0.1803922f, 0.1490196f}; // #B02E26
            case DyeColor::Black:
                return {0.1137255f, 0.1137255f, 0.1294118f}; // #1D1D21
            default:
                return {1.0f, 1.0f, 1.0f}; // 默认白色
        }
    }
};

} // namespace block
} // namespace mc
