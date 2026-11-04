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
 * @brief 差分能力：方块状态是否响应**随机刻**（`BlockBehaviour#isRandomlyTicking`）。
 *
 * `list`：全部已注册方块的全部状态
 * `run` ：一行 -> { "randomTicks": bool }
 *
 * ## 这个标志决定什么
 *
 * 随机刻是 Minecraft 里「自然发生」的引擎：每个游戏刻随机挑选一批方块位置，
 * 只对 `isRandomlyTicking` 为 true 的状态调用 `randomTick`。
 *
 * 它控制的行为包括：作物生长、树叶腐烂、草方块蔓延、冰融化、铜氧化、
 * 火焰熄灭、积雪、藤蔓蔓延……
 *
 * ## 为什么必须逐状态比对
 *
 * 原版设计上是**方块级**开关（属性里的 `randomTicks()`），但有少数方块按状态覆写：
 *
 * - **红石矿**：只有 `lit=true`（刚被点亮）时随机刻，未点亮时不该自己被调度
 * - **耕地的湿润态**：湿润耕地不再从周围吸水
 *
 * 若整体漏开随机刻，作物永远不长；若整体多开，所有方块每刻都在被调度，
 * 除了性能灾难外还会让「不该变化」的方块发生意外变化（如未点亮的红石矿自己发光）。
 *
 * 状态粒度的错误无法靠方块级比对发现 —— 只看 `redstone_ore` 一个默认状态时，
 * 它对不对都只覆盖了一半。
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
        const Block& block = entry.state->getBlock();
        json result{{"unit", unit}};
        result["randomTicks"] = block.isRandomlyTicking(*entry.state);
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_random_ticks", &writeList, &runLine)

} // namespace mc::diff
