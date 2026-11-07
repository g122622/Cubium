#include "probe.hpp"

#include "common/util/math/MathUtils.hpp"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @brief `Mth` 里一批此前未覆盖的纯计算函数
 *
 * 这些函数都不依赖世界状态，可以直接喂入任意输入比对。它们原本在 Cubium 里缺失，
 * 这次补齐实现的同时补上差分覆盖。
 *
 * 单元格式：`<函数>|<输入...>`
 *
 * 每个函数都有各自的陷阱：
 *
 * - `murmur`  ：移位必须是无符号右移，有符号右移会把符号位带下来；
 * - `invCube` ：整数部分是带符号除法（向零取整），负数与无符号移位结果不同；
 * - `sign`    ：负零应返回 0，NaN 应返回 -1；
 * - `quantize`：向下取整后乘回步长，负值不能向零截断；
 * - `catmull` ：五参数样条，系数符号写错不易察觉；
 * - `square`  ：负数输入返回负数（不是开方）；
 * - `multiple`：零除数在原版会抛异常，这里只测非零除数。
 */

/// 整数输入：覆盖 0、正负、极值、以及各位置为 1 的位模式。
const std::vector<i32>& intInputs()
{
    static const std::vector<i32> table = {
        0,
        1,
        -1,
        2,
        -2,
        7,
        -7,
        13,
        255,
        256,
        -256,
        65535,
        65536,
        -65536,
        2147483647,
        -2147483647 - 1,
        0x55555555,
        0x7FFFFFF,
        -123456789,
        123456789,
    };
    return table;
}

/// f32 输入：覆盖 0、正负零、极小极大、以及各典型量级。
const std::vector<f32>& floatInputs()
{
    static const std::vector<f32> table = {
        0.0f,
        -0.0f,
        1.0f,
        -1.0f,
        2.0f,
        0.5f,
        -0.5f,
        3.0f,
        8.0f,
        27.0f,
        -27.0f,
        0.001f,
        -0.001f,
        1.0e-30f,
        1.0e30f,
        123.456f,
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (const i32 value : intInputs()) {
        std::fprintf(out, "murmur|%d\n", value);
        std::fprintf(out, "square|%d\n", value);
    }
    for (const f32 value : floatInputs()) {
        std::fprintf(out, "invCube|%.17g\n", static_cast<double>(value));
        std::fprintf(out, "sign|%.17g\n", static_cast<double>(value));
    }
    // 量化：步长与数值组合
    for (const i32 step : {1, 2, 3, 7, 16, 64}) {
        for (const f64 value : {-100.5, -64.0, -1.0, -0.5, 0.0, 0.5, 1.0, 63.9, 64.0, 100.5, 1234.5678}) {
            std::fprintf(out, "quantize|%d|%.17g\n", step, value);
        }
    }
    // 棋盘距离：对称与非对称组合
    for (const auto& pair : std::vector<std::array<i32, 4>>{{0, 0, 0, 0},
             {0, 0, 5, 3},
             {0, 0, -5, 3},
             {0, 0, 3, 5},
             {10, -10, -10, 10},
             {100, 200, 100, 200},
             {0, 0, 1, 0},
             {0, 0, 0, 1}}) {
        std::fprintf(out, "chess|%d|%d|%d|%d\n", pair[0], pair[1], pair[2], pair[3]);
    }
    // 倍数判断
    for (const i32 divisor : {1, 2, 3, 4, 7, 16}) {
        for (const i32 value : {0, 1, -1, 6, -6, 7, -7, 16, -16, 100, -100}) {
            std::fprintf(out, "multiple|%d|%d\n", value, divisor);
        }
    }
    // 正方形边长
    for (const i32 count : {0, 1, 2, 3, 4, 5, 8, 9, 10, 15, 16, 17, 63, 64, 65, 1000, 1000000}) {
        std::fprintf(out, "squareSide|%d\n", count);
    }
    // Catmull-Rom：因子扫过 [0,1]，控制点用参差数值
    for (const f64 delta : {0.0, 0.25, 0.5, 0.75, 1.0, 0.1, 1.0 / 3.0, 0.9999999}) {
        std::fprintf(out, "catmull|%.17g|1.5|-2.25|3.125|-0.0625\n", delta);
        std::fprintf(out, "catmull|%.17g|-1.0|1.0|-1.0|1.0\n", delta);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string text = input.at("unit").get<std::string>();

    // 按 '|' 切分
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t pos = text.find('|', start);
        if (pos == std::string::npos) {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, pos - start));
        start = pos + 1;
    }
    if (parts.empty()) {
        return json{{"unit", text}, {"missing", true}}.dump();
    }

    const std::string& name = parts[0];
    json result{{"unit", text}};

    if (name == "murmur" && parts.size() == 2) {
        result["value"] = math::murmurHash3Mixer(static_cast<i32>(std::stol(parts[1])));
    } else if (name == "square" && parts.size() == 2) {
        result["value"] = math::square(static_cast<i32>(std::stol(parts[1])));
    } else if (name == "invCube" && parts.size() == 2) {
        const f32 value = static_cast<f32>(std::stod(parts[1]));
        result["value"] = encodeFloat(static_cast<f64>(math::fastInvCubeRoot(value)));
    } else if (name == "sign" && parts.size() == 2) {
        result["value"] = math::sign(std::stod(parts[1]));
    } else if (name == "quantize" && parts.size() == 3) {
        result["value"] = math::quantize(std::stod(parts[2]), static_cast<i32>(std::stol(parts[1])));
    } else if (name == "chess" && parts.size() == 5) {
        result["value"] = math::chessboardDistance(static_cast<i32>(std::stol(parts[1])),
            static_cast<i32>(std::stol(parts[2])),
            static_cast<i32>(std::stol(parts[3])),
            static_cast<i32>(std::stol(parts[4])));
    } else if (name == "multiple" && parts.size() == 3) {
        result["value"] =
            math::isMultipleOf(static_cast<i32>(std::stol(parts[1])), static_cast<i32>(std::stol(parts[2])));
    } else if (name == "squareSide" && parts.size() == 2) {
        result["value"] = math::smallestSquareSide(static_cast<i32>(std::stol(parts[1])));
    } else if (name == "catmull" && parts.size() == 6) {
        const f32 delta = static_cast<f32>(std::stod(parts[1]));
        const f32 p0 = static_cast<f32>(std::stod(parts[2]));
        const f32 p1 = static_cast<f32>(std::stod(parts[3]));
        const f32 p2 = static_cast<f32>(std::stod(parts[4]));
        const f32 p3 = static_cast<f32>(std::stod(parts[5]));
        result["value"] = encodeFloat(static_cast<f64>(math::catmullrom(delta, p0, p1, p2, p3)));
    } else {
        result["missing"] = true;
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_misc, &writeList, &runLine)

} // namespace mc::diff
