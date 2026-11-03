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
#include <array>
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
/// 16³ 体素网格。
using Cardinal16 = std::array<std::array<std::array<bool, 16>, 16>, 16>;

/**
 * @brief 把形状体素化到 16³ 网格。
 *
 * 每个体素取其**中心点**做包含判断。这样同一几何的不同盒子拆分方式得到完全相同的结果，
 * 而真正的形状差异（某个体素覆盖/未覆盖）一定被检出。
 */
Cardinal16 voxelize(const CollisionShape& shape)
{
    Cardinal16 grid{};
    const auto& boxes = shape.boxes();
    for (std::size_t y = 0; y < 16; ++y) {
        for (std::size_t z = 0; z < 16; ++z) {
            for (std::size_t x = 0; x < 16; ++x) {
                const f32 px = (static_cast<f32>(x) + 0.5f) / 16.0f;
                const f32 py = (static_cast<f32>(y) + 0.5f) / 16.0f;
                const f32 pz = (static_cast<f32>(z) + 0.5f) / 16.0f;
                for (const auto& box : boxes) {
                    if (px > box.minX && px < box.maxX && py > box.minY && py < box.maxY && pz > box.minZ &&
                        pz < box.maxZ) {
                        grid[y][z][x] = true;
                        break;
                    }
                }
            }
        }
    }
    return grid;
}

std::string describe(const CollisionShape& shape)
{
    if (shape.isEmpty()) {
        return "EMPTY";
    }
    // 体素化到 16³ 网格：形状的**语义**是「覆盖了方块内的哪些点」，
    // 而不是「由几个盒子拼出来」。同一个形状可以有无数种盒子拆分方式
    // （例如把 `box(0,0.5,0,1,1,0.5)` 拆成左右两个半盒），几何完全相同。
    // 逐盒输出会把这类表示差异误报成不一致，掩盖真正的问题。
    const Cardinal16 grid = voxelize(shape);
    std::string out;
    char buffer[8];
    for (std::size_t y = 0; y < 16; ++y) {
        for (std::size_t z = 0; z < 16; ++z) {
            for (std::size_t x = 0; x < 16; ++x) {
                out.push_back(grid[y][z][x] ? '1' : '0');
            }
        }
    }
    // 压缩：连续相同字符用 `<字符><次数>` 表示，避免每行 4096 字符。
    std::string packed;
    for (std::size_t i = 0; i < out.size();) {
        char ch = out[i];
        std::size_t run = 1;
        while (i + run < out.size() && out[i + run] == ch) {
            ++run;
        }
        // 用 `<字符>:<计数>` 分隔：计数可能是多位数（最大 4096），
        // 不加分隔符时无法判断 `0` 后面的 `516` 是一个计数还是 `5` 和 `16` 两段，
        // 会让离线分析工具解不开。
        packed.push_back(ch);
        packed.push_back(':');
        std::snprintf(buffer, sizeof(buffer), "%zu", run);
        packed += buffer;
        i += run;
    }
    return packed;
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
    // 形状按 BlockPos.ZERO 求值。注意：原版只有 FlowerBlock / BambooStalkBlock /
    // PointedDripstoneBlock 在 getShape 里应用了 offsetType 偏移，其余方块（草、树苗、
    // 蘑菇等）虽然有 offsetType，getShape 仍直接返回固定形状。偏移由方块自身的
    // getShapeAt 决定，探针不需要在这一层额外处理。
    // 用 getShapeAt 而非 getShape：前者会按方块自身是否在 getShape 里应用偏移来返回，
    // 与原版 `getShape(state, level, pos, ctx)` 的语义一致。
    //
    const BlockPos probePos(0, 0, 0);
    // 位置偏移已内建进无位置版本（`getCollisionShape` 默认转调 `getShapeAt`），
    // 因此这里统一走带位置的重载，不需要额外的「是否位置相关」判断。
    const CollisionShape collisionShape = state->getCollisionShapeAt(probePos);
    const CollisionShape supportShape = state->getBlockSupportShapeAt(probePos);

    const std::string result = "shape=" + describe(state->getShapeAt(probePos)) +
        "|collision=" + describe(collisionShape) + "|support=" + describe(supportShape);

    return json{{"unit", name}, {"result", result}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_shape", &writeList, &runLine)

} // namespace mc::diff
