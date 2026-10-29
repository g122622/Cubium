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
 * @file damage_reduction.cpp
 * @brief 差分能力：护甲减伤公式（含破甲附魔）。
 *
 * `list`：全部组合，每行一个 `{"damage":..,"armor":..,"toughness":..}`
 * `run` ：输入行就是上面那种组合对象
 *         → 该组合的最终伤害（含破甲 1..4 级）
 * 单元集合由 Java oracle 枚举（与 mob_category 相同：只有一侧负责「挑哪些样本」），
 * 探针的 list 实现只用于人工排查，不参与比对。
 *
 * 这两条公式是服务器最核心的数值逻辑之一，却完全由浮点常量与 clamp 区间拼成：
 * 系数写错一位、clamp 下限写成 0.2*armor 而不是 armor*0.2、破甲的修正加到错的量
 * 上，都只会让血量掉得比原版多一点或少一点，任何行为测试都测不出来。
 *
 * 单元取自「伤害 × 护甲 × 韧性」的乘积网格（护甲只取整数，韧性含 0/2/3/4 等常见值），
 * 覆盖 clamp 生效与不生效两段。
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "common/entity/combat/CombatRules.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

constexpr f32 kDamages[] = {0.0f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f, 12.0f, 15.0f, 20.0f, 30.0f, 50.0f};

/// 护甲取值：0 到 30（原版上限）逐点取值。
constexpr f32 kArmors[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
    9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 24.0f, 30.0f};

/// 韧性取值：0（无护甲）、2（钻石全套）、3（下界合金全套）、4/8/20（附魔与极限情况）。
constexpr f32 kToughnesses[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 8.0f, 20.0f};

/// 单元键的数值写法：两侧必须逐字符一致（oracle 用 Java 的 float 打印，如 "0.0"/"2.5"），
/// 因此这里不能走 std::to_string（六位小数）。
std::string numberText(f32 value)
{
    std::string text = fmt::format("{}", value);
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos) {
        text += ".0";
    }
    return text;
}

std::string unitText(f32 damage, f32 armor, f32 toughness)
{
    return numberText(damage) + "/" + numberText(armor) + "/" + numberText(toughness);
}

void writeList(FILE* out)
{
    // 单元集合由 Java oracle 枚举（它负责「挑哪些样本」），探针的 list 只供人工排查。
    // 网格取「伤害 × 护甲 × 韧性」的乘积，覆盖 clamp 生效与不生效两段。
    for (f32 damage : kDamages) {
        for (f32 armor : kArmors) {
            for (f32 toughness : kToughnesses) {
                const std::string line = json{{"damage", damage}, {"armor", armor}, {"toughness", toughness}}.dump();
                std::fprintf(out, "%s\n", line.c_str());
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json unit = json::parse(std::string(inputLine));
    const f32 damage = unit.at("damage").get<f32>();
    const f32 armor = unit.at("armor").get<f32>();
    const f32 toughness = unit.at("toughness").get<f32>();

    // 破甲附魔每级把「护甲有效率」压低 0.15（对齐 vanilla Breach），0 级即普通护甲减伤。
    // 比对键与 oracle 同构："damage/armor/toughness"。
    json result{{"unit", unitText(damage, armor, toughness)},
        {"damage", damage},
        {"armor", armor},
        {"toughness", toughness}};
    result["absorb"] = entity::combat::CombatRules::getDamageAfterAbsorb(damage, armor, toughness);
    for (i32 breachLevel = 1; breachLevel <= 4; ++breachLevel) {
        result["absorbBreach" + std::to_string(breachLevel)] =
            entity::combat::CombatRules::getDamageAfterAbsorb(damage, armor, toughness, breachLevel);
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("damage_reduction", &writeList, &runLine)

} // namespace mc::diff