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
 * @brief 差分能力：幽匿感测体的振动选优（`VibrationSelector#shouldReplaceVibration`）。
 *
 * `list`：三个维度 —— 已有候选的事件/距离/时刻 × 新候选的事件/距离/时刻
 * `run` ：按 addCandidate 流程喂入新候选，输出最终选中的是哪一条
 *
 * ## 选优规则（三级，逐级决胜）
 *
 * 1. **时刻不同**：不替换。后一 tick 的候选要等前一 tick 的振动处理完
 * 2. **时刻相同，距离不同**：**近的**胜
 * 3. **时刻、距离都相同**：**频率高的**胜（频率由事件查表得到）
 *
 * ## 为什么必须严格按这个顺序
 *
 * 三条规则写成并列判断会改变结果：
 *
 * - 先比频率再比距离 → 远处一次「爆炸」（频率 15）会盖掉近处的脚步声（频率 1），
 *   玩家走过去完全无反应
 * - 允许跨 tick 替换 → 高频事件会不断插队，低频事件永远得不到处理，
 *   感测体持续输出但总是同一个读数
 *
 * ## 场景
 *
 * 用一个固定的「已有候选」（距离 8、事件 step 频率 1）与新候选的各种组合比较，
 * 覆盖时刻相同/更晚、距离小于/等于/大于、频率低于/等于/高于。
 */

#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 已有候选：时刻 100、距离 8、事件 step（频率 1）
constexpr u64 BASE_TICK = 100;
constexpr f32 BASE_DISTANCE = 8.0f;

/// 新候选的事件选项：(标签, 事件, 频率)
struct EventOption {
    const char* label;
    const gameevent::GameEvent* event;
};

const EventOption EVENT_OPTIONS[] = {
    {"step", &gameevent::GameEvents::STEP},                 // 频率 1
    {"shear", &gameevent::GameEvents::SHEAR},               // 频率 6
    {"block_change", &gameevent::GameEvents::BLOCK_CHANGE}, // 频率 11
    {"explode", &gameevent::GameEvents::EXPLODE},           // 频率 15
};

/// 新候选的距离选项
constexpr f32 DISTANCE_OPTIONS[] = {4.0f, 8.0f, 12.0f};

/// 新候选的时刻选项：相同 / 晚 1 tick
constexpr u64 TICK_OPTIONS[] = {100, 101};

void writeList(FILE* out)
{
    // 单元形如 "<事件>:<距离>:<时刻>"
    for (const auto& option : EVENT_OPTIONS) {
        for (const f32 distance : DISTANCE_OPTIONS) {
            for (const u64 tick : TICK_OPTIONS) {
                std::fprintf(out,
                    "%s:%.0f:%llu\n",
                    option.label,
                    static_cast<f64>(distance),
                    static_cast<unsigned long long>(tick));
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    // 解析 "<事件>:<距离>:<时刻>"
    const std::size_t firstSep = unit.find(':');
    if (firstSep == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::size_t secondSep = unit.find(':', firstSep + 1);
    if (secondSep == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string eventLabel = unit.substr(0, firstSep);
    const f32 distance = static_cast<f32>(std::stod(unit.substr(firstSep + 1, secondSep - firstSep - 1)));
    const u64 tick = static_cast<u64>(std::stoull(unit.substr(secondSep + 1)));

    const gameevent::GameEvent* candidateEvent = nullptr;
    for (const auto& option : EVENT_OPTIONS) {
        if (eventLabel == option.label) {
            candidateEvent = option.event;
            break;
        }
    }
    if (candidateEvent == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    gameevent::VibrationSelector selector;

    // 先放入固定的已有候选
    gameevent::VibrationInfo baseInfo;
    baseInfo.gameEvent = &gameevent::GameEvents::STEP;
    baseInfo.distance = BASE_DISTANCE;
    baseInfo.pos = Vector3d(0.0, 64.0, 0.0);
    selector.addCandidate(baseInfo, BASE_TICK);

    // 再放入新候选
    gameevent::VibrationInfo newInfo;
    newInfo.gameEvent = candidateEvent;
    newInfo.distance = distance;
    newInfo.pos = Vector3d(1.0, 64.0, 1.0);
    selector.addCandidate(newInfo, tick);

    // 读取最终选中的候选（用足够晚的 tick 取，确保不会被「延迟 1 tick」挡住）
    const auto chosen = selector.chosenCandidate(tick + 10);

    json result{{"unit", unit}};
    if (!chosen.has_value() || chosen->gameEvent == nullptr) {
        result["chosen"] = nullptr;
        result["chosenDistance"] = nullptr;
    } else {
        result["chosen"] = std::string("minecraft:") + chosen->gameEvent->id();
        result["chosenDistance"] = chosen->distance;
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("vibration_selector", &writeList, &runLine)

} // namespace mc::diff
