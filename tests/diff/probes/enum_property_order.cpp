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
 * @brief 差分能力：`EnumProperty` 的值顺序（`getAllValues` / `cycle`）。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`EnumProperty<T>::allowedValues()` / `Property::indexOf` / `StateHolder::cycle`）。
 *
 * ## 为什么值得测
 *
 * 上一轮的 [`boolean_property_index`](./boolean_property_index.md) 证明：布尔属性的索引顺序
 * 差异**不影响外部互通**，因为网络层按「状态名」映射。
 *
 * **枚举属性同理——但多了一个语义维度：`cycle()`。**
 *
 * 原版：
 *
 * ```java
 * public <T extends Comparable<T>> S cycle(Property<T> prop) {
 *     return this.setValue(prop, findNextInCollection(prop.getPossibleValues(), this.getValue(prop)));
 * }
 * protected static <T> T findNextInCollection(List<T> list, T value) {
 *     int i = list.indexOf(value) + 1;
 *     return i == list.size() ? list.getFirst() : list.get(i);
 * }
 * ```
 *
 * Cubium：
 *
 * ```cpp
 * const auto& values = prop.allowedValues();
 * size_t currentIndex = m_valueIndices[slotIndex];
 * size_t nextIndex = (currentIndex + 1) % values.size();
 * return with(prop, values[nextIndex]);
 * ```
 *
 * **两者逻辑等价，差异完全来自 `getPossibleValues()` 的顺序。**
 *
 * `cycle()` 的消费者是**调试棒（Debug Stick）**：它右键循环方块状态的属性值。
 * 若顺序不同，同一个方块用调试棒点击后的演化路径会完全不同。
 *
 * ## 核心对照：`FACING`
 *
 * 原版 `BlockStateProperties.FACING` 用了**显式列表**，不是枚举声明顺序：
 *
 * ```java
 * public static final EnumProperty<Direction> FACING = EnumProperty.create(
 *     "facing", Direction.class,
 *     Direction.NORTH, Direction.EAST, Direction.SOUTH, Direction.WEST,
 *     Direction.UP, Direction.DOWN);
 * ```
 *
 * 而 Cubium 的 `FACING()` 走 `DirectionProperty::create("facing")`，
 * 内部用 `Directions::all()`：
 *
 * ```cpp
 * inline std::array<Direction, 6> all() {
 *     return {Direction::Down, Direction::Up, Direction::North,
 *             Direction::South, Direction::West, Direction::East};
 * }
 * ```
 *
 * | 位置 | 原版 `FACING` | Cubium `FACING` |
 * |---|---|---|
 * | 0 | `NORTH` | **`DOWN`** |
 * | 1 | `EAST` | **`UP`** |
 * | 2 | `SOUTH` | **`NORTH`** |
 * | 3 | `WEST` | **`SOUTH`** |
 * | 4 | `UP` | **`WEST`** |
 * | 5 | `DOWN` | **`EAST`** |
 *
 * **注意：底层 `Direction` 枚举本身的顺序两边完全一致**
 * （`DOWN=0, UP=1, NORTH=2, SOUTH=3, WEST=4, EAST=5`）——差异只在**属性**这一层。
 *
 * 单元格式：
 * - `<prop>|values` —— `allowedValues()` 按索引顺序，用逗号拼接（枚举用 Java 常量名）
 * - `<prop>|count` —— 值个数
 * - `<prop>|name` —— 属性名
 * - `<prop>|index|<VALUE>` —— `indexOf(VALUE)`
 * - `<prop>|at|<n>` —— `valueAt(n)`
 * - `<prop>|cycle|<VALUE>` —— `cycle()` 一步之后的值（模拟调试棒点击一次）
 */
#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/DirectionProperty.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using mc::BlockStateProperties;
using mc::Direction;
using mc::EnumProperty;
using nlohmann::json;

/// `Direction` 的 Java 常量名 ↔ Cubium 值。
struct DirRef {
    const char* javaName;
    Direction value;
};

constexpr DirRef DIRECTIONS[] = {
    {"DOWN", Direction::Down},
    {"UP", Direction::Up},
    {"NORTH", Direction::North},
    {"SOUTH", Direction::South},
    {"WEST", Direction::West},
    {"EAST", Direction::East},
};

std::string toJava(Direction value)
{
    for (const DirRef& entry : DIRECTIONS) {
        if (entry.value == value) {
            return entry.javaName;
        }
    }
    return "null";
}

const DirRef* findDir(const std::string& name)
{
    for (const DirRef& entry : DIRECTIONS) {
        if (name == entry.javaName) {
            return &entry;
        }
    }
    return nullptr;
}

/// 参与比较的属性：名字 + 取值的访问器。
struct PropRef {
    const char* name;
    const mc::DirectionProperty& (*accessor)();
};

constexpr PropRef DIRECTION_PROPS[] = {
    {"facing", &BlockStateProperties::FACING},
    {"horizontal_facing", &BlockStateProperties::HORIZONTAL_FACING},
};

void writeList(FILE* out)
{
    for (const PropRef& prop : DIRECTION_PROPS) {
        std::fprintf(out, "%s|values\n", prop.name);
        std::fprintf(out, "%s|count\n", prop.name);
        std::fprintf(out, "%s|name\n", prop.name);
        const bool sixWay = prop.name == std::string("facing");
        for (const DirRef& dir : DIRECTIONS) {
            // 水平属性不包含 UP/DOWN，跳过
            if (!sixWay && (dir.value == Direction::Up || dir.value == Direction::Down)) {
                continue;
            }
            std::fprintf(out, "%s|index|%s\n", prop.name, dir.javaName);
            std::fprintf(out, "%s|cycle|%s\n", prop.name, dir.javaName);
        }
        const int atCount = sixWay ? 6 : 4;
        for (int i = 0; i < atCount; ++i) {
            std::fprintf(out, "%s|at|%d\n", prop.name, i);
        }
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
        throw std::runtime_error("enum_property_order: 单元段数不足：" + unit);
    }

    const PropRef* ref = nullptr;
    for (const PropRef& entry : DIRECTION_PROPS) {
        if (parts[0] == entry.name) {
            ref = &entry;
            break;
        }
    }
    if (ref == nullptr) {
        throw std::runtime_error("enum_property_order: 未知属性：" + parts[0]);
    }
    const mc::DirectionProperty& prop = ref->accessor();

    const std::string mode = parts[1];
    std::string value;

    if (mode == "values" && parts.size() == 2) {
        std::string joined;
        const auto& values = prop.allowedValues();
        for (size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                joined.push_back(',');
            }
            joined += toJava(values[i]);
        }
        value = joined;
    } else if (mode == "count" && parts.size() == 2) {
        value = std::to_string(prop.allowedValues().size());
    } else if (mode == "name" && parts.size() == 2) {
        value = prop.name();
    } else if (mode == "index" && parts.size() == 3) {
        const DirRef* dir = findDir(parts[2]);
        if (dir == nullptr) {
            throw std::runtime_error("enum_property_order: 未知方向：" + parts[2]);
        }
        const auto index = prop.indexOf(dir->value);
        value = index ? std::to_string(*index) : "null";
    } else if (mode == "at" && parts.size() == 3) {
        const size_t index = static_cast<size_t>(std::stoull(parts[2]));
        if (index >= prop.allowedValues().size()) {
            value = "null";
        } else {
            value = toJava(prop.allowedValues()[index]);
        }
    } else if (mode == "cycle" && parts.size() == 3) {
        // 复刻 StateHolder::cycle 的语义：当前值在 allowedValues 中的下标 +1 后取模。
        const DirRef* dir = findDir(parts[2]);
        if (dir == nullptr) {
            throw std::runtime_error("enum_property_order: 未知方向：" + parts[2]);
        }
        const auto index = prop.indexOf(dir->value);
        if (!index) {
            throw std::runtime_error("enum_property_order: 值不在属性中：" + parts[2]);
        }
        const auto& values = prop.allowedValues();
        const size_t next = (*index + 1) % values.size();
        value = toJava(values[next]);
    } else {
        throw std::runtime_error("enum_property_order: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(enum_property_order, &writeList, &runLine)

} // namespace mc::diff
