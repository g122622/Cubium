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

#include "probe.hpp"

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @file fall_damage.cpp
 * @brief 摔落伤害的数值公式（原版 `LivingEntity.calculateFallDamage` / `calculateFallPower`）。
 *
 * 公式是 `floor((distance + 1e-6 - SAFE_FALL_DISTANCE) * multiplier * FALL_DAMAGE_MULTIPLIER)`，
 * 再减去子类减免。三个地方最容易写错：
 *
 *   - **`+1e-6` 的用意**：把「恰好等于安全距离」推到安全距离之上一点点，让 floor 取到 0，
 *     从而 3.0 格摔落不掉血、3.0001 格才掉 1 点。少写这一项会让恰好 4.0 格也掉 1 点。
 *   - **运算是 double**：伤害最终按整数格结算，中途落到 f32 会让 1e-6 被舍入吃掉，
 *     整数边界附近整格偏差。
 *   - **先 floor 再减法**：子类减免（青蛙 5、山羊 10）作用在**取整之后**的伤害上。
 *     先折算距离再取整，结果会因为 floor 不可逆而不同。
 *
 * `multiplier` 由落地方块给出：普通 1.0、干草块/蜂蜜块 0.2、石笋 2.0、史莱姆块 0。
 *
 * 单元格式：`<distance>|<multiplier>`，距离与倍率都是十进制字面量。
 */


/// 把距离与倍率拼成单元字符串。倍率保留原始十进制写法，避免与 oracle 侧不一致。
std::string formatUnit(f64 distance, f32 multiplier)
{
    char distanceText[64] = {};
    char multiplierText[64] = {};
    std::snprintf(distanceText, sizeof(distanceText), "%.6f", distance);
    std::snprintf(multiplierText, sizeof(multiplierText), "%.3f", static_cast<f64>(multiplier));
    return std::string(distanceText) + "|" + multiplierText;
}

const std::vector<std::string>& unitStrings()
{
    static const std::vector<std::string> table = [] {
        std::vector<std::string> list;

        // 安全距离与 0 附近的边界：验证 +1e-6 的取舍
        const f64 nearZero[] = {0.0, 0.5, 1.0, 2.0, 2.999999, 3.0, 3.000001, 3.5, 4.0, 4.000001, 5.0};
        for (const f64 distance : nearZero) {
            list.push_back(formatUnit(distance, 1.0f));
        }

        // 整数格与半格，覆盖 floor 的取舍
        for (i32 whole = 1; whole <= 25; ++whole) {
            list.push_back(formatUnit(static_cast<f64>(whole), 1.0f));
            list.push_back(formatUnit(static_cast<f64>(whole) + 0.5, 1.0f));
            list.push_back(formatUnit(static_cast<f64>(whole) + 0.999, 1.0f));
        }

        // 各种落地方块倍率：干草块/蜂蜜块 0.2、石笋 2.0、史莱姆块 0
        for (const f32 multiplier : {0.0f, 0.2f, 0.5f, 1.0f, 2.0f}) {
            for (const f64 distance : {3.0, 4.0, 5.0, 8.0, 12.5, 20.0, 40.0}) {
                list.push_back(formatUnit(distance, multiplier));
            }
        }

        return list;
    }();
    return table;
}

void writeList(std::FILE* out)
{
    for (const auto& unit : unitStrings()) {
        std::fprintf(out, "%s\n", unit.c_str());
    }
}


std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const size_t bar = requested.find('|');
    if (bar == std::string::npos) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    const f64 distance = std::stod(requested.substr(0, bar));
    const f32 multiplier = std::stof(requested.substr(bar + 1));

    // 每行都重新构造实体，避免属性修改器在用例之间残留。
    ecs::EntityRegistry registry("diff_fall_damage");
    Player player(EntityInstanceId{1}, "differ", registry);

    const i32 damage = player.calculateFallDamage(distance, multiplier);
    return json{{"unit", requested}, {"damage", damage}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(fall_damage, &writeList, &runLine)

} // namespace mc::diff