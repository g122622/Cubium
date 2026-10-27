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
 * @file loot_table_block.cpp
 * @brief 差分能力：方块类战利品表在「挖掘」上下文下的抽取结果。
 *
 * `list`：数据包里全部战利品表 ID
 * `run` ：{ "table": "minecraft:blocks/stone", "block": "minecraft:stone_state",
 *           "tool": "minecraft:diamond_pickaxe", "fortune": 3, "silkTouch": false }
 *         → 4 次掷骰的 `[{id, count}, ...]`
 *
 * 原版方块掉落表的条件集中在 `block_state_property`、`match_tool`、`survives_explosion`
 * 与 `table_bonus`（时运表）。四者都依赖上下文里的 `block_state` / `tool` / `origin`，
 * 所以这一路把方块状态与工具按**同一份输入**喂给两侧：Cubium 按 `block` 解析出
 * 方块状态，原版侧按 `block` 构造同名的 `BlockState`。
 */
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
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

    const std::string blockName = input.value("block", "minecraft:stone");
    const Block* block = BlockRegistry::instance().getBlock(ResourceLocation(blockName));
    if (block == nullptr) {
        return json{{"table", id}, {"missing", true}}.dump();
    }
    const BlockState& state = block->defaultState();

    const auto origin = parseOrigin(input);
    const i32 fortune = input.value("fortune", 0);
    const bool silkTouch = input.value("silkTouch", false);

    // 工具只在明确给了名字时构造：空工具与「没有工具」在原版是两种语义
    // （match_tool 条件会按工具为空判定），不能合并。
    std::unique_ptr<ItemStack> tool;
    if (input.contains("tool")) {
        auto parsed = ItemStack::fromJson(json{{"id", input.at("tool").get<std::string>()}, {"Count", 1}});
        if (parsed.success()) {
            tool = std::make_unique<ItemStack>(std::move(parsed.value()));
            // 附魔必须真实写到工具上：原版的 `match_tool` / `table_bonus` / `apply_bonus`
            // 都是直接读工具上的附魔，没有独立的「时运等级」上下文键。oracle 侧同样
            // 把附魔写进工具（见 LootTableBlockOracle#tool），两侧形态必须一致。
            if (fortune > 0) {
                tool->addEnchantment("minecraft:fortune", fortune);
            }
            if (silkTouch) {
                tool->addEnchantment("minecraft:silk_touch", 1);
            }
        }
    }

    // 原版 `block` 参数集把 tool 列为**必需**键；没有给工具时补一个空栈，
    // 这样 `match_tool` 条件会看到「手里的工具是空的」，与不提供工具等价。
    ItemStack emptyTool;
    ItemStack* toolRef = tool != nullptr ? tool.get() : &emptyTool;

    json samples = rollSamples(*table, *env.world, table->getParameterSet(), [&](loot::LootContextBuilder& builder) {
        builder.withParameter(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(&state));
        builder.withParameter(loot::LootParams::TOOL, toolRef);
        const Vector3 ownedOrigin = origin;
        builder.withOwnedValue(loot::LootParams::ORIGIN, ownedOrigin);
    });
    return json{{"table", id}, {"samples", std::move(samples)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE_WITH_SHUTDOWN("loot_table_block", &writeList, &runLine, &lootprobe::releaseEnvironment)

} // namespace mc::diff
