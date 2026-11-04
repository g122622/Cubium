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
 * @brief 差分能力：全部方块状态的地图颜色（`BlockBehaviour.getMapColor`）。
 *
 * `list`：全部方块状态（一个状态一个单元）
 * `run` ：一行 -> { "color": i32, "hasColor": bool }
 *
 * ## 为什么值得比
 *
 * 地图上每一格的颜色都由这一项决定：原版把方块的地图颜色取出来，再按高度套 4 级
 * 明暗。它只影响**地图渲染**，不影响任何游戏逻辑，因此抄错了既不会崩溃、也不会让
 * 玩家觉得哪里不对——直到有人做了张大地图，发现自家白羊毛屋顶在地图上是灰色的。
 *
 * 抄错的典型形态：把 16 种染色方块**全部**指到同一个颜色（复制粘贴时忘了改这一项），
 * 或者把 `terracotta` 系列错指到 `STONE`。这类错误逐方块比对才能发现。
 *
 * ## 单元按状态而非按方块
 *
 * 原版 `getMapColor(BlockGetter, BlockPos)` 是 **BlockState** 上的方法，颜色可以随状态
 * 变化（例如被水浸润的方块在地图上偏蓝）。按方块测会把状态维度抹掉。
 *
 * ## 与 `MaterialColorId` 的对应
 *
 * Cubium 的 `world::map::MaterialColorId` 与原版 `MapColor.id` 一一对应（0~61），
 * 因此这里直接输出数值 id 两侧对撞。`hasColor=false` 表示该方块没有显式设置
 * 地图颜色（原版会回退到 `MapColor.NONE`，id 为 0）。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/map/MaterialColor.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

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

/**
 * 状态的字符串表示：`ns:path[attr=value,...]`，属性按名字排序。
 *
 * 两侧的属性枚举顺序不同，不排序会把同一状态配成两个单元。
 */
std::string canonicalStateName(const std::string& raw)
{
    const std::size_t open = raw.find('[');
    if (open == std::string::npos) {
        return raw;
    }
    const std::size_t close = raw.rfind(']');
    if (close == std::string::npos || close <= open) {
        return raw;
    }

    const std::string head = raw.substr(0, open);
    const std::string body = raw.substr(open + 1, close - open - 1);

    std::vector<std::string> attrs;
    std::string current;
    for (char ch : body) {
        if (ch == ',') {
            if (!current.empty()) {
                attrs.push_back(current);
            }
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        attrs.push_back(current);
    }
    std::sort(attrs.begin(), attrs.end());

    std::string result = head + "[";
    for (std::size_t i = 0; i < attrs.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += attrs[i];
    }
    result += "]";
    return result;
}

struct StateEntry {
    std::string name;
    const BlockState* state;
};

const std::vector<StateEntry>& states()
{
    static const std::vector<StateEntry> s_states = []() {
        std::vector<StateEntry> all;
        BlockRegistry::instance().forEachBlockState([&all](const BlockState& state) {
            all.push_back({canonicalStateName(state.toString()), &state});
        });
        std::sort(all.begin(), all.end(), [](const StateEntry& a, const StateEntry& b) {
            return a.name < b.name;
        });
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
        if (entry.name == unit) {
            const world::map::MaterialColorId id = entry.state->getBlock().getMapColor(*entry.state, nullptr, nullptr);
            const i32 color = static_cast<i32>(static_cast<u8>(id));
            return json{{"unit", unit}, {"color", color}}.dump();
        }
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_map_color, &writeList, &runLine)

} // namespace mc::diff
