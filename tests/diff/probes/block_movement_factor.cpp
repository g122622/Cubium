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
 * @brief 差分能力：方块的速度倍率与跳跃倍率（`Block#getSpeedFactor` / `getJumpFactor`）。
 *
 * `list`：全部已注册方块
 * `run` ：一行 -> { "speedBits": f32 位模式, "jumpBits": f32 位模式 }
 *
 * ## 两个倍率
 *
 * - **速度倍率**：站在这个方块上时水平移动速度乘以它。灵魂沙、蜂蜜块都是 0.4
 * - **跳跃倍率**：从这个方块起跳时初速度乘以它。**只有蜂蜜块是 0.5**
 *
 * 其余全部方块是 1.0（100%）。
 *
 * ## 为什么灵魂沙和蜂蜜块不一样
 *
 * 灵魂沙只减速不限制跳跃（能正常跳出去），蜂蜜块两者都限制 —— 这是两种完全不同
 * 的陷阱体验。把蜂蜜块的跳跃倍数也写成 1.0，玩家就能轻松跳出蜂蜜陷阱；
 * 把灵魂沙的跳跃倍数写成 0.5，沙漠神殿的灵魂沙跑酷就变得几乎不可能。
 *
 * ## 与摩擦系数是两套机制
 *
 * 摩擦决定「是否会打滑」，倍率决定「走得多快」。两者独立累积，
 * 所以冰面（低摩擦）和灵魂沙（低速度倍率）的手感完全不同。
 *
 * ## 输出
 *
 * 一律输出 f32 位模式，避免十进制展开制造伪差异。
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
u32 floatBits(f32 value)
{
    u32 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
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
        if (entry.name != unit) {
            continue;
        }
        const BlockState& state = entry.block->defaultState();
        json result{{"unit", unit}};
        result["speedBits"] = floatBits(entry.block->getSpeedFactor(state, nullptr, nullptr));
        result["jumpBits"] = floatBits(entry.block->getJumpFactor(state, nullptr, nullptr));
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_movement_factor", &writeList, &runLine)

} // namespace mc::diff
