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
 * @brief 差分能力：方块状态的发光强度（`BlockBehaviour.getLightEmission`）。
 *
 * `list`：全部方块状态（一个状态一个单元）
 * `run` ：一行 -> { "emission": i32 }
 *
 * ## 与 `light_occlusion` 的分工
 *
 * `light_occlusion` 量的是**光穿过这一格后还剩多少**（`lightBlock` / `canOcclude` /
 * `propagatesSkylightDown`）；本能力量的是**这一格自己发出多少光**。两者是同一条光照
 * 管线上的输入与输出，缺任何一边都测不全：
 *
 * - 只测衰减：一个本该发光却写成 0 的火把，看起来「透光性完美」，但整间屋子不亮；
 * - 只测发光：一个本该不透光却写成透光的方块，会让光漏进本应黑暗的房间。
 *
 * ## 为什么按状态而非按方块
 *
 * 原版 `getLightEmission` 是 **BlockState** 上的方法，同一种方块的不同状态可以有不同
 * 亮度（历史上红石矿按 `lit`、海晶灯按 `waterlogged` 都有差别）。虽然当前原版里这类
 * 分歧很少，但接口本身是状态级的——按方块测会把状态维度整个抹掉，漏掉「某个状态写错」
 * 的可能。因此单元取到**每个状态**。
 *
 * ## 数值范围
 *
 * 0..15。原版用 `LightEngine.MAX_LEVEL` 截断，任何越界值都会被夹住；正因为会被夹住，
 * 抄成 16 与抄成 15 在游戏里看不出区别，靠肉眼永远发现不了——这一条只能靠比对。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"

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
 * 状态 ID -> 状态指针。
 *
 * 单元用**状态的字符串表示**（例如 `minecraft:oak_leaves[distance=1,...]`）而不是整数
 * ID：整数 ID 两侧的分配顺序不同，直接比会全是噪声。
 */
struct StateEntry {
    std::string name;
    const BlockState* state;
};

/**
 * 把状态的字符串表示规范成 `ns:path[attr=value,...]`，且**属性按名字排序**。
 *
 * 两侧的属性枚举顺序不同（Cubium 按注册顺序，原版按 Property 的 compareTo），
 * 直接比字符串会把同一状态配成两个单元，表现为一边有一边没有——那不是差分。
 * 这里统一排序，保证同一状态在两侧得到同一个名字。
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
            const i32 emission = static_cast<i32>(entry.state->getBlock().getLightLevel(*entry.state));
            return json{{"unit", unit}, {"emission", emission}}.dump();
        }
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_light_emission, &writeList, &runLine)

} // namespace mc::diff
