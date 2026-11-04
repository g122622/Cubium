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
 * @brief 差分能力：振动信息的 NBT 结构（`VibrationInfo.CODEC`）。
 *
 * `list`：字段组合（有无 source / 有无 projectile_owner）
 * `run` ：一行 -> { "gameEvent": id, "distanceBits": f32 位模式, "pos": [x,y,z],
 *                  "source": [4 个 int] | null, "projectileOwner": [4 个 int] | null }
 *
 * ## 字段
 *
 * 原版 `VibrationInfo.CODEC`：
 *
 * | 字段 | 类型 | 可空 |
 * |---|---|---|
 * | `game_event` | string | 否 |
 * | `distance` | float | 否 |
 * | `pos` | double 列表（3 项）| 否 |
 * | `source` | **UUID**（int 列表，4 项）| 是 |
 * | `projectile_owner` | **UUID**（int 列表，4 项）| 是 |
 *
 * ## 最容易写错的一处：UUID 不是 long
 *
 * 原版 `UUIDUtil.CODEC` 是 `Codec.INT_STREAM.comapFlatMap(fixedSize(4))`，
 * 即把 16 字节按大端拆成 **4 个有符号 int**。写成单个 `long` 会让存档与网络包
 * 都和原版对不上 —— 原版读不了 Cubium 存的、Cubium 也读不了原版存的。
 *
 * 这条在 Cubium 里确实写错过：`source` 原来是 `long`，`projectile_owner` 整个缺失。
 */

#include <bit>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

/// 单元：有无 source × 有无 projectile_owner
constexpr const char* UNITS[] = {
    "none",
    "source",
    "owner",
    "both",
};

/// 固定的测试 UUID：4 组 int 的正负号都覆盖到
constexpr const char* SOURCE_UUID = "0f0e0d0c-0b0a-0908-0706-050403020100";
constexpr const char* OWNER_UUID = "ffffffff-ffff-ffff-8000-000000000001";

void writeList(FILE* out)
{
    for (const char* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

/** 把 UUID 拆成原版那样的 4 个 int */
json uuidToJsonArray(const Uuid& uuid)
{
    json array = json::array();
    for (std::size_t group = 0; group < 4; ++group) {
        const u32 value = (static_cast<u32>(uuid[group * 4]) << 24) | (static_cast<u32>(uuid[group * 4 + 1]) << 16) |
            (static_cast<u32>(uuid[group * 4 + 2]) << 8) | static_cast<u32>(uuid[group * 4 + 3]);
        array.push_back(static_cast<i32>(value));
    }
    return array;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    bool withSource = false;
    bool withOwner = false;
    if (unit == "none") {
        // 都不带
    } else if (unit == "source") {
        withSource = true;
    } else if (unit == "owner") {
        withOwner = true;
    } else if (unit == "both") {
        withSource = true;
        withOwner = true;
    } else {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    gameevent::VibrationInfo info;
    info.gameEvent = &gameevent::GameEvents::BLOCK_CHANGE;
    info.distance = 12.5f;
    info.pos = Vector3d(1.25, 64.0, -3.75);
    if (withSource) {
        info.sourceUuid = util::uuidFromString(SOURCE_UUID);
    }
    if (withOwner) {
        info.projectileOwnerUuid = util::uuidFromString(OWNER_UUID);
    }

    nbt::CompoundTag tag;
    info.saveToNBT(tag);

    json result{{"unit", unit}};

    // game_event
    const auto eventIt = tag.value.find("game_event");
    result["gameEvent"] = (eventIt != tag.value.end() && eventIt->second != nullptr)
        ? json(std::string("minecraft:") + info.gameEvent->id())
        : json(nullptr);

    // distance（按位模式输出，避免十进制展开制造伪差异）
    const auto distIt = tag.value.find("distance");
    if (distIt != tag.value.end() && distIt->second != nullptr) {
        const auto* floatTag = dynamic_cast<const nbt::tags::float_tag*>(distIt->second.get());
        result["distanceBits"] = floatTag != nullptr ? json(std::bit_cast<u32>(floatTag->value)) : json(nullptr);
    } else {
        result["distanceBits"] = nullptr;
    }

    // pos
    json posJson = nullptr;
    const auto posIt = tag.value.find("pos");
    if (posIt != tag.value.end() && posIt->second != nullptr) {
        const auto* doubleList = dynamic_cast<const nbt::tags::double_list_tag*>(posIt->second.get());
        if (doubleList != nullptr) {
            posJson = json::array();
            for (const f64 value : doubleList->value) {
                posJson.push_back(value);
            }
        }
    }
    result["pos"] = posJson;

    // source / projectile_owner
    const auto readUuidField = [&tag](const char* key) -> json {
        const auto it = tag.value.find(key);
        if (it == tag.value.end() || it->second == nullptr) {
            return nullptr;
        }
        const auto* arrayTag = dynamic_cast<const nbt::tags::intarray_tag*>(it->second.get());
        if (arrayTag == nullptr) {
            return json("wrong-type");
        }
        json array = json::array();
        for (const i32 value : arrayTag->value) {
            array.push_back(value);
        }
        return array;
    };
    result["source"] = readUuidField("source");
    result["projectileOwner"] = readUuidField("projectile_owner");

    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("vibration_info_nbt", &writeList, &runLine)

} // namespace mc::diff
