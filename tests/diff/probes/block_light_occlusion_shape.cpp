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
 * @brief 差分能力：方块状态是否**按形状决定光照遮挡**
 *        （原版 `BlockBehaviour#useShapeForLightOcclusion`）。
 *
 * `list`：全部已注册方块的全部状态
 * `run` ：一行 -> { "useShape": bool }
 *
 * ## 这个标志决定什么
 *
 * 光照引擎判断一个方块是否挡住光时，有两种口径：
 *
 * - **`false`（默认）**：只看方块是否 `canOcclude`（不透明整格），与具体形状无关。
 * - **`true`**：按**碰撞/视觉形状**的实际包围盒累加遮挡，形状没占满就不挡光。
 *
 * ## 哪些方块必须是 true
 *
 * 台阶、楼梯、耕地、土径、雪、泥巴、灵魂沙这类**半格或略矮**的方块。
 * 它们的材质不透明，但实际只占半格空间，光应当从中穿过上半格。
 *
 * 写成 `false` 的后果：
 *
 * - 台阶下方**一片漆黑**（整格被当作不透光）
 * - 室内光照蔓延范围偏小，远处亮度掉得快
 * - 楼梯间、地下室天然采光异常
 *
 * 写成 `true` 的后果：整格不透明的方块被按形状判，若形状比整格略小（如带凹槽
 * 的方块），会在其上方凭空透光。
 *
 * 这个标志**不影响实体的碰撞**，只影响光照；所以错了不会撞穿，只会「暗得没道理」。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"

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
        result["useShape"] = entry.state->getBlock().useShapeForLightOcclusion(*entry.state);
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_light_occlusion_shape, &writeList, &runLine)

} // namespace mc::diff
