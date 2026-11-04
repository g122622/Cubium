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
 * @brief 差分能力：实体尺寸与包围盒（纯计算）。
 *
 * `list`：一组「宽 | 高 | X 缩放 | Y 缩放 | 位置 x | 位置 y | 位置 z」
 * `run` ：同上一行
 *         → { "width": "0.6", "height": "1.8", "eyeHeight": "1.53",
 *             "scaledWidth": "0.3", "scaledHeight": "0.9", "scaledEyeHeight": "0.765",
 *             "bbox": [ ... ] }
 *
 * 复刻原版 `record EntityDimensions(float width, float height, float eyeHeight, ...)`：
 *
 * ```java
 * private static float defaultEyeHeight(float h) { return h * 0.85F; }
 *
 * public EntityDimensions scale(float w, float h) {
 *     return new EntityDimensions(this.width * w, this.height * h,
 *                                 this.eyeHeight * h, ..., false);
 * }
 *
 * public AABB makeBoundingBox(double x, double y, double z) {
 *     float f = this.width / 2.0F;
 *     float f1 = this.height;
 *     return new AABB(x - f, y, z - f, x + f, y + f1, z + f);
 * }
 * ```
 *
 * ## 三个容易被忽略的细节
 *
 * 1. **眼睛高度只按高度缩放**。`scale(w, h)` 里 `eyeHeight * h`，与 `w` 无关。
 *    玩家潜行时宽高都缩到 0.6，眼睛高度也必须跟着高度走——写成 `eyeHeight * w`
 *    在宽高比不是 1 的实体上会立刻错。
 *
 * 2. **包围盒以中心为基准**，不是以脚底某角。`x ± width/2`，而 Y 方向是
 *    `[y, y + height]`（脚底在 y）。
 *
 * 3. **全部用 f32 计算**。`f = width / 2.0F` 是 float，`x - f` 里 `x` 是 double、
 *    `f` 先提升到 double——C++ 侧必须保持同样的提升顺序，否则大坐标下会差 ulp。
 *
 * TODO: Cubium 目前没有 `EntityDimensions` / `EntityAttachments`，本探针自带一份实现。
 * 待其实现后应改为直接调用，届时本能力才能覆盖「实体的宽高表是否正确」这一层。
 * 本能力当前覆盖的是**公式**：眼睛高度比例、缩放规则、包围盒展开。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `EntityDimensions.defaultEyeHeight`：眼睛高度占身高的比例。
constexpr f32 EYE_HEIGHT_RATIO = 0.85f;

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
 * @brief 覆盖用的场景表：`宽|高|X缩放|Y缩放|posX|posY|posZ`。
 *
 * 覆盖玩家原始尺寸、玩家潜行（宽高同缩）、幼年体、高瘦体（检验眼睛高度只跟高度）、
 * 宽扁体（检验包围盒 X/Z 半径）、以及大坐标（检验 double 提升）。
 */
constexpr const char* SCENES[] = {
    "0.6|1.8|1|1|0|0|0",               // 玩家原始
    "0.6|1.8|0.5|1|0|0|0",             // 只缩宽
    "0.6|1.8|1|0.5|0|0|0",             // 只缩高
    "0.6|1.8|0.5|0.5|0|0|0",           // 玩家潜行
    "0.3|0.9|1|1|0|0|0",               // 幼年体
    "0.6|2.9|1|1|0|0|0",               // 末影人（高瘦）
    "1.4|2.7|1|1|0|0|0",               // 铁傀儡（宽壮）
    "4|0.5|1|1|0|0|0",                 // 宽扁（宽远大于高）
    "0.6|1.8|1|1|100|64|-200",         // 大坐标
    "0.6|1.8|1|1|-1234.5|63.5|987.25", // 非整数大坐标
    "0.6|1.8|0.5|0.5|0.5|0.5|0.5",     // 缩放 + 非整数位置
    "0.8|1.95|1|1|0|0|0",              // 僵尸
};

void writeList(FILE* out)
{
    for (const auto* scene : SCENES) {
        std::fprintf(out, "%s\n", scene);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const auto parts = [&unit]() {
        std::vector<std::string> tokens;
        std::string current;
        for (char ch : unit) {
            if (ch == '|') {
                tokens.push_back(current);
                current.clear();
            } else {
                current.push_back(ch);
            }
        }
        tokens.push_back(current);
        return tokens;
    }();
    if (parts.size() != 7) {
        throw std::runtime_error("entity_dimensions: 单元应为 7 段 '宽|高|缩X|缩Y|px|py|pz'：" + unit);
    }

    const f32 width = static_cast<f32>(std::stod(parts[0]));
    const f32 height = static_cast<f32>(std::stod(parts[1]));
    const f32 scaleX = static_cast<f32>(std::stod(parts[2]));
    const f32 scaleY = static_cast<f32>(std::stod(parts[3]));
    const f64 posX = std::stod(parts[4]);
    const f64 posY = std::stod(parts[5]);
    const f64 posZ = std::stod(parts[6]);

    // 原版构造器：eyeHeight 默认 = height * 0.85F。
    const f32 eyeHeight = height * EYE_HEIGHT_RATIO;

    // 原版 scale(w, h)：宽乘 w、高乘 h、**眼睛高度乘 h**（不是 w）。
    const f32 scaledWidth = width * scaleX;
    const f32 scaledHeight = height * scaleY;
    const f32 scaledEyeHeight = eyeHeight * scaleY;

    // 原版 makeBoundingBox：f = width / 2.0F（float），再与 double 位置相加。
    const f32 halfWidth = scaledWidth / 2.0f;
    const f32 boxHeight = scaledHeight;

    const f64 minX = posX - static_cast<f64>(halfWidth);
    const f64 minY = posY;
    const f64 minZ = posZ - static_cast<f64>(halfWidth);
    const f64 maxX = posX + static_cast<f64>(halfWidth);
    const f64 maxY = posY + static_cast<f64>(boxHeight);
    const f64 maxZ = posZ + static_cast<f64>(halfWidth);

    return json{{"unit", unit},
        {"width", formatFloat(static_cast<f64>(width))},
        {"height", formatFloat(static_cast<f64>(height))},
        {"eyeHeight", formatFloat(static_cast<f64>(eyeHeight))},
        {"scaleX", formatFloat(static_cast<f64>(scaleX))},
        {"scaleY", formatFloat(static_cast<f64>(scaleY))},
        {"scaledWidth", formatFloat(static_cast<f64>(scaledWidth))},
        {"scaledHeight", formatFloat(static_cast<f64>(scaledHeight))},
        {"scaledEyeHeight", formatFloat(static_cast<f64>(scaledEyeHeight))},
        {"bboxMinX", formatFloat(minX)},
        {"bboxMinY", formatFloat(minY)},
        {"bboxMinZ", formatFloat(minZ)},
        {"bboxMaxX", formatFloat(maxX)},
        {"bboxMaxY", formatFloat(maxY)},
        {"bboxMaxZ", formatFloat(maxZ)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(entity_dimensions, &writeList, &runLine)

} // namespace mc::diff
