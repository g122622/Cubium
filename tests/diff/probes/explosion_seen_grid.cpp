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
/**
 * @brief 差分能力：爆炸可见度采样的网格生成（纯计算，不依赖世界）。
 *
 * `list`：一组实体碰撞箱尺寸
 * `run` ：{ "minX": "0", "maxX": "0.6", ... }
 *         → { "unit": "1.8", "sampleCount": 45, "stepX": "0.5", "offsetX": "0.25", ... }
 *
 * 复刻原版 `ServerExplosion.getSeenPercent` 里**网格生成**那一半：
 *
 * ```java
 * double d0 = 1.0 / ((aabb.maxX - aabb.minX) * 2.0 + 1.0);
 * double d3 = (1.0 - Math.floor(1.0 / d0) * d0) / 2.0;
 * for (double d5 = 0.0; d5 <= 1.0; d5 += d0)
 *     for (double d6 = 0.0; d6 <= 1.0; d6 += d1)
 *         for (double d7 = 0.0; d7 <= 1.0; d7 += d2) {
 *             double d8 = Mth.lerp(d5, aabb.minX, aabb.maxX);
 *             ...
 *         }
 * ```
 *
 * 另一半（对每个采样点做 `level.clip` 射线检测）需要世界，本能力不覆盖。
 *
 * ## 为什么网格本身值得单独钉住
 *
 * 采样点数 = `floor(1 / step) + 1`，而这个「+1」在 `f` 累加的浮点误差下**可能
 * 少一次或多一次**。Cubium 的 `Explosion::_getBlockDensity` 用 `f32` 做这一步，
 * 原版用 `double`；同样的碰撞箱会得到不同的点数，可见度分母随之改变，最终伤害
 * 也跟着变。
 *
 * ## 一个必须局部关闭快速浮点的坑
 *
 * 项目根 CMakeLists 全局开了 `-ffast-math`。它允许编译器重结合浮点表达式，于是
 * `floor(1 / step) * step` 会被折叠成 `(1 / step) * step`——后者**不保证等于 1**，
 * 在 `step` 恰为 1/3、1/6 这类值时留下 `2.78e-17` 的残差，而 Java 侧得到精确 0。
 *
 * 这个残差会一路传到采样点坐标上，让「看起来完全一样的两份实现」在第一个采样点
 * 就产生差异。这里用「恰好整除时把余数显式归零」的方式绕开它。
 *
 * TODO: 待 Cubium 把 `_getBlockDensity` 的网格计算改为 `double` 后，本能力应能
 * 直接复用它而不再自带一份实现。
 */
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "probe.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace mc::diff {
namespace {
using nlohmann::json;

std::string formatFloat(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

/**
 * @brief 原版 `(1.0 - floor(1.0 / step) * step) / 2.0`。
 *
 * 单独抽成函数是为了能在函数级关闭快速浮点：项目根 CMakeLists 全局开了
 * `-ffast-math`，它允许编译器把 `floor(1 / step) * step` 折叠成 `(1 / step) * step`，
 * 后者在 step 恰为 1/3、1/6 时留下 2.78e-17 的残差，而 Java 侧得精确 0。
 * 这里用「恰好整除时把余数显式归零」的方式绕开它，比依赖 pragma 更可靠。
 */
f64 computeOffset(f64 step)
{
    // 先算整数格数，再按原式求偏移。
    const f64 gridCount = std::floor(1.0 / step);
    const f64 covered = gridCount * step;
    // 原版 `1.0 - covered`。当步长恰好整除 1 时，Java 得到精确 0；这里显式保护
    // 这一点，避免 -ffast-math 把 covered 重结合成别的值而留下 2.78e-17 残差。
    const f64 remainder = std::abs(covered - 1.0) < 1e-12 ? 0.0 : (1.0 - covered);
    const f64 offset = remainder / 2.0;
    return offset;
}

/// 原版 `Mth.lerp(t, min, max) = min + t * (max - min)`。
f64 lerp(f64 t, f64 minValue, f64 maxValue)
{
    return math::lerp(minValue, maxValue, t);
}

struct Box {
    f64 minX;
    f64 minY;
    f64 minZ;
    f64 maxX;
    f64 maxY;
    f64 maxZ;
};

/**
 * @brief 覆盖用的碰撞箱尺寸表。
 *
 * 覆盖玩家/幼年体/方块/苦力怕/末影人/铁傀儡/极小体/大尺寸，以及三个轴步长全不同、
 * 原点非整数的情形。
 */
const Box BOXES[] = {
    {0.0, 0.0, 0.0, 0.6, 1.8, 0.6},        // 玩家
    {0.0, 0.0, 0.0, 0.3, 0.9, 0.3},        // 幼年体
    {0.0, 0.0, 0.0, 1.0, 1.0, 1.0},        // 方块
    {0.0, 0.0, 0.0, 0.6, 1.7, 0.6},        // 苦力怕
    {0.0, 0.0, 0.0, 0.6, 2.9, 0.6},        // 末影人
    {0.0, 0.0, 0.0, 1.4, 2.7, 1.4},        // 铁傀儡
    {0.0, 0.0, 0.0, 0.15, 0.15, 0.15},     // 极小体
    {0.0, 0.0, 0.0, 2.5, 2.5, 2.5},        // 大尺寸
    {0.0, 0.0, 0.0, 0.7, 1.9, 1.1},        // 三个轴步长全不同
    {-0.25, 1.5, -0.75, 0.35, 3.3, -0.15}, // 原点非整数
};

void writeList(FILE* out)
{
    for (const auto& box : BOXES) {
        std::fprintf(out,
            "%s|%s|%s|%s|%s|%s\n",
            formatFloat(box.minX).c_str(),
            formatFloat(box.minY).c_str(),
            formatFloat(box.minZ).c_str(),
            formatFloat(box.maxX).c_str(),
            formatFloat(box.maxY).c_str(),
            formatFloat(box.maxZ).c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    auto number = [&input](const char* key) {
        const auto& value = input.at(key);
        return value.is_string() ? std::stod(value.get<std::string>()) : value.get<f64>();
    };
    const Box box{number("minX"), number("minY"), number("minZ"), number("maxX"), number("maxY"), number("maxZ")};
    // 单元标识用 maxY：尺寸表里各箱子该值互不相同，单靠 minX 无法区分。
    const std::string unit = input.at("maxY").get<std::string>();
    // 原版全部用 double。
    const f64 stepX = 1.0 / ((box.maxX - box.minX) * 2.0 + 1.0);
    const f64 stepY = 1.0 / ((box.maxY - box.minY) * 2.0 + 1.0);
    const f64 stepZ = 1.0 / ((box.maxZ - box.minZ) * 2.0 + 1.0);
    // 原版：d3 = (1.0 - floor(1.0 / d0) * d0) / 2.0，注意用的是 1/d0 的 floor。
    // 局部关闭快速浮点，否则 floor(1/step)*step 会被折叠成 (1/step)*step。
    const f64 offsetX = computeOffset(stepX);
    const f64 offsetZ = computeOffset(stepZ);
    // 原版：三个步长只要有一个为负就直接返回 0。
    if (stepX < 0.0 || stepY < 0.0 || stepZ < 0.0) {
        return json{{"unit", unit},
            {"minX", input.at("minX")},
            {"minY", input.at("minY")},
            {"minZ", input.at("minZ")},
            {"maxX", input.at("maxX")},
            {"maxY", input.at("maxY")},
            {"maxZ", input.at("maxZ")},
            {"degenerate", true},
            {"sampleCount", 0}}
            .dump();
    }
    std::size_t sampleCount = 0;
    json first = json::array();
    json last = json::array();
    // 原版三重循环，全部用 double 累加。
    for (f64 fx = 0.0; fx <= 1.0; fx += stepX) {
        for (f64 fy = 0.0; fy <= 1.0; fy += stepY) {
            for (f64 fz = 0.0; fz <= 1.0; fz += stepZ) {
                const f64 sx = lerp(fx, box.minX, box.maxX) + offsetX;
                const f64 sy = lerp(fy, box.minY, box.maxY);
                const f64 sz = lerp(fz, box.minZ, box.maxZ) + offsetZ;
                if (sampleCount < 3) {
                    first.push_back(json::array({formatFloat(sx), formatFloat(sy), formatFloat(sz)}));
                }
                last.push_back(json::array({formatFloat(sx), formatFloat(sy), formatFloat(sz)}));
                if (last.size() > 3) {
                    last.erase(last.begin());
                }
                ++sampleCount;
            }
        }
    }
    return json{{"unit", unit},
        {"minX", input.at("minX")},
        {"minY", input.at("minY")},
        {"minZ", input.at("minZ")},
        {"maxX", input.at("maxX")},
        {"maxY", input.at("maxY")},
        {"maxZ", input.at("maxZ")},
        {"degenerate", false},
        {"stepX", formatFloat(stepX)},
        {"stepY", formatFloat(stepY)},
        {"stepZ", formatFloat(stepZ)},
        {"offsetX", formatFloat(offsetX)},
        {"offsetZ", formatFloat(offsetZ)},
        {"sampleCount", sampleCount},
        // 原版 getSeenPercent 返回 (float) i / j，j 就是采样点总数。
        {"visibleDenominator", sampleCount},
        {"firstSample", first},
        {"lastSample", last}}
        .dump();
}
} // namespace
MC_DIFF_DEFINE_PROBE(explosion_seen_grid, &writeList, &runLine)
} // namespace mc::diff
