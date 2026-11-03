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
 * @brief 差分能力：音符盒放在方块上方时演奏的乐器（`BlockStateBase#instrument()`）。
 *
 * `list`：每个方块一个单元
 * `run` ：一行 -> { "instrument": "<枚举名>" }
 *
 * ## 这个值决定什么
 *
 * 音符盒本身是同一个方块，但**脚下踩着什么**决定了它发出什么声音：
 *
 * - 站在石头 / 沙砾 / 黑曜石上 -> 大鼓（basedrum）
 * - 站在沙 / 红沙上 -> 小军鼓（snare）
 * - 站在玻璃上 -> 踩镲（hat）
 * - 站在木板 / 原木上 -> 贝斯（bass）
 * - 站在泥土上 -> 长笛（flute）
 * - 站在金块上 -> 钟（bell）
 * - 站在羊毛上 -> 吉他（guitar）
 * - 站在冰上 -> 风铃（chime）
 * - 站在骨块上 -> 木琴（xylophone）
 * - 站在铁块上 -> 铁琴（iron_xylophone）
 * - 站在灵魂沙上 -> 牛铃（cow_bell）
 * - 站在南瓜上 -> 迪吉里杜管（didgeridoo）
 * - 站在绿宝石块上 -> 电子音（bit）
 * - 站在干草块上 -> 班卓琴（banjo）
 * - 站在萤石上 -> 电钢琴（pling）
 * - 站在生物头颅上 -> 模仿对应生物叫声（仅 MOB_HEAD 类型在音符盒上方生效）
 * - 其余 -> 竖琴（harp）
 *
 * ## 抄错的后果
 *
 * 乐器是一张**巨大的硬编码映射表**，抄错不会崩溃、也不会影响任何方块行为，
 * 只会让红石音乐作品听起来不对：
 *
 * - 石头写成竖琴：所有石头鼓机失去鼓声，整首曲子只剩旋律轨；
 * - 玻璃漏掉踩镲：丛林神庙那类玻璃节奏轨彻底消失；
 * - 骨头写成竖琴而不是木琴：骷髅刷怪塔的提示音变了调；
 * - 羊毛写成贝斯而不是吉他：所有羊毛和弦乐作品音色都错了。
 *
 * 这些映射散落在上千处注册调用（`.instrument(...)`）里，没有编译期或运行期提示，
 * 只能逐方块比对。
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
    const Block* block;
};

const std::vector<BlockEntry>& blocks()
{
    static const std::vector<BlockEntry> s_blocks = []() {
        std::vector<BlockEntry> all;
        BlockRegistry::instance().forEachBlock([&all](Block& block) {
            all.push_back({block.blockLocation().toString(), &block});
        });
        std::sort(all.begin(), all.end(), [](const BlockEntry& a, const BlockEntry& b) {
            return a.name < b.name;
        });
        return all;
    }();
    return s_blocks;
}

/**
 * @brief 把乐器枚举映射为原版的枚举名。
 *
 * 用**原版名字**而不是整数下标：两侧枚举的声明顺序不同（Cubium 把玩家头放在最后，
 * 原版也是，但中间次序未必一致），按下标比会得到一堆伪差异；按名字比才有意义。
 */
const char* instrumentName(BlockProperties::Instrument instrument)
{
    switch (instrument) {
        case BlockProperties::Instrument::Harp:
            return "harp";
        case BlockProperties::Instrument::BaseDrum:
            return "basedrum";
        case BlockProperties::Instrument::Snare:
            return "snare";
        case BlockProperties::Instrument::Hat:
            return "hat";
        case BlockProperties::Instrument::Bass:
            return "bass";
        case BlockProperties::Instrument::Flute:
            return "flute";
        case BlockProperties::Instrument::Bell:
            return "bell";
        case BlockProperties::Instrument::Guitar:
            return "guitar";
        case BlockProperties::Instrument::Chime:
            return "chime";
        case BlockProperties::Instrument::Xylophone:
            return "xylophone";
        case BlockProperties::Instrument::IronXylophone:
            return "iron_xylophone";
        case BlockProperties::Instrument::CowBell:
            return "cow_bell";
        case BlockProperties::Instrument::Didgeridoo:
            return "didgeridoo";
        case BlockProperties::Instrument::Bit:
            return "bit";
        case BlockProperties::Instrument::Banjo:
            return "banjo";
        case BlockProperties::Instrument::Pling:
            return "pling";
        case BlockProperties::Instrument::Zombie:
            return "zombie";
        case BlockProperties::Instrument::Skeleton:
            return "skeleton";
        case BlockProperties::Instrument::Creeper:
            return "creeper";
        case BlockProperties::Instrument::Dragon:
            return "dragon";
        case BlockProperties::Instrument::WitherSkeleton:
            return "wither_skeleton";
        case BlockProperties::Instrument::Piglin:
            return "piglin";
        case BlockProperties::Instrument::CustomHead:
            return "custom_head";
    }
    return "unknown";
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
            return json{{"unit", unit}, {"instrument", instrumentName(entry.block->getInstrument())}}.dump();
        }
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_instrument", &writeList, &runLine)

} // namespace mc::diff
