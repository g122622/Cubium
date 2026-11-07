#include "probe.hpp"

#include "server/world/gen/density/DensityFunctions.hpp"

#include <nlohmann/json.hpp>

#include <array>
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
using world::gen::density::Lerp;
using world::gen::density::RangeChoice;
using world::gen::density::TwoArgument;
using world::gen::density::TwoArgumentType;

/**
 * @brief 组合型密度函数的用例
 *
 * 覆盖 `TwoArgument`（Add/Mul/Min/Max）、`Lerp`、`RangeChoice` 三类。它们都不依赖噪声，
 * 子函数全部用 `Constant`，因此整棵树可以完全由单元名字的参数量化重建。
 *
 * 单元格式：`<类型>|<参数...>`
 *
 * - `add|<a>|<b>` / `mul|<a>|<b>` / `min|<a>|<b>` / `max|<a>|<b>`
 * - `lerp|<d>|<s>|<e>`         d 是插值因子，s/e 是两端
 * - `range|<x>|<lo>|<hi>|<in>|<out>`    x 与区间 [lo, hi) 比较，命中取 in，否则取 out
 *
 * 用例重点覆盖：
 *
 * - `Mul` 在一边为 0 时的短路（`v1 == 0.0 ? 0.0 : ...`），另一侧即使是 inf/nan 也要短路；
 * - `Lerp` 的 d 在 0、1 两个边界以及区间外；
 * - `RangeChoice` 的区间边界（等于 lo 命中、等于 hi 不命中）；
 * - 非整齐数值，让 f64 乘法/加法的末位差异暴露出来。
 */
const std::vector<std::string>& units()
{
    static const std::vector<std::string> table = {
        // TwoArgument：四则里的三种 + 两个比较
        "add|0.1|0.2",
        "add|-1.5|0.33",
        "add|1e10|-1e10",
        "add|3|4",
        "mul|0|5",
        "mul|0|-1.5e300",
        "mul|2.5|0.4",
        "mul|-1.5|-0.6666666666",
        "mul|1e200|1e200",
        "min|1.5|-2.5",
        "min|-2.5|1.5",
        "min|0.1|0.1",
        "max|1.5|-2.5",
        "max|-2.5|1.5",
        "max|0.1|0.1",

        // Lerp：d 在边界与区间外
        "lerp|0|10|-4",
        "lerp|1|10|-4",
        "lerp|0.5|10|-4",
        "lerp|-0.5|10|-4",
        "lerp|1.5|10|-4",
        "lerp|0.3333333333|1.5625|-0.078125",
        "lerp|0.98927504|10|0",

        // RangeChoice：[lo, hi) 的边界
        "range|0.5|0|1|7|-7",
        "range|0|0|1|7|-7",
        "range|1|0|1|7|-7",
        "range|-1|0|1|7|-7",
        "range|0.9999999999|0|1|7|-7",
        "range|-64|-64|320|1.5|-1.5",
        "range|319|-64|320|1.5|-1.5",
        "range|320|-64|320|1.5|-1.5",
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

    if (parts.size() == 3) {
        TwoArgumentType type;
        if (kind == "add") {
            type = TwoArgumentType::Add;
        } else if (kind == "mul") {
            type = TwoArgumentType::Mul;
        } else if (kind == "min") {
            type = TwoArgumentType::Min;
        } else if (kind == "max") {
            type = TwoArgumentType::Max;
        } else {
            return nullptr;
        }
        return std::make_unique<TwoArgument>(
            std::make_unique<Constant>(std::stod(parts[1])), std::make_unique<Constant>(std::stod(parts[2])), type);
    }

    if (kind == "lerp" && parts.size() == 4) {
        return std::make_unique<Lerp>(std::make_unique<Constant>(std::stod(parts[1])),
            std::make_unique<Constant>(std::stod(parts[2])),
            std::make_unique<Constant>(std::stod(parts[3])));
    }

    if (kind == "range" && parts.size() == 6) {
        return std::make_unique<RangeChoice>(std::make_unique<Constant>(std::stod(parts[1])),
            std::stod(parts[2]),
            std::stod(parts[3]),
            std::make_unique<Constant>(std::stod(parts[4])),
            std::make_unique<Constant>(std::stod(parts[5])));
    }
    return nullptr;
}

void writeList(std::FILE* out)
{
    for (const auto& unit : units()) {
        std::fprintf(out, "%s\n", unit.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    // 直接构造生产代码的密度函数，不在这里重写 Add/Mul/Lerp 的算术。
    const auto function = buildFunction(splitPipe(requested));
    if (function == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    // 三个参数都是常量，采样坐标不影响结果；仍取一组非零坐标确认这一点。
    const f64 value = function->compute(17, 42, -23);
    return json{{"unit", requested},
        {"value", encodeFloat(value)},
        {"min", encodeFloat(function->minValue())},
        {"max", encodeFloat(function->maxValue())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(density_composite, &writeList, &runLine)

} // namespace mc::diff
