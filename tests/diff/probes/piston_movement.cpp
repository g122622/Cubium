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
 * @file piston_movement.cpp
 * @brief 差分能力：活塞推动结构计算（一次推动的可推动性、移动链与破坏链）。
 *
 * `list`：不枚举（场景完全由输入行携带，两侧读同一份输入，见文件末尾说明）
 * `run` ：{ "unit": "<场景名>", "piston": [x,y,z], "dir": 0..5, "extending": bool,
 *           "cells": [[x,y,z,"minecraft:stone"], ...] }
 *         → { "unit": "...", "canMove": bool, "toMove": [[x,y,z], ...], "toDestroy": [[x,y,z], ...] }
 *
 * ## 为什么这是「纯计算」能力
 *
 * 一次活塞推动的结构求解（`PistonStructureHelper`）**只读世界**：遍历推动链、检查粘液块/
 * 蜂蜜块的粘连、判定 12 方块上限与破坏链。它不写世界、不消耗随机数、不需要推进刻度。
 * 这与寻路的 `SceneRegion` 同类，是差分测试能安全覆盖的形态。
 *
 * 逐刻的活塞动画（`MovingPistonBlock` 推进、方块实体 tick、收回阶段）必须真实驱动世界，
 * 属于 world 型能力，当前不可行，由 `block_behavior` 的 GameTest 覆盖。详见 skill
 * `mc-add-diff-test` 的「world 型能力当前不可行」一节。
 */
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/PistonStructureHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

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

/**
 * @brief 稀疏方块表世界：只读，供 `PistonStructureHelper` 按坐标查方块。
 *
 * 除方块查询外的一切能力（实体、碰撞、物理、计划刻）都与本能力无关：结构求解只用
 * `getBlockState`、`getMinBuildHeight`/`getMaxBuildHeight` 与 `worldBorder()`。
 * 未显式设置的格子返回空气状态而不是 nullptr——`isStickyBlock`/`canStickTo` 需要真实状态对象。
 */
class PistonSceneWorld final : public IWorld {
public:
    void setBlock(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr || state->isAir()) {
            m_cells.erase(pos);
            return;
        }
        m_cells[pos] = state;
    }

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (!isWithinWorldBounds(x, y, z)) {
            return &VanillaBlocks::AIR->defaultState();
        }
        const auto found = m_cells.find(BlockPos(x, y, z));
        return found == m_cells.end() ? &VanillaBlocks::AIR->defaultState() : found->second;
    }

    bool setBlockState(i32 /*x*/, i32 /*y*/, i32 /*z*/, const BlockState* /*state*/) override
    {
        throw std::logic_error("piston_movement 场景是只读的：结构求解不应写入世界");
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return getBlockState(x, y, z)->getFluidState();
    }

    // ========== 世界尺寸与光照 ==========

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return world::MIN_BUILD_HEIGHT;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 /*x*/, i32 y, i32 /*z*/) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    // ========== 碰撞与实体（结构求解不涉及） ==========

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override
    {
        return {};
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId) override
    {
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override
    {
        return nullptr;
    }

    // ========== 维度与时间 ==========

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 6000;
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    [[nodiscard]] bool isRaining() const override
    {
        return false;
    }

    [[nodiscard]] bool isThundering() const override
    {
        return false;
    }

    // ========== 随机数 ==========

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    // ========== 世界边界与 tick ==========

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::logic_error("piston_movement 场景不推进刻度：结构求解不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("piston_movement 场景不推进刻度：结构求解不应调度计划刻");
    }

private:
    std::map<BlockPos, const BlockState*> m_cells;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
};

/** 把 0..5 的方向序号转成 `Direction`（与原版 `Direction.from3DDataValue` 同序）。 */
Direction directionOf(i32 index)
{
    switch (index) {
        case 0:
            return Direction::Down;
        case 1:
            return Direction::Up;
        case 2:
            return Direction::North;
        case 3:
            return Direction::South;
        case 4:
            return Direction::West;
        case 5:
            return Direction::East;
        default:
            throw std::runtime_error("invalid direction index");
    }
}

/** 解析方块描述字符串（原版命令的方块状态语法）。 */
const BlockState* parseState(const std::string& text)
{
    command::StringReader reader(text);
    return command::BlockStateArgumentType::blockState()->parse(reader).state();
}

json positionJson(const BlockPos& pos)
{
    return json::array({pos.x, pos.y, pos.z});
}

/**
 * `list` 不枚举场景。
 *
 * 场景（方块表 + 活塞位置 + 朝向）完全由编排层生成的输入行携带，两侧消费同一份输入，
 * 因此不存在「两侧各自枚举」的口径差，`list` 输出为空即可（编排层用 `units: computed`）。
 */
void writeList(FILE* /*out*/) {}

std::string runLine(std::string_view inputLine)
{
    ensureBlocks();
    const json input = json::parse(inputLine);

    PistonSceneWorld world;
    for (const auto& cell : input.at("cells")) {
        const auto& entry = cell.get_ref<const json::array_t&>();
        const BlockPos pos(entry.at(0).get<i32>(), entry.at(1).get<i32>(), entry.at(2).get<i32>());
        world.setBlock(pos, parseState(entry.at(3).get<std::string>()));
    }

    const auto& pistonEntry = input.at("piston").get_ref<const json::array_t&>();
    const BlockPos pistonPos(pistonEntry.at(0).get<i32>(), pistonEntry.at(1).get<i32>(), pistonEntry.at(2).get<i32>());
    const Direction facing = directionOf(input.at("dir").get<i32>());
    const bool extending = input.at("extending").get<bool>();

    blocks::PistonStructureHelper helper(world, pistonPos, facing, extending);
    const bool canMove = helper.canMove();

    json toMove = json::array();
    for (const auto& pos : helper.getBlocksToMove()) {
        toMove.push_back(positionJson(pos));
    }
    json toDestroy = json::array();
    for (const auto& pos : helper.getBlocksToDestroy()) {
        toDestroy.push_back(positionJson(pos));
    }

    return json{{"unit", input.at("unit").get<std::string>()},
        {"canMove", canMove},
        {"toMove", std::move(toMove)},
        {"toDestroy", std::move(toDestroy)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("piston_movement", &writeList, &runLine)

} // namespace mc::diff
