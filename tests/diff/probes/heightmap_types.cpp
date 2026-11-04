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
 * @brief 差分能力：高度图的类型判定（`Heightmap.Types`，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.level.levelgen.Heightmap`：
 *
 * ```java
 * static final Predicate<BlockState> NOT_AIR = state -> !state.isAir();
 * static final Predicate<BlockState> MATERIAL_MOTION_BLOCKING = BlockStateBase::blocksMotion;
 *
 * public static enum Types implements StringRepresentable {
 *     WORLD_SURFACE_WG(0, "WORLD_SURFACE_WG", Usage.WORLDGEN, NOT_AIR),
 *     WORLD_SURFACE(1, "WORLD_SURFACE", Usage.CLIENT, NOT_AIR),
 *     OCEAN_FLOOR_WG(2, "OCEAN_FLOOR_WG", Usage.WORLDGEN, MATERIAL_MOTION_BLOCKING),
 *     OCEAN_FLOOR(3, "OCEAN_FLOOR", Usage.LIVE_WORLD, MATERIAL_MOTION_BLOCKING),
 *     MOTION_BLOCKING(4, "MOTION_BLOCKING", Usage.CLIENT,
 *         s -> s.blocksMotion() || !s.getFluidState().isEmpty()),
 *     MOTION_BLOCKING_NO_LEAVES(5, "MOTION_BLOCKING_NO_LEAVES", Usage.CLIENT,
 *         s -> (s.blocksMotion() || !s.getFluidState().isEmpty())
 *              && !(s.getBlock() instanceof LeavesBlock));
 * }
 *
 * public static enum Usage { WORLDGEN, LIVE_WORLD, CLIENT; }
 *
 * private static final IntFunction<Types> BY_ID =
 *     ByIdMap.continuous(t -> t.id, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 * ```
 *
 * ## 关键点
 *
 * 1. **只有 6 种类型，不是 8 种。** id 是连续的 `0..5`，没有空洞。这与
 *    [`equipment_slot`](./equipment_slot.md) 的 id 有空洞形成对比。
 *
 * 2. **`WORLD_SURFACE` 与 `WORLD_SURFACE_WG` 谓词相同、Usage 不同。**
 *
 *    两者都用 `NOT_AIR`，区别只在 `Usage`（`CLIENT` vs `WORLDGEN`）。同理
 *    `OCEAN_FLOOR` 与 `OCEAN_FLOOR_WG` 也都用 `MATERIAL_MOTION_BLOCKING`。
 *
 *    **判定的只有三种谓词**，六种类型是「三种谓词 × 两种生命周期」的组合加上
 *    `MOTION_BLOCKING` 的两个变体。
 *
 * 3. **`MOTION_BLOCKING` 把液体也算作阻挡。**
 *
 *    ```java
 *    s -> s.blocksMotion() || !s.getFluidState().isEmpty()
 *    ```
 *
 *    水、岩浆这类**不阻挡移动但有流体**的方块，在 `MOTION_BLOCKING` 里算「高」，
 *    在 `OCEAN_FLOOR` 里**不算**（`OCEAN_FLOOR` 只用 `blocksMotion`）。这是海洋高度图与
 *    运动阻挡高度图的核心差异。
 *
 * 4. **`MOTION_BLOCKING_NO_LEAVES` 在上一条件基础上再排除树叶。**
 *
 *    ```java
 *    (s.blocksMotion() || !s.getFluidState().isEmpty()) && !(s.getBlock() instanceof LeavesBlock)
 *    ```
 *
 *    树叶**本身 `blocksMotion() == false`**，所以正常情况下它本来就不算阻挡。这条排除只在
 *    **树叶带流体**（实际不可能）或**树叶 overriding 了 blocksMotion** 时才起作用。
 *
 *    更重要的是：`MOTION_BLOCKING` 与 `MOTION_BLOCKING_NO_LEAVES` 对**普通树叶**的结果
 *    **相同**（都不算）。差异体现在实现了 `blocksMotion()` 的特殊树叶上。
 *
 * 单元格式：`<type>|<air>|<blocksMotion>|<hasFluid>|<isLeaves>`
 *
 * 每个方块用一个四元组属性描述，`1` 表示真、`0` 表示假：
 * - `air`：是否空气
 * - `blocksMotion`：是否阻挡移动
 * - `hasFluid`：流体状态是否非空
 * - `isLeaves`：是否是树叶方块
 *
 * 输出该高度图类型是否把此方块记入高度。
 *
 * TODO: 未覆盖 `Heightmap.getFirstAvailable`（需要真实的 `ChunkAccess` 与方块状态），
 * `primeHeightmaps` 的批量更新，以及 `Usage` 枚举在渲染/存档中的具体作用。
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 高度图用途。
enum class Usage { WORLDGEN, LIVE_WORLD, CLIENT };

/// 一个高度图类型。
struct HeightmapType {
    const char* name;
    i32 id;
    Usage usage;
    const char* predicateName;
};

/// 原版 6 种高度图类型。
constexpr HeightmapType TYPES[] = {
    {"WORLD_SURFACE_WG", 0, Usage::WORLDGEN, "NOT_AIR"},
    {"WORLD_SURFACE", 1, Usage::CLIENT, "NOT_AIR"},
    {"OCEAN_FLOOR_WG", 2, Usage::WORLDGEN, "MATERIAL_MOTION_BLOCKING"},
    {"OCEAN_FLOOR", 3, Usage::LIVE_WORLD, "MATERIAL_MOTION_BLOCKING"},
    {"MOTION_BLOCKING", 4, Usage::CLIENT, "MOTION_BLOCKING"},
    {"MOTION_BLOCKING_NO_LEAVES", 5, Usage::CLIENT, "MOTION_BLOCKING_NO_LEAVES"},
};

/// 一个合成方块状态的属性四元组。
struct BlockProps {
    bool air;
    bool blocksMotion;
    bool hasFluid;
    bool isLeaves;
};

/// 原版 `NOT_AIR`。
bool notAir(const BlockProps& props)
{
    return !props.air;
}

/// 原版 `MATERIAL_MOTION_BLOCKING`。
bool materialMotionBlocking(const BlockProps& props)
{
    return props.blocksMotion;
}

/// 原版 `MOTION_BLOCKING` 谓词。
bool motionBlocking(const BlockProps& props)
{
    return props.blocksMotion || props.hasFluid;
}

/// 原版 `MOTION_BLOCKING_NO_LEAVES` 谓词。
bool motionBlockingNoLeaves(const BlockProps& props)
{
    return (props.blocksMotion || props.hasFluid) && !props.isLeaves;
}

/// 类型名 → 类型。
const HeightmapType* findType(const std::string& name)
{
    for (const auto& type : TYPES) {
        if (name == type.name) {
            return &type;
        }
    }
    return nullptr;
}

/// 按谓词名求值。
bool evaluatePredicate(const std::string& predicateName, const BlockProps& props)
{
    if (predicateName == "NOT_AIR") {
        return notAir(props);
    }
    if (predicateName == "MATERIAL_MOTION_BLOCKING") {
        return materialMotionBlocking(props);
    }
    if (predicateName == "MOTION_BLOCKING") {
        return motionBlocking(props);
    }
    if (predicateName == "MOTION_BLOCKING_NO_LEAVES") {
        return motionBlockingNoLeaves(props);
    }
    throw std::runtime_error("heightmap_types: 未知谓词：" + predicateName);
}

/// 原版 `Types.BY_ID`：越界返回 `WORLD_SURFACE_WG`（下标 0）。
const HeightmapType* typeById(i32 id)
{
    if (id >= 0 && id < 6) {
        return &TYPES[id];
    }
    return &TYPES[0];
}

/// 用途名。
const char* usageName(Usage usage)
{
    switch (usage) {
        case Usage::WORLDGEN:
            return "WORLDGEN";
        case Usage::LIVE_WORLD:
            return "LIVE_WORLD";
        case Usage::CLIENT:
            return "CLIENT";
    }
    return "?";
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // info：全部 6 种
        "info|WORLD_SURFACE_WG",
        "info|WORLD_SURFACE",
        "info|OCEAN_FLOOR_WG",
        "info|OCEAN_FLOOR",
        "info|MOTION_BLOCKING",
        "info|MOTION_BLOCKING_NO_LEAVES",
        // 空气：所有类型都不算高度
        "test|WORLD_SURFACE|1|0|0|0",
        "test|MOTION_BLOCKING|1|0|0|0",
        "test|MOTION_BLOCKING_NO_LEAVES|1|0|0|0",
        // 普通固体方块：所有类型都算
        "test|WORLD_SURFACE|0|1|0|0",
        "test|OCEAN_FLOOR|0|1|0|0",
        "test|MOTION_BLOCKING|0|1|0|0",
        "test|MOTION_BLOCKING_NO_LEAVES|0|1|0|0",
        // 液体（不阻挡但有流体）：OCEAN_FLOOR 不算，MOTION_BLOCKING 算
        "test|WORLD_SURFACE|0|0|1|0",
        "test|OCEAN_FLOOR|0|0|1|0",
        "test|MOTION_BLOCKING|0|0|1|0",
        "test|MOTION_BLOCKING_NO_LEAVES|0|0|1|0",
        // 非空气但不阻挡、无流体（高草）
        "test|WORLD_SURFACE|0|0|0|0",
        "test|OCEAN_FLOOR|0|0|0|0",
        "test|MOTION_BLOCKING|0|0|0|0",
        // 普通树叶：原版 LeavesBlock 的 blocksMotion() 为 true
        "test|WORLD_SURFACE|0|1|0|1",
        "test|OCEAN_FLOOR|0|1|0|1",
        "test|MOTION_BLOCKING|0|1|0|1",
        "test|MOTION_BLOCKING_NO_LEAVES|0|1|0|1",
        // byId
        "byId|0",
        "byId|1",
        "byId|2",
        "byId|3",
        "byId|4",
        "byId|5",
        "byId|6",
        "byId|-1",
        "byId|999",
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("heightmap_types: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "info" && parts.size() == 2) {
        const HeightmapType* type = findType(parts[1]);
        if (type == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const std::string value =
            std::to_string(type->id) + "," + std::string(usageName(type->usage)) + "," + type->predicateName;
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "test" && parts.size() == 6) {
        const HeightmapType* type = findType(parts[1]);
        if (type == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const BlockProps props{parts[2] == "1", parts[3] == "1", parts[4] == "1", parts[5] == "1"};
        const bool matched = evaluatePredicate(type->predicateName, props);
        return json{{"unit", unit}, {"value", matched ? "true" : "false"}}.dump();
    }
    if (mode == "byId" && parts.size() == 2) {
        const HeightmapType* type = typeById(std::stoi(parts[1]));
        return json{{"unit", unit}, {"value", std::string(type->name)}}.dump();
    }

    throw std::runtime_error("heightmap_types: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(heightmap_types, &writeList, &runLine)

} // namespace mc::diff
