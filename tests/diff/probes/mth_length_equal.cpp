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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"

#include "probe.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 把 f64 的原始位模式格式化成 0x%016llx。
///
/// length 走 sqrt，f32 / f64 两版的结果在末位不同，必须比位模式。
std::string bitsOf(f64 value)
{
    u64 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return std::string(buffer);
}

/// 单元表。
///
/// equal 的用例围绕 1e-5 布置：差值为 0、5e-6（远小于阈值）、
/// 9.9999e-6 与 1.00001e-5（正好落在 f32 阈值两侧）——后者是能区分
/// 1.0e-5f 与 1.0e-5 两个常量的关键点。
constexpr const char* UNITS[] = {
    "len2sq|3|4",
    "len2sq|1|1",
    "len2sq|0|0",
    "len2sq|-3|-4",
    "len2sq|0.1|0.2",
    "len3sq|1|2|2",
    "len3sq|2|3|6",
    "len3sq|0|0|0",
    "len3sq|-2|3|-6",
    "len3sq|0.1|0.2|0.3",
    "len2|3|4",
    "len2|1|1",
    "len2|0|0",
    "len2|-3|-4",
    "len2|0.1|0.2",
    "len2f|3|4",
    "len2f|1|1",
    "len2f|0.1|0.2",
    "len2f|1e-20|1e-20",
    "len2f|1e20|1e20",
    "eqf|1|1",
    "eqf|1|1.00001",
    "eqf|1|1.0001",
    "eqf|1|1.000001",
    "eqf|0|0.000005",
    "eqf|0|0.00000999",
    "eqf|0|0.00001001",
    "eqd|1|1",
    "eqd|0|0.000005",
    "eqd|0|0.000009999",
    "eqd|0|0.0000099999",
    "eqd|0|0.0000100001",
    "eqd|0|0.00001",
    "eqd|0|0.0001",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_length_equal: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "len2sq" && parts.size() == 3) {
        const f64 result = math::lengthSquared(std::stod(parts[1]), std::stod(parts[2]));
        return json{{"unit", unit}, {"bits", bitsOf(result)}}.dump();
    }
    if (mode == "len3sq" && parts.size() == 4) {
        const f64 result = math::lengthSquared(std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]));
        return json{{"unit", unit}, {"bits", bitsOf(result)}}.dump();
    }
    if (mode == "len2" && parts.size() == 3) {
        const f64 result = math::length(std::stod(parts[1]), std::stod(parts[2]));
        return json{{"unit", unit}, {"bits", bitsOf(result)}}.dump();
    }
    if (mode == "len2f" && parts.size() == 3) {
        const f32 result = math::length(std::stof(parts[1]), std::stof(parts[2]));
        return json{{"unit", unit}, {"bits", bitsOf(static_cast<f64>(result))}}.dump();
    }
    if (mode == "eqf" && parts.size() == 3) {
        const bool result = math::equal(std::stof(parts[1]), std::stof(parts[2]));
        return json{{"unit", unit}, {"value", result}}.dump();
    }
    if (mode == "eqd" && parts.size() == 3) {
        const bool result = math::equal(std::stod(parts[1]), std::stod(parts[2]));
        return json{{"unit", unit}, {"value", result}}.dump();
    }

    throw std::runtime_error("mth_length_equal: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_length_equal, &writeList, &runLine)

} // namespace mc::diff
