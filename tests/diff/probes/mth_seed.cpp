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
 * @brief 差分能力：`Mth.getSeed`（方块坐标 → 确定性种子，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "<long>" }
 *
 * 复刻原版 `net.minecraft.util.Mth`（已标记 `@Deprecated`，但仍被大量结构生成调用）：
 *
 * ```java
 * @Deprecated
 * public static long getSeed(int x, int y, int z) {
 *     long i = x * 3129871 ^ z * 116129781L ^ y;
 *     i = i * i * 42317861L + i * 11L;
 *     return i >> 16;                       // 算术右移
 * }
 *
 * public static long getSeed(Vec3i pos) {
 *     return getSeed(pos.getX(), pos.getY(), pos.getZ());
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`x * 3129871` 是 `int` 乘法，会溢出；而 `z * 116129781L` 是 `long` 乘法。**
 *
 *    ```java
 *    long i = x * 3129871 ^ z * 116129781L ^ y;
 *    ```
 *
 *    注意字面量：`3129871`（无 `L` 后缀）是 `int`，`116129781L`（有 `L`）是 `long`。
 *
 *    - `x * 3129871` 两个 `int` 相乘，**结果按 32 位回绕**后再按符号扩展到 `long`
 *    - `z * 116129781L` 因为右侧是 `long`，`z` 被提升为 `long`，**结果是 64 位**
 *
 *    这个不对称是 Java 数值提升规则的结果，**不是笔误**。若 C++ 实现把
 *    `x * 3129871` 写成 `static_cast<i64>(x) * 3129871`，就会得到完全不同的种子。
 *
 * 2. **`y` 在异或链的第三位（最后），且不乘任何系数。**
 *
 *    `x ... ^ z ... ^ y`——`y` 只做裸异或。这意味着**交换 `x` 与 `y` 的赋值会改变结果**，
 *    而 `y` 的位分布直接影响低位。
 *
 * 3. **`i = i * i * 42317861L + i * 11L`——先用 `i` 乘自己，再混入 `i * 11`。**
 *
 *    展开是 `42317861·i² + 11·i`。两项都基于**同一个原始 `i`**（第二步的 `i * 11L` 用的是
 *    赋值前的 `i`，因为 Java 的 `=` 右侧整体求值后才写入）。若写成 `i = i*i; i = i*42317861 + i*11;`
 *    就会**用平方后的 `i` 再乘 11**，结果完全不同。
 *
 * 4. **返回 `i >> 16` 是算术右移，保留符号。**
 *
 *    不是 `>>>`。因此**结果可以为负**，且低 16 位被丢弃。
 *
 *    这也是 `getSeed` 输出值域比 `long` 窄的原因——实际只有 48 位有效。
 *
 * 单元格式：
 * - `seed|<x>|<y>|<z>` —— `getSeed(int, int, int)`
 * - `vec|<x>|<y>|<z>` —— `getSeed(Vec3i)`（应与 `seed` 完全一致）
 *
 * 输出是**有符号 64 位整数**，按**字符串**给出（避免 JSON 数字精度问题）。
 *
 * 已验证的对称性事实：
 * - `getSeed(0, 0, 0)` = `0`（全零输入是哈希的不动点）
 * - `getSeed(x, y, z)` ≠ `getSeed(z, y, x)`（`x` 与 `z` 的系数不同）
 *
 * TODO: 未覆盖 `Mth.createInsecureUUID`（需要 `RandomSource`）、`Mth.randomBetweenInclusive`、
 * 以及 `getSeed` 在各结构生成（`StructureStart`）中的实际调用点（属 world 型）。
 */
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

/// 原版 `Mth.getSeed(int, int, int)`：注意 `x*3129871` 是 int 乘法溢出。
i64 getSeed(i32 x, i32 y, i32 z)
{
    // x * 3129871 为 int 乘法（32 位回绕），再按符号扩展为 long。
    const i64 xTerm = static_cast<i64>(static_cast<i32>(static_cast<u32>(x) * 3129871U));
    // z * 116129781L 为 long 乘法（z 提升为 long）。
    const i64 zTerm = static_cast<i64>(z) * 116129781LL;
    i64 i = xTerm ^ zTerm ^ static_cast<i64>(y);
    i = i * i * 42317861LL + i * 11LL;
    return i >> 16;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // seed：原点与轴向单位
        "seed|0|0|0",
        "seed|1|0|0",
        "seed|0|1|0",
        "seed|0|0|1",
        "seed|-1|0|0",
        "seed|0|-1|0",
        "seed|0|0|-1",
        // seed：交换 x 与 z 应不相等（系数不同）
        "seed|5|3|7",
        "seed|7|3|5",
        // seed：交换 y 与 z（y 只做裸异或）
        "seed|5|7|3",
        "seed|5|3|7",
        // seed：坐标互换（y 位置特殊）
        "seed|100|200|300",
        "seed|300|200|100",
        "seed|200|100|300",
        // seed：int 溢出边界（x * 3129871 会溢出）
        "seed|2147483647|0|0",
        "seed|-2147483648|0|0",
        "seed|1000|0|0",
        // seed：实际常见的结构坐标
        "seed|0|64|0",
        "seed|128|-64|-256",
        "seed|-16|70|32",
        // vec：Vec3i 重载应与 seed 一致
        "vec|5|3|7",
        "vec|0|0|0",
        "vec|128|-64|-256",
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
    if (parts.size() != 4) {
        throw std::runtime_error("mth_seed: 单元需要 4 段：" + unit);
    }

    const std::string mode = parts[0];
    if (mode != "seed" && mode != "vec") {
        throw std::runtime_error("mth_seed: 无法识别的单元：" + unit);
    }

    const i64 result = getSeed(static_cast<i32>(std::stoll(parts[1])),
        static_cast<i32>(std::stoll(parts[2])),
        static_cast<i32>(std::stoll(parts[3])));
    return json{{"unit", unit}, {"value", std::to_string(result)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_seed", &writeList, &runLine)

} // namespace mc::diff
