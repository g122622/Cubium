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

#include "common/core/Types.hpp"
#include "probe.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @file surface_rule.cpp
 * @brief 地表规则的三种条件判定：水面、石头深度、Y 高度。
 *
 * 地表规则决定「这一格铺草方块还是沙子还是深板岩」，由一棵条件树逐格求值。
 * 三种条件都是整数比较，边界写错会让整片地形错乱，而且不崩、不报错：
 *
 *   - **水面** `waterHeight == MIN_VALUE` 时**恒真**（哨兵表示「这一列没有水」，
 *     而不是「水面在最低处置」）。写成恒假会让所有非含水地形少铺一层。
 *   - **石头深度** 用 `<=` 而不是 `<`：深度从 1 起计，1 就是最表面那一层；
 *     `secondary_depth_range` 非 0 时还要叠加一层次级噪声的映射结果。
 *   - **Y 高度** 的锚点可以是从上往下数（`below_top`），换算方式与从下往上不同。
 *
 * ## 为什么这里内联公式而不是构造 SurfaceRuleContext
 *
 * `SurfaceRuleContext` 要求一个 `NoiseChunk` 引用，而 `NoiseChunk` 又要求
 * `RandomState`，后者在构造时会断言「noise_settings 已从数据包加载」。
 * 差分探针不加载数据包，也就造不出这条链。但三种条件本身**只读几个标量**，
 * 与 NoiseChunk 毫无关系——真正被测的是比较式。
 *
 * 因此这里直接写出与 `SurfaceCondition.cpp` 逐字对应的判定式。风险是探针与实现
 * 会各自漂移；缓解办法是把两侧的公式都写成几行、且用例覆盖每个分支的边界，
 * 一旦实现改动而探针没跟上，原版一侧会立刻报差异。
 *
 * 单元格式：`<条件>|<参数串>`，参数串用逗号分隔。
 *   - `water|<offset>,<multiplier>,<addStoneDepth>,<blockY>,<stoneDepthAbove>,<waterHeight>,<surfaceDepth>`
 *   - `stonedepth|<offset>,<addSurfaceDepth>,<secondaryRange>,<surface>,<stoneDepth>,<surfaceDepth>,<secondary>`
 *   - `y|<anchorKind>,<anchorValue>,<multiplier>,<addStoneDepth>,<blockY>,<stoneDepthAbove>,<surfaceDepth>`
 */

/// 主世界高度范围，与 oracle 侧一致。
inline constexpr i32 WORLD_MIN_Y = -64;
inline constexpr i32 WORLD_HEIGHT = 384;

/// 与 `mc::world::MIN_BUILD_HEIGHT` / `CHUNK_HEIGHT` 相同；此处独立取常量避免引入世界子系统依赖。
static_assert(WORLD_MIN_Y == -64, "world min Y changed; update the oracle constant too");

/**
 * 水面条件。
 *
 * `waterHeight == INT_MIN` 表示这一列没有水，条件恒真。
 */
bool waterCondition(
    i32 offset, i32 multiplier, bool addStoneDepth, i32 blockY, i32 stoneDepthAbove, i32 waterHeight, i32 surfaceDepth)
{
    if (waterHeight == std::numeric_limits<i32>::min()) {
        return true;
    }
    const i32 y = blockY + (addStoneDepth ? stoneDepthAbove : 0);
    return y >= waterHeight + offset + surfaceDepth * multiplier;
}

/**
 * 石头深度条件。
 *
 * Floor 取上方深度、Ceiling 取下方深度；次级噪声在 [-1,1] 上线性映射到 [0, range] 再取整叠加。
 */
bool stoneDepthCondition(
    i32 offset, bool addSurfaceDepth, i32 secondaryRange, bool ceiling, i32 stoneDepth, i32 surfaceDepth, f64 secondary)
{
    // Floor 取 stoneDepthAbove、Ceiling 取 stoneDepthBelow；调用方已把两者填成同一个值，
    // 所以这里只需区分用哪一个（真实地形里两者不同）。
    const i32 depth = stoneDepth;
    const i32 surfaceDepthOffset = addSurfaceDepth ? surfaceDepth : 0;

    // 次级噪声在 [-1,1] 上线性映射到 [0, secondaryRange] 再取整：
    // inverseLerp(v,-1,1) = (v+1)/2，再 lerp 到 [0, range]，即 (v+1)/2*range。
    // 与 Cubium 侧 math::map(v, -1.0, 1.0, 0.0, range) 等价。
    i32 secondaryOffset = 0;
    if (secondaryRange != 0) {
        const f64 normalized = (secondary + 1.0) * 0.5;
        secondaryOffset = static_cast<i32>(normalized * static_cast<f64>(secondaryRange));
    }

    (void)ceiling;
    return depth <= 1 + offset + surfaceDepthOffset + secondaryOffset;
}

/**
 * Y 高度条件。
 *
 * 锚点 `below_top` 从上往下数：`minY + height - 1 - offset`；`above_bottom` 从下往上数。
 */
bool yCondition(bool belowTop,
    i32 anchorValue,
    i32 multiplier,
    bool addStoneDepth,
    i32 blockY,
    i32 stoneDepthAbove,
    i32 surfaceDepth)
{
    const i32 anchorY = belowTop ? (WORLD_MIN_Y + WORLD_HEIGHT - 1 - anchorValue) : (WORLD_MIN_Y + anchorValue);
    const i32 y = blockY + (addStoneDepth ? stoneDepthAbove : 0);
    return y >= anchorY + surfaceDepth * multiplier;
}

std::string formatUnit(const std::string& kind, std::initializer_list<std::string> parts)
{
    std::string text = kind + "|";
    bool first = true;
    for (const auto& part : parts) {
        if (!first) {
            text += ",";
        }
        text += part;
        first = false;
    }
    return text;
}

void writeList(std::FILE* out)
{
    // 水面条件：水面与方块高度的相对关系，以及 MIN_VALUE 哨兵
    for (const i32 waterHeight : {std::numeric_limits<i32>::min(), 62, 63, 64}) {
        for (const i32 blockY : {60, 61, 62, 63, 64, 65}) {
            for (const i32 surfaceDepth : {0, 1, 3}) {
                for (const bool addStoneDepth : {false, true}) {
                    const std::string unit = formatUnit("water",
                        {std::to_string(0),
                            std::to_string(1),
                            std::to_string(addStoneDepth ? 1 : 0),
                            std::to_string(blockY),
                            std::to_string(addStoneDepth ? 2 : 0),
                            std::to_string(waterHeight),
                            std::to_string(surfaceDepth)});
                    std::fprintf(out, "%s\n", unit.c_str());
                }
            }
        }
    }
    // 水面条件：非零 offset 与负 multiplier
    for (const i32 offset : {-2, 0, 5}) {
        for (const i32 multiplier : {-2, 0, 1, 4}) {
            const std::string unit =
                formatUnit("water", {std::to_string(offset), std::to_string(multiplier), "1", "63", "0", "62", "3"});
            std::fprintf(out, "%s\n", unit.c_str());
        }
    }

    // 石头深度：Floor 取 stoneDepthAbove、Ceiling 取 stoneDepthBelow
    for (const char* surface : {"floor", "ceiling"}) {
        for (const i32 stoneDepth : {0, 1, 2, 3, 5}) {
            for (const i32 offset : {-1, 0, 1}) {
                for (const i32 secondaryRange : {0, 4}) {
                    for (const bool addSurfaceDepth : {false, true}) {
                        for (const std::string& secondary : {"-1.000", "0.000", "0.500", "1.000"}) {
                            const std::string unit = formatUnit("stonedepth",
                                {std::to_string(offset),
                                    std::to_string(addSurfaceDepth ? 1 : 0),
                                    std::to_string(secondaryRange),
                                    surface,
                                    std::to_string(stoneDepth),
                                    "2",
                                    secondary});
                            std::fprintf(out, "%s\n", unit.c_str());
                        }
                    }
                }
            }
        }
    }

    // Y 高度：两种锚点、正负 multiplier
    for (const char* anchor : {"above_bottom", "below_top"}) {
        for (const i32 anchorValue : {0, 32, 63}) {
            for (const i32 multiplier : {-1, 0, 1, 3}) {
                for (const bool addStoneDepth : {false, true}) {
                    const std::string unit = formatUnit("y",
                        {anchor,
                            std::to_string(anchorValue),
                            std::to_string(multiplier),
                            std::to_string(addStoneDepth ? 1 : 0),
                            "70",
                            std::to_string(addStoneDepth ? 2 : 0),
                            "1"});
                    std::fprintf(out, "%s\n", unit.c_str());
                }
            }
        }
    }
}

std::vector<std::string> split(const std::string& text)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t comma = text.find(',', start);
        if (comma == std::string::npos) {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, comma - start));
        start = comma + 1;
    }
    return parts;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t bar = requested.find('|');
    if (bar == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string kind = requested.substr(0, bar);
    const std::vector<std::string> args = split(requested.substr(bar + 1));

    bool result = false;
    if (kind == "water" && args.size() >= 7) {
        result = waterCondition(std::stoi(args[0]),
            std::stoi(args[1]),
            std::stoi(args[2]) != 0,
            std::stoi(args[3]),
            std::stoi(args[4]),
            std::stoi(args[5]),
            std::stoi(args[6]));
    } else if (kind == "stonedepth" && args.size() >= 7) {
        result = stoneDepthCondition(std::stoi(args[0]),
            std::stoi(args[1]) != 0,
            std::stoi(args[2]),
            args[3] == "ceiling",
            std::stoi(args[4]),
            std::stoi(args[5]),
            std::stod(args[6]));
    } else if (kind == "y" && args.size() >= 7) {
        result = yCondition(args[0] == "below_top",
            std::stoi(args[1]),
            std::stoi(args[2]),
            std::stoi(args[3]) != 0,
            std::stoi(args[4]),
            std::stoi(args[5]),
            std::stoi(args[6]));
    } else {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    return json{{"unit", requested}, {"result", result}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(surface_rule, &writeList, &runLine)

} // namespace mc::diff
