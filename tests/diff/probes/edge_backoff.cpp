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

#include "probe.hpp"

#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/physics/collision/EntityCollisionContext.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @file edge_backoff.cpp
 * @brief 潜行时的边缘保护（原版 `Player.maybeBackOffFromEdge` + `canFallAtLeast`）。
 *
 * 玩家潜行走到方块边缘时不会掉下去，但**沿边缘斜向移动仍然可以滑行**——只削掉会让他
 * 掉下去的那个分量。原版为此逐 0.05 步回退，X 与 Z 分别处理，最后再处理斜向组合：
 *
 *   d3 = signum(dx) * 0.05;  while (dx != 0 && canFallAtLeast(dx, 0, step)) dx -= d3;
 *   d4 = signum(dz) * 0.05;  while (dz != 0 && canFallAtLeast(0, dz, step)) dz -= d4;
 *   while (dx != 0 && dz != 0 && canFallAtLeast(dx, dz, step)) { dx -= d3; dz -= d4; }
 *
 * 每个循环都在 |位移| <= 0.05 时归零并跳出，所以最多回退到 0。
 *
 * 关键点是**分轴回退**：如果把它写成「整块判定、要么全过要么全停」，玩家贴边斜走就会
 * 被完全卡住，而不是沿墙滑行。这正是本能力要卡住的行为差异。
 *
 * `canFallAtLeast(dx, dz, step)` 的判定是：把实体碰撞箱沿水平方向平移 (dx, dz)、
 * 沿垂直方向向下扩张 `step`，在这个盒子里**没有任何碰撞箱**就意味着「会掉下去」。
 * 盒子四周各内缩 1e-7，避免与脚下方块的侧壁误判为支撑。
 *
 * 探针直接驱动生产的 `Player::maybeBackOffFromEdge`（配一个只按坐标查方块的最小碰撞世界），
 * 不在探针里复刻回退循环——要测的正是那份实现。
 */

/// 一个方块：坐标 + 是否为实心方块（只区分空气与石头，足够覆盖边缘判定）。
struct BlockSpec {
    i32 x;
    i32 y;
    i32 z;
};

/// 只按坐标查方块的最小碰撞世界夹具。
class SceneWorld final : public ICollisionWorld {
public:
    void addSolid(i32 x, i32 y, i32 z)
    {
        m_solid[{(static_cast<i64>(y) << 32) ^ (static_cast<i64>(x) << 16) ^ static_cast<i64>(z)}] = {x, y, z};
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto key = (static_cast<i64>(y) << 32) ^ (static_cast<i64>(x) << 16) ^ static_cast<i64>(z);
        if (m_solid.find(key) != m_solid.end()) {
            return &VanillaBlocks::STONE->defaultState();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord, ChunkCoord) const override
    {
        // 夹具只按方块坐标查状态，不提供区块对象：碰撞收集走 getBlockState 已足够。
        return nullptr;
    }

private:
    std::map<i64, BlockSpec> m_solid;
};

/// 一个场景：地面布局 + 实体箱 + 期望移动。
struct Scene {
    const char* name;
    /// 实体碰撞箱
    f64 boxMinX, boxMinY, boxMinZ, boxMaxX, boxMaxY, boxMaxZ;
    /// 期望的水平移动（y 恒为 0：边缘保护只在不下落时生效）
    f64 moveX, moveZ;
    /// 台阶级高度（maxUpStep）
    f64 stepHeight;
    /// 实心方块坐标（y 为方块底面）
    std::vector<std::array<i32, 3>> solids;
};

const std::vector<Scene>& scenes()
{
    // 地面铺在 y=0（方块占据 [0,1)），实体站在 y=1.0 上。
    auto platform = [](i32 fromX, i32 toX, i32 fromZ, i32 toZ) {
        std::vector<std::array<i32, 3>> cells;
        for (i32 x = fromX; x <= toX; ++x) {
            for (i32 z = fromZ; z <= toZ; ++z) {
                cells.push_back({x, 0, z});
            }
        }
        return cells;
    };

    static const std::vector<Scene> table = {
        // 站在 3x3 平台正中的 x 位置：中心 x=1.5，走 0.5 后到 2.0，检测盒仍在平台内
        {"center_of_3x3_move_x", 1.7, 1.0, 0.7, 2.3, 2.8, 1.3, 0.5, 0.0, 0.6, platform(0, 2, 0, 2)},
        // 站在平台最外沿（中心 x=2.5，脚跨到 x=2.8 仍在方块 2 内），继续往 +X 走会掉下去
        {"walk_off_edge_x", 2.2, 1.0, 0.7, 2.8, 2.8, 1.3, 1.0, 0.0, 0.6, platform(0, 2, 0, 2)},
        // 斜向：X 会掉、Z 不会 → 应只削掉 X，保留 Z（沿边滑行）
        {"slide_along_edge", 1.9, 1.0, 0.7, 2.5, 2.8, 1.3, 1.0, 1.0, 0.6, platform(0, 2, 0, 2)},
        // 斜向且两个方向都会掉 → 都应削掉
        {"corner_both_fall", 1.9, 1.0, 1.9, 2.5, 2.8, 2.5, 1.0, 1.0, 0.6, platform(0, 2, 0, 2)},
        // 站在边缘但朝平台内侧走：不应被削
        {"move_inward", 1.9, 1.0, 0.7, 2.5, 2.8, 1.3, -1.0, 0.0, 0.6, platform(0, 2, 0, 2)},
        // 单方块平台，实体站在正中央（方块占据 [0,1]）：任何超过 0.2 的位移都会踏空
        {"single_block", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.5, 0.0, 0.6, platform(0, 0, 0, 0)},
        // 长条平台（只有 +X 方向延伸）：沿 Z 走会掉
        {"narrow_strip_z", 0.2, 1.0, -0.3, 0.8, 2.8, 0.3, 0.0, 1.0, 0.6, platform(0, 0, -1, 1)},
        // 一小步（小于一个方块）：不应被削
        {"small_step_inward", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.1, 0.0, 0.6, platform(0, 1, 0, 1)},
        // 恰好 0.05 的位移：正好是回退步长，应能在边缘停下
        {"exactly_step_epsilon", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.05, 0.0, 0.6, platform(0, 0, 0, 0)},
        // 台阶高度足够高：下方一格处有方块时不算会掉
        {"tall_step_absorbs", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.5, 0.0, 0.6, platform(0, 1, 0, 1)},
        // 负方向移动
        {"move_negative_x", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, -1.0, 0.0, 0.6, platform(0, 0, 0, 0)},
        // 没有地面：任何方向都应被削到 0
        {"no_ground", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.5, 0.0, 0.6, {}},
    };
    return table;
}

void ensureRegistries()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

void writeList(std::FILE* out)
{
    // 编排层直接比对两侧的单元列表，顺序必须一致。场景表按可读性分组书写，
    // 这里统一排序后再输出，与 oracle 侧同样排序的结果对齐。
    std::vector<std::string> names;
    names.reserve(scenes().size());
    for (const auto& scene : scenes()) {
        names.emplace_back(scene.name);
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureRegistries();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const Scene* scene = nullptr;
    for (const auto& candidate : scenes()) {
        if (requested == candidate.name) {
            scene = &candidate;
            break;
        }
    }
    if (scene == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    SceneWorld world;
    for (const auto& cell : scene->solids) {
        world.addSolid(cell[0], cell[1], cell[2]);
    }
    PhysicsEngine engine(world);

    // 直接驱动生产的 Player::maybeBackOffFromEdge，不在探针里复刻回退循环。
    ecs::EntityRegistry registry("diff_edge_backoff");
    Player player(EntityInstanceId{1}, "differ", registry);
    player.setPhysicsEngine(&engine);
    // 站立高度 1.8，与场景里写的碰撞箱一致；位置取碰撞箱的脚下中心。
    player.setPosition(static_cast<f32>((scene->boxMinX + scene->boxMaxX) * 0.5),
        static_cast<f32>(scene->boxMinY),
        static_cast<f32>((scene->boxMinZ + scene->boxMaxZ) * 0.5));
    player.setSneaking(true);

    const auto resolved =
        player.maybeBackOffFromEdge(Vector3(static_cast<f32>(scene->moveX), 0.0f, static_cast<f32>(scene->moveZ)));

    return json{{"unit", requested},
        {"resolvedX", encodeFloat(static_cast<f64>(resolved.x))},
        {"resolvedZ", encodeFloat(static_cast<f64>(resolved.z))}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(edge_backoff, &writeList, &runLine)

} // namespace mc::diff
