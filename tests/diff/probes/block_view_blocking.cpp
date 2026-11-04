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
 */

/**
 * @brief 差分能力：方块是否**挡住视线**、是否**造成窒息**。
 *
 * `list`：全部方块状态（一个状态一个单元），名称为 `ns:path[attr=value,...]` 且属性按名字排序
 * `run` ：一行 -> { "unit", "viewBlocking", "suffocating" }
 *
 * ## 这两个判定管什么
 *
 * | 判定 | 原版接口 | 后果 |
 *---|---|---|---|
 * | 挡视线 | `BlockState#isViewBlocking` | 第三人称相机被方块遮挡时自动拉近；贴墙时玩家看到的不是墙内 |
 * | 造成窒息 | `BlockState#isSuffocating` | 实体处在该格内时每秒扣血（溺水式窒息） |
 *
 * 两者都由方块属性上的谓词决定，**默认**为 `state.blocksMotion() && state.isCollisionShapeFullBlock()`，
 * 大量方块单独覆写（树叶、玻璃、耕地、活塞、潜影盒……）。谓词抄错的后果都不会崩溃：
 * 树叶照不到光会让相机穿进树冠、岩浆块不窒息会让玩家站在里面毫发无伤；反过来耕地、
 * 泥土路径被写成挡视线则第三人称镜头会莫名抖一下。只能逐状态与原版比对发现。
 *
 * ## 为什么按状态而不是按方块
 *
 * 雪（`layers=8` 才挡视线）、活塞（未伸出才挡）、潜影盒（被打开状态不挡）在同一方块内的
 * 不同状态结论不同，按方块取默认状态会把这一维度整个抹掉。
 *
 * ## 与相邻能力的分工
 *
 * - `block_solidity`：`isSolid`（生成刷怪用）与可替换性；
 * - `light_occlusion`：光穿过一格后剩多少（`getLightBlock` / `canOcclude`）；
 * - `block_redstone_conductor` / `face_sturdy`：红石导线与面支撑。
 *
 * 本能力只看「视线/窒息」这两个独立谓词。查询全部在原点使用空世界，与本项目一致
 * （原版同样允许 `EmptyBlockGetter`，谓词只看状态自身）。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"

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

struct StateEntry {
    std::string name;
    const BlockState* state;
};

/**
 * 状态的字符串表示统一成 `ns:path[attr=value,...]`，且**属性按名字排序**。
 *
 * 两侧的属性枚举顺序不同（本项目按注册顺序，原版按 Property 的 compareTo），
 * 不排序会把同一状态配成两个单元，表现为一边有一边没有——那不是差分。
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
        if (entry.name != unit) {
            continue;
        }
        const BlockState& state = *entry.state;
        const BlockPos origin(0, 0, 0);
        const Block& block = state.getBlock();

        // 两个判定都由方块属性上的谓词决定：未覆写时默认判据为
        // `blocksMotion() && 碰撞形状满格`，覆写时由谓词自己给出（如雪按 LAYERS、活塞按 EXTENDED）。
        const bool viewBlocking = block.isViewBlockingAt(state, origin);
        const bool suffocating = block.isSuffocatingAt(state, origin);

        return json{{"unit", unit}, {"viewBlocking", viewBlocking}, {"suffocating", suffocating}}.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_view_blocking", &writeList, &runLine)

} // namespace mc::diff
