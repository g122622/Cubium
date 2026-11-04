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
 * @file block_movement.cpp
 * @brief 差分能力：方块对实体移动的作用，以及几个决定「这格方块会不会自己动」的开关。
 *
 * `list`：全部方块 ID
 * `run` ：{ "block": "minecraft:honey_block" }
 *         → { "speedFactor": ..., "jumpFactor": ..., "ticksRandomly": ...,
 *             "signalSource": ..., "analogOutput": ... }
 *
 * 这些值决定「踩在这格方块上走起来是什么感觉」，以及「这格方块会不会自己变化」：
 *
 * - `speedFactor`：行走速度系数（原版 `Block#getSpeedFactor`）。蜂蜜块与灵魂沙为 0.4，
 *   踩上去立刻变慢。原版在 `Entity#getBlockSpeedFactor` 里每次移动都要查。
 * - `jumpFactor`：跳跃高度系数（原版 `Block#getJumpFactor`）。蜂蜜块为 0.5，跳起来只到
 *   一半高——玩家在蜂蜜块上跳不出去，是经典体验问题。
 * - `ticksRandomly`：是否参与随机刻（原版 `isRandomlyTicking`）。作物生长、草蔓延、
 *   树叶腐烂全靠它；漏标的方块永远不长，多标的方块会莫名其妙地变化。
 * - `signalSource`：是否输出红石信号（原版 `isSignalSource`）。
 * - `analogOutput`：比较器能否从它读数（原版 `hasAnalogOutputSignal`）。
 *
 * 与 `block_properties` 的分工：那个能力比硬度/亮度/滑度，本能力专比移动因子与
 * 这三个行为开关，不重叠。
 *
 * 之所以值得单独建一个能力：这五项都是纯数据，抄错不会有编译期提示。少写一个
 * `jumpFactor(0.5)` 玩家就会「在蜂蜜块上能正常跳」；漏标 `ticksRandomly` 的作物
 * 永远不生长——而这两类错误都不会崩溃、不会报错，只能逐条与原版比对发现。
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

/** f32 的位模式：跨语言逐位比较，避免十进制往返带来的假差异。 */
std::string f32Bits(f32 value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

void writeList(FILE* out)
{
    ensureBlocks();
    std::vector<std::string> names;
    BlockRegistry::instance().forEachBlock([&](Block& block) {
        names.push_back(block.blockLocation().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureBlocks();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("block").get<std::string>();

    Block* block = BlockRegistry::instance().getBlock(ResourceLocation(requested));
    if (block == nullptr) {
        return json{{"block", requested}, {"missing", true}}.dump();
    }

    const BlockState& state = block->defaultState();
    return json{{"block", requested},
        {"speedFactor", f32Bits(block->getSpeedFactor(state))},
        {"jumpFactor", f32Bits(block->getJumpFactor(state))},
        // 按状态查询：原版 isRandomlyTicking 带状态（红石矿只有 LIT 时随机刻）。
        {"ticksRandomly", state.isRandomlyTicking()},
        {"signalSource", block->canProvidePower(state)},
        {"analogOutput", block->hasComparatorInputOverride(state)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_movement, &writeList, &runLine)

} // namespace mc::diff
