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
 * @brief 差分能力：游戏事件的通知半径（`GameEvent.notificationRadius`）。
 *
 * `list`：每个事件一个单元
 * `run` ：一行 -> { "radius": i32 }
 *
 * ## 为什么值得比对
 *
 * 原版 {@code GameEvent} 是 `record GameEvent(int notificationRadius)`，60 个事件里
 * **57 个走默认半径 16**，只有三个显式不同：
 *
 * - `jukebox_play` / `jukebox_stop_play`：**10**
 * - `shriek`（监守者的尖啸）：**32**
 *
 * 半径决定事件能被多远的监听器收到——幽匿感测体、幽匿尖啸体、监守者、悦灵、
 * 铜傀儡这些「听声音」的机制全部依赖它。抄成默认 16 的后果不是崩溃，而是
 * **监守者的尖啸只惊动半张地图**、或者唱片机的声音传得比原版更远，
 * 这类偏差在游戏里几乎不可能靠手感发现。
 *
 * ## 同时验证事件集合本身
 *
 * 单元由 {@code allGameEventIds()} 生成，因此**事件名集合的差异会直接体现为
 * 单元缺失或多出**：少一个事件、或者 ID 拼错（例如把 `entity_mount` 写成
 * `entity_mounts`），原版侧会有这个单元而探针没有，报告里立刻可见。
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void writeList(FILE* out)
{
    for (const std::string& id : gameevent::GameEvents::allGameEventIds()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const gameevent::GameEvent* event = gameevent::GameEvents::getGameEventById(unit);
    if (event == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    return json{{"unit", unit}, {"radius", event->notificationRadius()}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("game_event_radius", &writeList, &runLine)

} // namespace mc::diff
