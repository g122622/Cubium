#include "probe.hpp"

#include "server/world/gen/density/DensityFunctions.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <bit>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using world::gen::density::Constant;
using world::gen::density::CubicSpline;
using world::gen::density::DensityFunction;
using world::gen::density::SplinePoint;

std::string doubleBits(f64 value)
{
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(std::bit_cast<u64>(value)));
    return buffer;
}

/**
 * @brief 一条样条用例
 *
 * 输入用 `Constant` 提供坐标值，这样整条链路只依赖样条本身，不引入噪声。
 * 每条用例给出控制点的 location / value / derivative 三元组，以及在哪些输入值上采样。
 *
 * 用例覆盖：
 *
 * - 单调递增与递减的 value；
 * - 全零导数（退化成线性插值）、非零导数（真正走三次 Hermite）；
 * - **控制点位置之间宽度不等**，因为 Hermite 里 `width` 参与了两次乘法；
 * - 端点的线性外推（输入落在首点之前、末点之后）；
 * - 输入恰好落在控制点上（此时 `t` 为 0 或 1，`t*(1-t)` 归零，只剩端点值）；
 * - 小数 location（原版是 float，小数值会带 f32 舍入）。
 */
struct SplineCase {
    const char* id;
    std::vector<std::array<f32, 3>> points; ///< {location, value, derivative}
};

const std::vector<SplineCase>& splineCases()
{
    static const std::vector<SplineCase> table = {
        // 两点、全零导数：退化成两段线性
        {"linear2", {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}}},

        // 三点、全零导数
        {"flat3", {{-1.0f, 0.5f, 0.0f}, {0.0f, -0.25f, 0.0f}, {1.0f, 0.75f, 0.0f}}},

        // 三点、非零导数：标准三次 Hermite
        {"hermite3", {{-1.0f, 0.5f, 0.25f}, {0.0f, -0.25f, 0.5f}, {1.0f, 0.75f, -0.125f}}},

        // 宽度不等：第一段窄、第二段宽，width 直接进 f8/f9
        {"uneven", {{0.0f, 0.0f, 1.0f}, {0.1f, 0.6f, -0.5f}, {2.0f, -0.3f, 0.2f}}},

        // 小数 location（原版用 float 存，0.1 / 0.3 这类会带 f32 舍入）
        {"fractional", {{0.1f, 0.5f, 0.125f}, {0.3f, -0.5f, 0.25f}, {0.7f, 1.5f, -0.375f}}},

        // 主世界 offset 那类形状：负到正、导数为零
        {"offset_like", {{-1.0f, 2.5f, 0.0f}, {-0.35f, 1.2f, 0.0f}, {-0.18f, -0.4f, 0.0f}, {0.25f, 0.9f, 0.0f}}},

        // 递减
        {"descending", {{-2.0f, 1.0f, -0.5f}, {0.0f, 0.0f, -0.25f}, {2.0f, -1.0f, 0.0f}}},

        // 单点：全部落在外推分支
        {"single", {{0.5f, 0.25f, 0.75f}}},
    };
    return table;
}

/// 采样输入值：覆盖外推、端点、区间内部、控制点上。
const std::vector<f32>& sampleInputs()
{
    static const std::vector<f32> table = {
        -3.0f,
        -2.0f,
        -1.0f,
        -0.5f,
        -0.35f,
        -0.18f,
        -0.1f,
        0.0f,
        0.05f,
        0.1f,
        0.2f,
        0.3f,
        0.5f,
        0.7f,
        1.0f,
        1.5f,
        2.0f,
        3.0f,
    };
    return table;
}

std::unique_ptr<CubicSpline> buildSpline(const SplineCase& splineCase)
{
    std::vector<SplinePoint> points;
    points.reserve(splineCase.points.size());
    for (const auto& point : splineCase.points) {
        points.emplace_back(point[0], point[1], point[2]);
    }
    return std::make_unique<CubicSpline>(std::make_shared<Constant>(0.0f), std::move(points));
}

void writeList(std::FILE* out)
{
    for (const auto& splineCase : splineCases()) {
        for (size_t i = 0; i < sampleInputs().size(); ++i) {
            std::fprintf(out, "%s@%zu\n", splineCase.id, i);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t at = requested.rfind('@');
    if (at == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const std::string caseId = requested.substr(0, at);
    const size_t index = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (index >= sampleInputs().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    for (const auto& splineCase : splineCases()) {
        if (caseId != splineCase.id) {
            continue;
        }
        // 直接构造生产代码的 CubicSpline，不在这里重写 Hermite 公式。
        const auto spline = buildSpline(splineCase);
        const f32 coordinate = sampleInputs()[index];
        const f64 value = spline->apply(coordinate, 0, 0, 0);
        return json{{"unit", requested},
            {"value", doubleBits(value)},
            {"min", doubleBits(spline->minValue())},
            {"max", doubleBits(spline->maxValue())}}
            .dump();
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(cubic_spline, &writeList, &runLine)

} // namespace mc::diff
