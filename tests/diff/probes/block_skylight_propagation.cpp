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
 * @brief 差分能力：方块状态是否**向下透传天空光**
 *        （原版 `BlockBehaviour#propagatesSkylightDown`）。
 *
 * `list`：全部已注册方块的全部状态
 * `run` ：一行 -> { "propagates": bool }
 *
 * ## 这个判据的作用
 *
 * 天空光从世界顶部往下传播时，遇���方块要判断「光能不能直接穿过这一格」。
 * 原版默认实现：
 *
 * ```java
 * return !isShapeFullBlock(state.getShape(...)) && state.getFluidState().isEmpty();
 * ```
 *
 * 即**形状不是整格且没有流体**时才透光。这个值还决定了 `getLightBlock` 的返回值
 * （透光则 0，否则 1）。
 *
 * ## 两个容易写错的点
 *
 * 1. **必须同时检查「形状」与「流体」**。只判形状会让水透光（水下没有阴影 →
 *    海底一片明亮）；只判流体会让台阶、栅栏、楼梯全部挡光（白天进屋一片漆黑）。
 * 2. **判据是「形状非整格」而不是「碰撞形状为空」**。树叶、草丛、栅栏、
 *    告示牌的碰撞形状都不是整格，它们都应当透光。用「碰撞为空」判定会让
 *    树叶下方凭空变暗（森林地面收不到天空光）。
 *
 * ## 与 useShapeForLightOcclusion 的区别
 *
 * 那个说的是「挡光时按不按形状算」，这个说的是「这一格本身透不透光」。
 * 两者独立：玻璃既非整格（透光）又按形状遮挡（但它本来就不挡）。
 */

#include <algorithm>
#include <cstdio>
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

void writeList(FILE* out)
{
    ensureInitialized();
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
        json result{{"unit", unit}};
        result["propagates"] = entry.state->getBlock().propagatesSkylightDown(*entry.state);
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_skylight_propagation", &writeList, &runLine)

} // namespace mc::diff
