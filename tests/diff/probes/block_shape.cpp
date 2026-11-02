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
 * @brief 差分能力：方块的**三种形状本身**（默认状态）。
 *
 * `list`：全部方块 ID（排序）
 * `run` ：{ "block": "minecraft:oak_slab" } → 三种形状的坐标
 *
 * ## 测哪三种形状
 *
 * | 形状 | 原版接口 | 用途 |
 * |---|---|---|
 * | 外形 | `getShape` | 碰撞、视觉、选中框 |
 * | 碰撞 | `getCollisionShape` | 实体移动阻挡 |
 * | 支撑 | `getBlockSupportShape` | 附着型方块的存活判定 |
 *
 * 三者在原版是**各自独立**的：很多方块外形与碰撞不同（空气、水、火），
 * 很多方块碰撞为转而支撑非空（灯笼、悬挂告示牌），也有反过来
 * （下界疣：外形非空、碰撞与支撑皆空）。
 *
 * ## 为什么需要这个能力
 *
 * `face_sturdy` 只能看到支撑判定的**布尔结论**。一个方块报差异时，无法区分是
 * 外形写错了、碰撞开关错了、还是支撑形状缺失——只能靠人肉逐个试。
 *
 * 本能力直接把三种形状的**坐标**摊开，于是：
 *
 * - 形状写错的，坐标一眼可见（例如 `cactus` 的 Y 上限 16 应为 15）；
 * - 碰撞开关错的，碰撞形状为空但原版非空；
 * - 支撑形状被覆写覆盖的，支撑形状与碰撞形状出现不该有的差异。
 */

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "probe.hpp"
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

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

const BlockState* stateByName(const std::string& name)
{
    const Block* block = BlockRegistry::instance().getBlock(ResourceLocation(name));
    if (block == nullptr) {
        return nullptr;
    }
    return &block->defaultState();
}

/**
 * @brief 把形状转成稳定字符串：空形状记作 `EMPTY`，否则输出各轴坐标列表。
 *
 * 与 `shape_face` 能力使用相同格式，便于跨能力对照。
 */
std::string describe(const CollisionShape& shape)
{
    if (shape.isEmpty()) {
        return "EMPTY";
    }
    // 输出全部盒子的坐标：多盒形状（楼梯、栅栏等）用坐标列表会丢失盒子归属，
    // 而逐盒输出能精确表达形状结构。
    const auto& boxes = shape.boxes();
    std::string out;
    char buffer[128];
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        if (i != 0) {
            out.push_back(';');
        }
        const auto& box = boxes[i];
        std::snprintf(buffer,
            sizeof(buffer),
            "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
            box.minX,
            box.minY,
            box.minZ,
            box.maxX,
            box.maxY,
            box.maxZ);
        out += buffer;
    }
    return out;
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

    const std::string name = json::parse(inputLine).at("unit").get<std::string>();
    const BlockState* state = stateByName(name);
    if (state == nullptr) {
        throw std::runtime_error("block_shape: 未知方块：" + name);
    }

    // 三种形状用固定顺序拼接，扁平字符串便于逐字节比对。
    const std::string result = "shape=" + describe(state->getShape()) +
        "|collision=" + describe(state->getCollisionShape()) + "|support=" + describe(state->getBlockSupportShape());

    return json{{"unit", name}, {"result", result}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_shape", &writeList, &runLine)

} // namespace mc::diff
