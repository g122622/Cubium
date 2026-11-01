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
 * @brief 差分能力：刷怪笼的生成延迟状态机（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行
 *         → { "spawnDelay": 20, "shouldSpawn": false, ... }
 *
 * 复刻原版 `BaseSpawner` 的两段纯计算：
 *
 * ```java
 * // 1) delay(...) —— 随机重置下一次延迟
 * private void delay(Level level, BlockPos pos) {
 *     RandomSource random = level.random;
 *     if (this.maxSpawnDelay <= this.minSpawnDelay) {
 *         this.spawnDelay = this.minSpawnDelay;                       // 区间退化 -> 取 min
 *     } else {
 *         this.spawnDelay = this.minSpawnDelay
 *             + random.nextInt(this.maxSpawnDelay - this.minSpawnDelay);
 *     }
 * }
 *
 * // 2) serverTick(...) 的延迟推进部分
 * if (isNearPlayer(...) && level.isSpawnerBlockEnabled()) {
 *     if (this.spawnDelay == -1) {
 *         this.delay(level, pos);                                     // -1 表示「需要重置」
 *     }
 *     if (this.spawnDelay > 0) {
 *         this.spawnDelay--;
 *     } else {
 *         ... 尝试生成 ...
 *     }
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **`-1` 是「哨兵值」，不是「还剩 -1 tick」**。`spawnDelay == -1` 时先调 `delay()`
 *    重新取值，再走正常的递减/生成流程。因此同一个 tick 内可能「先重置、再减 1」。
 *
 * 2. **区间退化时直接取 `minSpawnDelay`**，不调用随机数。条件是
 *    `maxSpawnDelay <= minSpawnDelay`（**包含相等**）。写成 `<` 会在两者相等时去调
 *    `nextInt(0)`，而 `nextInt(0)` 在原版是未定义/抛异常的。
 *
 * 3. **`nextInt` 的上界是 `max - min`**，不是 `max - min + 1`。即延迟区间是左闭右开的
 *    `[min, max)`。默认 200/800 时取到 200..799，**永远取不到 800**。
 *
 * ## 玩家范围与开关
 *
 * `serverTick` 的整体前置条件是 `isNearPlayer(...) && isSpawnerBlockEnabled()`：
 * 玩家不在范围内或刷怪笼被关闭时，**整个 tick 都不做任何事**，`spawnDelay` 保持不变
 * （既不重置也不递减）。这一点容易写成「仍然递减」。
 *
 * 单元格式：
 * - `delay|<min>|<max>|<roll>`——用给定的 `roll` 代替随机数，验证取值公式
 * - `tick|<spawnDelay>|<nearPlayer>|<enabled>|<min>|<max>|<roll>`
 *
 * TODO: 本能力不覆盖实际生成（`spawnCount` 次尝试、`spawnRange` 内的位置随机、附近实体
 * 数量上限），那部分需要世界与实体注册表。
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

/// 原版 `BaseSpawner.DEFAULT_MIN_SPAWN_DELAY`。
constexpr i32 DEFAULT_MIN_SPAWN_DELAY = 200;

/// 原版 `BaseSpawner.DEFAULT_MAX_SPAWN_DELAY`。
constexpr i32 DEFAULT_MAX_SPAWN_DELAY = 800;

/**
 * @brief 复刻 `BaseSpawner.delay`：给定随机 roll 时返回新的 spawnDelay。
 *
 * `roll` 对应 `random.nextInt(max - min)` 的返回值。
 */
i32 computeDelay(i32 minSpawnDelay, i32 maxSpawnDelay, i32 roll)
{
    if (maxSpawnDelay <= minSpawnDelay) {
        return minSpawnDelay;
    }
    return minSpawnDelay + roll;
}

struct TickResult {
    i32 spawnDelay = 0;
    bool reset = false;
    bool decremented = false;
    bool shouldSpawn = false;
};

/**
 * @brief 复刻 `serverTick` 里延迟推进那一段。
 */
TickResult serverTick(i32 spawnDelay, bool nearPlayer, bool enabled, i32 minSpawnDelay, i32 maxSpawnDelay, i32 roll)
{
    TickResult result;
    result.spawnDelay = spawnDelay;

    // 前置条件不满足：整个 tick 什么都不做。
    if (!nearPlayer || !enabled) {
        return result;
    }

    // -1 是「需要重置」的哨兵值。
    if (result.spawnDelay == -1) {
        result.spawnDelay = computeDelay(minSpawnDelay, maxSpawnDelay, roll);
        result.reset = true;
    }

    if (result.spawnDelay > 0) {
        result.spawnDelay--;
        result.decremented = true;
    } else {
        result.shouldSpawn = true;
    }

    return result;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // delay：默认区间，roll = 0 -> 200
    "delay|200|800|0",
    // delay：默认区间，roll = 599 -> 799（上界取不到 800）
    "delay|200|800|599",
    // delay：区间退化（max == min）-> 直接取 min，忽略 roll
    "delay|200|200|0",
    // delay：区间反向（max < min）-> 直接取 min
    "delay|300|200|0",
    // delay：极小区间 min=0, max=1 -> roll 只能是 0
    "delay|0|1|0",
    // delay：min=0 时取到 0
    "delay|0|10|0",
    // tick：初始 20，正常递减
    "tick|20|true|true|200|800|0",
    // tick：剩 1 -> 递减到 0，本 tick 不生成
    "tick|1|true|true|200|800|0",
    // tick：剩 0 -> 本 tick 生成
    "tick|0|true|true|200|800|0",
    // tick：-1 哨兵 -> 先重置再递减
    "tick|-1|true|true|200|800|0",
    // tick：-1 哨兵，区间退化 -> 重置为 min 再递减
    "tick|-1|true|true|250|250|0",
    // tick：玩家不在范围 -> 完全不动作
    "tick|20|false|true|200|800|0",
    // tick：刷怪笼被禁用 -> 完全不动作
    "tick|20|true|false|200|800|0",
    // tick：-1 且玩家不在范围 -> 不重置
    "tick|-1|false|true|200|800|0",
    // tick：0 且玩家不在范围 -> 不生成
    "tick|0|false|true|200|800|0",
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
    if (parts.empty()) {
        throw std::runtime_error("spawner_delay: 空单元");
    }

    if (parts[0] == "delay" && parts.size() == 4) {
        const i32 minSpawnDelay = std::stoi(parts[1]);
        const i32 maxSpawnDelay = std::stoi(parts[2]);
        const i32 roll = std::stoi(parts[3]);
        const i32 delay = computeDelay(minSpawnDelay, maxSpawnDelay, roll);
        return json{{"unit", unit},
            {"mode", "delay"},
            {"spawnDelay", delay},
            {"reset", false},
            {"decremented", false},
            {"shouldSpawn", false}}
            .dump();
    }

    if (parts[0] == "tick" && parts.size() == 7) {
        const i32 spawnDelay = std::stoi(parts[1]);
        const bool nearPlayer = parts[2] == "true";
        const bool enabled = parts[3] == "true";
        const i32 minSpawnDelay = std::stoi(parts[4]);
        const i32 maxSpawnDelay = std::stoi(parts[5]);
        const i32 roll = std::stoi(parts[6]);
        const TickResult result = serverTick(spawnDelay, nearPlayer, enabled, minSpawnDelay, maxSpawnDelay, roll);
        return json{{"unit", unit},
            {"mode", "tick"},
            {"spawnDelay", result.spawnDelay},
            {"reset", result.reset},
            {"decremented", result.decremented},
            {"shouldSpawn", result.shouldSpawn}}
            .dump();
    }

    throw std::runtime_error("spawner_delay: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("spawner_delay", &writeList, &runLine)

} // namespace mc::diff
