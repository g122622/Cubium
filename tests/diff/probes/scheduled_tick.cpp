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
 * @brief 差分能力：`ScheduledTick` 的排序与去重（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版：
 *
 * ```java
 * public record ScheduledTick<T>(T type, BlockPos pos, long triggerTick,
 *                                TickPriority priority, long subTickOrder) {
 *
 *     public static final Comparator<ScheduledTick<?>> DRAIN_ORDER = (a, b) -> {
 *         int i = Long.compare(a.triggerTick, b.triggerTick);
 *         if (i != 0) { return i; }
 *         i = a.priority.compareTo(b.priority);
 *         return i != 0 ? i : Long.compare(a.subTickOrder, b.subTickOrder);
 *     };
 *
 *     public static final Comparator<ScheduledTick<?>> INTRA_TICK_DRAIN_ORDER = (a, b) -> {
 *         int i = a.priority.compareTo(b.priority);
 *         return i != 0 ? i : Long.compare(a.subTickOrder, b.subTickOrder);
 *     };
 *
 *     public static final Strategy<ScheduledTick<?>> UNIQUE_TICK_HASH = new Strategy<>() {
 *         public boolean equals(ScheduledTick<?> a, ScheduledTick<?> b) {
 *             ...
 *             return a.type() == b.type() && a.pos().equals(b.pos());
 *         }
 *     };
 * }
 *
 * // TickPriority
 * EXTREMELY_HIGH(-3), VERY_HIGH(-2), HIGH(-1), NORMAL(0), LOW(1), VERY_LOW(2), EXTREMELY_LOW(3);
 * ```
 *
 * ## 四个关键点
 *
 * 1. **两个比较器的主键完全不同。** `DRAIN_ORDER` 先比 `triggerTick`（跨 tick 排序），
 *    `INTRA_TICK_DRAIN_ORDER` **完全不看 `triggerTick`**，直接比 `priority`。后者用于「同一
 *    tick 内」的容器排序，此时 `triggerTick` 必然相同，省去一次比较。
 *
 * 2. **`TickPriority` 的枚举声明顺序就是排序顺序**，且声明值恰好从 `-3` 递增到 `3` 与优先级
 *    一致——`EXTREMELY_HIGH` 排最前。若按「名字里 HIGH 应该排在后面」的直觉实现会全盘反转。
 *
 * 3. **去重只认 `type` 与 `pos`，完全忽略时间与优先级。** `UNIQUE_TICK_HASH.equals` 的实现是
 *    `a.type() == b.type() && a.pos().equals(b.pos())`。所以同一位置同一类型的两次调度**永远
 *    视为重复**——即使触发时间不同。这是「同一方块同一 tick 类型只会被执行一次」的保证。
 *
 * 4. **`hashCode` 用 `31 * pos.hashCode() + type.hashCode()`。** 这个乘数 31 只影响散列分布，
 *    不影响去重语义，但重新实现时必须与 `equals` 一致。
 *
 * 单元格式：`<模式>|<参数…>`
 * - `drain|<t1>|<p1>|<s1>|<t2>|<p2>|<s2>` —— 输出 `-1/0/1`
 * - `intra|<p1>|<s1>|<p2>|<s2>` —— 输出 `-1/0/1`
 * - `unique|<t1>|<pos1>|<sy1>|<t2>|<pos2>|<sy2>` —— 输出 `same` / `diff`
 * - `sort|<项>[,<项>…]` —— 按 `DRAIN_ORDER` 排序，项格式 `<tick>/<priority>/<sub>`
 *
 * `priority` 用枚举名（`EXTREMELY_HIGH` 等）。
 *
 * TODO: 未覆盖 `LevelTicks` 的容量上限与 `LevelChunkTicks` 的容器结构、`SavedTick` 的相对
 * 时间换算。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `TickPriority` 的枚举声明顺序（也是排序顺序）。
constexpr const char* PRIORITY_NAMES[] = {
    "EXTREMELY_HIGH", "VERY_HIGH", "HIGH", "NORMAL", "LOW", "VERY_LOW", "EXTREMELY_LOW"};

/// 原版 `TickPriority` 的声明值。
constexpr i32 PRIORITY_VALUES[] = {-3, -2, -1, 0, 1, 2, 3};

/// 取优先级的序号（即 `compareTo` 的比较键）。
i32 priorityOrdinal(const std::string& name)
{
    for (i32 i = 0; i < 7; ++i) {
        if (name == PRIORITY_NAMES[i]) {
            return i;
        }
    }
    throw std::runtime_error("scheduled_tick: 未知优先级：" + name);
}

/// 一条调度项。
struct Tick {
    i64 triggerTick = 0;
    i32 priority = 0; // 序数
    i64 subTickOrder = 0;
    std::string type;
    i64 pos = 0;
};

/// 原版 `ScheduledTick.DRAIN_ORDER`。
i32 drainCompare(const Tick& a, const Tick& b)
{
    if (a.triggerTick != b.triggerTick) {
        return a.triggerTick < b.triggerTick ? -1 : 1;
    }
    if (a.priority != b.priority) {
        return a.priority < b.priority ? -1 : 1;
    }
    if (a.subTickOrder != b.subTickOrder) {
        return a.subTickOrder < b.subTickOrder ? -1 : 1;
    }
    return 0;
}

/// 原版 `ScheduledTick.INTRA_TICK_DRAIN_ORDER`（不看 triggerTick）。
i32 intraCompare(const Tick& a, const Tick& b)
{
    if (a.priority != b.priority) {
        return a.priority < b.priority ? -1 : 1;
    }
    if (a.subTickOrder != b.subTickOrder) {
        return a.subTickOrder < b.subTickOrder ? -1 : 1;
    }
    return 0;
}

/// 原版 `UNIQUE_TICK_HASH.equals`：只比较 type 与 pos。
bool uniqueEquals(const Tick& a, const Tick& b)
{
    return a.type == b.type && a.pos == b.pos;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // drain：tick 不同
    "drain|10|NORMAL|0|20|NORMAL|0",
    "drain|20|NORMAL|0|10|NORMAL|0",
    // drain：tick 相同，优先级不同
    "drain|10|HIGH|0|10|NORMAL|0",
    "drain|10|NORMAL|0|10|HIGH|0",
    "drain|10|EXTREMELY_HIGH|0|10|EXTREMELY_LOW|0",
    "drain|10|EXTREMELY_LOW|0|10|EXTREMELY_HIGH|0",
    // drain：tick 与优先级相同，看 subTickOrder
    "drain|10|NORMAL|5|10|NORMAL|3",
    "drain|10|NORMAL|3|10|NORMAL|5",
    "drain|10|NORMAL|0|10|NORMAL|0",
    // 所有优先级两两比较（验证声明顺序即排序顺序）
    "drain|0|VERY_HIGH|0|0|HIGH|0",
    "drain|0|HIGH|0|0|NORMAL|0",
    "drain|0|NORMAL|0|0|LOW|0",
    "drain|0|LOW|0|0|VERY_LOW|0",
    "drain|0|VERY_LOW|0|0|EXTREMELY_LOW|0",
    // intra：忽略 triggerTick
    "intra|NORMAL|0|NORMAL|5",
    "intra|NORMAL|5|NORMAL|0",
    "intra|HIGH|0|NORMAL|0",
    "intra|NORMAL|0|HIGH|0",
    "intra|EXTREMELY_HIGH|9|EXTREMELY_LOW|0",
    "intra|NORMAL|3|NORMAL|3",
    // unique：type 与 pos 都相同
    "unique|fnv|1|fnv|1",
    "unique|fnv|1|fnv|2",
    "unique|fnv|1|fluid|1",
    "unique|fnv|1|fluid|2",
    "unique|block|9|block|9",
    "unique|fluid|5|fluid|5",
    // 排序：混合
    "sort|10/NORMAL/0,5/HIGH/2,5/EXTREMELY_HIGH/5,5/NORMAL/1",
    "sort|0/NORMAL/3,0/NORMAL/1,0/NORMAL/2",
    "sort|100/LOW/0,50/VERY_HIGH/9,75/NORMAL/4",
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

std::vector<std::string> splitBy(const std::string& text, char separator)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == separator) {
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
        throw std::runtime_error("scheduled_tick: 单元至少两段：" + unit);
    }

    if (parts[0] == "drain" && parts.size() == 7) {
        Tick a{std::stoll(parts[1]), priorityOrdinal(parts[2]), std::stoll(parts[3]), "t", 0};
        Tick b{std::stoll(parts[4]), priorityOrdinal(parts[5]), std::stoll(parts[6]), "t", 0};
        return json{{"unit", unit}, {"value", drainCompare(a, b)}}.dump();
    }

    if (parts[0] == "intra" && parts.size() == 5) {
        Tick a{0, priorityOrdinal(parts[1]), std::stoll(parts[2]), "t", 0};
        Tick b{0, priorityOrdinal(parts[3]), std::stoll(parts[4]), "t", 0};
        return json{{"unit", unit}, {"value", intraCompare(a, b)}}.dump();
    }

    if (parts[0] == "unique" && parts.size() == 5) {
        // 原版 `UNIQUE_TICK_HASH.equals` 用 `==` 比较 type（引用比较），type 是同一对象即同类型。
        // 这里用名字字符串代表 type 的身份，内容相同即同类型。
        Tick a{0, 3, 0, parts[1], std::stoll(parts[2])};
        Tick b{0, 3, 0, parts[3], std::stoll(parts[4])};
        return json{{"unit", unit}, {"value", uniqueEquals(a, b) ? "same" : "diff"}}.dump();
    }

    if (parts[0] == "sort" && parts.size() == 2) {
        std::vector<Tick> ticks;
        for (const auto& item : splitBy(parts[1], ',')) {
            const auto fields = splitBy(item, '/');
            if (fields.size() != 3) {
                throw std::runtime_error("scheduled_tick: 排序项应为 tick/priority/sub：" + item);
            }
            ticks.push_back(Tick{std::stoll(fields[0]), priorityOrdinal(fields[1]), std::stoll(fields[2]), "t", 0});
        }
        std::stable_sort(ticks.begin(), ticks.end(), [](const Tick& a, const Tick& b) {
            return drainCompare(a, b) < 0;
        });

        std::string order;
        for (const auto& tick : ticks) {
            if (!order.empty()) {
                order += ",";
            }
            order += std::to_string(tick.triggerTick) + "/" + PRIORITY_NAMES[tick.priority] + "/" +
                std::to_string(tick.subTickOrder);
        }
        return json{{"unit", unit}, {"value", order}}.dump();
    }

    throw std::runtime_error("scheduled_tick: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(scheduled_tick, &writeList, &runLine)

} // namespace mc::diff
