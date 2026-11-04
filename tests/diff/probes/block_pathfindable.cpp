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
 * @brief 差分能力：方块状态在三种寻路类型下是否**可通行**
 *        （原版 `BlockBehaviour#isPathfindable`）。
 *
 * `list`：<状态>#<寻路类型>，三种类型各一项
 * `run` ：一行 -> { "pathfindable": bool }
 *
 * ## 三种寻路类型
 *
 * | 类型 | 谁在用 | 原版默认判据 |
 * |---|---|---|
 * | LAND | 陆地行走（`WalkNodeEvaluator`） | 碰撞形状**不完整**即可通行 |
 * | WATER | 水中游动 | **只认水**（`getFluidState().is(WATER)`） |
 * | AIR | 空中飞行 | 碰撞形状不完整即可通行 |
 *
 * ## 两处最容易写错
 *
 * 1. **判据是「不完整」而不是「为空」**：台阶、楼梯、栅栏门的碰撞形状非空但不完整，
 *    生物能从它们上方走过去。用 `isEmpty()` 判定会让生物绕开所有台阶，
 *    表现为「村民卡在台阶前不动」。
 * 2. **WATER 类型不看形状只看是不是水**：水中型生物只能游过水方块，
 *    哪怕那个方块碰撞形状是空的（空气中的鱼会掉下来而不是游过去）。
 *    反过来，水的碰撞形状是空的，但只有 WATER 类型能通行 —— 若三种类型
 *    统一按形状判，陆行生物会走进水里。
 *
 * 这两个错误的共同点是 AI 不会崩溃，只会「走奇怪的路线」或「卡住不动」。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/PathComputationType.hpp"
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

constexpr const char* TYPE_SUFFIXES[] = {"land", "water", "air"};

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

const std::vector<StateEntry>& states()
{
    static const std::vector<StateEntry> s_states = []() {
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

PathComputationType typeFromName(const std::string& name)
{
    if (name == "water") {
        return PathComputationType::Water;
    }
    if (name == "air") {
        return PathComputationType::Air;
    }
    return PathComputationType::Land;
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const StateEntry& entry : states()) {
        for (const char* suffix : TYPE_SUFFIXES) {
            std::fprintf(out, "%s#%s\n", entry.name.c_str(), suffix);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t sep = unit.rfind('#');
    if (sep == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string stateName = unit.substr(0, sep);
    const std::string typeName = unit.substr(sep + 1);

    bool knownType = false;
    for (const char* suffix : TYPE_SUFFIXES) {
        if (typeName == suffix) {
            knownType = true;
            break;
        }
    }
    if (!knownType) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    for (const StateEntry& entry : states()) {
        if (entry.name != stateName) {
            continue;
        }
        json result{{"unit", unit}};
        result["pathfindable"] = entry.state->getBlock().isPathfindable(*entry.state, typeFromName(typeName));
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_pathfindable", &writeList, &runLine)

} // namespace mc::diff
