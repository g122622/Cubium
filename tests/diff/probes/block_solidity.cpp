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
 * @brief 差分能力：方块的「可放置 / 可穿过」两态（`isSolid` 与 `canBeReplaced`）。
 *
 * `list`：每个方块一个单元
 * `run` ：一行 -> { "solid": bool, "replaceable": bool }
 *
 * ## 两个值分别决定什么
 *
 * - **solid**：原版 `BlockStateBase#isSolid`（内部字段 `legacySolid`）。它决定
 *   「这个东西算不算实体方块」——怪物能否在其上生成、能否贴着它放置方块、
 *   玩家是否被它挡住视线。算法是：`forceSolidOn` → 真；`forceSolidOff` → 假；
 *   否则取决于**碰撞形状是否占满整格**。
 * - **replaceable**：原版 `BlockStateBase#canBeReplaced()`（无参版本），直接对应
 *   注册时的 `Properties.replaceable`。它决定**能否直接被别的方块覆盖**——
 *   草丛、雪层、水、空气可以，石头不行。
 *
 * ## 为什么值得逐方块比对
 *
 * 两个都是布尔量，抄错的表现极其隐蔽：
 *
 * - `replaceable` 写成真：玩家那面墙可以被随手放上去的方块**直接替换掉**，
 *   而不是「放不进去」；
 * - `replaceable` 写成假：草丛上放不了花、雪层上放不了东西，玩家会觉得「这里放不了」；
 * - `solid` 写成真：萤火虫灌木、藤蔓这类**不该挡怪**的方块会让屋顶上刷不出怪，
 *   或者反过来让本该能刷怪的地面一片死寂；
 * - `solid` 写成假：栅栏、墙这类**看着能站**的方块不再被当成实体方块，
 *   依赖它的放置判定与刷怪判定一起偏。
 *
 * 两个值都散落在上千处注册调用里（`forceSolidOn`/`forceSolidOff`/`replaceable`），
 * 没有编译期或运行期提示，只能逐方块比对。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        Items::initialize();
        return true;
    }();
    (void)s_initialized;
}

struct BlockEntry {
    std::string name;
    const Block* block;
};

const std::vector<BlockEntry>& blocks()
{
    static const std::vector<BlockEntry> s_blocks = []() {
        std::vector<BlockEntry> all;
        BlockRegistry::instance().forEachBlock([&all](Block& block) {
            all.push_back({block.blockLocation().toString(), &block});
        });
        std::sort(all.begin(), all.end(), [](const BlockEntry& a, const BlockEntry& b) {
            return a.name < b.name;
        });
        return all;
    }();
    return s_blocks;
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const BlockEntry& entry : blocks()) {
        std::fprintf(out, "%s\n", entry.name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const BlockEntry& entry : blocks()) {
        if (entry.name == unit) {
            const BlockState& state = entry.block->defaultState();
            return json{{"unit", unit}, {"solid", state.isSolid()}, {"replaceable", entry.block->isReplaceable()}}
                .dump();
        }
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_solidity, &writeList, &runLine)

} // namespace mc::diff
