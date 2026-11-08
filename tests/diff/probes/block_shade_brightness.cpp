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
 * @brief 差分能力：方块的**遮光亮度**（`BlockBehaviour#getShadeBrightness`）。
 *
 * `list`：全部已注册方块
 * `run` ：一行 -> { "bits": f32 位模式, "skyDown": 是否透传天空光 }
 *
 * ## 规则
 *
 * <pre>
 *   碰撞形状是完整方块 -> 0.2
 *   否则              -> 1.0
 * </pre>
 *
 * 0.2 表示「这块方块上方会形成一道阴影」——渲染时顶点 AO 会把贴图压暗；
 * 1.0 表示「不遮挡，正常受光」。
 *
 * ## 三个容易写错的特例
 *
 * 有几个方块**碰撞形状不完整却仍要返回 0.2**（原版专门覆写过）：
 *
 * - **泥巴（MudBlock）**：形状是 14/16 的矮块，但视觉上是实心的，必须有阴影
 * - **灵魂沙**：形状 14/16，同样需要阴影
 * - **雪层**：满层（8 层）返回 0.2，其余层数 1.0
 *
 * 反过来，**屏障/结构空位**这类不可见方块返回 1.0（不投影）。
 *
 * 把泥巴和灵魂沙按「形状不完整 -> 1.0」的默认规则处理，它们会看起来像
 * 「浮在地面上的亮块」，和周围方块的明暗对不上 —— 这是最典型的视觉错位。
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
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"

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
        result["bits"] = floatBits(entry.block->getShadeBrightness(state, nullptr, nullptr));
        result["skyDown"] = entry.block->propagatesSkylightDown(state, nullptr, nullptr);
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_shade_brightness, &writeList, &runLine)

} // namespace mc::diff
