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

#include "common/entity/player/CooldownTracker.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;
using entity::player::CooldownTracker;

/**
 * @file item_cooldowns.cpp
 * @brief 物品冷却追踪（原版 `ItemCooldowns`）—— 每个单元是一串操作，逐条输出进度。
 *
 * 冷却进度决定准星旁的白色遮罩高度，也决定「这个物品现在能不能用」。
 * 原版公式是
 *
 *   progress = clamp((endTime - (tickCount + partialTicks)) / (endTime - startTime), 0, 1)
 *
 * 三个易错点：
 *
 *   1. **分母是冷却总长**，不是剩余值；写成 `remaining / total` 在 partialTicks=0 时等价，
 *      但一旦 partialTicks 非零就会偏。
 *   2. **partialTicks 加在 tickCount 上**（`tickCount + partial`），不是从剩余值里减，
 *      两者在分母不为 1 时结果不同。
 *   3. **到期项在 tick() 里被移除**，且移除发生在 tickCount 自增之后、用 `<=` 比较，
 *      所以「刚好到期」的那一 tick 就已经查不到冷却了。
 *
 * 单元用「同一个冷却组上的一系列操作」描述，输出每一步之后的进度、剩余 tick 与是否仍在冷却。
 */

/// 一步操作。
struct Step {
    /// "tick" 推进一 tick；"set" 设置冷却；"remove" 移除冷却；"query" 只查询
    const char* kind;
    /// set 时的冷却时长（tick）
    i32 ticks;
    /// 查询时的 partialTicks
    f32 partial;
};

/// 一个单元。
struct Case {
    const char* name;
    std::vector<Step> steps;
};

const std::vector<Case>& cases()
{
    static const std::vector<Case> table = {
        // 设 20 tick 冷却后立刻查询：进度应为 1.0
        {"set_then_query", {{"set", 20, 0.0f}, {"query", 0, 0.0f}}},
        // 冷却设置后推进一半
        {"set_advance_half",
            {{"set", 20, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"query", 0, 0.0f}}},
        // 推进到恰好到期：tick() 用 <= 判定，此时应已移除
        {"expire_exactly",
            {{"set", 20, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"query", 0, 0.0f}}},
        // 到期后多推进一 tick：结果应与上一条相同（都是 0）
        {"expire_past",
            {{"set", 5, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"query", 0, 0.0f}}},
        // partialTicks 参与：progress 应比 partial=0 时更小
        {"partial_ticks",
            {{"set", 20, 0.0f},
                {"tick", 0, 0.0f},
                {"query", 0, 0.0f},
                {"query", 0, 0.5f},
                {"query", 0, 0.25f},
                {"query", 0, 0.75f}}},
        // partialTicks 超过剩余：clamp 到 0
        {"partial_beyond_end", {{"set", 4, 0.0f}, {"query", 0, 3.5f}, {"query", 0, 4.0f}, {"query", 0, 10.0f}}},
        // 负的 partialTicks：clamp 到 1
        {"partial_negative", {{"set", 10, 0.0f}, {"query", 0, -1.0f}, {"query", 0, -0.5f}}},
        // 重复设置同一冷却组：结束时间被重置
        {"reset_cooldown",
            {{"set", 20, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"set", 10, 0.0f},
                {"query", 0, 0.0f},
                {"tick", 0, 0.0f},
                {"query", 0, 0.0f}}},
        // 小于等于 0 的冷却时长：原版 addCooldown 仍会写入，Cubium 走 remove 分支
        {"zero_cooldown", {{"set", 0, 0.0f}, {"query", 0, 0.0f}}},
        {"negative_cooldown", {{"set", -5, 0.0f}, {"query", 0, 0.0f}}},
        // 设置后显式移除
        {"remove_after_set", {{"set", 20, 0.0f}, {"query", 0, 0.0f}, {"remove", 0, 0.0f}, {"query", 0, 0.0f}}},
        // 未设置就查询
        {"query_without_set", {{"query", 0, 0.0f}}},
        // 极长冷却
        {"very_long", {{"set", 72000, 0.0f}, {"query", 0, 0.0f}, {"tick", 0, 0.0f}, {"query", 0, 0.0f}}},
        // 冷却时长 1 tick
        {"one_tick", {{"set", 1, 0.0f}, {"query", 0, 0.0f}, {"query", 0, 0.5f}, {"tick", 0, 0.0f}, {"query", 0, 0.0f}}},
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (const auto& caseItem : cases()) {
        std::fprintf(out, "%s\n", caseItem.name);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const Case* found = nullptr;
    for (const auto& candidate : cases()) {
        if (requested == candidate.name) {
            found = &candidate;
            break;
        }
    }
    if (found == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    const ResourceLocation group("minecraft", "diff_probe_item");
    CooldownTracker tracker;

    // 每步之后的状态按顺序累积成一个数组，便于在报告里一眼看出是从哪一步开始分叉。
    json states = json::array();
    for (const auto& step : found->steps) {
        const std::string kind(step.kind);
        if (kind == "tick") {
            tracker.tick();
            continue;
        }
        if (kind == "set") {
            tracker.setCooldown(group, step.ticks);
            continue;
        }
        if (kind == "remove") {
            tracker.removeCooldown(group);
            continue;
        }
        // query
        states.push_back(
            json{{"progress", encodeFloat(static_cast<f64>(tracker.getCooldownProgress(group, step.partial)))},
                {"active", tracker.hasCooldown(group)}});
    }

    return json{{"unit", requested}, {"states", states}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(item_cooldowns, &writeList, &runLine)

} // namespace mc::diff
