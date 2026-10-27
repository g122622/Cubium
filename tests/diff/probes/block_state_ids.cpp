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
 * @file block_state_ids.cpp
 * @brief 差分能力：方块状态的全局 ID（网络协议与存档里的数字编号）。
 *
 * `list`：全部方块 ID（单元是方块）
 * `run` ：{ "block": "minecraft:oak_stairs" }
 *         → { "ids": { "minecraft:oak_stairs[facing=east,...]": 1234, ... } }
 *
 * 原版把每个 BlockState 注册进 `Block.BLOCK_STATE_REGISTRY`，分配一个全局唯一的 id；
 * 区块数据（SectionBlocks / 调色板）、`/setblock` 的方块实体同步、调试输出里都用它。
 * Cubium 的 `JavaBlockStateIdMap` 负责同一次分配（内部 stateId ↔ Java 全局 id），这里
 * 通过与原版逐状态比对来确认两边给出的编号完全一致——编号错一位，客户端就会画出别的方块。
 */
#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/network/backend/java/mappings/JavaBlockStateIdMap.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

bool ensureBlocks()
{
    static bool s_initialized = false;
    static bool s_ok = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_ok = network::backend::java::JavaBlockStateIdMap::instance().initialize().success();
        s_initialized = true;
    }
    return s_ok;
}

/** 原版 BlockState.getSerializedName() 的形式：`minecraft:oak_stairs[facing=east,half=bottom]`。 */
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

    // 输出按状态名排序的 map：原版的编号不是状态名的函数（依赖注册顺序），
    // 所以两边都比「名字 → id」这组对应关系。
    std::map<std::string, u32> ids;
    for (const auto& state : block->stateContainer().validStates()) {
        const std::string name = stateName(*state, requested);
        ids.emplace(name, network::backend::java::JavaBlockStateIdMap::instance().toJavaGlobalId(state->stateId()));
    }

    return json{{"block", requested}, {"ids", ids}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_state_ids", &writeList, &runLine)

} // namespace mc::diff
