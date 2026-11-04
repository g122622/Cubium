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
 * @brief 差分能力：方块的可燃性参数（`FireBlock.setFlammable` 的两个值）。
 *
 * `list`：每个方块一个单元
 * `run` ：一行 -> { "encouragement": i32, "flammability": i32 }
 *
 * ## 两个值的含义
 *
 * 原版 `FireBlock.setFlammable(block, igniteOdds, burnOdds)`：
 *
 * - **encouragement（`igniteOdds`）**：火焰蔓延到这块方块的概率权重。火在邻居处
 *   按 `getIgniteOdds`/`encouragement` 掷骰，值越大越容易被点着。木板是 5，树叶是 30。
 * - **flammability（`burnOdds`）**：已经在燃烧的方块被烧毁的概率权重。值越大烧得越快。
 *   木板是 20，羊毛是 60。
 *
 * 两个值独立：树叶极易被点燃（30）但烧尽速度和木板差不多（60），羊毛难燃（30）
 * 但一着就烧光（60）。
 *
 * ## 抄错的后果
 *
 * 火焰传播是一套**概率系统**，出错不会崩溃，只会让火势「感觉不太对」：
 *
 * - 木板写成 (1, 1)：房子着了之后烧得很慢，玩家有充足时间扑灭——原版里木屋是
 *   十几秒烧穿；
 * - 树叶的 encouragement 写成 5（木板的值）：森林大火蔓延速度骤降，烧不起来；
 * - 羊毛的 flammability 写成 20：羊毛变得耐烧，与「羊毛极易烧毁」的直觉相反。
 *
 * 这类偏差只能靠逐方块比对发现——207 个可燃方块的数值全部硬编码在原版
 * `FireBlock.bootStrap()` 里。
 *
 * ## 一个容易漏掉的规则
 *
 * 原版 `getIgniteOdds` / `getBurnOdds` 对 **`waterlogged=true` 的方块直接返回 0**：
 * 含水方块不会燃烧。本能力的单元取方块**默认状态**，含水与否由另一条规则表达，
 * 这里不重复覆盖。
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
#include "common/world/block/FireInfoRegistry.hpp"

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

struct BlockEntry {
    std::string name;
    u32 blockId;
};

const std::vector<BlockEntry>& blocks()
{
    static const std::vector<BlockEntry> s_blocks = []() {
        std::vector<BlockEntry> all;
        BlockRegistry::instance().forEachBlock([&all](Block& block) {
            all.push_back({block.blockLocation().toString(), block.blockId()});
        });
        std::sort(all.begin(), all.end(), [](const BlockEntry& a, const BlockEntry& b) {
            return a.name < b.name;
        });
        return all;
    }();
    return s_blocks;
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const BlockEntry& entry : blocks()) {
        std::fprintf(out, "%s\n", entry.name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const BlockEntry& entry : blocks()) {
        if (entry.name == unit) {
            const blocks::FireInfo info = blocks::FireInfoRegistry::instance().getFireInfo(entry.blockId);
            return json{{"unit", unit}, {"encouragement", info.encouragement}, {"flammability", info.flammability}}
                .dump();
        }
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_fire_info", &writeList, &runLine)

} // namespace mc::diff
