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
 * @brief 差分能力：方块状态是否是**红石信号源**（原版 `BlockBehaviour#isSignalSource`）。
 *
 * `list`：全部已注册方块的全部状态
 * `run` ：一行 -> { "signalSource": bool }
 *
 * ## 这个标志决定什么
 *
 * 红石粉在计算强度时，会先问「我这个方向上有没有信号源」。`isSignalSource`
 * 为 true 的方块会：
 *
 * - 被红石粉当作**强充能**来源（强度 15）
 * - 参与比较器与中继器的输入判定
 * - 让相邻的活塞、门、铁轨等响应
 *
 * 反过来，有些方块**看起来该是信号源却不是**，这是最容易写错的地方：
 *
 * - **铁轨**：被充能后自己变形状/启动矿车，但不向周围输出信号
 * - **命令方块**：不是信号源，只向比较器输出「成功执行次数」
 * - **合成器（Crafter）**：不是信号源
 * - **信标**：不是信号源，也不向比较器输出读数（「输出金字塔等级」是常见误解）
 * - **幽匿催化体**：不向比较器输出
 *
 * 这些「不是」是原版刻意的设计。多写一个会把整个红石电路的行为带偏，
 * 少写一个会让合法的信号源失效。
 *
 * ## 按状态比对
 *
 * 部分方块的状态决定是否为信号源（如探测铁轨只有 powered 时才输出、
 * 陷阱箱盖子开合、讲台有书才有信号），所以必须逐状态而非逐方块。
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
        result["signalSource"] = entry.state->getBlock().canProvidePower(*entry.state);
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_signal_source", &writeList, &runLine)

} // namespace mc::diff
