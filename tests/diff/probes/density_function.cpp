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
using world::gen::density::DensityFunction;
using world::gen::density::Mapped;
using world::gen::density::MappedType;
using world::gen::density::YClampedGradient;

std::string doubleBits(f64 value)
{
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(std::bit_cast<u64>(value)));
    return buffer;
}

/**
 * @brief 一条密度函数用例
 *
 * 只覆盖**不依赖噪声、不依赖其他密度函数**的那几类纯函数——它们可以完全由单元
 * 名字里的参数重建，两侧各造一份即可比对，不需要导出随机状态。噪声类
 * （`NoiseDensity` / `ShiftedNoise`）与缓存类（`Cache2D` / `FlatCache`）
 * 留待后续能力。
 *
 * 单元格式：`<类型>|<参数...>@<采样点下标>`
 *
 * - `const|<v>`                   恒定值
 * - `ygrad|<fromY>|<toY>|<fromV>|<toV>`  Y 轴钳位线性映射

 * - `mapabs|<v>` / `mapsq|<v>` / `mapcube|<v>` / `maphalf|<v>` / `mapquarter|<v>` /
 *   `mapsqueeze|<v>` / `mapinvert|<v>`   单参数变换
 */
const std::vector<std::string>& units()
{
    static const std::vector<std::string> table = {
        // 恒定值：包含零、负数、需要多位的十进制
        "const|0",
        "const|1",
        "const|-1",
        "const|0.1",
        "const|-0.3333333333",
        "const|1e-7",

        // YClampedGradient：原版地形最常用的一类（从 Y=-64 的实心到 Y=320 的空气）
        // fromY > toY（反向）与 fromY == toY 都包含，后者会让 inverseLerp 除以 0
        "ygrad|-64|320|1.5625|-0.078125",
        "ygrad|-64|320|0|1",
        "ygrad|0|256|-1|1",
        "ygrad|100|0|1|-1",
        "ygrad|0|0|0|1",
        "ygrad|-64|-64|0.5|-0.5",
        "ygrad|20|80|0.1|-0.7",

        // Mapped：七种变换各取正负与非整值
        "mapabs|-0.3333333333",
        "mapabs|0.3333333333",
        "mapabs|0",
        "mapsq|-1.5",
        "mapsq|1.5",
        "mapsq|0.1",
        "mapcube|-2.5",
        "mapcube|2.5",
        "mapcube|0.1",
        "maphalf|-1.5",
        "maphalf|1.5",
        "maphalf|0.1",
        "mapquarter|-1.5",
        "mapquarter|1.5",
        "mapquarter|0.1",
        "mapsqueeze|-1.5",
        "mapsqueeze|1.5",
        "mapsqueeze|0.1",
        "mapinvert|-1.5",
        "mapinvert|1.5",
        "mapinvert|0.1",
    };
    return table;
}

/// 采样坐标：Y 覆盖世界范围内的关键层位，X/Z 用少量非零值确认不影响纯 Y 函数。
const std::vector<std::array<i32, 3>>& samplePoints()
{
    static const std::vector<std::array<i32, 3>> table = {
        {0, -64, 0},
        {0, -32, 0},
        {0, 0, 0},
        {0, 20, 0},
        {0, 64, 0},
        {0, 100, 0},
        {0, 256, 0},
        {0, 320, 0},
        {17, 0, -23},
        {-1000, 50, 2000},
    };
    return table;
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t at = text.find('|', start);
        if (at == std::string::npos) {
            parts.push_back(text.substr(start));
            return parts;
        }
        parts.push_back(text.substr(start, at - start));
        start = at + 1;
    }
}

std::unique_ptr<DensityFunction> buildFunction(const std::vector<std::string>& parts)
{
    if (parts.empty()) {
        return nullptr;
    }
    const std::string& kind = parts[0];
    if (kind == "const" && parts.size() == 2) {
        return std::make_unique<Constant>(std::stod(parts[1]));
    }
    if (kind == "ygrad" && parts.size() == 5) {
        return std::make_unique<YClampedGradient>(
            std::stoi(parts[1]), std::stoi(parts[2]), std::stod(parts[3]), std::stod(parts[4]));
    }
    if (parts.size() == 2) {
        MappedType type;
        if (kind == "mapabs") {
            type = MappedType::Abs;
        } else if (kind == "mapsq") {
            type = MappedType::Square;
        } else if (kind == "mapcube") {
            type = MappedType::Cube;
        } else if (kind == "maphalf") {
            type = MappedType::HalfNegative;
        } else if (kind == "mapquarter") {
            type = MappedType::QuarterNegative;
        } else if (kind == "mapsqueeze") {
            type = MappedType::Squeeze;
        } else if (kind == "mapinvert") {
            type = MappedType::Invert;
        } else {
            return nullptr;
        }
        return std::make_unique<Mapped>(std::make_unique<Constant>(std::stod(parts[1])), type);
    }
    return nullptr;
}

void writeList(std::FILE* out)
{
    for (const auto& unit : units()) {
        for (size_t i = 0; i < samplePoints().size(); ++i) {
            std::fprintf(out, "%s@%zu\n", unit.c_str(), i);
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
    const std::string spec = requested.substr(0, at);
    const size_t pointIndex = static_cast<size_t>(std::stoul(requested.substr(at + 1)));
    if (pointIndex >= samplePoints().size()) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    // 直接构造生产代码的密度函数，不在这里重写 compute 的算法。
    const auto function = buildFunction(splitPipe(spec));
    if (function == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const auto& point = samplePoints()[pointIndex];
    const f64 value = function->compute(point[0], point[1], point[2]);
    return json{{"unit", requested},
        {"value", doubleBits(value)},
        {"min", doubleBits(function->minValue())},
        {"max", doubleBits(function->maxValue())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(density_function, &writeList, &runLine)

} // namespace mc::diff
