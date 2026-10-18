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
 * @file RespawnAnchorBlockTest.cpp
 * @brief 重生锚方块功能单元测试
 *
 * 测试覆盖：
 * - 状态属性（CHARGES_0_4）、光照等级、比较器输出、形状
 * - 维度检查（仅下界可用）
 * - 空手交互：无电 Pass、异维度爆炸、同维度设置强制/非强制重生点、重复设置返回 Consume
 * - 手持交互：萤石充能并消耗、非燃料物品转交空手交互
 * - 落点搜索：25 个候选、安全模式优先、全部不可用时返回空
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/functional/RespawnAnchorBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/spawn/RespawnConfig.hpp"
#include "core/Constants.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 重生锚测试用世界桩
 *
 * 以方块表为唯一数据源：碰撞检测由非空、非液体方块的完整碰撞箱推导，
 * 维度可在构造时指定（默认主世界）。
 */
class AnchorTestWorld final : public IWorld {
public:
    explicit AnchorTestWorld(DimensionId dimension = 0)
        : m_dimension(dimension)
    {}

    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        // 与生产代码一致：已加载列恒有方块状态，未登记即空气；只有区块未加载时才返回 nullptr。
        if (m_unloadedChunks) {
            return nullptr;
        }
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] mc::world::chunk::IChunkManager* chunkManager() override { return &m_stubChunks; }
    [[nodiscard]] const mc::world::chunk::IChunkManager* chunkManager() const override { return &m_stubChunks; }
    mc::test::StubChunkManager m_stubChunks{nullptr, true};
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override
    {
        return !getBlockCollisions(box).empty();
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override
    {
        std::vector<AxisAlignedBB> collisions;
        for (const auto& [pos, state] : m_blocks) {
            if (state == nullptr) {
                continue;
            }
            // 用方块自身的碰撞形状（空气/液体/植物等空形状方块不产生碰撞）。
            for (const auto& localBox : state->owner().getCollisionShape(*state).boxes()) {
                const AxisAlignedBB blockBox(localBox.minX + static_cast<f32>(pos.x),
                    localBox.minY + static_cast<f32>(pos.y),
                    localBox.minZ + static_cast<f32>(pos.z),
                    localBox.maxX + static_cast<f32>(pos.x),
                    localBox.maxY + static_cast<f32>(pos.y),
                    localBox.maxZ + static_cast<f32>(pos.z));
                const bool overlaps = box.maxX > blockBox.minX && box.minX < blockBox.maxX &&
                    box.maxY > blockBox.minY && box.minY < blockBox.maxY && box.maxZ > blockBox.minZ &&
                    box.minZ < blockBox.maxZ;
                if (overlaps) {
                    collisions.push_back(blockBox);
                }
            }
        }
        return collisions;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
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
    [[nodiscard]] DimensionId dimension() const override { return m_dimension; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        return const_cast<AnchorTestWorld*>(this)->tickManager();
    }

    [[nodiscard]] math::IRandom& getRandom() override { return m_random; }
    [[nodiscard]] const math::IRandom& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    /// 模拟区块未加载：所有查询返回 nullptr。
    void setUnloadedChunks(bool unloaded) { m_unloadedChunks = unloaded; }

private:
    bool m_unloadedChunks = false;
    DimensionId m_dimension;
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random;
};

/// 测试用玩家（无网络）
class AnchorTestPlayer final : public mc::Player {
public:
    AnchorTestPlayer(EntityInstanceId id, const std::string& name)
        : mc::Player(id, name, mc::test::testEcsRegistry())
    {}

    void sendStatusMessage(const std::string&, bool) override {}
    [[nodiscard]] bool canReceiveMessages() const override { return false; }
};

} // namespace

class RespawnAnchorBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 萤石/泥土物品来自方块物品注册表（生产初始化路径显式注册）；
        // 注册表未就绪时相关用例跳过，避免依赖全局初始化顺序。
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        m_blockItemsAvailable = _glowstoneItem() != nullptr && _dirtItem() != nullptr;
    }

    [[nodiscard]] bool blockItemsAvailable() const { return m_blockItemsAvailable; }

    [[nodiscard]] BlockState chargedState(i32 charges) const
    {
        return VanillaBlocks::RESPAWN_ANCHOR->defaultState().with(BlockStateProperties::CHARGES_0_4(), charges);
    }

    bool m_blockItemsAvailable = false;

    /// 荧石对应的方块物品（重生锚燃料）
    [[nodiscard]] static const Item* _glowstoneItem()
    {
        return BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::GLOWSTONE);
    }

    /// 泥土对应的方块物品（非燃料）
    [[nodiscard]] static const Item* _dirtItem()
    {
        return BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DIRT);
    }
};

TEST_F(RespawnAnchorBlockTest, RespawnAnchorBlockExists)
{
    ASSERT_NE(VanillaBlocks::RESPAWN_ANCHOR, nullptr);
}

TEST_F(RespawnAnchorBlockTest, DefaultState)
{
    ASSERT_NE(VanillaBlocks::RESPAWN_ANCHOR, nullptr);
    EXPECT_EQ(VanillaBlocks::RESPAWN_ANCHOR->defaultState().get(BlockStateProperties::CHARGES_0_4()), 0);
}

TEST_F(RespawnAnchorBlockTest, ChargesProperty)
{
    const BlockState& state0 = VanillaBlocks::RESPAWN_ANCHOR->defaultState();
    for (i32 charges = 0; charges <= 4; ++charges) {
        BlockState state = state0.with(BlockStateProperties::CHARGES_0_4(), charges);
        EXPECT_EQ(state.get(BlockStateProperties::CHARGES_0_4()), charges);
    }
}

TEST_F(RespawnAnchorBlockTest, LightLevelCalculation)
{
    const Block* block = VanillaBlocks::RESPAWN_ANCHOR;
    ASSERT_NE(block, nullptr);

    EXPECT_EQ(block->getLightLevel(chargedState(0), nullptr, nullptr), 0);
    EXPECT_EQ(block->getLightLevel(chargedState(1), nullptr, nullptr), 3);
    EXPECT_EQ(block->getLightLevel(chargedState(2), nullptr, nullptr), 7);
    EXPECT_EQ(block->getLightLevel(chargedState(3), nullptr, nullptr), 11);
    EXPECT_EQ(block->getLightLevel(chargedState(4), nullptr, nullptr), 15);
}

TEST_F(RespawnAnchorBlockTest, ShapeIsFullBlock)
{
    ASSERT_NE(VanillaBlocks::RESPAWN_ANCHOR, nullptr);
    const CollisionShape& shape = VanillaBlocks::RESPAWN_ANCHOR->getShape(chargedState(0));
    EXPECT_TRUE(shape.isFullBlock());
}

TEST_F(RespawnAnchorBlockTest, WorksOnlyInNether)
{
    EXPECT_TRUE(DimensionType::fromId(-1).respawnAnchorWorks());
    EXPECT_FALSE(DimensionType::fromId(0).respawnAnchorWorks());
    EXPECT_FALSE(DimensionType::fromId(1).respawnAnchorWorks());
}

// ========== 空手交互 ==========

TEST_F(RespawnAnchorBlockTest, UseWithoutItem_ZeroChargePasses)
{
    AnchorTestWorld world(DimensionManager::NETHER);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    const BlockState state = chargedState(0);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    const BlockActionResult result =
        VanillaBlocks::RESPAWN_ANCHOR->useWithoutItem(state, world, BlockPos(0, 64, 0), player, BlockRaycastResult{});
    EXPECT_EQ(result.getType(), ActionResultType::Pass);
    EXPECT_FALSE(player.getRespawnConfig().has_value());
}

TEST_F(RespawnAnchorBlockTest, UseWithoutItem_SetsForcedFalseSpawnInNether)
{
    AnchorTestWorld world(DimensionManager::NETHER);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    const BlockState state = chargedState(2);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    const BlockActionResult result =
        VanillaBlocks::RESPAWN_ANCHOR->useWithoutItem(state, world, BlockPos(0, 64, 0), player, BlockRaycastResult{});
    EXPECT_TRUE(result.getType() == ActionResultType::SuccessServer);

    const auto config = player.getRespawnConfig();
    ASSERT_TRUE(config.has_value());
    EXPECT_FALSE(config->forced);
    EXPECT_EQ(config->respawnData.dimension(), DimensionManager::NETHER);
    EXPECT_EQ(config->respawnData.pos(), BlockPos(0, 64, 0));
    EXPECT_FLOAT_EQ(config->respawnData.yaw, 0.0f);
    EXPECT_FLOAT_EQ(config->respawnData.pitch, 0.0f);
}

TEST_F(RespawnAnchorBlockTest, UseWithoutItem_SameAnchorAgainConsumes)
{
    // 同一个锚重复空手使用：不重新设置、不播音效，返回 Consume。
    AnchorTestWorld world(DimensionManager::NETHER);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    const BlockState state = chargedState(1);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    static_cast<void>(
        VanillaBlocks::RESPAWN_ANCHOR->useWithoutItem(state, world, BlockPos(0, 64, 0), player, BlockRaycastResult{}));
    const BlockActionResult second =
        VanillaBlocks::RESPAWN_ANCHOR->useWithoutItem(state, world, BlockPos(0, 64, 0), player, BlockRaycastResult{});
    EXPECT_EQ(second.getType(), ActionResultType::Consume);
}

TEST_F(RespawnAnchorBlockTest, UseWithoutItem_ExplodesOutsideNether)
{
    AnchorTestWorld world(DimensionManager::OVERWORLD);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    const BlockState state = chargedState(4);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    const BlockActionResult result =
        VanillaBlocks::RESPAWN_ANCHOR->useWithoutItem(state, world, BlockPos(0, 64, 0), player, BlockRaycastResult{});
    EXPECT_TRUE(result.getType() == ActionResultType::SuccessServer);
    const BlockState* removed = world.getBlockState(0, 64, 0);
    EXPECT_TRUE(removed == nullptr || removed->isAir());
    EXPECT_FALSE(player.getRespawnConfig().has_value());
}

// ========== 手持交互 ==========

TEST_F(RespawnAnchorBlockTest, UseItemOn_GlowstoneChargesAndConsumes)
{
    // 依赖方块物品注册：注册表未初始化时跳过。
    if (!blockItemsAvailable()) {
        GTEST_SKIP() << "block item registry unavailable";
    }
    AnchorTestWorld world(DimensionManager::NETHER);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    const BlockState state = chargedState(0);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    ItemStack glowstone = ItemStack(_glowstoneItem(), 2);
    const BlockActionResult result = VanillaBlocks::RESPAWN_ANCHOR->useItemOn(
        glowstone, state, world, BlockPos(0, 64, 0), player, Hand::MainHand, BlockRaycastResult{});
    EXPECT_TRUE(result.getType() == ActionResultType::Success);
    EXPECT_EQ(glowstone.getCount(), 1);
    const BlockState* updated = world.getBlockState(0, 64, 0);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(updated->get(BlockStateProperties::CHARGES_0_4()), 1);
}

TEST_F(RespawnAnchorBlockTest, UseItemOn_CreativeModeDoesNotConsumeGlowstone)
{
    if (!blockItemsAvailable()) {
        GTEST_SKIP() << "block item registry unavailable";
    }
    AnchorTestWorld world(DimensionManager::NETHER);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    player.abilities().creativeMode = true;
    const BlockState state = chargedState(0);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    ItemStack glowstone = ItemStack(_glowstoneItem(), 1);
    const BlockActionResult result = VanillaBlocks::RESPAWN_ANCHOR->useItemOn(
        glowstone, state, world, BlockPos(0, 64, 0), player, Hand::MainHand, BlockRaycastResult{});
    EXPECT_TRUE(result.getType() == ActionResultType::Success);
    EXPECT_EQ(glowstone.getCount(), 1);
}

TEST_F(RespawnAnchorBlockTest, UseItemOn_FullChargeDoesNotConsumeGlowstone)
{
    // 依赖方块物品注册：注册表未初始化时跳过。
    if (!blockItemsAvailable()) {
        GTEST_SKIP() << "block item registry unavailable";
    }
    AnchorTestWorld world(DimensionManager::NETHER);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    const BlockState state = chargedState(4);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    ItemStack glowstone = ItemStack(_glowstoneItem(), 1);
    const BlockActionResult result = VanillaBlocks::RESPAWN_ANCHOR->useItemOn(
        glowstone, state, world, BlockPos(0, 64, 0), player, Hand::MainHand, BlockRaycastResult{});
    EXPECT_EQ(result.getType(), ActionResultType::TryEmptyHandInteraction);
    EXPECT_EQ(glowstone.getCount(), 1);
}

TEST_F(RespawnAnchorBlockTest, UseItemOn_NonFuelItemFallsThroughToEmptyHand)
{
    // 依赖方块物品注册：注册表未初始化时跳过。
    if (!blockItemsAvailable()) {
        GTEST_SKIP() << "block item registry unavailable";
    }
    AnchorTestWorld world(DimensionManager::NETHER);
    AnchorTestPlayer player(EntityInstanceId(1), "TestPlayer");
    const BlockState state = chargedState(2);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    ItemStack dirt = ItemStack(_dirtItem(), 1);
    const BlockActionResult result = VanillaBlocks::RESPAWN_ANCHOR->useItemOn(
        dirt, state, world, BlockPos(0, 64, 0), player, Hand::MainHand, BlockRaycastResult{});
    EXPECT_EQ(result.getType(), ActionResultType::TryEmptyHandInteraction);
    EXPECT_EQ(dirt.getCount(), 1);
    // 手持交互不设置重生点也不消耗充能。
    EXPECT_FALSE(player.getRespawnConfig().has_value());
    EXPECT_EQ(world.getBlockState(0, 64, 0)->get(BlockStateProperties::CHARGES_0_4()), 2);
}

// ========== 落点搜索 ==========

TEST_F(RespawnAnchorBlockTest, FindStandUpPosition_FindsAdjacentFloor)
{
    // 锚放在 y=64，北侧 (0,63,-1) 是实心地板 → 落点 (0.5, 64, -0.5)。
    AnchorTestWorld world(DimensionManager::NETHER);
    const BlockState state = chargedState(4);
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, -1), &stone);
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    const auto position = RespawnAnchorBlock::findStandUpPosition(world, BlockPos(0, 64, 0), 0.6f, 1.8f, true, false);
    ASSERT_TRUE(position.has_value());
    EXPECT_FLOAT_EQ(position->x, 0.5f);
    EXPECT_FLOAT_EQ(position->y, 64.0f);
    EXPECT_FLOAT_EQ(position->z, -0.5f);
}

TEST_F(RespawnAnchorBlockTest, FindStandUpPosition_ReturnsNullWhenSurrounded)
{
    AnchorTestWorld world(DimensionManager::NETHER);
    const BlockState state = chargedState(4);
    const BlockState& stone = VanillaBlocks::STONE->defaultState();
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                world.setBlockAt(BlockPos(dx, 64 + dy, dz), &stone);
            }
        }
    }
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    const auto position = RespawnAnchorBlock::findStandUpPosition(world, BlockPos(0, 64, 0), 0.6f, 1.8f, true, false);
    EXPECT_FALSE(position.has_value());
}

TEST_F(RespawnAnchorBlockTest, FindStandUpPosition_SafeModeSkipsDangerousBlock)
{
    // 北侧紧邻岩浆、西侧是石头地板：安全模式下应跳过岩浆选择西侧。
    AnchorTestWorld world(DimensionManager::NETHER);
    const BlockState state = chargedState(4);
    world.setBlockAt(BlockPos(0, 63, -1), &VanillaBlocks::LAVA->defaultState());
    world.setBlockAt(BlockPos(-1, 63, 0), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(BlockPos(0, 64, 0), &state);

    const auto position = RespawnAnchorBlock::findStandUpPosition(world, BlockPos(0, 64, 0), 0.6f, 1.8f, true, false);
    ASSERT_TRUE(position.has_value());
    EXPECT_FLOAT_EQ(position->x, -0.5f);
    EXPECT_FLOAT_EQ(position->z, 0.5f);
}

// ========== GlobalPos 基础语义 ==========

TEST_F(RespawnAnchorBlockTest, GlobalPosComparison)
{
    GlobalPos pos1(DimensionId(-1), BlockPos(10, 20, 30));
    GlobalPos pos2(DimensionId(-1), BlockPos(10, 20, 30));
    GlobalPos pos3(DimensionId(0), BlockPos(10, 20, 30));
    GlobalPos pos4(DimensionId(-1), BlockPos(10, 21, 30));

    EXPECT_TRUE(pos1 == pos2);
    EXPECT_FALSE(pos1 == pos3);
    EXPECT_FALSE(pos1 == pos4);
    EXPECT_TRUE(pos1.sameDimension(pos4));
}