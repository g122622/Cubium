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
 * @brief 差分能力：`Relative` 的位掩码编解码与集合工厂（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.entity.Relative`：
 *
 * ```java
 * public enum Relative {
 *     X(0), Y(1), Z(2),
 *     Y_ROT(3), X_ROT(4),                    // 注意：Y_ROT 在 X_ROT 之前
 *     DELTA_X(5), DELTA_Y(6), DELTA_Z(7),
 *     ROTATE_DELTA(8);
 *
 *     public static final Set<Relative> ALL = Set.of(values());
 *     public static final Set<Relative> ROTATION = Set.of(X_ROT, Y_ROT);
 *     public static final Set<Relative> DELTA = Set.of(DELTA_X, DELTA_Y, DELTA_Z, ROTATE_DELTA);
 *
 *     public static Set<Relative> unpack(int mask) {
 *         Set<Relative> set = EnumSet.noneOf(Relative.class);
 *         for (Relative r : values()) if (r.isSet(mask)) set.add(r);
 *         return set;
 *     }
 *
 *     public static int pack(Set<Relative> set) {
 *         int i = 0;
 *         for (Relative r : set) i |= r.getMask();
 *         return i;
 *     }
 *
 *     public static Set<Relative> rotation(boolean yRot, boolean xRot) {
 *         Set<Relative> set = EnumSet.noneOf(Relative.class);
 *         if (yRot) set.add(Y_ROT);       // 第一个参数是 Y_ROT
 *         if (xRot) set.add(X_ROT);
 *         return set;
 *     }
 *
 *     public static Set<Relative> position(boolean x, boolean y, boolean z) { ... }   // X / Y / Z
 *     public static Set<Relative> direction(boolean x, boolean y, boolean z) { ... }  // DELTA_X/Y/Z
 *     public static Set<Relative> union(Set<Relative>... sets) { ... }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`rotation(boolean yRot, boolean xRot)` 的第一个参数是 `Y_ROT`，不是 `X_ROT`。**
 *
 *    ```java
 *    public static Set<Relative> rotation(boolean yRot, boolean xRot) {
 *        if (yRot) set.add(Y_ROT);
 *        if (xRot) set.add(X_ROT);
 *    }
 *    ```
 *
 *    **这是最容易搞反的一处**——`/tp` 命令的 `~ ~ ~ ~ ~` 语法里顺序是 yaw 在前、pitch 在后，
 *    与这里的参数顺序一致。
 *
 * 2. **枚举序数里 `Y_ROT(3)` 在 `X_ROT(4)` 之前，但位掩码是独立字段。**
 *
 *    | 枚举 | bit | `1 << bit` |
 *    |---|---|---|
 *    | `X` | 0 | 1 |
 *    | `Y` | 1 | 2 |
 *    | `Z` | 2 | 4 |
 *    | **`Y_ROT`** | **3** | **8** |
 *    | **`X_ROT`** | **4** | **16** |
 *    | `DELTA_X` | 5 | 32 |
 *    | `DELTA_Y` | 6 | 64 |
 *    | `DELTA_Z` | 7 | 128 |
 *    | `ROTATE_DELTA` | 8 | 256 |
 *
 *    **`Y_ROT` 的位是 `8`，`X_ROT` 是 `16`**——名字里的 X/Y 与位序相反。
 *
 * 3. **`pack` 遍历集合顺序无关，因为按位或满足交换律。**
 *
 *    `pack(Set.of(X, Y))` 与 `pack(Set.of(Y, X))` 都得到 `3`。
 *
 * 4. **`unpack` 忽略高位。**
 *
 *    `unpack(0x100 | 0x1)` 得 `{X}`——`ROTATE_DELTA` 的位是 8，`0x100 = 256 = 1 << 8`
 *    正好对应 `ROTATE_DELTA`。但超过 bit 8 的位（如 `1 << 20`）**被静默忽略**。
 *
 * 单元格式：
 * - `pack|<名字>[,<名字>…]` —— `pack`；空集合写作 `-`
 * - `unpack|<mask>` —— `unpack`，输出按枚举序数排序的名字列表
 * - `rotation|<yRot>|<xRot>` —— `rotation`
 * - `position|<x>|<y>|<z>` —— `position`
 * - `direction|<x>|<y>|<z>` —— `direction`
 * - `union|<集合A>|<集合B>[|<集合C>…]` —— `union`，集合写作逗号分隔，空集写作 `-`
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

/// 9 个 `Relative` 的名字，按枚举序数排序。
constexpr const char* RELATIVE_NAMES[] = {
    "x", "y", "z", "y_rot", "x_rot", "delta_x", "delta_y", "delta_z", "rotate_delta"};

constexpr i32 RELATIVE_COUNT = 9;

i32 relativeIndex(const std::string& name)
{
    for (i32 i = 0; i < RELATIVE_COUNT; ++i) {
        if (name == RELATIVE_NAMES[i]) {
            return i;
        }
    }
    throw std::runtime_error("relative_mask: 未知 Relative：" + name);
}

/// 渲染一个位掩码为按枚举序数排序的名字列表；空集输出 `-`。
std::string renderMask(i32 mask)
{
    std::string out;
    for (i32 i = 0; i < RELATIVE_COUNT; ++i) {
        if ((mask & (1 << i)) != 0) {
            if (!out.empty()) {
                out.push_back(',');
            }
            out += RELATIVE_NAMES[i];
        }
    }
    return out.empty() ? "-" : out;
}

/// 解析逗号分隔的名字列表为位掩码；`-` 表示空集。
i32 parseMask(const std::string& text)
{
    if (text == "-") {
        return 0;
    }
    i32 mask = 0;
    std::string token;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == ',') {
            if (!token.empty()) {
                mask |= 1 << relativeIndex(token);
                token.clear();
            }
        } else {
            token.push_back(text[i]);
        }
    }
    return mask;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // pack：单个与组合
        "pack|x",
        "pack|y_rot",
        "pack|x_rot",
        "pack|x,y",
        "pack|x,y,z",
        "pack|-",
        "pack|x,y,z,y_rot,x_rot,delta_x,delta_y,delta_z,rotate_delta",
        // unpack：边界
        "unpack|0",
        "unpack|1",
        "unpack|8",
        "unpack|16",
        "unpack|511",
        "unpack|256",
        "unpack|1048576",
        // rotation：参数顺序关键
        "rotation|true|false",
        "rotation|false|true",
        "rotation|true|true",
        "rotation|false|false",
        // position
        "position|true|false|false",
        "position|false|true|false",
        "position|true|true|true",
        "position|false|false|false",
        // direction
        "direction|true|false|false",
        "direction|true|true|true",
        "direction|false|false|false",
        // union
        "union|x|y",
        "union|x,y|-",
        "union|x,y|y,z|z",
        "union|-|-",
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

bool parseBool(const std::string& text)
{
    return text == "true";
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("relative_mask: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "pack" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", parseMask(parts[1])}}.dump();
    }
    if (mode == "unpack" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", renderMask(static_cast<i32>(std::stoll(parts[1])))}}.dump();
    }
    if (mode == "rotation" && parts.size() == 3) {
        // 第一个参数是 yRot（Y_ROT），第二个是 xRot（X_ROT）。
        i32 mask = 0;
        if (parseBool(parts[1])) {
            mask |= 1 << 3;
        }
        if (parseBool(parts[2])) {
            mask |= 1 << 4;
        }
        return json{{"unit", unit}, {"value", renderMask(mask)}}.dump();
    }
    if (mode == "position" && parts.size() == 4) {
        i32 mask = 0;
        if (parseBool(parts[1])) {
            mask |= 1 << 0;
        }
        if (parseBool(parts[2])) {
            mask |= 1 << 1;
        }
        if (parseBool(parts[3])) {
            mask |= 1 << 2;
        }
        return json{{"unit", unit}, {"value", renderMask(mask)}}.dump();
    }
    if (mode == "direction" && parts.size() == 4) {
        i32 mask = 0;
        if (parseBool(parts[1])) {
            mask |= 1 << 5;
        }
        if (parseBool(parts[2])) {
            mask |= 1 << 6;
        }
        if (parseBool(parts[3])) {
            mask |= 1 << 7;
        }
        return json{{"unit", unit}, {"value", renderMask(mask)}}.dump();
    }
    if (mode == "union" && parts.size() >= 3) {
        i32 mask = 0;
        for (std::size_t i = 1; i < parts.size(); ++i) {
            mask |= parseMask(parts[i]);
        }
        return json{{"unit", unit}, {"value", renderMask(mask)}}.dump();
    }

    throw std::runtime_error("relative_mask: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("relative_mask", &writeList, &runLine)

} // namespace mc::diff
