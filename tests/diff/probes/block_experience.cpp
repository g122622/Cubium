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
 * @file block_experience.cpp
 * @brief 差分能力：方块被破坏时掉落的经验值。
 *
 * `list`：全部方块 ID
 * `run` ：{ "block": "minecraft:diamond_ore", "samples": 8 }
 *         → 每次采样得到的经验值序列
 *
 * 原版把经验区间藏在实体类里（`DropExperienceBlock.xpRange`）而不是数据包，手抄出错
 * 不会有任何编译期提示：少一个矿、区间差一、把「固定值」写成「区间」都会让经验
 * 数量在游戏里对不上，而这类偏差在测试里几乎不可能被发现。采样值还决定随机数消耗
 * 次数——固定值不消耗、区间消耗一次——所以这里比对的是**采样序列**而不是取值范围。
 */
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 每个方块采样的次数：够看出固定值与区间，也够暴露「差一」。
constexpr int SAMPLE_COUNT = 8;

/// 采样种子：两侧必须一致，否则比对的是两段随机序列。
/// 必须写满 16 个十六进制位：Java 侧是 `0x000B10C0E1E4C0DE0L`，少写一位就是另一个种子。
constexpr u64 SAMPLE_SEED = 0x000B10C0E1E4C0DE0ULL;

void ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

std::vector<std::string> blockNames()
{
    std::vector<std::string> names;
    BlockRegistry::instance().forEachBlock([&](Block& block) {
        names.push_back(block.blockLocation().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

void writeList(FILE* out)
{
    ensureBlocks();
    for (const auto& name : blockNames()) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureBlocks();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("block").get<std::string>();

    const Block* block = BlockRegistry::instance().getBlock(ResourceLocation(requested));
    if (block == nullptr) {
        return json{{"block", requested}, {"missing", true}}.dump();
    }

    // 每个方块从同一个种子重新开始，采样序列因此与「其他方块有没有消耗随机数」无关。
    math::Xoroshiro128ppRandom random(SAMPLE_SEED);
    json samples = json::array();
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        samples.push_back(block->dropExperience(random));
    }
    return json{{"block", requested}, {"samples", std::move(samples)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_experience", &writeList, &runLine)

} // namespace mc::diff