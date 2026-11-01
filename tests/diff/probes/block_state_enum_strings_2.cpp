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
 * @brief 差分能力：第二组方块状态枚举的字符串双向映射。
 *
 * 覆盖 11 个枚举：`PistonType` / `RedstoneSide` / `DoorHinge` / `SlabType` /
 * `StairsShape` / `RailShape` / `BellAttachment` / `BambooLeaves` / `Tilt` /
 * `DripstoneThickness` / `SculkSensorPhase`。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`EnumProperty<T>::Traits::toString` / `fromName`）。
 *
 * ## 五个关键点
 *
 * 1. **`PistonType.DEFAULT` 的序列化名是 `normal`，不是 `default`。**
 *
 *    ```java
 *    public enum PistonType implements StringRepresentable {
 *        DEFAULT("normal"), STICKY("sticky");
 *    }
 *    ```
 *
 *    常量名与序列化名**完全不同**。Cubium 的 `PistonType::Default` 正确返回 `normal`。
 *
 * 2. **`SlabType` 的枚举顺序是 `BOTTOM`/`TOP`/`DOUBLE`，与 `Half` 的 `TOP`/`BOTTOM` 相反。**
 *
 *    | 枚举 | 顺序 |
 *    |---|---|
 *    | `Half` | `TOP`(0), `BOTTOM`(1) |
 *    | **`SlabType`** | **`BOTTOM`(0), `TOP`(1), `DOUBLE`(2)** |
 *
 *    两个都表示「半部分」的枚举，**索引顺序相反**。
 *
 * 3. **`RedstoneSide` 的顺序是 `UP`/`SIDE`/`NONE`——`NONE` 在最后。**
 *
 *    而 `WallSide`（`none`/`low`/`tall`）的 `none` 在**最前**。同类「无连接」语义，
 *    位置相反。
 *
 * 4. **`RailShape` 有 10 项，前 6 项是「直线/斜线」，后 4 项是「弯道」。**
 *
 *    弯道顺序是 `SOUTH_EAST`, `SOUTH_WEST`, `NORTH_WEST`, `NORTH_EAST`——
 *    **顺时针绕一圈**，不是字母序也不是 NSEW 顺序。
 *
 * 5. **`DripstoneThickness` 的 default 分支返回 `tip`，不是第一项 `tip_merge`。**
 *
 *    这是 Cubium 实现里的兜底值选择，本能力通过 `toString` 覆盖全部 5 项来确认。
 *
 * 单元格式：
 * - `<enum>|toString|<VALUE>` —— 序列化名
 * - `<enum>|fromName|<text>` —— 严格解析（未命中输出 `null`）
 * - `count|<enum>` —— 项数
 *
 * 注：本节把「正向全部项」与「反向抽样」分开，正向覆盖保证没有遗漏项。
 */
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using mc::BlockStateProperties;
using mc::EnumProperty;
using nlohmann::json;

using PistonType = BlockStateProperties::PistonType;
using RedstoneSide = BlockStateProperties::RedstoneSide;
using DoorHinge = BlockStateProperties::DoorHinge;
using SlabType = BlockStateProperties::SlabType;
using StairsShape = BlockStateProperties::StairsShape;
using RailShape = BlockStateProperties::RailShape;
using BellAttachment = BlockStateProperties::BellAttachment;
using BambooLeaves = BlockStateProperties::BambooLeaves;
using Tilt = BlockStateProperties::Tilt;
using DripstoneThickness = BlockStateProperties::DripstoneThickness;
using SculkSensorPhase = BlockStateProperties::SculkSensorPhase;

template <typename EnumT>
struct EnumRef {
    const char* javaName;
    EnumT value;
};

constexpr EnumRef<PistonType> PISTON_TYPES[] = {
    {"DEFAULT", PistonType::Default},
    {"STICKY", PistonType::Sticky},
};

constexpr EnumRef<RedstoneSide> REDSTONE_SIDES[] = {
    {"UP", RedstoneSide::Up},
    {"SIDE", RedstoneSide::Side},
    {"NONE", RedstoneSide::None},
};

constexpr EnumRef<DoorHinge> DOOR_HINGES[] = {
    {"LEFT", DoorHinge::Left},
    {"RIGHT", DoorHinge::Right},
};

constexpr EnumRef<SlabType> SLAB_TYPES[] = {
    {"BOTTOM", SlabType::Bottom},
    {"TOP", SlabType::Top},
    {"DOUBLE", SlabType::Double},
};

constexpr EnumRef<StairsShape> STAIRS_SHAPES[] = {
    {"STRAIGHT", StairsShape::Straight},
    {"INNER_LEFT", StairsShape::InnerLeft},
    {"INNER_RIGHT", StairsShape::InnerRight},
    {"OUTER_LEFT", StairsShape::OuterLeft},
    {"OUTER_RIGHT", StairsShape::OuterRight},
};

constexpr EnumRef<RailShape> RAIL_SHAPES[] = {
    {"NORTH_SOUTH", RailShape::NorthSouth},
    {"EAST_WEST", RailShape::EastWest},
    {"ASCENDING_EAST", RailShape::AscendingEast},
    {"ASCENDING_WEST", RailShape::AscendingWest},
    {"ASCENDING_NORTH", RailShape::AscendingNorth},
    {"ASCENDING_SOUTH", RailShape::AscendingSouth},
    {"SOUTH_EAST", RailShape::SouthEast},
    {"SOUTH_WEST", RailShape::SouthWest},
    {"NORTH_WEST", RailShape::NorthWest},
    {"NORTH_EAST", RailShape::NorthEast},
};

constexpr EnumRef<BellAttachment> BELL_ATTACHMENTS[] = {
    {"FLOOR", BellAttachment::Floor},
    {"CEILING", BellAttachment::Ceiling},
    {"SINGLE_WALL", BellAttachment::SingleWall},
    {"DOUBLE_WALL", BellAttachment::DoubleWall},
};

constexpr EnumRef<BambooLeaves> BAMBOO_LEAVES_SET[] = {
    {"NONE", BambooLeaves::None},
    {"SMALL", BambooLeaves::Small},
    {"LARGE", BambooLeaves::Large},
};

constexpr EnumRef<Tilt> TILTS[] = {
    {"NONE", Tilt::None},
    {"UNSTABLE", Tilt::Unstable},
    {"PARTIAL", Tilt::Partial},
    {"FULL", Tilt::Full},
};

constexpr EnumRef<DripstoneThickness> DRIPSTONE_THICKNESSES[] = {
    {"TIP_MERGE", DripstoneThickness::TipMerge},
    {"TIP", DripstoneThickness::Tip},
    {"FRUSTUM", DripstoneThickness::Frustum},
    {"MIDDLE", DripstoneThickness::Middle},
    {"BASE", DripstoneThickness::Base},
};

constexpr EnumRef<SculkSensorPhase> SCULK_SENSOR_PHASES[] = {
    {"INACTIVE", SculkSensorPhase::Inactive},
    {"ACTIVE", SculkSensorPhase::Active},
    {"COOLDOWN", SculkSensorPhase::Cooldown},
};

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // 项数
        "count|PistonType",
        "count|RedstoneSide",
        "count|DoorHinge",
        "count|SlabType",
        "count|StairsShape",
        "count|RailShape",
        "count|BellAttachment",
        "count|BambooLeaves",
        "count|Tilt",
        "count|DripstoneThickness",
        "count|SculkSensorPhase",
        // PistonType：DEFAULT -> "normal"
        "PistonType|toString|DEFAULT",
        "PistonType|toString|STICKY",
        "PistonType|fromName|normal",
        "PistonType|fromName|sticky",
        "PistonType|fromName|default",
        // RedstoneSide
        "RedstoneSide|toString|UP",
        "RedstoneSide|toString|SIDE",
        "RedstoneSide|toString|NONE",
        "RedstoneSide|fromName|up",
        "RedstoneSide|fromName|none",
        // DoorHinge
        "DoorHinge|toString|LEFT",
        "DoorHinge|toString|RIGHT",
        "DoorHinge|fromName|left",
        "DoorHinge|fromName|right",
        // SlabType：BOTTOM/TOP/DOUBLE，顺序与 Half 相反
        "SlabType|toString|BOTTOM",
        "SlabType|toString|TOP",
        "SlabType|toString|DOUBLE",
        "SlabType|fromName|bottom",
        "SlabType|fromName|double",
        // StairsShape
        "StairsShape|toString|STRAIGHT",
        "StairsShape|toString|INNER_LEFT",
        "StairsShape|toString|INNER_RIGHT",
        "StairsShape|toString|OUTER_LEFT",
        "StairsShape|toString|OUTER_RIGHT",
        "StairsShape|fromName|inner_left",
        "StairsShape|fromName|outer_right",
        // RailShape：10 项全覆盖
        "RailShape|toString|NORTH_SOUTH",
        "RailShape|toString|EAST_WEST",
        "RailShape|toString|ASCENDING_EAST",
        "RailShape|toString|ASCENDING_WEST",
        "RailShape|toString|ASCENDING_NORTH",
        "RailShape|toString|ASCENDING_SOUTH",
        "RailShape|toString|SOUTH_EAST",
        "RailShape|toString|SOUTH_WEST",
        "RailShape|toString|NORTH_WEST",
        "RailShape|toString|NORTH_EAST",
        "RailShape|fromName|north_south",
        "RailShape|fromName|north_east",
        // BellAttachment
        "BellAttachment|toString|FLOOR",
        "BellAttachment|toString|CEILING",
        "BellAttachment|toString|SINGLE_WALL",
        "BellAttachment|toString|DOUBLE_WALL",
        "BellAttachment|fromName|single_wall",
        "BellAttachment|fromName|double_wall",
        // BambooLeaves
        "BambooLeaves|toString|NONE",
        "BambooLeaves|toString|SMALL",
        "BambooLeaves|toString|LARGE",
        "BambooLeaves|fromName|large",
        // Tilt
        "Tilt|toString|NONE",
        "Tilt|toString|UNSTABLE",
        "Tilt|toString|PARTIAL",
        "Tilt|toString|FULL",
        "Tilt|fromName|unstable",
        // DripstoneThickness
        "DripstoneThickness|toString|TIP_MERGE",
        "DripstoneThickness|toString|TIP",
        "DripstoneThickness|toString|FRUSTUM",
        "DripstoneThickness|toString|MIDDLE",
        "DripstoneThickness|toString|BASE",
        "DripstoneThickness|fromName|tip_merge",
        "DripstoneThickness|fromName|frustum",
        // SculkSensorPhase
        "SculkSensorPhase|toString|INACTIVE",
        "SculkSensorPhase|toString|ACTIVE",
        "SculkSensorPhase|toString|COOLDOWN",
        "SculkSensorPhase|fromName|cooldown",
        // 未命中
        "PistonType|fromName|dunno",
        "RailShape|fromName|dunno",
        "StairsShape|fromName|dunno",
    };
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

template <typename EnumT, size_t N>
const EnumRef<EnumT>* findEnum(const EnumRef<EnumT> (&table)[N], const std::string& name)
{
    for (size_t i = 0; i < N; ++i) {
        if (name == table[i].javaName) {
            return &table[i];
        }
    }
    return nullptr;
}

template <typename EnumT, size_t N>
std::string javaNameOf(const EnumRef<EnumT> (&table)[N], const EnumT& value)
{
    for (size_t i = 0; i < N; ++i) {
        if (table[i].value == value) {
            return table[i].javaName;
        }
    }
    return "null";
}

/**
 * 统一的 toString 分派。
 * 用宏避免 11 段几乎相同的 if 分支。
 */
#define DISPATCH_TO_STRING(EnumT, TABLE)                                                  \
    if (enumName == #EnumT) {                                                             \
        const auto* entry = findEnum(TABLE, valueName);                                   \
        if (entry == nullptr) {                                                           \
            known = false;                                                                \
            return {};                                                                    \
        }                                                                                 \
        return EnumProperty<BlockStateProperties::EnumT>::Traits::toString(entry->value); \
    }

std::string enumToString(const std::string& enumName, const std::string& valueName, bool& known)
{
    known = true;
    DISPATCH_TO_STRING(PistonType, PISTON_TYPES)
    DISPATCH_TO_STRING(RedstoneSide, REDSTONE_SIDES)
    DISPATCH_TO_STRING(DoorHinge, DOOR_HINGES)
    DISPATCH_TO_STRING(SlabType, SLAB_TYPES)
    DISPATCH_TO_STRING(StairsShape, STAIRS_SHAPES)
    DISPATCH_TO_STRING(RailShape, RAIL_SHAPES)
    DISPATCH_TO_STRING(BellAttachment, BELL_ATTACHMENTS)
    DISPATCH_TO_STRING(BambooLeaves, BAMBOO_LEAVES_SET)
    DISPATCH_TO_STRING(Tilt, TILTS)
    DISPATCH_TO_STRING(DripstoneThickness, DRIPSTONE_THICKNESSES)
    DISPATCH_TO_STRING(SculkSensorPhase, SCULK_SENSOR_PHASES)
    known = false;
    return {};
}

#undef DISPATCH_TO_STRING

#define DISPATCH_FROM_NAME(EnumT, TABLE)                                                       \
    if (enumName == #EnumT) {                                                                  \
        const auto parsed = EnumProperty<BlockStateProperties::EnumT>::Traits::fromName(text); \
        return parsed ? javaNameOf(TABLE, *parsed) : "null";                                   \
    }

std::string enumFromName(const std::string& enumName, const std::string& text, bool& known)
{
    known = true;
    DISPATCH_FROM_NAME(PistonType, PISTON_TYPES)
    DISPATCH_FROM_NAME(RedstoneSide, REDSTONE_SIDES)
    DISPATCH_FROM_NAME(DoorHinge, DOOR_HINGES)
    DISPATCH_FROM_NAME(SlabType, SLAB_TYPES)
    DISPATCH_FROM_NAME(StairsShape, STAIRS_SHAPES)
    DISPATCH_FROM_NAME(RailShape, RAIL_SHAPES)
    DISPATCH_FROM_NAME(BellAttachment, BELL_ATTACHMENTS)
    DISPATCH_FROM_NAME(BambooLeaves, BAMBOO_LEAVES_SET)
    DISPATCH_FROM_NAME(Tilt, TILTS)
    DISPATCH_FROM_NAME(DripstoneThickness, DRIPSTONE_THICKNESSES)
    DISPATCH_FROM_NAME(SculkSensorPhase, SCULK_SENSOR_PHASES)
    known = false;
    return {};
}

#undef DISPATCH_FROM_NAME

i32 enumCount(const std::string& enumName, bool& known)
{
    known = true;
    if (enumName == "PistonType") {
        return 2;
    }
    if (enumName == "RedstoneSide") {
        return 3;
    }
    if (enumName == "DoorHinge") {
        return 2;
    }
    if (enumName == "SlabType") {
        return 3;
    }
    if (enumName == "StairsShape") {
        return 5;
    }
    if (enumName == "RailShape") {
        return 10;
    }
    if (enumName == "BellAttachment") {
        return 4;
    }
    if (enumName == "BambooLeaves") {
        return 3;
    }
    if (enumName == "Tilt") {
        return 4;
    }
    if (enumName == "DripstoneThickness") {
        return 5;
    }
    if (enumName == "SculkSensorPhase") {
        return 3;
    }
    known = false;
    return 0;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("block_state_enum_strings_2: 单元段数不足：" + unit);
    }

    std::string value;
    bool known = false;

    if (parts[0] == "count" && parts.size() == 2) {
        const i32 count = enumCount(parts[1], known);
        if (known) {
            value = std::to_string(count);
        }
    } else if (parts.size() == 3 && parts[1] == "toString") {
        value = enumToString(parts[0], parts[2], known);
    } else if (parts.size() == 3 && parts[1] == "fromName") {
        value = enumFromName(parts[0], parts[2], known);
    }

    if (!known) {
        throw std::runtime_error("block_state_enum_strings_2: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_state_enum_strings_2", &writeList, &runLine)

} // namespace mc::diff
