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
using world::gen::density::DensityFunction;
using world::gen::density::EndIslands;

std::string doubleBits(f64 value)
{
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(std::bit_cast<u64>(value)));
    return buffer;
}

/**
 * @brief 末地岛屿密度的用例
 *
 * `EndIslands` 是末地地形的主密度函数：把方块坐标除以 8 得到岛屿网格坐标，在该网格上
 * 求一个「岛屿高度」，再映射到 `[-0.84375, 0.5625]`。它只依赖种子，不含分支表以外的状态。
 *
 * 每 8 格共享同一个采样值，所以采样点覆盖了方块坐标对 8 的余数的不同类型。
 *
 * 内部有一个 25x25 的邻域搜索，只在 `k1² + l1² > 4096`（距中心超过 64 格）且噪声
 * 小于 -0.9 时才产生外岛。因此采样点必须覆盖「中心区」与「远区」两类，否则那个分支
 * 一次都不会进入。
 *
 * 单元格式：`<种子>@<采样点下标>`
 */
const std::vector<u64>& seeds()
{
    static const std::vector<u64> table = {
        0ULL, // 原版 codec 的默认种子
        1ULL,
        1234567ULL,
        987654321ULL,
        0x0123456789ABCDEFULL,
    };
    return table;
}

/// 采样坐标。前几个落在中心区（不触发外岛），后面逐步远离以进入外岛分支。
const std::vector<std::array<i32, 3>>& samplePoints()
{
    static const std::vector<std::array<i32, 3>> table = {
        // 原点与紧邻：余数为 0 与 1，覆盖 k/l 的四种组合
        {0, 0, 0},
        {1, 0, 0},
        {0, 0, 1},
        {1, 0, 1},
        // 中心区内但已偏移
        {7, 0, 7},
        {8, 0, 8},
        {16, 64, 16},
        // 只沿一轴远离：k1² + l1² 恰好跨过 4096，触发外岛分支
        {520, 0, 0},
        {0, 0, 520},
        {-520, 0, 0},
        // 两轴同时远离：邻域搜索的边界
        {512, 0, 512},
        {-512, 0, -512},
        // 更大的距离：跨多个岛屿网格
        {4000, 0, -4000},
        {16384, 128, 16384},
        // 极端坐标：f32 精度的边界
        {100000, 0, -100000},
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (const u64 seed : seeds()) {
        for (size_t i = 0; i < samplePoints().size(); ++i) {
            std::fprintf(out, "%llu@%zu\n", static_cast<unsigned long long>(seed), i);
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
    const u64 seed = static_cast<u64>(std::stoull(requested.substr(0, at)));
    const size_t index = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (index >= samplePoints().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    // 直接构造生产代码的 EndIslands，不在这里重写岛屿高度公式。
    const EndIslands function(seed);
    const auto& point = samplePoints()[index];
    const f64 value = function.compute(point[0], point[1], point[2]);
    return json{{"unit", requested},
        {"value", doubleBits(value)},
        {"min", doubleBits(function.minValue())},
        {"max", doubleBits(function.maxValue())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(end_islands, &writeList, &runLine)

} // namespace mc::diff
