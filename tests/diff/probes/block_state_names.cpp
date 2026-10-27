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
 * @file block_state_names.cpp
 * @brief 差分能力：方块状态最简名称（指令 `/setblock minecraft:oak_stairs[facing=east]` 的写法）。
 *
 * `list`：全部方块 ID（单元是方块，不是状态）
 * `run` ：{ "block": "minecraft:oak_stairs" }
 *         → 默认状态名、全部状态名（方块名 + 按属性名排序的 [属性=值]）
 *
 * 名字是命令、数据包、调试输出的公共词汇：值写错（`true`/`false` 大小写、朝向枚举
 * 拼写）或属性顺序与原版不同，玩家复制粘贴的指令就会失效。属性顺序必须按名字排序，
 * 这一点原版也不保证与注册顺序一致。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

/** 原版 BlockState.toString() 的形式：`minecraft:oak_stairs[facing=east,half=bottom,...]`。 */
std::string stateName(const BlockState& state, std::string_view blockId)
{
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const auto& entry : state.values()) {
        pairs.emplace_back(entry.property->name(), entry.property->valueToString(entry.valueIndex));
    }
    if (pairs.empty()) {
        return std::string(blockId);
    }
    std::sort(pairs.begin(), pairs.end());
    std::string result(blockId);
    result.push_back('[');
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (i != 0) {
            result.push_back(',');
        }
        result += pairs[i].first;
        result.push_back('=');
        result += pairs[i].second;
    }
    result.push_back(']');
    return result;
}

void writeList(FILE* out)
{
    ensureBlocks();
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
    ensureBlocks();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("block").get<std::string>();

    Block* block = BlockRegistry::instance().getBlock(ResourceLocation(requested));
    if (block == nullptr) {
        return json{{"block", requested}, {"missing", true}}.dump();
    }

    std::vector<std::string> states;
    for (const auto& state : block->stateContainer().validStates()) {
        states.push_back(stateName(*state, requested));
    }
    std::sort(states.begin(), states.end());

    return json{
        {"block", requested}, {"default", stateName(block->defaultState(), requested)}, {"states", std::move(states)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_state_names", &writeList, &runLine)

} // namespace mc::diff
