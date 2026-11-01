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
 * @brief 差分能力：方块状态枚举的字符串双向映射。
 *
 * 覆盖 5 个方块状态属性枚举：
 * `AttachFace` / `BedPart` / `ChestType` / `DoubleBlockHalf` / `Half`。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`EnumProperty<T>::Traits::toString` / `fromName`，见 `common/util/property/EnumProperty.cpp`）。
 *
 * 这些字符串会写进方块状态字符串（如 `minecraft:oak_bed[part=head,facing=north]`）
 * 与 `block_state_ids` 的调色板，因此名字错会导致**存档与网络不兼容**。
 *
 * 原版侧：
 *
 * ```java
 * public enum AttachFace implements StringRepresentable {
 *     FLOOR("floor"), WALL("wall"), CEILING("ceiling");
 * }
 * public enum BedPart implements StringRepresentable {
 *     HEAD("head"), FOOT("foot");
 * }
 * public enum ChestType implements StringRepresentable {
 *     SINGLE("single"), LEFT("left"), RIGHT("right");
 * }
 * public enum DoubleBlockHalf implements StringRepresentable {
 *     UPPER("upper"), LOWER("lower");
 * }
 * public enum Half implements StringRepresentable {
 *     TOP("top"), BOTTOM("bottom");
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **`Half` 与 `DoubleBlockHalf` 是两个不同的枚举。**
 *
 *    | 枚举 | 项 |
 *    |---|---|
 *    | `Half` | `TOP("top")` / `BOTTOM("bottom")` |
 *    | `DoubleBlockHalf` | `UPPER("upper")` / `LOWER("lower")` |
 *
 *    **`Half` 用 top/bottom，`DoubleBlockHalf` 用 upper/lower**——名字完全不同，
 *    混用会让所有楼梯与高植物的方块状态对不上。
 *
 * 2. **`ChestType` 的 `LEFT` / `RIGHT` 是从玩家视角还是箱子视角？**
 *
 *    序列化名就是 `left` / `right`，本能力只验证名字，不验证朝向语义。
 *
 * 3. **所有名字都是单个小写单词，无下划线、无驼峰。**
 *
 * 单元格式：
 * - `<enum>|toString|<VALUE>` —— 序列化名
 * - `<enum>|fromName|<text>` —— 严格解析（未命中输出 `null`）
 * - `count|<enum>` —— 项数
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

using nlohmann::json;

// 这五个枚举是 mc::BlockStateProperties 类的嵌套成员。
using mc::BlockStateProperties;

template <typename EnumT>
struct EnumRef {
    const char* javaName;
    EnumT value;
};

constexpr EnumRef<BlockStateProperties::AttachFace> ATTACH_FACES[] = {
    {"FLOOR", BlockStateProperties::AttachFace::Floor},
    {"WALL", BlockStateProperties::AttachFace::Wall},
    {"CEILING", BlockStateProperties::AttachFace::Ceiling},
};

constexpr EnumRef<BlockStateProperties::BedPart> BED_PARTS[] = {
    {"HEAD", BlockStateProperties::BedPart::Head},
    {"FOOT", BlockStateProperties::BedPart::Foot},
};

constexpr EnumRef<BlockStateProperties::ChestType> CHEST_TYPES[] = {
    {"SINGLE", BlockStateProperties::ChestType::Single},
    {"LEFT", BlockStateProperties::ChestType::Left},
    {"RIGHT", BlockStateProperties::ChestType::Right},
};

constexpr EnumRef<BlockStateProperties::DoubleBlockHalf> DOUBLE_BLOCK_HALVES[] = {
    {"UPPER", BlockStateProperties::DoubleBlockHalf::Upper},
    {"LOWER", BlockStateProperties::DoubleBlockHalf::Lower},
};

constexpr EnumRef<BlockStateProperties::Half> HALVES[] = {
    {"TOP", BlockStateProperties::Half::Top},
    {"BOTTOM", BlockStateProperties::Half::Bottom},
};

constexpr i32 ATTACH_FACE_COUNT = 3;
constexpr i32 BED_PART_COUNT = 2;
constexpr i32 CHEST_TYPE_COUNT = 3;
constexpr i32 DOUBLE_BLOCK_HALF_COUNT = 2;
constexpr i32 HALF_COUNT = 2;

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        "count|AttachFace",
        "count|BedPart",
        "count|ChestType",
        "count|DoubleBlockHalf",
        "count|Half",
        // AttachFace
        "AttachFace|toString|FLOOR",
        "AttachFace|toString|WALL",
        "AttachFace|toString|CEILING",
        "AttachFace|fromName|floor",
        "AttachFace|fromName|ceiling",
        "AttachFace|fromName|dunno",
        "AttachFace|fromName|FLOOR",
        // BedPart
        "BedPart|toString|HEAD",
        "BedPart|toString|FOOT",
        "BedPart|fromName|head",
        "BedPart|fromName|foot",
        "BedPart|fromName|dunno",
        // ChestType
        "ChestType|toString|SINGLE",
        "ChestType|toString|LEFT",
        "ChestType|toString|RIGHT",
        "ChestType|fromName|single",
        "ChestType|fromName|left",
        "ChestType|fromName|right",
        "ChestType|fromName|dunno",
        // DoubleBlockHalf：注意是 upper/lower，不是 top/bottom
        "DoubleBlockHalf|toString|UPPER",
        "DoubleBlockHalf|toString|LOWER",
        "DoubleBlockHalf|fromName|upper",
        "DoubleBlockHalf|fromName|lower",
        "DoubleBlockHalf|fromName|top",
        // Half：注意是 top/bottom，不是 upper/lower
        "Half|toString|TOP",
        "Half|toString|BOTTOM",
        "Half|fromName|top",
        "Half|fromName|bottom",
        "Half|fromName|upper",
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

/// 统一的转字符串入口。
std::string enumToString(const std::string& enumName, const std::string& valueName, bool& found)
{
    found = true;
    if (enumName == "AttachFace") {
        const auto* entry = findEnum(ATTACH_FACES, valueName);
        if (entry == nullptr) {
            found = false;
            return {};
        }
        return EnumProperty<BlockStateProperties::AttachFace>::Traits::toString(entry->value);
    }
    if (enumName == "BedPart") {
        const auto* entry = findEnum(BED_PARTS, valueName);
        if (entry == nullptr) {
            found = false;
            return {};
        }
        return EnumProperty<BlockStateProperties::BedPart>::Traits::toString(entry->value);
    }
    if (enumName == "ChestType") {
        const auto* entry = findEnum(CHEST_TYPES, valueName);
        if (entry == nullptr) {
            found = false;
            return {};
        }
        return EnumProperty<BlockStateProperties::ChestType>::Traits::toString(entry->value);
    }
    if (enumName == "DoubleBlockHalf") {
        const auto* entry = findEnum(DOUBLE_BLOCK_HALVES, valueName);
        if (entry == nullptr) {
            found = false;
            return {};
        }
        return EnumProperty<BlockStateProperties::DoubleBlockHalf>::Traits::toString(entry->value);
    }
    if (enumName == "Half") {
        const auto* entry = findEnum(HALVES, valueName);
        if (entry == nullptr) {
            found = false;
            return {};
        }
        return EnumProperty<BlockStateProperties::Half>::Traits::toString(entry->value);
    }
    found = false;
    return {};
}

/// 统一的反向解析入口；未命中返回 "null"。
std::string enumFromName(const std::string& enumName, const std::string& text, bool& known)
{
    known = true;
    if (enumName == "AttachFace") {
        const auto parsed = EnumProperty<BlockStateProperties::AttachFace>::Traits::fromName(text);
        return parsed ? javaNameOf(ATTACH_FACES, *parsed) : "null";
    }
    if (enumName == "BedPart") {
        const auto parsed = EnumProperty<BlockStateProperties::BedPart>::Traits::fromName(text);
        return parsed ? javaNameOf(BED_PARTS, *parsed) : "null";
    }
    if (enumName == "ChestType") {
        const auto parsed = EnumProperty<BlockStateProperties::ChestType>::Traits::fromName(text);
        return parsed ? javaNameOf(CHEST_TYPES, *parsed) : "null";
    }
    if (enumName == "DoubleBlockHalf") {
        const auto parsed = EnumProperty<BlockStateProperties::DoubleBlockHalf>::Traits::fromName(text);
        return parsed ? javaNameOf(DOUBLE_BLOCK_HALVES, *parsed) : "null";
    }
    if (enumName == "Half") {
        const auto parsed = EnumProperty<BlockStateProperties::Half>::Traits::fromName(text);
        return parsed ? javaNameOf(HALVES, *parsed) : "null";
    }
    known = false;
    return {};
}

i32 enumCount(const std::string& enumName, bool& known)
{
    known = true;
    if (enumName == "AttachFace") {
        return ATTACH_FACE_COUNT;
    }
    if (enumName == "BedPart") {
        return BED_PART_COUNT;
    }
    if (enumName == "ChestType") {
        return CHEST_TYPE_COUNT;
    }
    if (enumName == "DoubleBlockHalf") {
        return DOUBLE_BLOCK_HALF_COUNT;
    }
    if (enumName == "Half") {
        return HALF_COUNT;
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
        throw std::runtime_error("block_state_enum_strings: 单元段数不足：" + unit);
    }

    std::string value;
    bool known = false;

    if (parts[0] == "count" && parts.size() == 2) {
        const i32 count = enumCount(parts[1], known);
        if (known) {
            value = std::to_string(count);
        }
    } else if (parts.size() == 3 && parts[1] == "toString") {
        bool found = false;
        value = enumToString(parts[0], parts[2], found);
        if (!found) {
            throw std::runtime_error("block_state_enum_strings: 未知枚举或值：" + unit);
        }
        known = true;
    } else if (parts.size() == 3 && parts[1] == "fromName") {
        value = enumFromName(parts[0], parts[2], known);
    }

    if (!known) {
        throw std::runtime_error("block_state_enum_strings: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_state_enum_strings", &writeList, &runLine)

} // namespace mc::diff
