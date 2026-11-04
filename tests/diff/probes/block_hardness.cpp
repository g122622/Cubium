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
 * @brief 差分能力：全部方块的硬度与爆炸抗性。
 *
 * `list`：每个方块一个单元
 * `run` ：一行 -> { "hardness": 0x..., "resistance": 0x... }
 *
 * 两个值都以 **f32 位模式**（`0x%08x`）输出：跨语言比浮点必须比位，否则
 * `1.5` 与 `1.4999999` 在 JSON 序列化后可能看起来相同，或被 Gson 展开成 double
 * 而产生伪差异。
 *
 * ## 为什么值得全表比对
 *
 * 硬度决定挖掘时间，抗性决定能否被爆炸破坏。两者都是散落在上千处方块注册调用里
 * 的常量：
 *
 * - 硬度差一点，挖这块方块的手感就不对；差一个数量级（0.5 写成 5），玩家会以为
 *   自己拿错了工具；
 * - **硬度 -1 表示不可破坏**（基岩、屏障、命令方块这类）。写成 0 会让它一碰就碎，
 *   写成 50 会让它「挖得动但永远挖不完」——都是很难在游戏里一眼看出的错误；
 * - 抗性决定 TNT 能否炸掉它。抗性写成 0 的黑曜石会被一颗 TNT 清空。
 *
 * 这两个值不会有任何编译期或运行期提示，只能逐方块比对。
 *
 * ## 与 `block_destroy_progress` 的关系
 *
 * `block_destroy_progress` 验证的是「给定工具与硬度，每 tick 推进多少」这套公式；
 * 本能力验证的是公式的**输入**。公式对而输入错，挖掘时间照样不对。
 */

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
        if (entry.name == unit) {
            char hardness[16];
            char resistance[16];
            std::snprintf(hardness, sizeof(hardness), "0x%08x", floatBits(entry.block->hardness()));
            std::snprintf(resistance, sizeof(resistance), "0x%08x", floatBits(entry.block->resistance()));
            return json{{"unit", unit}, {"hardness", hardness}, {"resistance", resistance}}.dump();
        }
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_hardness", &writeList, &runLine)

} // namespace mc::diff
