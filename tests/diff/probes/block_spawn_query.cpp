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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file block_spawn_query.cpp
 * @brief 差分能力：`BlockState` 上与「能否生成」相关的四条静态判据。
 *
 * `list`：全部方块状态（规范名，属性按名字排序）
 * `run` ：{ "unit": "minecraft:oak_slab[type=bottom]" }
 *         → { "collisionFull", "redstoneConductor", "signalSource", "supportFull" }
 *
 * 这四个字段一一对应原版生成流程里的判据，把「生物生成位置不对」拆成可定位的分支：
 *
 * - `collisionFull`：`BlockState#isCollisionShapeFullBlock`；
 * - `redstoneConductor`：`BlockState#isRedstoneConductor`；
 * - `signalSource`：`BlockState#isSignalSource`；
 * - `supportFull`：`Block.isFaceFull(state.getBlockSupportShape(), UP)`，
 *   即 `ON_GROUND` 判定「脚下能否站住」用的支撑面满格检查。
 *
 * 后两者常被误当成「碰撞形状满格」：泥巴、灵魂沙碰撞箱比整格矮，既不是导体也不是
 * 信号源，原版仍允许在其上生成；台阶、楼梯碰撞箱不满格却是导体，原版据此拒绝。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/redstone/RedstoneHelper.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = [] {
        datapack::initializeRegistries();
        return true;
    }();
    (void)s_initialized;
}

/// 状态的规范字符串：`minecraft:stone` 或 `minecraft:oak_log[axis=x]`（属性按名字排序）。
std::string stateToString(const BlockState& state)
{
    std::string out = state.getBlock().blockLocation().toString();
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const auto& entry : state.values()) {
        pairs.emplace_back(entry.property->name(), entry.property->valueToString(entry.valueIndex));
    }
    if (pairs.empty()) {
        return out;
    }
    std::sort(pairs.begin(), pairs.end());
    out.push_back('[');
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        if (i != 0) {
            out.push_back(',');
        }
        out += pairs[i].first;
        out.push_back('=');
        out += pairs[i].second;
    }
    out.push_back(']');
    return out;
}

struct StateEntry {
    std::string name;
    const BlockState* state;
};

/// 全部方块状态（按规范名排序并去重）。
const std::vector<StateEntry>& states()
{
    static const std::vector<StateEntry> s_states = [] {
        ensureInitialized();
        std::vector<StateEntry> all;
        BlockRegistry::instance().forEachBlock([&all](Block& block) {
            for (const BlockState* state : block.defaultState().getAllStates()) {
                all.push_back({stateToString(*state), state});
            }
        });
        std::sort(all.begin(), all.end(), [](const StateEntry& a, const StateEntry& b) {
            return a.name < b.name;
        });
        all.erase(std::unique(all.begin(),
                      all.end(),
                      [](const StateEntry& a, const StateEntry& b) {
                          return a.name == b.name;
                      }),
            all.end());
        return all;
    }();
    return s_states;
}

void writeList(FILE* out)
{
    for (const StateEntry& entry : states()) {
        std::fprintf(out, "%s\n", entry.name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const StateEntry& entry : states()) {
        if (entry.name != unit) {
            continue;
        }
        const BlockState& state = *entry.state;
        return json{{"unit", unit},
            {"collisionFull", state.getCollisionShape().coversFullBlock()},
            {"redstoneConductor", world::redstone::RedstoneHelper::isRedstoneConductorState(state)},
            {"signalSource", state.getBlock().canProvidePower(state)},
            {"supportFull", Block::isFaceFull(state.getBlock().getBlockSupportShape(state), Direction::Up)}}
            .dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_spawn_query, &writeList, &runLine)

} // namespace mc::diff
