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
 * @file block_destroy_progress.cpp
 * @brief 差分能力：每种方块「用正确工具 / 用错工具」时每 tick 的破坏进度。
 *
 * `list`：全部方块 ID
 * `run` ：{ "block": "minecraft:stone" }
 *         → hardness / correctToolProgress / wrongToolProgress
 *
 * 原版的公式（vanilla `BlockBehaviour.getDestroyProgress`）是
 *   progress = digSpeed / destroySpeed / (hasCorrectToolForDrops ? 30 : 100)
 * 其中 **destroySpeed 就是界面里显示的「硬度」**：石头 1.5，深板岩 3.0，
 * 黑曜石 50。进度是「每 tick 挖掉多少」，1/progress 就是挖穿所需的 tick 数。
 *
 * 把这两个量拆开报，是因为它们各自错了都会「看起来正常」：硬度错会让所有工具
 * 的挖掘时间一起伸缩；工具除数错只会让「用错工具」的惩罚变轻或变重。
 *
 * 只在方块需要正确工具且被玩家用错工具时两者才同时起作用。
 */
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `BlockBehaviour.getDestroyProgress` 的两个除数。
constexpr f32 kCorrectToolDivisor = 30.0f;
constexpr f32 kWrongToolDivisor = 100.0f;

std::string floatBits(f32 value)
{
    u32 bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "f32 必须是 4 字节");
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
}

void ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        // 工具速度要用真实的钻石镐物品堆来计算，物品注册表必须先就绪
        //（不初始化时 Items::DIAMOND_PICKAXE 是空指针）。
        Items::initialize();
        s_initialized = true;
    }
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

    // 原版 hardness < 0 表示不可破坏（基岩 -1、命令方块 -1、屏障 -1），
    // getDestroyProgress 直接返回 0，任何工具都挖不动。
    const f32 hardness = block->hardness();
    if (hardness < 0.0f) {
        return json{{"block", requested},
            {"hardness", floatBits(hardness)},
            {"correctToolProgress", floatBits(0.0f)},
            {"wrongToolProgress", floatBits(0.0f)}}
            .dump();
    }
    // 硬度为 0（草、火把、花）：原版一挖即破（进度 1.0），不进除法。
    if (hardness == 0.0f) {
        return json{{"block", requested},
            {"hardness", floatBits(hardness)},
            {"correctToolProgress", floatBits(1.0f)},
            {"wrongToolProgress", floatBits(1.0f)}}
            .dump();
    }

    // 直接用真实的钻石镐物品堆，不硬编码工具速度：
    //   工具速度 = ItemStack::getDestroySpeed（方块不适用时物品侧本就回落到 1.0）；
    //   是否「正确工具」 = 方块不要求正确工具时恒真，否则看物品的 canHarvestBlock。
    const BlockState& state = block->defaultState();
    ItemStack diamondPickaxe(*Items::DIAMOND_PICKAXE, 1);
    const f32 toolSpeed = diamondPickaxe.getDestroySpeed(state);
    const bool needsTool = state.requiresTool();
    const bool correctTool = !needsTool || diamondPickaxe.canHarvestBlock(state);

    const f32 correct = toolSpeed / hardness / kCorrectToolDivisor;
    // 错误工具：空手速度 1.0、除数 100（原版对这类方块的额外惩罚）。
    const f32 wrong = correctTool ? correct : (1.0f / hardness / kWrongToolDivisor);

    return json{{"block", requested},
        {"hardness", floatBits(hardness)},
        {"correctToolProgress", floatBits(correct)},
        {"wrongToolProgress", floatBits(wrong)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_destroy_progress", &writeList, &runLine)

} // namespace mc::diff