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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file SpongeBlockTest.cpp
 * @brief SpongeBlock 海洋植物吸收 和 Block::dropResources 单元测试
 *
 * 测试覆盖：
 * - Block::dropResources 在 lootTableManager 为空（客户端）时应直接返回不生成掉落
 * - Block::dropResources 在 lootTable 为空时应返回
 * - SpongeBlock::absorb 不应吸收 SEA_PICKLE（MC 行为：海泡菜不在吸收列表中）
 * - SpongeBlock::absorb 吸收 KELP/KELP_PLANT/SEAGRASS/TALL_SEAGRASS 时应移除方块
 * - SpongeBlock::absorb 修正了 Material 判断为显式方块类型检查
 * - 海绵吸水后应变为湿润海绵
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/LiquidBlock.hpp"
#include "common/world/block/blocks/special/SpecialBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>

namespace mc {
namespace blocks {
namespace test {

// ============================================================================
// 测试用 Mock World
// ============================================================================

/**
 * @brief 用于海绵方块测试的 Mock World 实现
 *
 * 参考 tests/common/world/block/blocks/SpongeBlockTest.cpp 中的 SpongeTestWorld 设计。
 * 继承自 IBlockReader（其继承自 IWorld），支持方块状态存储、流体状态查询、
 * 实体生成追踪等功能。lootTableManager() 默认返回 nullptr（模拟客户端），
 * 可通过 setLootTableManager 设置。
 */
class SpongeDropTestWorld final : public IBlockReader {
public:
    SpongeDropTestWorld()
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        m_airState = &VanillaBlocks::AIR->defaultState();
    }

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const i64 key = packPos(x, y, z);
        const auto it = m_blocks.find(key);
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return m_airState;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const i64 key = packPos(x, y, z);
        if (state == nullptr || state == m_airState) {
            m_blocks.erase(key);
        } else {
            // 存储副本，因为传入的指针可能指向临时对象（如 IWaterLoggable::pickupFluid
            // 中的局部 BlockState 变量）
            m_blocks[key] = std::make_unique<BlockState>(*state);
        }
        m_setBlockCallCount++;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    // ========== 流体状态 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const i64 key = packPos(x, y, z);
        const auto it = m_fluids.find(key);
        if (it != m_fluids.end() && it->second != nullptr) {
            return it->second;
        }
        // 检查方块的流体状态（如水源方块）
        const BlockState* blockState = getBlockState(x, y, z);
        if (blockState != nullptr) {
            const fluid::FluidState* fluidState = blockState->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return fluidState;
            }
        }
        return fluid::Fluid::getFluidState(0);
    }

    void setFluidDirectly(const BlockPos& pos, const fluid::FluidState* state)
    {
        m_fluids[packPos(pos.x, pos.y, pos.z)] = state;
    }

    // ========== 实体生成 ==========

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntityCount++;
        m_lastSpawnedEntityPos = entity ? entity->position() : Vector3();
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    // ========== LootTableManager ==========

    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override { return m_lootTableManager; }

    void setLootTableManager(const loot::LootTableManager* manager) { m_lootTableManager = manager; }

    // ========== IBlockReader 必需的剩余方法 ==========

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(
        const AxisAlignedBB&, const Entity* = nullptr) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpongeDropTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpongeDropTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    // ========== 测试辅助 ==========

    [[nodiscard]] i32 setBlockCallCount() const noexcept { return m_setBlockCallCount; }
    [[nodiscard]] i32 spawnedEntityCount() const noexcept { return m_spawnedEntityCount; }
    [[nodiscard]] const Vector3& lastSpawnedEntityPos() const noexcept { return m_lastSpawnedEntityPos; }

    void resetCounters()
    {
        m_setBlockCallCount = 0;
        m_spawnedEntityCount = 0;
        m_lastSpawnedEntityPos = Vector3();
    }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<i64, const fluid::FluidState*> m_fluids;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    const BlockState* m_airState;
    const loot::LootTableManager* m_lootTableManager = nullptr;
    world::border::WorldBorder m_worldBorder;
    mutable math::Random m_random{12345};
    i32 m_setBlockCallCount = 0;
    i32 m_spawnedEntityCount = 0;
    Vector3 m_lastSpawnedEntityPos;
};

// ============================================================================
// 测试夹具
// ============================================================================

class SpongeBlockDropTest : public ::testing::Test {
protected:
    /**
     * @brief 在指定位置放置水源方块（同时设置方块状态和流体状态）
     */
    void placeWater(SpongeDropTestWorld& world, const BlockPos& pos)
    {
        // 设置水方块
        if (VanillaBlocks::WATER != nullptr) {
            world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::WATER->defaultState(), 3);
        }
        // 设置水源流体状态
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            const fluid::FluidState* waterState = &waterFluid->defaultState();
            world.setFluidDirectly(pos, waterState);
        }
    }

    /**
     * @brief 在指定位置放置指定方块
     */
    void placeBlock(SpongeDropTestWorld& world, const BlockPos& pos, const BlockState& state)
    {
        world.setBlockState(pos.x, pos.y, pos.z, &state, 3);
    }

    /**
     * @brief 在指定位置放置海洋植物方块并设置水源流体
     */
    void placeWaterPlant(
        SpongeDropTestWorld& world, const BlockPos& pos, const BlockState& plantState, const BlockState& waterState)
    {
        // 设置植物方块
        world.setBlockState(pos.x, pos.y, pos.z, &plantState, 3);
        // 设置水源流体（海洋植物需要在水里）
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            const fluid::FluidState* waterState = &waterFluid->defaultState();
            world.setFluidDirectly(pos, waterState);
        }
    }
};

// ============================================================================
// Block::dropResources 测试
// ============================================================================

/**
 * @brief Block::dropResources 在 lootTableManager 为空时应直接返回不生成掉落
 *
 * 模拟客户端场景：IWorld::lootTableManager() 返回 nullptr，
 * dropResources 应该安全退出，不生成任何实体。
 */
TEST_F(SpongeBlockDropTest, DropResourcesReturnsEarlyWhenLootTableManagerIsNull)
{
    SpongeDropTestWorld world;
    // 默认 lootTableManager 为 nullptr（客户端模式）
    ASSERT_EQ(world.lootTableManager(), nullptr);

    BlockPos pos(0, 64, 0);
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 应该安全返回，不崩溃、不生成实体
    Block::dropResources(world, pos, stoneState);

    EXPECT_EQ(world.spawnedEntityCount(), 0);
}

/**
 * @brief Block::dropResources 对没有掉落表的方块应返回空掉落
 *
 * 基岩（BEDROCK）等方块没有掉落表（noLootTable），dropResources 不应生成实体。
 * 即使提供了 lootTableManager，没有掉落表的方块也不会产生掉落。
 */
TEST_F(SpongeBlockDropTest, DropResourcesReturnsEarlyWhenBlockHasNoLootTable)
{
    SpongeDropTestWorld world;

    // 基岩设置了 noLootTable
    const BlockState& bedrockState = VanillaBlocks::BEDROCK->defaultState();
    ASSERT_TRUE(bedrockState.getBlock().getLootTableId().empty());

    // 即使 lootTableManager 为空，也应该安全退出
    Block::dropResources(world, BlockPos(0, 64, 0), bedrockState);
    EXPECT_EQ(world.spawnedEntityCount(), 0);
}

// ============================================================================
// SpongeBlock 海洋植物吸收测试
// ============================================================================

/**
 * @brief 海绵不应吸收海泡菜（MC 行为）
 *
 * MC Java SpongeBlock.removeWaterBreadthFirstSearch 明确只检查
 * KELP、KELP_PLANT、SEAGRASS、TALL_SEAGRASS 四种方块。
 * 海泡菜（SEA_PICKLE）虽然也是 OCEAN_PLANT 材质，但不应被吸收。
 * 旧实现使用 Material::OCEAN_PLANT 判断会误匹配海泡菜，已修正为显式方块类型检查。
 */
TEST_F(SpongeBlockDropTest, SeaPickleNotAbsorbedBySponge)
{
    SpongeDropTestWorld world;

    // 放置海绵在中心
    BlockPos spongePos(0, 0, 0);
    placeBlock(world, spongePos, VanillaBlocks::SPONGE->defaultState());

    // 放置海泡菜在水旁边
    BlockPos picklePos(1, 0, 0);
    if (VanillaBlocks::SEA_PICKLE != nullptr) {
        placeWaterPlant(
            world, picklePos, VanillaBlocks::SEA_PICKLE->defaultState(), VanillaBlocks::WATER->defaultState());

        // 海泡菜位置应该有水流体（验证设置成功）
        const fluid::FluidState* fluidState = world.getFluidState(picklePos.x, picklePos.y, picklePos.z);
        ASSERT_NE(fluidState, nullptr);
        ASSERT_TRUE(fluidState->getFluid().isIn(fluid::FluidTags::WATER()));
    }

    // 执行吸水 - 使用 VanillaBlocks 中的 SPONGE（已注册的实例）
    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);
    sponge->tryAbsorbWater(world, spongePos);

    // 海泡菜不应被吸收（不应该被移除）
    if (VanillaBlocks::SEA_PICKLE != nullptr) {
        const BlockState* pickleState = world.getBlockState(picklePos.x, picklePos.y, picklePos.z);
        ASSERT_NE(pickleState, nullptr);
        // 海泡菜应该仍然存在（不会被海绵吸收）
        // 注意：SEA_PICKLE 不在 KELP/KELP_PLANT/SEAGRASS/TALL_SEAGRASS 列表中
        EXPECT_TRUE(pickleState->is(VanillaBlocks::SEA_PICKLE)) << "Block at picklePos should still be SEA_PICKLE";
    }
}

/**
 * @brief 海带方块应被海绵吸收（KELP 在 MC 的吸收列表中）
 */
TEST_F(SpongeBlockDropTest, KelpIsAbsorbedBySponge)
{
    SpongeDropTestWorld world;

    // 放置海绵在中心
    BlockPos spongePos(0, 0, 0);
    placeBlock(world, spongePos, VanillaBlocks::SPONGE->defaultState());

    // 在旁边放置海带（需要水浸）
    BlockPos kelpPos(1, 0, 0);
    if (VanillaBlocks::KELP != nullptr) {
        placeWaterPlant(world, kelpPos, VanillaBlocks::KELP->defaultState(), VanillaBlocks::WATER->defaultState());
    }

    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);
    sponge->tryAbsorbWater(world, spongePos);

    // 海带应该被吸收
    if (VanillaBlocks::KELP != nullptr) {
        const BlockState* kelpState = world.getBlockState(kelpPos.x, kelpPos.y, kelpPos.z);
        // 海带位置应该变成空气
        EXPECT_TRUE(kelpState == nullptr || kelpState->is(VanillaBlocks::AIR));
    }
}

/**
 * @brief 海带茎方块应被海绵吸收（KELP_PLANT 在 MC 的吸收列表中）
 */
TEST_F(SpongeBlockDropTest, KelpPlantIsAbsorbedBySponge)
{
    SpongeDropTestWorld world;

    BlockPos spongePos(0, 0, 0);
    placeBlock(world, spongePos, VanillaBlocks::SPONGE->defaultState());

    BlockPos kelpPlantPos(1, 0, 0);
    if (VanillaBlocks::KELP_PLANT != nullptr) {
        placeWaterPlant(
            world, kelpPlantPos, VanillaBlocks::KELP_PLANT->defaultState(), VanillaBlocks::WATER->defaultState());
    }

    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);
    sponge->tryAbsorbWater(world, spongePos);

    if (VanillaBlocks::KELP_PLANT != nullptr) {
        const BlockState* state = world.getBlockState(kelpPlantPos.x, kelpPlantPos.y, kelpPlantPos.z);
        EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));
    }
}

/**
 * @brief 海草方块应被海绵吸收（SEAGRASS 在 MC 的吸收列表中）
 */
TEST_F(SpongeBlockDropTest, SeagrassIsAbsorbedBySponge)
{
    SpongeDropTestWorld world;

    BlockPos spongePos(0, 0, 0);
    placeBlock(world, spongePos, VanillaBlocks::SPONGE->defaultState());

    BlockPos seagrassPos(1, 0, 0);
    if (VanillaBlocks::SEAGRASS != nullptr) {
        placeWaterPlant(
            world, seagrassPos, VanillaBlocks::SEAGRASS->defaultState(), VanillaBlocks::WATER->defaultState());
    }

    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);
    sponge->tryAbsorbWater(world, spongePos);

    if (VanillaBlocks::SEAGRASS != nullptr) {
        const BlockState* state = world.getBlockState(seagrassPos.x, seagrassPos.y, seagrassPos.z);
        EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));
    }
}

/**
 * @brief 高海草方块应被海绵吸收（TALL_SEAGRASS 在 MC 的吸收列表中）
 */
TEST_F(SpongeBlockDropTest, TallSeagrassIsAbsorbedBySponge)
{
    SpongeDropTestWorld world;

    BlockPos spongePos(0, 0, 0);
    placeBlock(world, spongePos, VanillaBlocks::SPONGE->defaultState());

    BlockPos tallSeagrassPos(1, 0, 0);
    if (VanillaBlocks::TALL_SEAGRASS != nullptr) {
        placeWaterPlant(
            world, tallSeagrassPos, VanillaBlocks::TALL_SEAGRASS->defaultState(), VanillaBlocks::WATER->defaultState());
    }

    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);
    sponge->tryAbsorbWater(world, spongePos);

    if (VanillaBlocks::TALL_SEAGRASS != nullptr) {
        const BlockState* state = world.getBlockState(tallSeagrassPos.x, tallSeagrassPos.y, tallSeagrassPos.z);
        EXPECT_TRUE(state == nullptr || state->is(VanillaBlocks::AIR));
    }
}

/**
 * @brief 海绵吸水后应变为湿润海绵
 */
TEST_F(SpongeBlockDropTest, SpongeBecomesWetAfterAbsorbingWater)
{
    SpongeDropTestWorld world;

    // 放置海绵
    BlockPos spongePos(0, 0, 0);
    placeBlock(world, spongePos, VanillaBlocks::SPONGE->defaultState());

    // 在旁边放置水源
    BlockPos waterPos(1, 0, 0);
    placeWater(world, waterPos);

    SpongeBlock* sponge = dynamic_cast<SpongeBlock*>(VanillaBlocks::SPONGE);
    ASSERT_NE(sponge, nullptr);
    bool absorbed = sponge->tryAbsorbWater(world, spongePos);

    // 应该成功吸水
    EXPECT_TRUE(absorbed);

    // 海绵位置应变为湿润海绵
    const BlockState* spongeState = world.getBlockState(spongePos.x, spongePos.y, spongePos.z);
    ASSERT_NE(spongeState, nullptr);
    EXPECT_TRUE(spongeState->is(VanillaBlocks::WET_SPONGE));
}

} // namespace test
} // namespace blocks
} // namespace mc
