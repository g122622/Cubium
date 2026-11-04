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
 * @brief 差分能力：`Mth.packDegrees` / `Mth.unpackDegrees`（角度单字节压缩）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static byte packDegrees(float degrees) {
 *     return (byte) floor(degrees * 256.0F / 360.0F);
 * }
 *
 * public static float unpackDegrees(byte packed) {
 *     return packed * 360 / 256.0F;
 * }
 * ```
 *
 * 这对函数是**实体旋转角的网络同步格式**：一个 `byte`（`-128..127`）表示 `-180..180` 度，
 * 分辨率 `360/256 = 1.40625` 度。
 *
 * ## 四个关键点
 *
 * 1. **`packDegrees` 不做任何 clamp，`(byte)` 转换是截断到 8 位。**
 *
 *    `floor(x * 256 / 360)` 的结果再强转 `byte`，Java 的 `(byte)` 是**保留低 8 位**：
 *
 *    - `180 × 256 / 360 = 128`，`(byte) 128` → **`-128`**（回绕！）
 *    - `-180 × 256 / 360 = -128`，`(byte) -128` → `-128`
 *    - `179` → `127`（正常）
 *    - `-179` → `-127`
 *
 *    **`+180` 与 `-180` 都打包成 `-128`**——两者在单字节表示下不可区分。
 *
 *    超出 `[-180, 180]` 的输入会继续回绕：`360` → `256` → `(byte) 256` = `0`。
 *
 * 2. **`floor` 是 `Mth.floor`（向负无穷取整），不是 `(int)` 截断。**
 *
 *    对负数两者不同：`-1.5` 的 `Mth.floor` 是 `-2`，而 `(int)` 是 `-1`。
 *
 *    本能力里 `x * 256 / 360` 对负角度会产生负小数，因此该差异**会实际生效**。
 *
 * 3. **`unpackDegrees` 的 `packed * 360` 是 `int` 乘法，分母才是 `float`。**
 *
 *    ```java
 *    return packed * 360 / 256.0F;
 *    ```
 *
 *    `packed`（`byte`）先提升为 `int`，乘 `360` 得 `int`，再除 `256.0F` 得 `float`。
 *
 *    若 C++ 写成 `packed * 360.0F / 256.0F`，中间结果位宽不同，**浮点舍入可能差 1 ULP**。
 *
 * 4. **往返不是恒等变换。**
 *
 *    `unpack(pack(x))` 会把 `x` 量化到最近的 `360/256` 网格，**误差最大 `1.40625` 度**。
 *
 *    但 `unpack(pack(unpack(b)))` 对合法 `byte` `b` 是**恒等**的（因为网格点本身就是精确值）。
 *
 * 单元格式：
 * - `pack|<degrees>` —— `packDegrees(float) → byte`
 * - `unpack|<packed>` —— `unpackDegrees(byte) → float`
 * - `round|<degrees>` —— `unpackDegrees(packDegrees(x))`，验证量化往返
 *
 * `pack` 输出**有符号十进制整数**（`-128..127`）；`unpack` / `round` 输出按 `%.9g` 格式化的
 * `float` 字符串。
 */
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `Mth.floor(float)`：先向零截断成 int，再对负小数减一。
i32 mthFloor(f32 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value < static_cast<f32>(truncated) ? truncated - 1 : truncated;
}

/// 原版 `Mth.packDegrees`：`(byte) floor(degrees * 256 / 360)`。
i8 packDegrees(f32 degrees)
{
    return static_cast<i8>(static_cast<u8>(mthFloor(degrees * 256.0F / 360.0F) & 0xFF));
}

/// 原版 `Mth.unpackDegrees`：`packed * 360 / 256.0F`，中间是 int 乘法。
f32 unpackDegrees(i8 packed)
{
    return static_cast<f32>(static_cast<i32>(packed) * 360) / 256.0F;
}

/// 格式化 `float`：`%.9g` 并去尾零。
std::string formatNumber(f32 value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0 ? "Infinity" : "-Infinity";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<f64>(value));
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

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // pack：值域两端（关键：180 回绕成 -128）
        "pack|0",
        "pack|90",
        "pack|179",
        "pack|180",
        "pack|-179",
        "pack|-180",
        // pack：超出值域（继续回绕）
        "pack|181",
        "pack|360",
        "pack|-360",
        "pack|270",
        // pack：分辨率网格点
        "pack|1.40625",
        "pack|2.8125",
        "pack|-1.40625",
        // pack：负小数（Mth.floor 与 (int) 的分歧点）
        "pack|-0.5",
        "pack|-2.8125",
        "pack|0.5",
        // unpack：byte 端点
        "unpack|0",
        "unpack|1",
        "unpack|127",
        "unpack|-128",
        "unpack|-1",
        "unpack|64",
        // round：往返量化
        "round|0",
        "round|90",
        "round|179",
        "round|180",
        "round|-180",
        "round|100",
        "round|-100",
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

f32 parseF(const std::string& text)
{
    return static_cast<f32>(std::stod(text));
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 2) {
        throw std::runtime_error("mth_pack_degrees: 单元需要 2 段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "pack") {
        const i8 packed = packDegrees(parseF(parts[1]));
        return json{{"unit", unit}, {"value", static_cast<i32>(packed)}}.dump();
    }
    if (mode == "unpack") {
        const f32 result = unpackDegrees(static_cast<i8>(std::stoi(parts[1])));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }
    if (mode == "round") {
        const f32 result = unpackDegrees(packDegrees(parseF(parts[1])));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }

    throw std::runtime_error("mth_pack_degrees: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_pack_degrees, &writeList, &runLine)

} // namespace mc::diff
