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
 * @brief 差分能力：方块的移动手感三件套——摩擦、速度倍率、跳跃倍率。
 *
 * `list`：每个方块一个单元
 * `run` ：一行 -> { "friction": 0x..., "speed": 0x..., "jump": 0x... }
 *
 * 三个值都以 **f32 位模式**（`0x%08x`）输出：跨语言比浮点必须比位。
 *
 * ## 为什么三个要一起比
 *
 * 它们共同决定「站在这块方块上走路是什么感觉」，而且**互相替代**：
 *
 * - 冰改的是 `friction`（0.98），玩家在上面滑得停不下来；
 * - 蜂蜜块**不改** `friction`，改的是 `speedFactor(0.4)` + `jumpFactor(0.5)`，
 *   所以走在蜂蜜块上是「走得慢、跳不高」而不是「打滑」；
 * - 灵魂沙改的也是 `speedFactor(0.4)`，但**不改** `jumpFactor`——能跳，只是走得慢。
 *
 * 只比 `friction` 会漏掉蜂蜜块与灵魂沙的大半行为；只比 `speedFactor` 会把冰判成
 * 普通方块。三者放一起，任何一个抄错都跑不掉。
 *
 * ## 抄错的后果
 *
 * 全是「手感差一点」，没有崩溃、没有报错：
 *
 * - 冰的摩擦写成默认 0.6：踩上去完全不滑，滑冰场变成普通地面；
 * - 蜂蜜块的 speedFactor 写成 1.0：蜂蜜块不再黏脚，红石装置里靠它压速度的设计失效；
 * - 灵魂沙的 jumpFactor 被误写成 0.5：玩家在灵魂沙上跳不起来，下界赶路全卡住。
 *
 * 这类偏差只能靠逐方块比对发现——而且这三个值在原版里是**方块级常量**，
 * 全部散落在上千处注册调用中。
 */

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
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

/// f32 的位模式，用于跨语言精确比对。
std::string bits(f32 value)
{
    u32 raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", raw);
    return buffer;
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
            const BlockState& state = entry.block->defaultState();
            return json{{"unit", unit},
                {"friction", bits(entry.block->getSlipperiness(state, nullptr, nullptr, nullptr))},
                {"speed", bits(entry.block->speedFactor())},
                {"jump", bits(entry.block->jumpFactor())}}
                .dump();
        }
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_friction", &writeList, &runLine)

} // namespace mc::diff
