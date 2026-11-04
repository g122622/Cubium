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
 * @brief 差分能力：爆炸射线破坏判定（纯计算，不依赖世界）。
 *
 * `list`：固定的一组场景名（见下方 SCENES）
 * `run` ：{ "scene": "tnt", "seed": "0" }
 *         → { "scene": "tnt", "seed": "0", "radius": "4",
 *             "initialStrength": "4.32", "destroyedCount": 417,
 *             "firstDestroyed": [ ... ] }
 *
 * 复刻原版 `ServerExplosion.calculateExplodedPositions()`：
 *
 * ```java
 * float f = this.radius * (0.7F + this.level.random.nextFloat() * 0.6F);
 * for (float f1 = 0.3F; f > 0.0F; f -= 0.22500001F) {
 *     ...
 *     f -= (optional.get() + 0.3F) * 0.3F;
 *     if (f > 0.0F && shouldBlockExplode(...)) { set.add(blockpos); }
 *     d4 += d0 * 0.3F; ...
 * }
 * ```
 *
 * 三条射线方向来自 16×16×16 网格的**外壳**（`j/k/l` 任一为 0 或 15），共
 * 16³ − 14³ = 1352 条，每条独立消耗一次 `nextFloat()`。
 *
 * ## 为什么能做成纯计算
 *
 * 原版这一步只依赖三样东西：爆炸中心、半径、以及「每个方块位置的抗性」。本能力用
 * **以中心为原点的均匀抗性体素**代替真实世界——把空间划分成若干层，每层给定抗性，
 * 抗性 0 表示空气（不消耗强度）。这样既不依赖方块注册表，又能覆盖算法的全部路径：
 *
 * - 空气不消耗强度（继续沿射线前进）；
 * - 抗性 > 0 时 `f -= (res + 0.3) * 0.3`，高抗性直接终止射线；
 * - `f > 0` 的边界判定（正好衰减到 0 时不破坏）。
 *
 * ## 三个容易写错的常量
 *
 * - `0.22500001F` 不是 `0.225F`。这个差值来自原版对 `(0.3 * 0.75)` 的浮点累加，
 *   用 `0.225F` 会在若干步后产生 1 ulp 偏差，进而让边界上的方块判定翻转。
 * - `0.3F` 的射线步长与抗性公式里的 `0.3F` 是同一个字面量，但用途不同。
 * - 初始强度 `radius * (0.7 + rand * 0.6)`，随机数每个射线方向各消耗一次。
 *
 * TODO: 本能力刻意不读数据包、不读方块表，因此不覆盖「方块抗性表本身是否与原版一致」。
 * 那部分由 `block_properties` 等能力从别的角度覆盖。
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 一个爆炸场景：半径 + 一层层同心壳的抗性。
struct ExplosionScene {
    std::string name;
    f32 radius;
    /// 从中心向外每一层「半径（以方块计）」的抗性；最后一层之外视为高抗性（终止射线）。
    std::vector<std::pair<f32, f32>> shells;
};

/**
 * @brief 场景表。
 *
 * `shells` 的含义：距离中心小于该层半径的方块使用该层抗性。抗性 0 视为空气。
 * 例如 `{{2.0f, 0.0f}, {4.0f, 3.0f}, {8.0f, 100.0f}}` 表示：2 格内是空气、
 * 2~4 格是抗性 3 的方块、4~8 格是抗性 100 的方块（足以终止射线）。
 */
const std::vector<ExplosionScene> SCENES = {
    // TNT：半径 4，周围全是抗性 0.3 的「普通方块」。
    {"tnt", 4.0f, {{64.0f, 0.3f}}},
    // TNT 在空气中：全为空气，射线不衰减，能覆盖到半径 4 的最大范围。
    {"tnt_air", 4.0f, {{64.0f, 0.0f}}},
    // 苦力怕：半径 3，中心 1.5 格空气，外面是抗性 0.3。
    {"creeper", 3.0f, {{2.0f, 0.0f}, {64.0f, 0.3f}}},
    // 黑曜石墙：半径 6，4 格内是抗性 1200（不可破坏）。
    {"obsidian", 6.0f, {{4.0f, 0.0f}, {64.0f, 1200.0f}}},
    // 混合层：空气 → 弱方块 → 强方块，覆盖「衰减到 0 就停」的边界。
    {"mixed", 5.0f, {{2.0f, 0.0f}, {3.0f, 1.5f}, {6.0f, 30.0f}, {64.0f, 1200.0f}}},
    // 大半径：覆盖更长的射线与更多的步进次数。
    {"charged", 8.0f, {{3.0f, 0.0f}, {10.0f, 2.0f}, {64.0f, 500.0f}}},
};

/// 原版常量：射线步长。
constexpr f32 RAY_STEP_SIZE = 0.3f;
/// 原版常量：每次步进的强度衰减。注意是 0.22500001F 而不是 0.225F。
constexpr f32 STRENGTH_DECAY = 0.22500001f;
/// 原版常量：抗性系数。
constexpr f32 RESISTANCE_COEFFICIENT = 0.3f;

std::string formatFloat(f32 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", static_cast<double>(value));
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

/**
 * @brief 取某个位置方块在当前场景下的爆炸抗性。
 *
 * 抗性按「到中心的欧氏距离」分层查表；超出最后一层时返回一个足够大的值，让射线终止。
 */
f32 resistanceAt(const ExplosionScene& scene, f64 x, f64 y, f64 z)
{
    // 用 double 求平方和再收窄：与 Java 侧 (double) x * x + ... 的求值精度一致。
    const f32 distance =
        static_cast<f32>(std::sqrt(static_cast<f64>(x) * x + static_cast<f64>(y) * y + static_cast<f64>(z) * z));
    for (const auto& [limit, resistance] : scene.shells) {
        if (distance < limit) {
            return resistance;
        }
    }
    return 10000.0f;
}

/**
 * @brief 复刻 `ServerExplosion.calculateExplodedPositions()`。
 *
 * @return 被摧毁的方块坐标集合（去重后按固定顺序输出）
 */
std::vector<std::array<i32, 3>> calculateExplodedPositions(const ExplosionScene& scene, i64 seed)
{
    math::JavaLegacyRandom random(static_cast<u64>(seed));

    // 原版用 HashSet<BlockPos> 去重；这里用排序后的 vector 保证输出顺序确定。
    std::vector<std::array<i32, 3>> destroyed;

    for (i32 j = 0; j < 16; ++j) {
        for (i32 k = 0; k < 16; ++k) {
            for (i32 l = 0; l < 16; ++l) {
                // 只取 16×16×16 网格的外壳。
                if (j != 0 && j != 15 && k != 0 && k != 15 && l != 0 && l != 15) {
                    continue;
                }

                // 方向向量按原版用 f32 计算：j / 15.0F * 2.0F - 1.0F，再整体归一化。
                const f32 rawX = static_cast<f32>(j) / 15.0f * 2.0f - 1.0f;
                const f32 rawY = static_cast<f32>(k) / 15.0f * 2.0f - 1.0f;
                const f32 rawZ = static_cast<f32>(l) / 15.0f * 2.0f - 1.0f;
                const f32 length = std::sqrt(rawX * rawX + rawY * rawY + rawZ * rawZ);
                const f32 d0 = rawX / length;
                const f32 d1 = rawY / length;
                const f32 d2 = rawZ / length;

                // 初始强度：radius * (0.7 + random * 0.6)。每条射线消耗一次 nextFloat。
                f32 strength = scene.radius * (0.7f + random.nextFloat() * 0.6f);

                // 爆炸中心固定在原点，射线的起点也在原点。
                // 原版 d4/d5/d6 是 double：步进累积误差会直接改变 floor 的取整结果。
                f64 x = 0.0;
                f64 y = 0.0;
                f64 z = 0.0;
                for (f32 step = 0.3f; strength > 0.0f; strength -= STRENGTH_DECAY) {
                    const i32 bx = static_cast<i32>(std::floor(x));
                    const i32 by = static_cast<i32>(std::floor(y));
                    const i32 bz = static_cast<i32>(std::floor(z));

                    // 抗性必须按**方块坐标**取，而不是射线浮点位置：原版查的是
                    // level.getBlockState(blockpos)，blockpos 已经 floor 过。用浮点位置
                    // 会让边界上的方块被归到相邻层。
                    const f32 resistance = resistanceAt(
                        scene, static_cast<f64>(bx) + 0.5, static_cast<f64>(by) + 0.5, static_cast<f64>(bz) + 0.5);
                    if (resistance > 0.0f) {
                        // 强度衰减 = (resistance + 0.3) * 0.3
                        strength -= (resistance + RESISTANCE_COEFFICIENT) * RESISTANCE_COEFFICIENT;
                    }

                    // 原版空气走 Optional.empty()，既不衰减也**不会**被加入破坏集合。
                    // 抗性 0 在这里表示空气，必须同样排除；抗性 10000 表示不可破坏层。
                    if (strength > 0.0f && resistance > 0.0f && resistance < 10000.0f) {
                        destroyed.push_back({bx, by, bz});
                    }

                    x += static_cast<f64>(d0) * RAY_STEP_SIZE;
                    y += static_cast<f64>(d1) * RAY_STEP_SIZE;
                    z += static_cast<f64>(d2) * RAY_STEP_SIZE;
                }
            }
        }
    }

    // 去重并按固定顺序排序，两侧输出才能逐字节对齐。
    std::sort(destroyed.begin(), destroyed.end());
    destroyed.erase(std::unique(destroyed.begin(), destroyed.end()), destroyed.end());
    return destroyed;
}

void writeList(FILE* out)
{
    for (const auto& scene : SCENES) {
        std::fprintf(out, "%s\n", scene.name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("scene").get<std::string>();
    const i64 seed = std::stoll(input.at("seed").get<std::string>());

    for (const auto& scene : SCENES) {
        if (scene.name != requested) {
            continue;
        }

        const auto destroyed = calculateExplodedPositions(scene, seed);

        // 只输出前若干个坐标 + 总数：全部输出会让文件过大，而总数与首尾样本
        // 已经足以捕捉任何一步的判定差异（一条射线错一位就会被排序后的首项暴露）。
        json first = json::array();
        for (std::size_t i = 0; i < destroyed.size() && i < 8; ++i) {
            first.push_back(json::array({destroyed[i][0], destroyed[i][1], destroyed[i][2]}));
        }
        json last = json::array();
        for (std::size_t i = destroyed.size() > 8 ? destroyed.size() - 8 : 0; i < destroyed.size(); ++i) {
            last.push_back(json::array({destroyed[i][0], destroyed[i][1], destroyed[i][2]}));
        }

        // 初始强度单独算一次（与射线无关，只依赖种子与半径），方便定位问题。
        math::JavaLegacyRandom probeRandom(static_cast<u64>(seed));
        const f32 firstStrength = scene.radius * (0.7f + probeRandom.nextFloat() * 0.6f);

        return json{{"scene", requested},
            {"seed", input.at("seed").get<std::string>()},
            {"radius", formatFloat(scene.radius)},
            {"firstStrength", formatFloat(firstStrength)},
            {"destroyedCount", destroyed.size()},
            {"firstDestroyed", first},
            {"lastDestroyed", last}}
            .dump();
    }
    return json{{"scene", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(explosion_ray, &writeList, &runLine)

} // namespace mc::diff
