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
 * @brief 差分能力：怪物生成的光照判定（`Monster.isDarkEnoughToSpawn`）。
 *
 * `list`：一组场景
 * `run` ：一行 -> { "dark": bool, "lightTestSample": i32 }
 *
 * ## 原版算法（Monster.java:86-99）
 *
 * ```java
 * if (getBrightness(SKY,   pos) > random.nextInt(32))            return false;
 * int limit = dimensionType.monsterSpawnBlockLightLimit();
 * if (limit < 15 && getBrightness(BLOCK, pos) > limit)           return false;
 * int j = level.isThundering()
 *     ? getMaxLocalRawBrightness(pos, 10)
 *     : getMaxLocalRawBrightness(pos);
 * return j <= dimensionType.monsterSpawnLightTest().sample(random);
 * ```
 *
 * 三个容易写错的点：
 *
 * 1. **天空光与 int(32) 比**，不是与常量比——它等价于「天空光 0..31 的随机阈值」，
 *    所以即使天空光 1 也有 31/32 概率通过。把它简化成「天空光必须为 0」是常见错误；
 * 2. **方块光上限只在该维度 limit < 15 时才检查**。limit 为 15（下界）时这条完全不生效；
 * 3. 最后比较的是**综合亮度**（天空与方块取大，并受天气/时间衰减），
 *    不是单独的方块光。
 *
 * ## 单元格式
 *
 * `sky|block|combined|thunderCombined|limit|lightMin|lightMax|seed`
 *
 * - `sky` / `block`：原始天空光 / 方块光（0..15）；
 * - `combined`：非雷暴时的 `getMaxLocalRawBrightness(pos)`；
 * - `thunderCombined`：雷暴时的 `getMaxLocalRawBrightness(pos, 10)`；
 * - `limit`：`monsterSpawnBlockLightLimit`；
 * - `lightMin` / `lightMax`：`monsterSpawnLightTest` 的 UniformInt 上下界
 *   （下界等于上界即为常数 IntProvider）；
 * - `seed`：固定随机种子，保证两侧随机流一致。
 *
 * 输出同时带 `lightTestSample`，用它在差异出现时区分「随机流不同」与「判定逻辑不同」。
 */

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

std::vector<std::string> splitPipe(std::string_view text)
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

/**
 * 场景表：`sky|block|combined|thunderCombined|limit|lightMin|lightMax|seed`。
 *
 * 覆盖三个维度的真实参数（主世界 limit=0 / Uniform(0,7)、下界 limit=15 / 常数 7、
 * 末地 limit=0 / 常数 15），并针对三个易错点各设一组对照。
 */
constexpr const char* SCENES[] = {
    // --- 主世界：limit=0，lightTest=Uniform(0,7) ---
    "0|0|0|0|0|0|7|1",
    "0|0|0|0|0|0|7|2",
    "0|1|1|1|0|0|7|3", // 方块光 1 > limit 0 -> 拒绝
    "5|0|5|5|0|0|7|4", // 天空光 5，看随机阈值
    "15|0|15|15|0|0|7|5",
    "31|0|15|15|0|0|7|6", // 天空光 31：几乎必然拒绝
    "0|0|7|7|0|0|7|7",
    "0|0|8|8|0|0|7|8", // 8 > 采样上限 7 -> 拒绝

    // --- 下界：limit=15（**不检查方块光**），lightTest=常数 7 ---
    "0|15|15|15|15|7|7|9", // 方块光 15 但 limit=15，这一条不生效
    "0|15|7|7|15|7|7|10",
    "0|0|7|7|15|7|7|11",

    // --- 末地：limit=0，lightTest=常数 15 ---
    "0|0|15|15|0|15|15|12",
    "0|0|0|0|0|15|15|13",
    "0|14|14|14|0|15|15|14", // 方块光 14 > limit 0 -> 拒绝

    // --- 雷暴：使用 thunderCombined 而非 combined ---
    "0|0|0|15|0|0|7|15",
    "0|0|15|0|0|0|7|16",

    // --- 边界：limit=14（仍检查）与 limit=15（不检查）的分界 ---
    "0|14|14|14|14|14|14|17",
    "0|15|15|15|14|14|14|18",
    "0|15|15|15|15|14|14|19",
};

void writeList(FILE* out)
{
    for (const char* scene : SCENES) {
        std::fprintf(out, "%s\n", scene);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const std::vector<std::string> parts = splitPipe(unit);
    if (parts.size() != 8) {
        throw std::runtime_error("mob_spawn_light_level: 单元应为 8 段：" + unit);
    }

    const i32 sky = std::stoi(parts[0]);
    const i32 block = std::stoi(parts[1]);
    const i32 combined = std::stoi(parts[2]);
    const i32 thunderCombined = std::stoi(parts[3]);
    const i32 limit = std::stoi(parts[4]);
    const i32 lightMin = std::stoi(parts[5]);
    const i32 lightMax = std::stoi(parts[6]);
    const i32 seed = std::stoi(parts[7]);

    // 走 Cubium 的**真实实现**，不在这里复刻算法。
    // 随机源必须是 JavaLegacyRandom：原版 RandomSource.create() 返回 LegacyRandomSource
    // （48 位 LCG），而 math::Random 是 Xoroshiro128++，两者序列完全不同。
    math::JavaLegacyRandom random(seed);
    const bool dark = mc::MonsterEntity::isDarkEnoughToSpawn(
        sky, block, combined, thunderCombined, limit, lightMin, lightMax, random);

    // 同口径重放随机流，把 lightTest 的采样值也输出，便于区分
    // 「随机流不同」与「判定逻辑不同」。
    //
    // 只在判定真的走到第 3 段时才输出采样值：前两段提前返回时原版根本不消耗第二次随机，
    // 输出 -1 与之对齐。否则会拿「本不该发生的采样」去比对，制造伪差异。
    math::JavaLegacyRandom replay(seed);
    const i32 skyRoll = replay.nextInt(32);
    const bool rejectedEarly = sky > skyRoll || (limit < 15 && block > limit);
    const i32 sample = rejectedEarly ? -1 : (lightMin + replay.nextInt(lightMax - lightMin + 1));

    return json{{"unit", unit}, {"dark", dark}, {"lightTestSample", sample}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mob_spawn_light_level, &writeList, &runLine)

} // namespace mc::diff
