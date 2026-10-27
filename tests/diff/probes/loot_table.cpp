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
 * @file loot_table.cpp
 * @brief 差分能力：战利品表在**无参数上下文**下的抽取结果。
 *
 * `list`：数据包里全部战利品表 ID（`minecraft:blocks/stone`）
 * `run` ：{ "table": "minecraft:blocks/stone", "origin": [0.5, 64, 0.5] }
 *         → 用固定种子掷骰 4 次，每次给出 `[{id, count}, ...]`
 *
 * 这一路刻意**不提供** `this_entity` / `block_state` 等参数：它是最严格的骨架测试，
 * 池的掷骰次数、条目权重、函数链（set_count / apply_bonus / enchant_* 等）都在这里暴露。
 * 依赖参数的掉落表由 loot_table_block / loot_table_entity 两个探针覆盖。
 */
#include <cstdio>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/world/block/BlockState.hpp"

#include "loot_probe_common.hpp"
#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace lootprobe;

void writeList(FILE* out)
{
    for (const auto& id : environment().tableIds) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    auto& env = environment();
    const json input = json::parse(inputLine);
    const std::string id = input.at("table").get<std::string>();

    const loot::LootTable* table = env.tables.getTable(id);
    if (table == nullptr) {
        return json{{"table", id}, {"missing", true}}.dump();
    }

    const Vector3 origin = parseOrigin(input);
    // 这一路只给「哪些参数能跨表通用」的那部分：origin、一个中性方块状态与空工具。
    // 原版 1.21.11 把 block 参数集的必需键定为 block_state + origin + tool；补上这三个
    // 固定值后，方块类表能真正掷骰，而实体类表仍会因缺 this_entity / damage_source
    // 被两侧同样跳过——差异因此落在「同一套最小上下文下产出是否一致」上。
    const BlockState& neutralState = block_registry::BaseBlocks::AIR->defaultState();
    ItemStack emptyTool;
    json samples = rollSamples(*table,
        *env.world,
        table->getParameterSet(),
        [origin, &neutralState, &emptyTool](loot::LootContextBuilder& builder) {
            builder.withOwnedValue(loot::LootParams::ORIGIN, origin);
            builder.withParameter(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(&neutralState));
            builder.withParameter(loot::LootParams::TOOL, &emptyTool);
        });
    return json{{"table", id}, {"samples", std::move(samples)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE_WITH_SHUTDOWN("loot_table", &writeList, &runLine, &lootprobe::releaseEnvironment)

} // namespace mc::diff
