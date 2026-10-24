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
 * @file block_properties.cpp
 * @brief 差分能力：方块物理/光学属性（按方块类型，取其默认状态）。
 *
 * `list`：全部方块 ID（不是状态——这些属性是方块级的，状态级另有状态差分）
 * `run` ：{ "block": "minecraft:stone" }
 *         → 硬度、亮度、滑度、是否流体
 *
 * 这些值是生存判定的直接输入：硬度决定挖掘时间，抗爆决定会不会被炸掉，
 * 亮度/不透明度决定光照传播。它们都是纯数据，手抄出错不会有任何编译期提示，
 * 只能靠与原版逐条比对发现。
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
    // 暂不比 resistance / opacity / opaque / replaceable / solid：这三项在 Cubium 里来自材质默认值而不是
    // 逐方块注册，是已知的初始化缺口（见 tests/diff/README.md「未对齐项」），
    // 补齐前不应该混进本 harness 的通过条件。
    return json{{"block", requested},
        {"hardness", f32Bits(block->hardness())},
        {"light", block->getLightLevel(state)},
        {"slipperiness", f32Bits(block->getSlipperiness(state))},
        {"liquid", state.isLiquid()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_properties", &writeList, &runLine)

} // namespace mc::diff