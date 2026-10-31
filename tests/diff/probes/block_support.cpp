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
 * @file block_support.cpp
 * @brief 差分能力：方块能否在给定底座上「存活」（`canSurvive` / `isValidPosition`）。
 *
 * `list`：方块 ID（按默认状态评估）
 * `run` ：{ "block": "minecraft:oak_sapling", "bases": ["minecraft:dirt", ...] }
 *         → { "block": "...", "support": { "minecraft:dirt": true, ... } }
 *
 * 支撑判定是**只读**计算：`isValidPosition(state, IBlockReader&, pos)` 只查邻域方块，
 * 不写世界、不调度计划刻、不消耗随机数。因此探针给一个稀疏方块表世界即可驱动它——
 * 与寻路的 `SceneRegion` 同一形态。
 *
 * 底座集合覆盖各类「能否站上去」的关键分类：完整方块、台阶、耕地、沙子、水、岩浆、
 * 栅栏、树叶、以及空气（悬空）。两套固定底座分别从**下方**（向上支撑）与**侧面**（攀附）
 * 两个方向评估，因为 `canSurvive` 的覆写覆盖了这两类附着方式。
 */
#include <algorithm>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
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
 * @brief 稀疏方块表世界：只读，供 `isValidPosition` 查邻域。
 *
 * 未设置的格子返回空气状态。非方块的接口（实体、物理、计划刻）在本能力里一旦被调用
 * 就说明前提被打破，因此直接抛异常而不是静默返回默认值。
 */
class SupportWorld final : public IBlockReader {
public:
    void setBlock(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr || state->isAir()) {
            m_cells.erase(pos);
            return;
        }
        m_cells[pos] = state;
    }

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
        throw std::logic_error("block_support 场景是只读的：支撑判定不应写入世界");
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return getBlockState(x, y, z)->getFluidState();
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return world::MIN_BUILD_HEIGHT;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

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

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

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
        throw std::logic_error("block_support 不推进刻度：支撑判定不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("block_support 不推进刻度：支撑判定不应调度计划刻");
    }

private:
    std::map<BlockPos, const BlockState*> m_cells;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
};

/** 解析方块描述字符串（原版命令的方块状态语法）。 */
const BlockState* parseState(const std::string& text)
{
    command::StringReader reader(text);
    return command::BlockStateArgumentType::blockState()->parse(reader).state();
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

    const BlockState* state = nullptr;
    if (const Block* block = BlockRegistry::instance().getBlock(ResourceLocation(requested)); block != nullptr) {
        state = &block->defaultState();
    }
    if (state == nullptr) {
        return json{{"block", requested}, {"missing", true}}.dump();
    }

    json support = json::object();
    for (const auto& baseEntry : input.at("bases")) {
        const std::string baseName = baseEntry.get<std::string>();
        const BlockState* base = parseState(baseName);

        // 底座同时放在「正下方」与「四个水平侧面」。附着方式有两类：
        //   - 下方：植物、压力板、站立告示牌；
        //   - 侧面：墙上的告示牌/悬挂告示牌/头、按钮、火把、灯笼、珊瑚。
        // 只放下方会让所有侧挂方块被误判成「不支持」——那是探针没摆对场景，不是缺陷。
        // 两侧必须用完全相同的摆法，否则比较的就是探针差异而非实现差异。
        SupportWorld world;
        world.setBlock(BlockPos(0, -1, 0), base);
        world.setBlock(BlockPos(0, 0, -1), base);
        world.setBlock(BlockPos(0, 0, 1), base);
        world.setBlock(BlockPos(-1, 0, 0), base);
        world.setBlock(BlockPos(1, 0, 0), base);

        const BlockPos pos(0, 0, 0);
        support[baseName] = state->getBlock().isValidPosition(*state, world, pos);
    }

    return json{{"block", requested}, {"support", std::move(support)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_support", &writeList, &runLine)

} // namespace mc::diff
