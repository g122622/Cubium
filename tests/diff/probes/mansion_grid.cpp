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
 * @file mansion_grid.cpp
 * @brief 差分能力：林地府邸的布局网格。
 *
 * 府邸起点由「网格布局 → 放置器」两段组成，两者共用同一条随机流。构件序列一旦错位，
 * 分不清是走廊递归、房间识别（网格）错，还是外墙/房间放置错。这条能力把网格单独拎出来：
 * 给定种子，输出基础网格、三楼网格与三层房间网格的全部数值。
 *
 * 单元由 `harnesses/mansion_grid.ts` 给出，本探针不提供 `list`。
 */

#include <cstddef>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "probe.hpp"
#include "server/world/gen/structure/structures/WoodlandMansionStructure.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace world::gen::structure;

/** 把一个网格序列化成二维数组。 */
json dumpGrid(const woodland_mansion::SimpleGrid& grid)
{
    json rows = json::array();
    for (i32 y = 0; y < grid.height(); ++y) {
        json row = json::array();
        for (i32 x = 0; x < grid.width(); ++x) {
            row.push_back(grid.get(x, y));
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::string runLine(std::string_view inputLine)
{
    const auto input = json::parse(inputLine.begin(), inputLine.end());
    const auto seed = static_cast<u64>(input.at("seed").get<i64>());

    // 原版 `WoodlandMansionPieces.generateMansion` 先构造 Grid（消耗随机流），再构造 Placer。
    math::JavaLegacyRandom rng(seed);
    woodland_mansion::MansionGrid grid(rng);

    json floors = json::array();
    for (i32 floor = 0; floor < 3; ++floor) {
        floors.push_back(dumpGrid(grid.floorRoom(floor)));
    }

    return json{{"unit", input.value("unit", "")},
        {"base", dumpGrid(grid.baseGrid())},
        {"third", dumpGrid(grid.thirdFloorGrid())},
        {"rooms", std::move(floors)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mansion_grid, nullptr, &runLine)

} // namespace mc::diff
