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
 * @brief 差分能力：**全部 40 个** `EnumProperty` 的值顺序。
 *
 * 这是 [`enum_property_order`](./enum_property_order.md) 的扩展版：后者只覆盖
 * `FACING` / `HORIZONTAL_FACING`（已修复），本能力覆盖 Cubium `BlockStateProperties`
 * 里的**全部枚举属性**。
 *
 * **本探针调用 Cubium 的真实实现**（`EnumProperty<T>::allowedValues()` /
 * `IProperty::valueToString`）。
 *
 * 每个属性产出 3 个单元：
 * - `values` —— 全部值按索引顺序，用逗号拼接（**序列化名**，非 Java 常量名）
 * - `count` —— 值个数
 * - `name` —— 属性字符串名（`name()`）
 *
 * ## 为什么只比 `values` 就够
 *
 * [`enum_property_order`](./enum_property_order.md) 已验证：`cycle()` 与 `indexOf`
 * 的差异**完全来自** `getPossibleValues()` 的顺序，逻辑本身两边等价。因此只要
 * `values` 序列逐项一致，`index` / `at` / `cycle` 必然全部一致。
 *
 * ## 原版与 Cubium 的字段名差异
 *
 * 属性的**字符串名**必须相同（否则方块状态解析失败），但**字段名**不必：
 *
 * | 原版字段 | Cubium 访问器 | 字符串名 |
 * |---|---|---|
 * | `FACING_HOPPER` | `FACING_EXCEPT_UP` | `facing` |
 * | `DOOR_HINGE` | `HINGE` | `hinge` |
 * | `NOTEBLOCK_INSTRUMENT` | `NOTE_BLOCK_INSTRUMENT` | `instrument` |
 * | `STRUCTUREBLOCK_MODE` | `STRUCTURE_MODE` | `mode` |
 * | `MODE_COMPARATOR` | `COMPARATOR_MODE` | `mode` |
 * | `EAST_WALL` | `WALL_HEIGHT_EAST` | `east` |
 * | `EAST_REDSTONE` | `REDSTONE_EAST` | `east` |
 * | `BAMBOO_LEAVES` | `BAMBOO_LEAVES_PROP` | `leaves` |
 *
 * 单元用 **Cubium 访问器名**做前缀，oracle 侧维护同样的映射表。
 *
 * ## 已知陷阱
 *
 * - **`VERTICAL_DIRECTION`**：原版是显式列表 `UP, DOWN`；Cubium 用谓词遍历
 *   `Directions::all()`，即 `DOWN, UP`——**顺序相反**。
 * - **`FACING_EXCEPT_UP`**：原版用 `Direction.values()` 过滤掉 `UP`，得到
 *   `DOWN, NORTH, SOUTH, WEST, EAST`；Cubium 用 `Directions::all()` 过滤，结果相同
 *   （因为两侧底层枚举顺序一致）。
 * - **`RAIL_SHAPE_STRAIGHT`**：两侧都用显式 6 项列表，需逐项核对。
 *
 * 本能力<b>不需要 bootstrap</b>（纯属性访问，不触碰注册表）。
 */
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/Properties.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using mc::BlockStateProperties;
using nlohmann::json;

/// 一个枚举属性：访问器名 + 取值信息的三个取值函数。
struct PropSpec {
    const char* accessor;
    std::string (*values)();
    std::string (*count)();
    std::string (*propName)();
};

/**
 * 对任意类型的 `EnumProperty` 生成 Spec。
 *
 * 用 `IProperty` 基类接口访问，避免为每个枚举写类型转换：
 * `valueCount()` / `valueToString(index)` / `name()` 都在基类上。
 */
template <typename PropRef>
PropSpec makeSpec(const char* accessor)
{
    return PropSpec{
        accessor,
        +[]() -> std::string {
            // 显式转成 IProperty&，避免重载解析选到 valueToString(const T&)。
            const ::mc::IProperty& prop = PropRef::get();
            std::string joined;
            const size_t n = prop.valueCount();
            for (size_t i = 0; i < n; ++i) {
                if (i != 0) {
                    joined.push_back(',');
                }
                joined += prop.valueToString(i);
            }
            return joined;
        },
        +[]() -> std::string {
            const ::mc::IProperty& prop = PropRef::get();
            return std::to_string(prop.valueCount());
        },
        +[]() -> std::string {
            const auto& prop = PropRef::get();
            return prop.name();
        },
    };
}

/// 把访问器包装成带 `get()` 的结构体。
#define PROP_REF(Name)                           \
    struct Name##Ref {                           \
        static const auto& get()                 \
        {                                        \
            return BlockStateProperties::Name(); \
        }                                        \
    };

PROP_REF(FACING)
PROP_REF(HORIZONTAL_FACING)
PROP_REF(VERTICAL_DIRECTION)
PROP_REF(FACING_EXCEPT_UP)
PROP_REF(AXIS)
PROP_REF(HORIZONTAL_AXIS)
PROP_REF(CHEST_TYPE)
PROP_REF(DOUBLE_BLOCK_HALF)
PROP_REF(HINGE)
PROP_REF(BED_PART)
PROP_REF(BELL_ATTACHMENT)
PROP_REF(STAIRS_SHAPE)
PROP_REF(SLAB_TYPE)
PROP_REF(WALL_HEIGHT_NORTH)
PROP_REF(WALL_HEIGHT_EAST)
PROP_REF(WALL_HEIGHT_SOUTH)
PROP_REF(WALL_HEIGHT_WEST)
PROP_REF(ATTACH_FACE)
PROP_REF(BAMBOO_LEAVES_PROP)
PROP_REF(HALF)
PROP_REF(RAIL_SHAPE)
PROP_REF(RAIL_SHAPE_STRAIGHT)
PROP_REF(REDSTONE_NORTH)
PROP_REF(REDSTONE_EAST)
PROP_REF(REDSTONE_SOUTH)
PROP_REF(REDSTONE_WEST)
PROP_REF(PISTON_TYPE)
PROP_REF(COMPARATOR_MODE)
PROP_REF(NOTE_BLOCK_INSTRUMENT)
PROP_REF(STRUCTURE_MODE)
PROP_REF(TEST_BLOCK_MODE)
PROP_REF(DRIPSTONE_THICKNESS)
PROP_REF(TILT)
PROP_REF(SCULK_SENSOR_PHASE)
PROP_REF(TRIAL_SPAWNER_STATE)
PROP_REF(VAULT_STATE)
PROP_REF(CREAKING_HEART_STATE)
PROP_REF(SIDE_CHAIN_PART)
PROP_REF(COPPER_GOLEM_POSE)
PROP_REF(ORIENTATION)

#undef PROP_REF

/// 全部属性（顺序即单元表顺序）。
const std::vector<PropSpec>& allSpecs()
{
    static const std::vector<PropSpec> specs = {
        makeSpec<FACINGRef>("FACING"),
        makeSpec<HORIZONTAL_FACINGRef>("HORIZONTAL_FACING"),
        makeSpec<VERTICAL_DIRECTIONRef>("VERTICAL_DIRECTION"),
        makeSpec<FACING_EXCEPT_UPRef>("FACING_EXCEPT_UP"),
        makeSpec<AXISRef>("AXIS"),
        makeSpec<HORIZONTAL_AXISRef>("HORIZONTAL_AXIS"),
        makeSpec<CHEST_TYPERef>("CHEST_TYPE"),
        makeSpec<DOUBLE_BLOCK_HALFRef>("DOUBLE_BLOCK_HALF"),
        makeSpec<HINGERef>("HINGE"),
        makeSpec<BED_PARTRef>("BED_PART"),
        makeSpec<BELL_ATTACHMENTRef>("BELL_ATTACHMENT"),
        makeSpec<STAIRS_SHAPERef>("STAIRS_SHAPE"),
        makeSpec<SLAB_TYPERef>("SLAB_TYPE"),
        makeSpec<WALL_HEIGHT_NORTHRef>("WALL_HEIGHT_NORTH"),
        makeSpec<WALL_HEIGHT_EASTRef>("WALL_HEIGHT_EAST"),
        makeSpec<WALL_HEIGHT_SOUTHRef>("WALL_HEIGHT_SOUTH"),
        makeSpec<WALL_HEIGHT_WESTRef>("WALL_HEIGHT_WEST"),
        makeSpec<ATTACH_FACERef>("ATTACH_FACE"),
        makeSpec<BAMBOO_LEAVES_PROPRef>("BAMBOO_LEAVES_PROP"),
        makeSpec<HALFRef>("HALF"),
        makeSpec<RAIL_SHAPERef>("RAIL_SHAPE"),
        makeSpec<RAIL_SHAPE_STRAIGHTRef>("RAIL_SHAPE_STRAIGHT"),
        makeSpec<REDSTONE_NORTHRef>("REDSTONE_NORTH"),
        makeSpec<REDSTONE_EASTRef>("REDSTONE_EAST"),
        makeSpec<REDSTONE_SOUTHRef>("REDSTONE_SOUTH"),
        makeSpec<REDSTONE_WESTRef>("REDSTONE_WEST"),
        makeSpec<PISTON_TYPERef>("PISTON_TYPE"),
        makeSpec<COMPARATOR_MODERef>("COMPARATOR_MODE"),
        makeSpec<NOTE_BLOCK_INSTRUMENTRef>("NOTE_BLOCK_INSTRUMENT"),
        makeSpec<STRUCTURE_MODERef>("STRUCTURE_MODE"),
        makeSpec<TEST_BLOCK_MODERef>("TEST_BLOCK_MODE"),
        makeSpec<DRIPSTONE_THICKNESSRef>("DRIPSTONE_THICKNESS"),
        makeSpec<TILTRef>("TILT"),
        makeSpec<SCULK_SENSOR_PHASERef>("SCULK_SENSOR_PHASE"),
        makeSpec<TRIAL_SPAWNER_STATERef>("TRIAL_SPAWNER_STATE"),
        makeSpec<VAULT_STATERef>("VAULT_STATE"),
        makeSpec<CREAKING_HEART_STATERef>("CREAKING_HEART_STATE"),
        makeSpec<SIDE_CHAIN_PARTRef>("SIDE_CHAIN_PART"),
        makeSpec<COPPER_GOLEM_POSERef>("COPPER_GOLEM_POSE"),
        makeSpec<ORIENTATIONRef>("ORIENTATION"),
    };
    return specs;
}

void writeList(FILE* out)
{
    for (const PropSpec& spec : allSpecs()) {
        std::fprintf(out, "%s|values\n", spec.accessor);
        std::fprintf(out, "%s|count\n", spec.accessor);
        std::fprintf(out, "%s|name\n", spec.accessor);
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
    if (parts.size() != 2) {
        throw std::runtime_error("enum_property_order_all: 单元段数不符：" + unit);
    }

    const PropSpec* found = nullptr;
    for (const PropSpec& spec : allSpecs()) {
        if (parts[0] == spec.accessor) {
            found = &spec;
            break;
        }
    }
    if (found == nullptr) {
        throw std::runtime_error("enum_property_order_all: 未知属性：" + parts[0]);
    }

    std::string value;
    if (parts[1] == "values") {
        value = found->values();
    } else if (parts[1] == "count") {
        value = found->count();
    } else if (parts[1] == "name") {
        value = found->propName();
    } else {
        throw std::runtime_error("enum_property_order_all: 未知单元类型：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enum_property_order_all", &writeList, &runLine)

} // namespace mc::diff
