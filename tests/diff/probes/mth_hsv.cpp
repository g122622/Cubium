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
 * @brief `Mth.hsvToArgb` / `hsvToRgb` 的差分能力
 *
 * **本探针调用生产代码** `math::hsvToArgb` / `math::hsvToRgb`，不在探针里重写转换逻辑。
 *
 * 色相按 60 度一档切成六段，每段走不同的分量排列。用例取四组集合的笛卡尔积
 * （色相 × 饱和度 × 明度 × alpha），而不是手挑几个颜色——手挑很难同时覆盖到
 * 「每段的每一条选区」「段边界」「饱和度为 0」这些只在特定组合下才出现的路径。
 *
 * 单元格式：`<色相>|<饱和度>|<明度>|<alpha>@<序号>`
 */
struct NamedFloat {
    const char* id;
    f32 value;
};

/// 色相：六段的中点 + 段边界 + 关键回绕点
const std::vector<NamedFloat>& hueCases()
{
    static const std::vector<NamedFloat> table = {
        // 六段中点
        {"h_0", 0.0833333f},
        {"h_1", 0.25f},
        {"h_2", 0.4166667f},
        {"h_3", 0.5833333f},
        {"h_4", 0.75f},
        {"h_5", 0.9166667f},
        // 段边界（每段的起点）
        {"b_0", 0.0f},
        {"b_1", 0.1666667f},
        {"b_2", 0.3333333f},
        {"b_3", 0.5f},
        {"b_4", 0.6666667f},
        {"b_5", 0.8333333f},
        // 回绕：1.0 应等价于 0.0
        {"wrap_1", 1.0f},
        // 纯红端点
        {"red", 0.0f},
    };
    return table;
}

/// 饱和度：0（灰）、中间值、1（纯色）
const std::vector<NamedFloat>& saturationCases()
{
    static const std::vector<NamedFloat> table = {
        {"s_0", 0.0f},
        {"s_q", 0.25f},
        {"s_h", 0.5f},
        {"s_1", 1.0f},
    };
    return table;
}

/// 明度：0（黑）、中间值、1（全亮）
const std::vector<NamedFloat>& valueCases()
{
    static const std::vector<NamedFloat> table = {
        {"v_0", 0.0f},
        {"v_q", 0.25f},
        {"v_h", 0.5f},
        {"v_1", 1.0f},
    };
    return table;
}

/// alpha 通道：0、中间、满、越界
const std::vector<i32>& alphaCases()
{
    static const std::vector<i32> table = {0, 127, 255, 256};
    return table;
}

void writeList(std::FILE* out)
{
    size_t index = 0;
    for (const auto& hue : hueCases()) {
        for (const auto& sat : saturationCases()) {
            for (const auto& val : valueCases()) {
                for (const i32 alpha : alphaCases()) {
                    std::fprintf(out, "%s|%s|%s|%d@%zu\n", hue.id, sat.id, val.id, alpha, index);
                    ++index;
                }
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string text = input.at("unit").get<std::string>();

    const size_t at = text.rfind('@');
    if (at == std::string::npos) {
        return json{{"unit", text}, {"missing", true}}.dump();
    }
    const std::string triple = text.substr(0, at);

    // 解析 `<色相>|<饱和度>|<明度>|<alpha>`
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t pos = triple.find('|', start);
        const std::string piece = triple.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
        parts.push_back(piece);
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    if (parts.size() != 4) {
        return json{{"unit", text}, {"missing", true}}.dump();
    }

    // 按 id 查表，避免从单元名里反解浮点（十进制往返会引入误差）
    f32 hue = 0.0f;
    f32 saturation = 0.0f;
    f32 value = 0.0f;
    bool foundHue = false;
    bool foundSat = false;
    bool foundVal = false;
    for (const auto& item : hueCases()) {
        if (parts[0] == item.id) {
            hue = item.value;
            foundHue = true;
            break;
        }
    }
    for (const auto& item : saturationCases()) {
        if (parts[1] == item.id) {
            saturation = item.value;
            foundSat = true;
            break;
        }
    }
    for (const auto& item : valueCases()) {
        if (parts[2] == item.id) {
            value = item.value;
            foundVal = true;
            break;
        }
    }
    if (!foundHue || !foundSat || !foundVal) {
        return json{{"unit", text}, {"missing", true}}.dump();
    }
    const i32 alpha = std::stoi(parts[3]);

    // 直接调用生产代码。
    const u32 argb = math::hsvToArgb(hue, saturation, value, alpha);
    const u32 rgb = math::hsvToRgb(hue, saturation, value);
    return json{{"unit", text}, {"argb", argb}, {"rgb", rgb}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_hsv, &writeList, &runLine)

} // namespace mc::diff
