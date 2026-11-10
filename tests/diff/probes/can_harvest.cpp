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
 * @file can_harvest.cpp
 * @brief 差分能力：每个方块在每种手持工具下「能否正常掉落」。
 *
 * `list`：全部方块 ID
 * `run` ：{ "block": "minecraft:stone" }
 *         → { "harvest": { "empty": false, "wooden_pickaxe": false, ... } }
 *
 * 原版判定链是 `Player#hasCorrectToolForDrops`：
 *   `!state.requiresCorrectToolForDrops() || heldItem.isCorrectToolForDrops(state)`
 * 后半段由物品的 `Tool` 组件按 `Tool.Rule`（方块标签 → correct_for_drops）求值。
 *
 * 这是生存模式最直接的判定之一：挖石头不带镐就什么也得不到，挖钻石要有铁镐以上。
 * 它同时依赖「方块声明需要什么工具」与「工具声明自己能挖什么」两半数据，任一侧抄错
 * 都会让玩家白挖——掉落为空、没有报错、也没有崩溃。只能逐条与原版比对发现。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/HarvestToolTable.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }

    // 与服务端 RegistryBootstrap 同序初始化注册表（方块 → 物品 → 方块物品 → 标签）。
    // Items::ensureInitialized 会补上 ensureRegistries 未覆盖的初始化，但必须在重建
    // 规则表**之前**完成：BlockRegistry::applyHarvestToolTable 依赖方块对象已就位。
    datapack::initializeRegistries();
    Items::ensureInitialized();

    // 采集规则（mineable/<tool> + needs_*_tool）来自数据包标签，与 Items/VanillaBlocks 一样
    // 属于启动期数据，与服务端启动流程同一路径：数据包 → 标签表 → 规则表。
    block_registry::HarvestToolTable::rebuild();
    BlockRegistry::instance().applyHarvestToolTable();

    s_initialized = true;
}

/**
 * 参与比对的手持工具。键名两侧共用，原版 oracle 按同一张表构造对应物品。
 *
 * 覆盖五级镐（对应原版 Tier：木/石/铁/金/钻石/下界合金）、斧、锹、锄、剑、
 * 剪刀与空手。金质工具在原版与钻石同等级（Tier 都是 3），因此两者的判定结果应当
 * 完全一致——这一对本身就是一个很好的交叉验证。
 */
struct HandTool {
    const char* key;
    Item* const* item; ///< nullptr 表示空手
};

std::vector<HandTool> tools()
{
    return {
        {"empty", nullptr},
        {"wooden_pickaxe", &Items::WOODEN_PICKAXE},
        {"stone_pickaxe", &Items::STONE_PICKAXE},
        {"iron_pickaxe", &Items::IRON_PICKAXE},
        {"golden_pickaxe", &Items::GOLDEN_PICKAXE},
        {"diamond_pickaxe", &Items::DIAMOND_PICKAXE},
        {"netherite_pickaxe", &Items::NETHERITE_PICKAXE},
        {"wooden_axe", &Items::WOODEN_AXE},
        {"stone_axe", &Items::STONE_AXE},
        {"iron_axe", &Items::IRON_AXE},
        {"golden_axe", &Items::GOLDEN_AXE},
        {"diamond_axe", &Items::DIAMOND_AXE},
        {"netherite_axe", &Items::NETHERITE_AXE},
        {"wooden_shovel", &Items::WOODEN_SHOVEL},
        {"stone_shovel", &Items::STONE_SHOVEL},
        {"iron_shovel", &Items::IRON_SHOVEL},
        {"golden_shovel", &Items::GOLDEN_SHOVEL},
        {"diamond_shovel", &Items::DIAMOND_SHOVEL},
        {"netherite_shovel", &Items::NETHERITE_SHOVEL},
        {"wooden_hoe", &Items::WOODEN_HOE},
        {"stone_hoe", &Items::STONE_HOE},
        {"iron_hoe", &Items::IRON_HOE},
        {"golden_hoe", &Items::GOLDEN_HOE},
        {"diamond_hoe", &Items::DIAMOND_HOE},
        {"netherite_hoe", &Items::NETHERITE_HOE},
        {"wooden_sword", &Items::WOODEN_SWORD},
        {"stone_sword", &Items::STONE_SWORD},
        {"iron_sword", &Items::IRON_SWORD},
        {"golden_sword", &Items::GOLDEN_SWORD},
        {"diamond_sword", &Items::DIAMOND_SWORD},
        {"netherite_sword", &Items::NETHERITE_SWORD},
        {"shears", &Items::SHEARS},
    };
}

void writeList(FILE* out)
{
    ensureInitialized();
    std::vector<std::string> names;
    BlockRegistry::instance().forEachBlock([&](Block& block) {
        names.push_back(block.blockLocation().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("block").get<std::string>();

    Block* block = BlockRegistry::instance().getBlock(ResourceLocation(requested));
    if (block == nullptr) {
        return json{{"block", requested}, {"missing", true}}.dump();
    }

    const BlockState& state = block->defaultState();

    // 对齐原版 Player#hasCorrectToolForDrops：
    //   !requiresCorrectToolForDrops || item.isCorrectToolForDrops(state)
    // 不需要工具的方块（草、花、泥土……）任何手持物都能采集，包括空手。
    json harvest = json::object();
    for (const HandTool& tool : tools()) {
        if (tool.item == nullptr || *tool.item == nullptr) {
            // 空手：只有不需要工具的方块能采集。
            harvest[tool.key] = !state.requiresTool();
            continue;
        }
        ItemStack stack(*tool.item, 1);
        harvest[tool.key] = !state.requiresTool() || stack.canHarvestBlock(state);
    }

    return json{{"block", requested}, {"harvest", harvest}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(can_harvest, &writeList, &runLine)

} // namespace mc::diff
