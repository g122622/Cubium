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
 * @brief 差分能力：漏斗的传输冷却时序（纯计算）。
 *
 * `list`：一组「初始冷却|是否启用|是否有物品可动」
 * `run` ：同上一行
 *         → { "cooldownAfter": 8, "attempted": true, "moved": true, "cooldownSet": true }
 *
 * 复刻原版 `HopperBlockEntity.pushItemsTick` 与 `tryMoveItems`：
 *
 * ```java
 * public static void pushItemsTick(...) {
 *     hopper.cooldownTime--;                        // 无条件递减
 *     hopper.tickedGameTime = level.getGameTime();
 *     if (!hopper.isOnCooldown()) {
 *         hopper.setCooldown(0);
 *         tryMoveItems(level, pos, state, hopper, () -> suckInItems(level, hopper));
 *     }
 * }
 *
 * private static boolean tryMoveItems(...) {
 *     if (level.isClientSide()) { return false; }
 *     if (!hopper.isOnCooldown() && state.getValue(HopperBlock.ENABLED)) {
 *         boolean flag = false;
 *         if (!hopper.isEmpty()) { flag = ejectItems(...); }
 *         if (!hopper.inventoryFull()) { flag |= supplier.getAsBoolean(); }
 *         if (flag) {
 *             hopper.setCooldown(8);                // MOVE_ITEM_SPEED
 *             setChanged(...);
 *             return true;
 *         }
 *     }
 *     return false;
 * }
 *
 * private boolean isOnCooldown() { return this.cooldownTime > 0; }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **冷却在每个 tick 开头无条件递减**，包括冷却已经是 0 或负数的情形。因此
 *    `cooldownTime` 会一路减到 `-1` 并停在那里（`-1` 是构造器的初始值，表示「从未运行」）。
 *    写成 `if (cooldown > 0) cooldown--` 会让它停在 0——**行为上看不出区别**，因为
 *    `isOnCooldown()` 只在 `> 0` 时为真；但 NBT 里存下的值会不同（`TransferCooldown`
 *    会被持久化）。
 *
 * 2. **传输成功才设置冷却**。`setCooldown(8)` 位于 `if (flag)` 内——没有物品可动时冷却
 *    保持 0，下一个 tick 会立刻再次尝试。
 *
 * 3. **递减发生在「是否冷却」判定之前**。所以 `cooldownTime == 1` 的那个 tick：
 *    先减到 0，然后 `isOnCooldown()` 为假，**本 tick 就能传输**。即从设置冷却到下一次
 *    传输之间恰好间隔 8 个 tick（第 1 次设置、第 2..8 次递减、第 9 次尝试）。
 *
 * 单元格式：`cooldown|enabled|hasMovable`
 *
 * TODO: 本能力把「弹出」与「吸入」抽象成单一布尔 `hasMovable`，未覆盖
 * `ejectItems` / `suckInItems` 各自的槽位遍历顺序与方向选取（需要世界与容器）。
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

/// 原版 `HopperBlockEntity.MOVE_ITEM_SPEED`。
constexpr i32 MOVE_ITEM_SPEED = 8;

struct HopperOutput {
    i32 cooldownAfter = 0;
    bool attempted = false;
    bool moved = false;
    bool cooldownSet = false;
};

/**
 * @brief 复刻 `pushItemsTick` + `tryMoveItems` 的冷却时序。
 */
HopperOutput pushItemsTick(i32 cooldown, bool enabled, bool hasMovable)
{
    HopperOutput out;

    // 1) 无条件递减（包括 0 与负数）。
    out.cooldownAfter = cooldown - 1;

    // 2) 递减后再判定是否冷却。
    const bool onCooldown = out.cooldownAfter > 0;
    if (onCooldown) {
        return out;
    }

    // 3) setCooldown(0) 之后进入 tryMoveItems。
    out.cooldownAfter = 0;
    out.attempted = true;

    if (!enabled) {
        return out;
    }

    // 4) 有物品可动才算传输成功，并设置冷却。
    if (hasMovable) {
        out.moved = true;
        out.cooldownAfter = MOVE_ITEM_SPEED;
        out.cooldownSet = true;
    }

    return out;
}

/**
 * @brief 单元表：`cooldown|enabled|hasMovable`。
 *
 * 重点关注 `cooldown == 1` 的边界：先递减到 0，然后本 tick 就能传输。
 */
constexpr const char* UNITS[] = {
    // 初始状态 -1：递减到 -2，不冷却，尝试传输
    "-1|true|true",
    // 冷却 0：递减到 -1，尝试传输
    "0|true|true",
    // 冷却 1（边界）：递减到 0，本 tick 能传输 -> 重新设为 8
    "1|true|true",
    // 冷却 2：递减到 1，仍冷却，不尝试
    "2|true|true",
    // 冷却 8（刚设置后）：递减到 7，不尝试
    "8|true|true",
    // 冷却 9：递减到 8，不尝试（说明设置 8 后要等 8 个 tick）
    "9|true|true",
    // 冷却 1 但被禁用：递减到 0，尝试但不传输
    "1|false|true",
    // 冷却 1、启用、无物品可动：不设冷却
    "1|true|false",
    // 冷却 0、禁用、有物品：不传输
    "0|false|true",
    // 冷却 5、无物品：仍冷却中，不尝试
    "5|true|false",
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
    if (parts.size() != 3) {
        throw std::runtime_error("hopper_cooldown: 单元应为 3 段 'cooldown|enabled|hasMovable'：" + unit);
    }

    const i32 cooldown = std::stoi(parts[0]);
    const bool enabled = parts[1] == "true";
    const bool hasMovable = parts[2] == "true";

    const HopperOutput out = pushItemsTick(cooldown, enabled, hasMovable);

    return json{{"unit", unit},
        {"cooldownAfter", out.cooldownAfter},
        {"attempted", out.attempted},
        {"moved", out.moved},
        {"cooldownSet", out.cooldownSet}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("hopper_cooldown", &writeList, &runLine)

} // namespace mc::diff
