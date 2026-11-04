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
 * @brief 差分能力：药水效果的叠加合并语义（纯计算）。
 *
 * `list`：一组「原等级|原时长|原环境|新等级|新时长|新环境」
 * `run` ：同上一行
 *         → { "resultAmplifier": 2, "resultDuration": 200, "resultAmbient": false,
 *             "changed": true, "hiddenAmplifier": -1, "hiddenDuration": 0 }
 *
 * 复刻原版 `MobEffectInstance.update(MobEffectInstance)`：
 *
 * ```java
 * boolean flag = false;
 * if (incoming.amplifier > this.amplifier) {
 *     if (incoming.isShorterDurationThan(this)) {   // 旧的有更长的时长 -> 存进 hiddenEffect
 *         MobEffectInstance old = this.hiddenEffect;
 *         this.hiddenEffect = new MobEffectInstance(this);
 *         this.hiddenEffect.hiddenEffect = old;
 *     }
 *     this.amplifier = incoming.amplifier;
 *     this.duration = incoming.duration;
 *     flag = true;
 * } else if (this.isShorterDurationThan(incoming)) {
 *     if (incoming.amplifier == this.amplifier) {
 *         this.duration = incoming.duration;
 *         flag = true;
 *     } else if (this.hiddenEffect == null) {
 *         this.hiddenEffect = new MobEffectInstance(incoming);
 *     } else {
 *         this.hiddenEffect.update(incoming);
 *     }
 * }
 * if (!incoming.ambient && this.ambient || flag) {
 *     this.ambient = incoming.ambient;
 * }
 * ```
 *
 * 其中 `isShorterDurationThan(o)` = `!isInfiniteDuration() && (duration < o.duration ||
 * o.isInfiniteDuration())`，`isInfiniteDuration()` = `duration == -1`。
 *
 * ## 为什么这段逻辑值得单独钉住
 *
 * 它有三层分支且互相耦合：
 *
 * - 等级更高时**无条件**采用新时长，哪怕新的更短——此时旧效果被塞进 `hiddenEffect`，
 *   等新效果过期后重新生效。这条「降级回退」是最容易漏掉的一支。
 * - 等级相同才直接刷新时长；等级更低则只进 `hiddenEffect`，不改变当前值。
 * - `ambient` 的更新条件 `(!incoming.ambient && this.ambient) || flag` 与主分支**独立**，
 *   即使没改动等级时长也可能变。
 *
 * Cubium 的 `EffectInstance` 目前**没有 `update()`**，本能力因此直接按原版公式计算，
 * 输出的结果字段是待实现的行为基准。
 *
 * TODO: 待 Cubium 实现 `EffectInstance::update` 后，探针应改为调用它以覆盖真实实现。
 * 本能力当前覆盖的是**语义**：三层分支与 hiddenEffect 的链式回退。
 */
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `MobEffectInstance.INFINITE_DURATION`。
constexpr i32 INFINITE_DURATION = -1;

/// 一个效果实例的可比状态。
struct EffectState {
    i32 amplifier = 0;
    i32 duration = 0;
    bool ambient = false;
};

/**
 * @brief 原版 `isInfiniteDuration()`。
 */
bool isInfiniteDuration(const EffectState& state)
{
    return state.duration == INFINITE_DURATION;
}

/**
 * @brief 原版 `isShorterDurationThan(other)`。
 */
bool isShorterDurationThan(const EffectState& self, const EffectState& other)
{
    return !isInfiniteDuration(self) && (self.duration < other.duration || isInfiniteDuration(other));
}

/**
 * @brief 复刻原版 `update(incoming)` 的可观察结果。
 *
 * 只跟踪当前值与第一层 hiddenEffect：更深层的链在大分支上不会被读到，而本能力要验证
 * 的是「当前值是什么」与「是否发生了降级回退」。
 */
struct MergeResult {
    EffectState current;
    bool changed = false;
    /// hiddenEffect 是否存在及其等级/时长（不存在时 amplifier = -1）
    i32 hiddenAmplifier = -1;
    i32 hiddenDuration = 0;
};

MergeResult merge(const EffectState& self, const EffectState& incoming)
{
    EffectState current = self;
    bool changed = false;
    bool hasHidden = false;
    EffectState hidden;

    if (incoming.amplifier > current.amplifier) {
        // 旧值时长更长（或新的无限长）时，先把旧值压进 hiddenEffect。
        if (isShorterDurationThan(incoming, current)) {
            hidden = current;
            hasHidden = true;
        }
        current.amplifier = incoming.amplifier;
        current.duration = incoming.duration;
        changed = true;
    } else if (isShorterDurationThan(current, incoming)) {
        if (incoming.amplifier == current.amplifier) {
            current.duration = incoming.duration;
            changed = true;
        } else if (!hasHidden) {
            hidden = incoming;
            hasHidden = true;
        }
        // 已存在 hiddenEffect 时原版递归 update 它；本能力只比较第一层，不再下探。
    }

    // ambient 的更新条件与主分支独立，且**更新时也会把 flag 置真**——
    // 原版这段末尾有 `flag = true;`，漏掉它会让「只变 ambient」的情形被误判为无变化。
    if ((!incoming.ambient && current.ambient) || changed) {
        current.ambient = incoming.ambient;
        changed = true;
    }

    MergeResult result;
    result.current = current;
    result.changed = changed;
    if (hasHidden) {
        result.hiddenAmplifier = hidden.amplifier;
        result.hiddenDuration = hidden.duration;
    }
    return result;
}

/**
 * @brief 场景表：`原等级|原时长|原环境|新等级|新时长|新环境`。
 *
 * 覆盖 update() 的全部分支：
 * - 新等级更高 + 新时长更长（直接采用）
 * - 新等级更高 + 新时长更短（降级回退到 hiddenEffect）——最容易漏的一支
 * - 新等级更高 + 新时长无限
 * - 等级相同 + 新时长更长 / 更短 / 无限 / 全无限
 * - 新等级更低 + 新时长更长（只进 hiddenEffect）
 * - ambient 独立变化（不改等级时不改 ambient / 改等级时 ambient 跟随）
 */
constexpr const char* SCENES[] = {
    "0|200|false|1|300|false", // 等级更高、时长更长
    "1|300|false|2|100|false", // 等级更高、时长更短 -> hiddenEffect 回退
    "0|200|false|1|-1|false",  // 等级更高、新时长无限
    "1|-1|false|2|100|false",  // 等级更高、旧时长无限 -> 回退
    "1|200|false|1|300|false", // 等级相同、时长更长
    "1|300|false|1|200|false", // 等级相同、时长更短（不刷新）
    "1|200|false|1|-1|false",  // 等级相同、新时长无限
    "1|-1|false|1|300|false",  // 等级相同、旧时长无限（不刷新）
    "1|-1|false|1|-1|false",   // 全无限
    "2|300|false|1|500|false", // 等级更低、时长更长 -> 只进 hiddenEffect
    "2|300|false|1|100|false", // 等级更低、时长更短（无变化）
    "1|200|true|2|300|false",  // 改等级时 ambient 跟随新值
    "1|200|true|1|300|true",   // 改时长时 ambient 跟随新值
    "1|200|true|1|100|false",  // 无变化（等级低、时长短）时 ambient 不变
    "0|0|false|0|-1|false",    // 零时长 -> 无限
    "0|-1|false|0|0|false",    // 无限 -> 零时长（不刷新）
};

void writeList(FILE* out)
{
    for (const auto* scene : SCENES) {
        std::fprintf(out, "%s\n", scene);
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
    if (parts.size() != 6) {
        throw std::runtime_error(
            "mob_effect_merge: 单元应为 6 段 '原等级|原时长|原环境|新等级|新时长|新环境'：" + unit);
    }

    const EffectState self{std::stoi(parts[0]), std::stoi(parts[1]), parts[2] == "true"};
    const EffectState incoming{std::stoi(parts[3]), std::stoi(parts[4]), parts[5] == "true"};

    const MergeResult merged = merge(self, incoming);

    return json{{"unit", unit},
        {"resultAmplifier", merged.current.amplifier},
        {"resultDuration", merged.current.duration},
        {"resultAmbient", merged.current.ambient},
        {"changed", merged.changed},
        {"hiddenAmplifier", merged.hiddenAmplifier},
        {"hiddenDuration", merged.hiddenDuration}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mob_effect_merge, &writeList, &runLine)

} // namespace mc::diff
