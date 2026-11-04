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
 * @brief 差分能力：方块默认状态的**爆炸抗性**（`Block#getExplosionResistance`）。
 *
 * `list`：全部已注册方块
 * `run` ：一行 -> { "bits": f32 位模式 }
 *
 * ## 这个值决定什么
 *
 * 爆炸破坏方块时，每个方块按 `抗性` 与爆炸威力算「是否被摧毁」：
 *
 * <pre>
 *   摧毁阈值 ≈ 威力 / 抗性
 * </pre>
 *
 * 抗性写错的后果不是崩、不是报错，而是「TNT 炸不动这堵墙」或「炸得比预期干净」——
 * 前者让玩家以为方块无敌，后者让建筑保护失效。
 *
 * ## 继承关系上的易错点
 *
 * 原版很多方块的抗性**不来自自身属性，而是转交底层方块**：
 *
 * - `StairBlock`：`return base.getExplosionResistance()`
 * - `SlabBlock`、`TrapDoorBlock`、`FenceBlock`、`WallBlock` 等同样转交
 * - 玻璃类多数是 0.3（能炸），但 `glass_pane` 也走同一个值
 *
 * Cubium 把抗性存在 `BlockState::resistance()` 里、由属性工厂统一设置，
 * 这是另一种合理做法，但必须确认最终值与原版逐个方块一致。
 *
 * ## 输出
 *
 * 一律输出 f32 位模式（原版抗性是 float），避免十进制展开制造伪差异。
 */

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

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

/// f32 的位模式，用于跨语言精确比对。
u32 floatBits(f32 value)
{
    u32 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
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
        if (entry.name != unit) {
            continue;
        }
        const BlockState& state = entry.block->defaultState();
        json result{{"unit", unit}};
        result["bits"] = floatBits(entry.block->getExplosionResistance(state));
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_blast_resistance, &writeList, &runLine)

} // namespace mc::diff
