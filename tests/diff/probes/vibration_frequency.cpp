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
 * @brief 差分能力：游戏事件到振动频率的映射（`VibrationSystem#VIBRATION_FREQUENCY_FOR_EVENT`）。
 *
 * `list`：全部 40 个有频率的事件
 * `run` ：一行 -> { "frequency": 频率 0..15 }
 *
 * ## 这张表决定什么
 *
 * 幽匿感测体收到游戏事件后，按这张表把事件归入 1..15 的频段：
 *
 * - 频段决定**红石比较器输出强度**
 * - 频段决定**紫水晶共振方块**发出哪个 RESONATE_N 事件
 * - 表里没有的事件频率为 0，**不产生任何振动**
 *
 * ## 抄错的后果
 *
 * 这张表是「手感」的核心：玩家靠比较器读数区分是脚步声还是方块变化。
 * 一旦分组整体偏移，所有振动装置的输出强度都会同时错 —— 但不会崩、不会报错，
 * 只是读数看起来「差不多」，极难靠手动测试发现。
 *
 * 这里踩过一次：整张表按自己的理解重新分了组（把 block_* 事件集中到低频），
 * 40 条里 35 条与原版不符。所以这张表必须逐条固定下来。
 */

#include <cstdio>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 有振动频率的游戏事件（裸路径 id）
constexpr const char* EVENTS[] = {
    "block_activate",
    "block_attach",
    "block_change",
    "block_close",
    "block_deactivate",
    "block_destroy",
    "block_detach",
    "block_open",
    "block_place",
    "container_close",
    "container_open",
    "drink",
    "eat",
    "elytra_glide",
    "entity_action",
    "entity_damage",
    "entity_die",
    "entity_dismount",
    "entity_interact",
    "entity_mount",
    "entity_place",
    "equip",
    "explode",
    "flap",
    "fluid_pickup",
    "fluid_place",
    "hit_ground",
    "instrument_play",
    "item_interact_finish",
    "lightning_strike",
    "note_block_play",
    "prime_fuse",
    "projectile_land",
    "projectile_shoot",
    "shear",
    "splash",
    "step",
    "swim",
    "teleport",
    "unequip",
};

void writeList(FILE* out)
{
    for (const char* id : EVENTS) {
        std::fprintf(out, "%s\n", id);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    // 单元名沿用原版常量名（大写），比较时统一转小写，
    // 这样 list 与 run 两侧不必各自维护一份大小写约定。
    std::string lowered = unit;
    for (char& ch : lowered) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }

    const char* matched = nullptr;
    for (const char* id : EVENTS) {
        if (lowered == id) {
            matched = id;
            break;
        }
    }
    if (matched == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    // GameEvent 的构造只接受 const char*，所以传表里那个字面量而不是 std::string
    const gameevent::GameEvent event(matched);
    json result{{"unit", unit}};
    result["frequency"] = gameevent::VibrationSystem::getGameEventFrequency(event);
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(vibration_frequency, &writeList, &runLine)

} // namespace mc::diff
