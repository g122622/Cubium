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
 * @brief 差分能力：振动选择器的 NBT 结构与往返（`VibrationSelector.CODEC`）。
 *
 * `list`：三种状态（无候选 / 有候选 / 只有 tick 没有 event）
 * `run` ：一行 -> 保存后的字段 + 往返后恢复出的内容
 *
 * ## 字段
 *
 * 原版 `VibrationSelector.CODEC` 只有两项：
 *
 * | 字段 | 类型 | 说明 |
 * |---|---|---|
 * | `event` | 嵌套复合（`VibrationInfo.CODEC`）| **可选** |
 * | `tick` | long | 无候选时写 **-1** |
 *
 * ## 两个容易写错的点
 *
 * 1. **无候选时 `tick` 必须写 -1，不能省略** —— 原版用 `orElse(-1L)`，
 *    字段存在但值是哨兵。省略字段会让「读取失败」和「没有候选」在语义上混起来。
 * 2. **`event` 存在但 `tick < 0` 时不能恢复候选** —— 这两个字段是一对，
 *    只信一个会把损坏的存档当成有效候选恢复出来。
 */

#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

constexpr const char* UNITS[] = {
    "empty",
    "candidate",
};

void writeList(FILE* out)
{
    for (const char* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

/// 读取 tag 里的 tick（原样，含哨兵 -1）
json readTick(const nbt::CompoundTag& tag)
{
    const auto it = tag.value.find("tick");
    if (it == tag.value.end() || it->second == nullptr) {
        return nullptr;
    }
    const auto* longTag = dynamic_cast<const nbt::tags::long_tag*>(it->second.get());
    return longTag != nullptr ? json(longTag->value) : json(nullptr);
}

/// 读取 tag 里嵌套 event 的关键字段
json readEvent(const nbt::CompoundTag& tag)
{
    const auto it = tag.value.find("event");
    if (it == tag.value.end() || it->second == nullptr) {
        return nullptr;
    }
    const auto* eventTag = dynamic_cast<const nbt::CompoundTag*>(it->second.get());
    if (eventTag == nullptr) {
        return json("wrong-type");
    }

    json out = json::object();

    const auto eventIt = eventTag->value.find("game_event");
    if (eventIt != eventTag->value.end() && eventIt->second != nullptr) {
        const auto* stringTag = dynamic_cast<const nbt::tags::string_tag*>(eventIt->second.get());
        out["gameEvent"] = stringTag != nullptr ? json(std::string("minecraft:") + stringTag->value) : json(nullptr);
    } else {
        out["gameEvent"] = nullptr;
    }

    const auto distIt = eventTag->value.find("distance");
    if (distIt != eventTag->value.end() && distIt->second != nullptr) {
        const auto* floatTag = dynamic_cast<const nbt::tags::float_tag*>(distIt->second.get());
        out["distance"] = floatTag != nullptr ? json(floatTag->value) : json(nullptr);
    } else {
        out["distance"] = nullptr;
    }

    return out;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    gameevent::VibrationSelector selector;
    if (unit == "candidate") {
        gameevent::VibrationInfo info;
        info.gameEvent = &gameevent::GameEvents::SHEAR;
        info.distance = 6.5f;
        info.pos = Vector3d(2.0, 64.0, 3.0);
        selector.addCandidate(info, 4200);
    } else if (unit != "empty") {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    nbt::CompoundTag tag;
    selector.saveToNBT(tag);

    json result{{"unit", unit}};
    result["savedTick"] = readTick(tag);
    result["savedEvent"] = readEvent(tag);
    // 字段是否存在，用来确认「无候选时是否仍写 tick」
    result["hasTickField"] = tag.value.find("tick") != tag.value.end();
    result["hasEventField"] = tag.value.find("event") != tag.value.end();

    // 往返
    gameevent::VibrationSelector loaded;
    loaded.loadFromNBT(tag);
    const auto chosen = loaded.chosenCandidate(5000);
    if (chosen.has_value() && chosen->gameEvent != nullptr) {
        result["roundTripEvent"] = std::string("minecraft:") + chosen->gameEvent->id();
        result["roundTripDistance"] = chosen->distance;
    } else {
        result["roundTripEvent"] = nullptr;
        result["roundTripDistance"] = nullptr;
    }

    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(vibration_selector_nbt, &writeList, &runLine)

} // namespace mc::diff
