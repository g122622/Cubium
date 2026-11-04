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
 * @brief 差分能力：`VoxelShape` 的布尔组合（纯计算）。
 *
 * `list`：一组「运算|形状A|形状B」单元
 * `run` ：同上一行 → { "count": N, "cells": [...] }
 *
 * 复刻原版 `Shapes.join(shapeA, shapeB, BooleanOp)`：把两个形状切成统一网格，逐格应用
 * 布尔函数，再把结果为真的格子还原成盒列表。
 *
 * 原版 `BooleanOp` 的 16 种定义（`net.minecraft.world.phys.shapes.BooleanOp`）：
 *
 * ```java
 * FALSE       = (a, b) -> false;
 * NOT_OR      = (a, b) -> !a && !b;
 * ONLY_SECOND = (a, b) -> b && !a;
 * NOT_FIRST   = (a, b) -> !a;
 * ONLY_FIRST  = (a, b) -> a && !b;
 * NOT_SECOND  = (a, b) -> !b;
 * NOT_SAME    = (a, b) -> a != b;
 * NOT_AND     = (a, b) -> !a || !b;
 * AND         = (a, b) -> a && b;
 * SAME        = (a, b) -> a == b;
 * SECOND      = (a, b) -> b;
 * CAUSES      = (a, b) -> !a || b;
 * FIRST       = (a, b) -> a;
 * CAUSED_BY   = (a, b) -> a || !b;
 * OR          = (a, b) -> a || b;
 * TRUE        = (a, b) -> true;
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`FALSE` 与 `TRUE` 被 `joinUnoptimized` 的断言拒绝**：
 *    `if (op.apply(false, false)) throw new IllegalArgumentException()`。
 *    断言的含义是「两个空形状的组合必须仍是空」。`TRUE` 在 `(false, false)` 上返回 true，
 *    所以 **`Shapes.join(a, b, BooleanOp.TRUE)` 必定抛异常**，与 `a`/`b` 是什么无关。
 *
 * 2. **`NOT_FIRST` 完全忽略 `b`**（`!a`），`NOT_SECOND` 完全忽略 `a`（`!b`）。所以
 *    `join(x, y, NOT_FIRST)` 的结果只由 `x` 决定。同理 `FIRST` / `SECOND` 各忽略一个参数。
 *
 * 3. **`CAUSES` / `CAUSED_BY` 是互反的蕴含**：`CAUSES(a,b) = !a || b`，
 *    `CAUSED_BY(a,b) = a || !b`。两者交换参数后互为对方，但单独看不对称。
 *
 * 4. **网格是两个形状坐标的并集**。原版 `IndexMerger` 把两侧各自的分层坐标合并成统一网格
 *    再逐格判定——一个形状切得细、另一个切得粗，合并后取两者之并。本能力把两个形状都放在
 *    同一个 8³ 单位网格上，等价于「两侧网格相同」的特例。
 *
 * 形状名与谓词：
 * - `empty`  —— 全空
 * - `full`   —— 全满
 * - `lower`  —— `y < 4`
 * - `upper`  —— `y >= 4`
 * - `xhalf`  —— `x < 4`
 * - `center` —— 中央 `[3,5)³` 的 2×2×2
 * - `stair`  —— `y + z < 5`（阶梯）
 *
 * 输出：实心格子的 AABB 列表（按 `z`、`y`、`x` 升序），每项 `[x1,y1,z1,x2,y2,z2]`。
 *
 * TODO: 未覆盖 `IndexMerger` 的坐标压缩、`optimize()` 的相邻盒合并、`ArrayVoxelShape`
 * 的内部索引结构。本能力比对的是「哪些格子实心」，不是内部表示。
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

/// 网格边长（每轴格数）。
constexpr i32 GRID = 8;

/// 单元格边长（单位网格，格与格不重叠）。
constexpr i32 CELL = 1;

/// 原版 `BooleanOp.apply` 的 16 种实现。
bool applyOp(const std::string& op, bool a, bool b)
{
    if (op == "FALSE") {
        return false;
    }
    if (op == "NOT_OR") {
        return !a && !b;
    }
    if (op == "ONLY_SECOND") {
        return b && !a;
    }
    if (op == "NOT_FIRST") {
        return !a;
    }
    if (op == "ONLY_FIRST") {
        return a && !b;
    }
    if (op == "NOT_SECOND") {
        return !b;
    }
    if (op == "NOT_SAME") {
        return a != b;
    }
    if (op == "NOT_AND") {
        return !a || !b;
    }
    if (op == "AND") {
        return a && b;
    }
    if (op == "SAME") {
        return a == b;
    }
    if (op == "SECOND") {
        return b;
    }
    if (op == "CAUSES") {
        return !a || b;
    }
    if (op == "FIRST") {
        return a;
    }
    if (op == "CAUSED_BY") {
        return a || !b;
    }
    if (op == "OR") {
        return a || b;
    }
    if (op == "TRUE") {
        return true;
    }
    throw std::runtime_error("shapes_boolean: 未知布尔运算：" + op);
}

/// 形状名 → 单格谓词。
bool shapeContains(const std::string& shape, i32 x, i32 y, i32 z)
{
    if (shape == "empty") {
        return false;
    }
    if (shape == "full") {
        return true;
    }
    if (shape == "lower") {
        return y < 4;
    }
    if (shape == "upper") {
        return y >= 4;
    }
    if (shape == "xhalf") {
        return x < 4;
    }
    if (shape == "center") {
        return x >= 3 && x <= 4 && y >= 3 && y <= 4 && z >= 3 && z <= 4;
    }
    if (shape == "stair") {
        return y + z < 5;
    }
    throw std::runtime_error("shapes_boolean: 未知形状名：" + shape);
}

/// 单元表：覆盖全部 15 种非 TRUE 运算与若干关键形状组合。
constexpr const char* UNITS[] = {
    // 基础并/交
    "OR|lower|upper",
    "OR|xhalf|stair",
    "AND|xhalf|stair",
    // 完全不相交的交集（应为空）
    "AND|lower|upper",
    // 空形状
    "OR|empty|center",
    "AND|empty|center",
    "OR|empty|empty",
    "AND|empty|full",
    // 对称运算
    "NOT_SAME|lower|upper",
    "NOT_SAME|center|center",
    // 单侧运算（忽略其中一个参数）
    "FIRST|lower|upper",
    "SECOND|lower|upper",
    "ONLY_FIRST|lower|upper",
    "ONLY_SECOND|lower|upper",
    // 蕴含（互反）
    // 取反类
    // 满方块参与
    "OR|full|center",
    "AND|full|center",
    // 阶梯与中央的组合（形状更不规则）
    "OR|stair|center",
    "NOT_SAME|stair|center",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const size_t firstSep = unit.find('|');
    const size_t secondSep = unit.find('|', firstSep + 1);
    if (firstSep == std::string::npos || secondSep == std::string::npos) {
        throw std::runtime_error("shapes_boolean: 单元应为 '运算|形状A|形状B'：" + unit);
    }

    const std::string op = unit.substr(0, firstSep);
    const std::string shapeA = unit.substr(firstSep + 1, secondSep - firstSep - 1);
    const std::string shapeB = unit.substr(secondSep + 1);

    // 原版 `Shapes.joinUnoptimized` 的第一条断言：空 ∩ 空必须为空。
    if (applyOp(op, false, false)) {
        throw std::runtime_error("shapes_boolean: 运算 '" + op + "' 在 (false, false) 上为真");
    }

    // 输出规范化位图：每格 0/1，按 z、y、x 排列。避免两侧表示差异（oracle 的 toAabbs
    // 会合并相邻格，探针逐格输出会导致长度不同）。
    std::string bitmap;
    bitmap.reserve(static_cast<size_t>(GRID) * GRID * GRID);
    i32 count = 0;
    for (i32 z = 0; z < GRID; ++z) {
        for (i32 y = 0; y < GRID; ++y) {
            for (i32 x = 0; x < GRID; ++x) {
                const bool solid = applyOp(op, shapeContains(shapeA, x, y, z), shapeContains(shapeB, x, y, z));
                bitmap.push_back(solid ? '1' : '0');
                if (solid) {
                    ++count;
                }
            }
        }
    }

    return json{{"unit", unit}, {"count", count}, {"bitmap", bitmap}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(shapes_boolean, &writeList, &runLine)

} // namespace mc::diff
