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

#include "probe.hpp"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "server/world/gen/carver/CarvingMask.hpp"

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @file carving_mask.cpp
 * @brief 雕刻掩码（原版 `CarvingMask`）的写入与查询行为。
 *
 * 掩码记录「这一格已经被刻过」，雕刻器每次写入前都要查询它，避免同一条洞穴被重复雕刻。
 * 它看起来只是一个位数组，但**索引公式与越界语义**决定了两个可观察行为：
 *
 *   1. 索引按 `x & 15 | (z & 15) << 4 | (y - minY) << 8` 组装。X、Z 被 15 掩码意味着
 *      局部坐标超出 0..15 时不是报错，而是**回绕到同一区块内的另一列**；Y 则原样参与
 *      位移，没有任何范围检查。
 *   2. `get` 先问 additionalMask 再问位数组，两者是「或」的关系。原版用它来表示
 *      「跨区块的雕刻已经覆盖过这一格」，漏掉它会让相邻区块重复雕刻。
 *
 * 这些行为都不会崩溃、不报错，只会让洞穴在区块边界处多刻或少刻——玩家看到的是一条
 * 「有断层的洞穴」，几乎不可能靠肉眼定位到掩码的索引公式上。
 *
 * 探针直接驱动生产代码 `mc::CarvingMask`，不重写索引算法：本能力要测的正是这份实现。
 */

/// 一个场景：在给定几何配置的掩码上执行一串操作，最后由查询结果与统计量复原。
struct MaskScene {
    const char* id;
    i32 minY;
    i32 height;
    /// 操作序列：{"set", x, y, z} 或 {"get", x, y, z}。
    std::vector<json> operations;
};

/**
 * @brief 场景表：几何配置 x 坐标分布 x 操作序列。
 *
 * 三种几何配置分别对应主世界（-64/384）、下界（0/128）、末地（0/256）：
 * `minY` 为负时索引移位会把 `y - minY` 变成更大的正数，是最容易算错的情形。
 *
 * 坐标组的选取原则是**每一组都专门压一种回绕/越界边界**：
 *   - `inside`：0..15 与 Y 的两端（minY、minY+height-1），正交验证基本读写；
 *   - `wrap_xz`：16..31、-1..-16，原版掩码后会落回 0..15，怀疑实现漏了掩码会立刻暴露；
 *   - `y_edges`：Y 恰好取到 minY、minY+height-1 与紧邻的可用边界，验证「块内最后一行」
 *     与「块内第一行」不会互相覆盖（`(y - minY) << 8` 在高度不是 256 时最容易与 X/Z 位冲突）；
 *   - `collide`：X/Z 回绕后与 Y 位移产生的索引相同，检验两者是否互相踩踏。
 *
 * 不构造 Y 超出 [minY, minY + height) 的写入：原版 `carveEllipsoid` 的循环边界把 Y 夹在
 * `[minGenY + 1, minGenY + genDepth - 1 - j1]`、X/Z 夹在 `[0, 15]`，`BitSet.set(负索引)`
 * 在原版逻辑里不可达。构造它会测到一段两边都不该走的代码，制造无意义的失败。
 */
const std::vector<MaskScene>& maskScenes()
{
    static const std::vector<MaskScene> table = {
        {"overworld_inside",
            -64,
            384,
            {
                {"set", 0, -64, 0},
                {"set", 15, 319, 15},
                {"set", 7, 64, 8},
                {"get", 0, -64, 0},
                {"get", 15, 319, 15},
                {"get", 7, 64, 8},
                {"get", 1, -64, 0},
                {"get", 0, -63, 0},
                {"get", 0, -64, 1},
            }},
        {"overworld_wrap_xz",
            -64,
            384,
            {
                {"set", 16, 0, 0},
                {"set", 0, 0, 16},
                {"set", -1, 0, -1},
                {"get", 0, 0, 0},
                {"get", 16, 0, 0},
                {"get", -1, 0, -1},
                {"get", 15, 0, 15},
            }},
        {"overworld_y_edges",
            -64,
            384,
            {
                // 块内首行与末行：位移 (y - minY) 分别是 0 与 height-1，后者若被写进
                // X/Z 的位段就会与 column(0,0) 冲突。
                {"set", 0, -64, 0},
                {"set", 0, 319, 0},
                {"get", 0, -64, 0},
                {"get", 0, 319, 0},
                {"get", 0, -63, 0},
                {"get", 0, 318, 0},
                // 雕刻器实际可用的 Y 范围是 [minGenY+1, minGenY+depth-1-7]，取这两个端点。
                {"set", 5, -63, 5},
                {"set", 5, 312, 5},
                {"get", 5, -63, 5},
                {"get", 5, 312, 5},
            }},
        {"overworld_collide",
            -64,
            384,
            {
                // X/Z 回绕到 0..15 后，与 Y 位移落在同一索引上：二者必须互相踩踏。
                {"set", 17, 0, -3},
                {"get", 1, 0, 13},
                {"get", 17, 0, -3},
                {"set", 33, 64, 18},
                {"get", 1, 64, 2},
                {"set", 0, 1, 0},
                {"get", 0, 1, 0},
            }},
        {"nether_inside",
            0,
            128,
            {
                {"set", 0, 0, 0},
                {"set", 15, 127, 15},
                {"get", 0, 127, 0},
                {"get", 15, 0, 15},
                {"get", 8, 64, 8},
            }},
        {"nether_edges",
            0,
            128,
            {
                // 下界 minY=0：位移不产生负数，与主世界的负 minY 是两条不同的路径。
                {"set", 3, 0, 3},
                {"set", 3, 120, 3},
                {"get", 3, 0, 3},
                {"get", 3, 120, 3},
                {"get", 3, 1, 3},
            }},
        {"end_inside",
            0,
            256,
            {
                {"set", 5, 100, 9},
                {"set", 9, 100, 5},
                {"get", 5, 100, 9},
                {"get", 9, 100, 5},
                {"get", 9, 100, 9},
            }},
    };
    return table;
}

void writeList(std::FILE* out)
{
    for (const MaskScene& scene : maskScenes()) {
        std::fprintf(out, "%s\n", scene.id);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    for (const MaskScene& scene : maskScenes()) {
        if (requested != scene.id) {
            continue;
        }

        // 直接驱动生产实现：掩码的索引公式与边界语义都在它里面。
        CarvingMask mask(scene.minY, scene.height);

        // 逐条执行操作，并把每次 get 的结果按顺序记下来：读取行为本身也是被测对象
        // （原版 get 先问 additionalMask，再问位数组）。
        std::vector<bool> reads;
        json sets = json::array();
        for (const json& operation : scene.operations) {
            const std::string kind = operation.at(0).get<std::string>();
            const auto x = static_cast<BlockCoord>(operation.at(1).get<i32>());
            const auto y = operation.at(2).get<i32>();
            const auto z = static_cast<BlockCoord>(operation.at(3).get<i32>());
            if (kind == "set") {
                mask.setCarved(x, y, z);
                sets.push_back({x, y, z});
            } else {
                reads.push_back(mask.isCarved(x, y, z));
            }
        }

        // 扫描整个预分配范围，统计被标记的格数：写入遗漏与回绕碰撞都体现在这个总数上。
        i32 marked = 0;
        for (i32 y = scene.minY; y < scene.minY + scene.height; ++y) {
            for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                    if (mask.isCarved(static_cast<BlockCoord>(x), y, static_cast<BlockCoord>(z))) {
                        ++marked;
                    }
                }
            }
        }

        return json{
            {"unit", requested},
            {"minY", scene.minY},
            {"height", scene.height},
            {"reads", reads},
            {"marked", marked},
        }
            .dump();
    }
    return json{{"unit", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(carving_mask, &writeList, &runLine)

} // namespace mc::diff
