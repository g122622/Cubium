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
 * @brief 差分能力：`BooleanProperty` 的内部值顺序（`getInternalIndex`）。
 *
 * **本探针调用 Cubium 的真实实现**（`BooleanProperty::allowedValues()` /
 * `Property::indexOf` / `Property::valueAt`，见 `common/util/property/BooleanProperty.hpp`
 * 与 `Property.hpp`）。
 *
 * ## 为什么这是一个真缺陷候选
 *
 * 原版：
 *
 * ```java
 * public final class BooleanProperty extends Property<Boolean> {
 *     private static final List<Boolean> VALUES = List.of(true, false);   // ← true 在前
 *     private static final int TRUE_INDEX = 0;
 *     private static final int FALSE_INDEX = 1;
 *
 *     @Override
 *     public int getInternalIndex(Boolean b) {
 *         return b ? TRUE_INDEX : FALSE_INDEX;      // true -> 0, false -> 1
 *     }
 * }
 * ```
 *
 * 字节码确认（`javap -c`）：
 *
 * ```
 * public int getInternalIndex(java.lang.Boolean);
 *   0: aload_1
 *   1: invokevirtual  Boolean.booleanValue:()Z
 *   4: ifeq           11
 *   7: iconst_0                     // true  -> 0
 *   8: goto           12
 *  11: iconst_1                     // false -> 1
 *  12: ireturn
 * ```
 *
 * Cubium：
 *
 * ```cpp
 * explicit BooleanProperty(const std::string& name)
 *     : Property<bool>(name, {false, true})      // ← false 在前
 * {}
 * ```
 *
 * `Property::indexOf` 用 `m_valueToIndex` 查表，该表按 `m_values` 的顺序构建，于是：
 *
 * | 值 | 原版索引 | Cubium 索引 |
 * |---|---|---|
 * | `true` | **0** | **1** |
 * | `false` | **1** | **0** |
 *
 * ## 影响面
 *
 * `getInternalIndex` 决定布尔属性在**方块状态调色板**里的偏移：
 *
 * ```
 * 多属性状态索引 = ... + prop1.getInternalIndex(v1) * stride1 + ...
 * ```
 *
 * Cubium 与 Java 版服务端对同一世界的 `block_state_ids` 调色板索引会**按布尔属性数量成比例偏移**，
 * 导致：
 * - 网络包里的方块状态 ID 与 Java 客户端解释不一致；
 * - 存档里的 `block_states` 调色板与实际方块对不上。
 *
 * **本项目仅支持 Java 版客户端连接**，因此这是互通缺陷。
 *
 * 单元格式：
 * - `values|<property>` —— `allowedValues()` 按索引顺序的逗号拼接（`true`/`false`）
 * - `index|<property>|<value>` —— `indexOf(value)`
 * - `at|<property>|<n>` —— `valueAt(n)` 的字符串形式
 * - `count|<property>` —— 值个数
 * - `name|<property>` —— 属性名（`getName()`）
 *
 * 目前只覆盖 `BooleanProperty`，因为它是唯一原版把索引**硬编码**（而非按 `values()` 顺序）
 * 的属性类型。
 */
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/Properties.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using mc::BlockStateProperties;
using nlohmann::json;

struct BoolPropRef {
    const char* name;
    const mc::BooleanProperty& (*accessor)();
};

/// 覆盖 Cubium 预定义的全部布尔属性（与 Properties.hpp 中的访问器一一对应）。
constexpr BoolPropRef BOOL_PROPERTIES[] = {
    {"attached", &BlockStateProperties::ATTACHED},
    {"berries", &BlockStateProperties::BERRIES},
    {"bloom", &BlockStateProperties::BLOOM},
    {"bottom", &BlockStateProperties::BOTTOM},
    {"can_summon", &BlockStateProperties::CAN_SUMMON},
    {"conditional", &BlockStateProperties::CONDITIONAL},
    {"cracked", &BlockStateProperties::CRACKED},
    {"crafting", &BlockStateProperties::CRAFTING},
    {"disarmed", &BlockStateProperties::DISARMED},
    {"down", &BlockStateProperties::DOWN},
    {"drag", &BlockStateProperties::DRAG},
    {"east", &BlockStateProperties::EAST},
    {"enabled", &BlockStateProperties::ENABLED},
    {"extended", &BlockStateProperties::EXTENDED},
    {"eye", &BlockStateProperties::EYE},
    {"falling", &BlockStateProperties::FALLING},
    {"hanging", &BlockStateProperties::HANGING},
    {"has_book", &BlockStateProperties::HAS_BOOK},
    {"has_bottle_0", &BlockStateProperties::HAS_BOTTLE_0},
    {"has_bottle_1", &BlockStateProperties::HAS_BOTTLE_1},
    {"has_bottle_2", &BlockStateProperties::HAS_BOTTLE_2},
    {"has_record", &BlockStateProperties::HAS_RECORD},
    {"in_wall", &BlockStateProperties::IN_WALL},
    {"inverted", &BlockStateProperties::INVERTED},
    {"lit", &BlockStateProperties::LIT},
    {"locked", &BlockStateProperties::LOCKED},
    {"natural", &BlockStateProperties::NATURAL},
    {"north", &BlockStateProperties::NORTH},
    {"occupied", &BlockStateProperties::OCCUPIED},
    {"ominous", &BlockStateProperties::OMINOUS},
    {"open", &BlockStateProperties::OPEN},
    {"persistent", &BlockStateProperties::PERSISTENT},
    {"powered", &BlockStateProperties::POWERED},
    {"short", &BlockStateProperties::SHORT},
    {"shrieking", &BlockStateProperties::SHRIEKING},
    {"signal_fire", &BlockStateProperties::SIGNAL_FIRE},
    {"slot_0_occupied", &BlockStateProperties::SLOT_0_OCCUPIED},
    {"slot_1_occupied", &BlockStateProperties::SLOT_1_OCCUPIED},
    {"slot_2_occupied", &BlockStateProperties::SLOT_2_OCCUPIED},
    {"slot_3_occupied", &BlockStateProperties::SLOT_3_OCCUPIED},
    {"slot_4_occupied", &BlockStateProperties::SLOT_4_OCCUPIED},
    {"slot_5_occupied", &BlockStateProperties::SLOT_5_OCCUPIED},
    {"snowy", &BlockStateProperties::SNOWY},
    {"south", &BlockStateProperties::SOUTH},
    {"tip", &BlockStateProperties::TIP},
    {"triggered", &BlockStateProperties::TRIGGERED},
    {"unstable", &BlockStateProperties::UNSTABLE},
    {"up", &BlockStateProperties::UP},
    {"waterlogged", &BlockStateProperties::WATERLOGGED},
    {"west", &BlockStateProperties::WEST},
};

const BoolPropRef* findBoolProp(const std::string& name)
{
    for (const BoolPropRef& entry : BOOL_PROPERTIES) {
        if (name == entry.name) {
            return &entry;
        }
    }
    return nullptr;
}

void writeList(FILE* out)
{
    for (const BoolPropRef& entry : BOOL_PROPERTIES) {
        std::fprintf(out, "values|%s\n", entry.name);
        std::fprintf(out, "count|%s\n", entry.name);
        std::fprintf(out, "name|%s\n", entry.name);
        std::fprintf(out, "index|%s|true\n", entry.name);
        std::fprintf(out, "index|%s|false\n", entry.name);
        std::fprintf(out, "at|%s|0\n", entry.name);
        std::fprintf(out, "at|%s|1\n", entry.name);
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
    if (parts.size() < 2) {
        throw std::runtime_error("boolean_property_index: 单元段数不足：" + unit);
    }

    const std::string mode = parts[0];
    const BoolPropRef* ref = findBoolProp(parts[1]);
    if (ref == nullptr) {
        throw std::runtime_error("boolean_property_index: 未知布尔属性：" + parts[1]);
    }
    const mc::BooleanProperty& prop = ref->accessor();

    std::string value;

    if (mode == "values" && parts.size() == 2) {
        // 按索引顺序拼接，用于直接观察顺序
        std::string joined;
        const auto& values = prop.allowedValues();
        for (size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                joined.push_back(',');
            }
            joined += values[i] ? "true" : "false";
        }
        value = joined;
    } else if (mode == "count" && parts.size() == 2) {
        value = std::to_string(prop.allowedValues().size());
    } else if (mode == "name" && parts.size() == 2) {
        value = prop.name();
    } else if (mode == "index" && parts.size() == 3) {
        if (parts[2] != "true" && parts[2] != "false") {
            throw std::runtime_error("boolean_property_index: 非布尔值：" + parts[2]);
        }
        const bool target = parts[2] == "true";
        const auto index = prop.indexOf(target);
        value = index ? std::to_string(*index) : "null";
    } else if (mode == "at" && parts.size() == 3) {
        const size_t index = static_cast<size_t>(std::stoull(parts[2]));
        if (index >= prop.allowedValues().size()) {
            value = "null";
        } else {
            value = prop.valueToString(index);
        }
    } else {
        throw std::runtime_error("boolean_property_index: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(boolean_property_index, &writeList, &runLine)

} // namespace mc::diff
