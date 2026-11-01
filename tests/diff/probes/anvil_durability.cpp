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
 * @brief 差分能力：铁砧合并同物品时的耐久叠加（纯计算）。
 *
 * `list`：一组「主物品最大耐久|主物品损伤|副物品最大耐久|副物品损伤」
 * `run` ：同上一行
 *         → { "newDamage": 120, "costAdded": 2, "repaired": true }
 *
 * 复刻原版 `AnvilMenu.createResult` 里合并耐久那一段：
 *
 * ```java
 * int l  = itemstack.getMaxDamage()   - itemstack.getDamageValue();   // 主物品剩余耐久
 * int i1 = itemstack2.getMaxDamage()  - itemstack2.getDamageValue();  // 副物品剩余耐久
 * int j1 = i1 + itemstack1.getMaxDamage() * 12 / 100;                 // 副耐久 + 上限的 12%
 * int k1 = l + j1;
 * int l1 = itemstack1.getMaxDamage() - k1;                            // 新损伤值
 * if (l1 < 0) { l1 = 0; }
 * if (l1 < itemstack1.getDamageValue()) {
 *     itemstack1.setDamageValue(l1);
 *     i += 2;                                                         // 代价 +2
 * }
 * ```
 *
 * ## 四个容易写错的点
 *
 * 1. **`* 12 / 100` 的顺序**。必须先乘后除。先除后乘（`maxDamage / 100 * 12`）对
 *    大多数耐久值结果不同——整数除法会先丢掉低位。
 *
 * 2. **「有改善」才计入**。`l1 < itemstack1.getDamageValue()` 这个判断意味着：若合并
 *    反而更糟（副物品几乎报废、12% 加成不足以补偿），损伤值**不会被改动**，代价也
 *    **不会 +2**。合并本身仍可能发生（附魔照合并），但耐久这一项不生效。
 *
 * 3. **新损伤值钳到 0**。`l1 < 0` 时取 0，而不是取负（负耐久会变成无敌）。
 *
 * 4. **`maxDamage * 12` 用 int 相乘**。原版这里是 `int` 运算，`maxDamage` 极大时
 *    （超过约 1.79 亿）会溢出。Cubium 的 `Cubium` 侧同为 `i32`，两侧应当一致；
 *    本能力的上限用例专门覆盖这一点。
 *
 * 本能力只看耐久这一层，不涉及附魔合并与重命名。
 *
 * TODO: 本能力未覆盖「材料修复」（`isValidRepairItem`）那条分支，它的伤害削减是
 * `min(damage, maxDamage / 4)` 逐材料循环，需要材料数量作为输入，属另一层。
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

/// 合并时副物品额外贡献的耐久占主物品上限的百分比。
constexpr i32 DURABILITY_BONUS_PERCENT = 12;

/// 合并成功时增加的经验代价。
constexpr i32 MERGE_COST = 2;

struct MergeResult {
    i32 newDamage = 0;
    i32 costAdded = 0;
    bool repaired = false;
};

/**
 * @brief 复刻 createResult 里合并同物品耐久那一段。
 *
 * @param primaryMax   主物品的最大耐久
 * @param primaryDamage 主物品当前的损伤值
 * @param secondaryMax 副物品的最大耐久
 * @param secondaryDamage 副物品当前的损伤值
 */
MergeResult mergeDurability(i32 primaryMax, i32 primaryDamage, i32 secondaryMax, i32 secondaryDamage)
{
    const i32 primaryDurability = primaryMax - primaryDamage;
    const i32 secondaryDurability = secondaryMax - secondaryDamage;

    // 必须先乘后除：先除后乘会先丢掉低位，结果不同。
    const i32 bonus = secondaryDurability + primaryMax * DURABILITY_BONUS_PERCENT / 100;

    const i32 totalDurability = primaryDurability + bonus;
    i32 newDamage = primaryMax - totalDurability;

    // 新损伤值钳到 0，不能取负。
    if (newDamage < 0) {
        newDamage = 0;
    }

    MergeResult result;
    result.newDamage = primaryDamage;
    result.costAdded = 0;
    result.repaired = false;

    // 只有确实改善了才计入。
    if (newDamage < primaryDamage) {
        result.newDamage = newDamage;
        result.costAdded = MERGE_COST;
        result.repaired = true;
    }
    return result;
}

/**
 * @brief 单元表：`primaryMax|primaryDamage|secondaryMax|secondaryDamage`。
 *
 * 覆盖：
 * - 常规合并（两件都有损伤）；
 * - 主物品满耐久（合并无改善，代价不 +2）；
 * - 副物品接近报废（12% 加成不足以补偿，可能无改善）；
 * - 两件都报废；
 * - `* 12 / 100` 的顺序敏感值（如 maxDamage = 100 附近）；
 * - 大耐久值（钻石剑 1561、下界合金剑 2031、鞘翅 432）；
 * - 超过约 1.79 亿的溢出边界。
 */
constexpr const char* UNITS[] = {
    // 常规：钻石剑两件都半损
    "1561|800|1561|800",
    // 主物品满耐久 -> 无改善
    "1561|0|1561|800",
    // 主物品几乎报废、副物品全新
    "1561|1500|1561|0",
    // 两件都报废（耐久为 1）
    "1561|1560|1561|1560",
    // 副物品全损（耐久 0）
    "1561|800|1561|1561",
    // 小耐久值，顺序敏感：100 的 12% = 12
    "100|50|100|50",
    // 小耐久值：99 的 12% = 11（先除后乘会得 0）
    "99|50|99|50",
    // 极小耐久：10 的 12% = 1
    "10|5|10|5",
    // 耐久 1：12% = 0
    "1|0|1|0",
    // 顺序敏感且不会钳到 0：正确得 newDamage=1152，先除后乘得 1159
    "1561|1500|1561|1400",
    // 下界合金剑
    "2031|1000|2031|1000",
    // 鞘翅
    "432|200|432|200",
    // 三叉戟
    "250|100|250|100",
    // 大耐久值：接近 int 溢出边界 (2147483647 / 12 ≈ 178956970)
    "178956970|1000|178956970|1000",
    // 刚好超过溢出边界
    "178956971|1000|178956971|1000",
    // 上限本身
    "2000000000|1000|2000000000|1000",
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
        throw std::runtime_error(
            "anvil_durability: 单元应为 4 段 'primaryMax|primaryDamage|secondaryMax|secondaryDamage'：" + unit);
    }

    const i32 primaryMax = std::stoi(parts[0]);
    const i32 primaryDamage = std::stoi(parts[1]);
    const i32 secondaryMax = std::stoi(parts[2]);
    const i32 secondaryDamage = std::stoi(parts[3]);

    const MergeResult result = mergeDurability(primaryMax, primaryDamage, secondaryMax, secondaryDamage);

    return json{{"unit", unit},
        {"primaryMax", primaryMax},
        {"primaryDamage", primaryDamage},
        {"secondaryMax", secondaryMax},
        {"secondaryDamage", secondaryDamage},
        {"newDamage", result.newDamage},
        {"costAdded", result.costAdded},
        {"repaired", result.repaired}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("anvil_durability", &writeList, &runLine)

} // namespace mc::diff
