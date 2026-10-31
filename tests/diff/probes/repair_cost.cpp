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
 * @brief 差分能力：铁砧的修复代价累积（纯计算）。
 *
 * `list`：一组「基础代价|材料代价|修复材料数|经验代价」
 * `run` ：同上一行
 *         → { "baseCost": 3, "nextCost": 7, "increased": true }
 *
 * 复刻原版 `AnvilMenu.createResult` 里修复代价那一段：
 *
 * ```java
 * int i3 = itemstack1.getOrDefault(DataComponents.REPAIR_COST, 0);
 * if (i3 < itemstack2.getOrDefault(DataComponents.REPAIR_COST, 0)) {
 *     i3 = itemstack2.getOrDefault(DataComponents.REPAIR_COST, 0);
 * }
 * if (k != i || k == 0) {
 *     i3 = calculateIncreasedRepairCost(i3);
 * }
 * itemstack1.set(DataComponents.REPAIR_COST, i3);
 *
 * public static int calculateIncreasedRepairCost(int cost) {
 *     return (int) Math.min(cost * 2L + 1L, 2147483647L);
 * }
 * ```
 *
 * 其中 `i` 是已消耗的修复材料数（`repairItemCountCost`），`k` 是经验代价。
 *
 * ## 三个反直觉的点
 *
 * 1. **取两者中较大的代价**，不是相加。主物品与副物品各自的 REPAIR_COST 只取 max。
 *
 * 2. **`k != i || k == 0` 这个条件会「跳过增长」**。当经验代价恰好等于消耗的材料数时，
 *    `k != i` 为假；若此时 `k` 也不是 0，整条判断为假，代价**不翻倍**。例如
 *    「消耗 5 个材料、经验代价恰为 5」的情形不会推高代价——这是一个几乎无法从规则
 *    描述中推出来的边界。
 *
 * 3. **翻倍用 long，结果截断到 int 上限**。`cost * 2L + 1L` 是 64 位运算，最后
 *    `Math.min(..., Integer.MAX_VALUE)` 再收窄。用 32 位直接算会在
 *    `cost > 1073741823` 时溢出成负数。
 *
 * 本能力只看代价这一层，不涉及附魔合并与重命名（那些与物品、附魔表耦合，属另一层）。
 *
 * TODO: 本能力未覆盖 `AnvilMenu.createResult` 的完整链路（材料修复的伤害削减、
 * 同物品合并、附魔合并、重命名、`cost >= 40` 的封禁）。待 Cubium 实现铁砧菜单后，
 * 应扩展为覆盖整条链路。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `calculateIncreasedRepairCost` 的截断上限。
constexpr i32 REPAIR_COST_CAP = 2147483647;

/**
 * @brief 原版 `AnvilMenu.calculateIncreasedRepairCost`。
 *
 * 用 64 位中间值：原版是 `cost * 2L + 1L` 再 `Math.min(..., Integer.MAX_VALUE)`。
 */
i32 calculateIncreasedRepairCost(i32 cost)
{
    const i64 doubled = static_cast<i64>(cost) * 2 + 1;
    return static_cast<i32>(std::min<i64>(doubled, REPAIR_COST_CAP));
}

/// 结果结构。
struct RepairCostResult {
    i32 nextCost = 0;
    bool increased = false;
};

/**
 * @brief 复刻 createResult 里代价累积那一段。
 *
 * @param primaryCost  主物品的 REPAIR_COST
 * @param secondaryCost 副物品的 REPAIR_COST
 * @param repairCount  已消耗的修复材料数（原版变量 `i`）
 * @param xpCost       经验代价（原版变量 `k`）
 */
RepairCostResult applyRepairCost(i32 primaryCost, i32 secondaryCost, i32 repairCount, i32 xpCost)
{
    // 取两者中较大的代价。
    i32 cost = primaryCost;
    if (cost < secondaryCost) {
        cost = secondaryCost;
    }

    // 反直觉的条件：经验代价恰等于材料数且非零时，不推高代价。
    const bool shouldIncrease = (xpCost != repairCount) || (xpCost == 0);

    RepairCostResult result;
    result.nextCost = shouldIncrease ? calculateIncreasedRepairCost(cost) : cost;
    result.increased = shouldIncrease;
    return result;
}

/**
 * @brief 单元表：`primaryCost|secondaryCost|repairCount|xpCost`。
 *
 * 覆盖：
 * - 常规路径（经验代价 != 材料数，代价翻倍）；
 * - **经验代价 == 材料数且非零**（跳过增长）——最关键的一支；
 * - 经验代价 == 材料数 == 0（此时仍增长，因为第二项 `k == 0` 为真）；
 * - 两侧代价取 max 的两个方向；
 * - 接近 int 上限时的截断（用 32 位算会溢出成负）。
 */
constexpr const char* UNITS[] = {
    // 常规：0 -> 1
    "0|0|0|1",
    // 常规：3 -> 7
    "3|0|2|5",
    // 经验代价恰等于材料数（2 == 2）-> 不增长
    "3|0|2|2",
    // 经验代价 == 材料数 == 0 -> 仍增长（k == 0 分支）
    "0|0|0|0",
    // 经验代价 == 材料数 == 5 -> 不增长
    "10|0|5|5",
    // 副物品代价更大 -> 取副
    "3|9|1|4",
    // 主物品代价更大 -> 取主
    "9|3|1|4",
    // 两侧相等
    "5|5|1|4",
    // 材料数 0、经验代价非 0 -> 增长
    "4|0|0|7",
    // 接近上限：1073741823 -> 截断到 INT_MAX
    "1073741823|0|1|9",
    // 已在上限：继续截断
    "2147483647|0|1|9",
    // 上限且跳过增长 -> 原样返回
    "2147483647|0|4|4",
    // 较大材料数与经验代价相等
    "100|0|20|20",
    // 较大材料数，经验代价不等
    "100|0|20|21",
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
    if (parts.size() != 4) {
        throw std::runtime_error("repair_cost: 单元应为 4 段 'primaryCost|secondaryCost|repairCount|xpCost'：" + unit);
    }

    const i32 primaryCost = std::stoi(parts[0]);
    const i32 secondaryCost = std::stoi(parts[1]);
    const i32 repairCount = std::stoi(parts[2]);
    const i32 xpCost = std::stoi(parts[3]);

    const RepairCostResult result = applyRepairCost(primaryCost, secondaryCost, repairCount, xpCost);

    return json{{"unit", unit},
        {"primaryCost", primaryCost},
        {"secondaryCost", secondaryCost},
        {"repairCount", repairCount},
        {"xpCost", xpCost},
        {"nextCost", result.nextCost},
        {"increased", result.increased}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("repair_cost", &writeList, &runLine)

} // namespace mc::diff
