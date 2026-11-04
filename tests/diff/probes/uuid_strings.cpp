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
 * @brief 差分能力：UUID 字符串解析与格式化。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`mc::util::uuidFromString` / `uuidToString` / `uuidToStringWithDashes`）。
 *
 * Cubium 侧（`common/util/UuidUtils.hpp`）：
 *
 * ```cpp
 * using Uuid = std::array<u8, 16>;
 *
 * Uuid uuidFromString(const std::string&);          // 失败返回全零数组
 * std::string uuidToString(const Uuid&);            // 32 位小写十六进制
 * std::string uuidToStringWithDashes(const Uuid&);  // 36 位标准格式
 * ```
 *
 * 原版侧：
 *
 * ```java
 * java.util.UUID.fromString(String);   // JDK 实现，格式非法时抛 IllegalArgumentException
 * java.util.UUID.toString();            // 36 位标准格式
 * ```
 *
 * ## 四个关键点
 *
 * 1. **JDK 的 `UUID.fromString` 只接受带连字符的 36 位格式。**
 *
 *    用 JDK 实测：
 *
 *    | 输入 | `UUID.fromString` |
 *    |---|---|
 *    | `069a79f4-44e9-4726-a5be-fca90e38aaf5` | ✓ |
 *    | **`069a79f444e94726a5befca90e38aaf5`**（32 位）** | **`IllegalArgumentException`** |
 *    | `069A79F4-44E9-4726-A5BE-FCA90E38AAF5` | ✓（转小写）|
 *    | 空串 / `dunno` | `IllegalArgumentException` |
 *
 *    **Cubium 的 `uuidFromString` 接受 32 位无连字符格式**——这是两者的关键差异。
 *
 * 2. **Cubium 解析失败返回全零数组，JDK 抛异常。**
 *
 *    这是「哨兵值 vs 异常」的经典差异，本能力用 `null` 表示 JDK 的异常。
 *
 * 3. **`uuidToString` 输出 32 位无连字符小写，`uuidToStringWithDashes` 输出 36 位。**
 *
 *    原版 `UUID.toString()` 只有 36 位一种形式。
 *
 * 4. **往返性：`uuidFromString(uuidToStringWithDashes(x)) == x`。**
 *
 * 单元格式：
 * - `parse|<text>` —— `uuidFromString`，失败输出 `null`
 * - `toString|<uuid36>` —— `uuidToString`（32 位）
 * - `toStringDashes|<uuid36>` —— `uuidToStringWithDashes`（36 位）
 * - `roundTrip|<uuid36>` —— 往返是否保真
 * - `size` —— UUID 字节数
 */
#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/UuidUtils.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 把 16 字节 UUID 渲染成 36 位标准格式（小写）。
std::string toDashed(const Uuid& uuid)
{
    return util::uuidToStringWithDashes(uuid);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        "size",
        // 36 位标准格式：应能解析
        "parse|069a79f4-44e9-4726-a5be-fca90e38aaf5",
        "parse|00000000-0000-0000-0000-000000000000",
        "parse|ffffffff-ffff-ffff-ffff-ffffffffffff",
        // 大写：应能解析
        "parse|069A79F4-44E9-4726-A5BE-FCA90E38AAF5",
        // 32 位无连字符：JDK 拒绝，Cubium 接受——关键差异点
        "parse|069a79f444e94726a5befca90e38aaf5",
        // 非法输入
        "parse|",
        "parse|dunno",
        "parse|069a79f4-44e9-4726-a5be-fca90e38aaf",
        "parse|069a79f444e94726a5befca90e38aaf5-",
        "parse|-069a79f4-44e9-4726-a5be-fca90e38aaf5",
        "parse|069a79f4_44e9_4726_a5be_fca90e38aaf5",
        // 格式化
        "toString|069a79f4-44e9-4726-a5be-fca90e38aaf5",
        "toString|00000000-0000-0000-0000-000000000000",
        "toStringDashes|069a79f4-44e9-4726-a5be-fca90e38aaf5",
        "toStringDashes|00000000-0000-0000-0000-000000000000",
        // 往返
        "roundTrip|069a79f4-44e9-4726-a5be-fca90e38aaf5",
        "roundTrip|ffffffff-ffff-ffff-ffff-ffffffffffff",
    };
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
    if (parts.empty()) {
        throw std::runtime_error("uuid_strings: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "size" && parts.size() == 1) {
        value = std::to_string(sizeof(Uuid));
    } else if (parts.size() != 2) {
        throw std::runtime_error("uuid_strings: 单元段数不符：" + unit);
    } else if (mode == "parse") {
        // Cubium 的 uuidFromString 失败时返回全零数组（无法与外部的合法全零 UUID 区分）。
        // 这里如实输出解析结果；若原版抛异常（输出 null）而 Cubium 得到全零，
        // 就会形成可见差异。
        value = toDashed(util::uuidFromString(parts[1]));
    } else if (mode == "toString") {
        value = util::uuidToString(util::uuidFromString(parts[1]));
    } else if (mode == "toStringDashes") {
        value = toDashed(util::uuidFromString(parts[1]));
    } else if (mode == "roundTrip") {
        const Uuid original = util::uuidFromString(parts[1]);
        const Uuid back = util::uuidFromString(toDashed(original));
        value = (original == back) ? "true" : "false";
    } else {
        throw std::runtime_error("uuid_strings: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(uuid_strings, &writeList, &runLine)

} // namespace mc::diff
