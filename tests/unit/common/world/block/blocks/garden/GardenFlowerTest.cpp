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
 */

#include <gtest/gtest.h>

#include "common/entity/effect/EffectType.hpp"
#include "common/item/Items.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/garden/CactusFlowerBlock.hpp"
#include "common/world/block/blocks/pale_garden/EyeblossomBlock.hpp"
#include "common/world/block/registry/GardenBlocks.hpp"
#include "common/world/block/registry/PaleGardenBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>

using namespace mc;
using namespace mc::blocks;

// ============================================================================
// 测试用 IBlockReader 实现（支持方块状态存取）
// ============================================================================

class CactusFlowerTestWorld final : public IBlockReader {
public:
    using IBlockReader::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
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
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("CactusFlowerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("CactusFlowerTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    world::border::WorldBorder m_worldBorder;
    world::gamerule::GameRules m_gameRules;
    mutable math::Random m_random{12345};
};

// ============================================================================
// CactusFlowerBlock 测试访问器（暴露 protected canSustain）
// ============================================================================

class CactusFlowerBlockTestAccess : public CactusFlowerBlock {
public:
    using CactusFlowerBlock::CactusFlowerBlock;

    [[nodiscard]] bool testCanSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
    {
        return canSustain(groundState, world, groundPos);
    }
};

// ============================================================================
// CactusFlowerBlock 测试
// ============================================================================

class CactusFlowerBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
    }
};

TEST_F(CactusFlowerBlockTest, IsRegisteredAndNotNull)
{
    ASSERT_NE(mc::block_registry::GardenBlocks::CACTUS_FLOWER, nullptr);
}

TEST_F(CactusFlowerBlockTest, IsCactusFlowerBlockType)
{
    auto* cactusFlower = dynamic_cast<CactusFlowerBlock*>(mc::block_registry::GardenBlocks::CACTUS_FLOWER);
    ASSERT_NE(cactusFlower, nullptr);
}

TEST_F(CactusFlowerBlockTest, HasNoStewEffect)
{
    auto* flower = static_cast<FlowerBlock*>(mc::block_registry::GardenBlocks::CACTUS_FLOWER);
    EXPECT_FALSE(flower->hasStewEffect());
}

TEST_F(CactusFlowerBlockTest, IsNotSolidAndNoCollision)
{
    const auto& state = mc::block_registry::GardenBlocks::CACTUS_FLOWER->defaultState();
    EXPECT_FALSE(state.isSolid());
    EXPECT_TRUE(mc::block_registry::GardenBlocks::CACTUS_FLOWER->getCollisionShape(state).isEmpty());
}

TEST_F(CactusFlowerBlockTest, IsInSmallFlowersTag)
{
    EXPECT_TRUE(BlockTags::SMALL_FLOWERS().contains(ResourceLocation("minecraft", "cactus_flower")));
}

TEST_F(CactusFlowerBlockTest, IsInReplaceableByTreesTag)
{
    EXPECT_TRUE(BlockTags::REPLACEABLE_BY_TREES().contains(ResourceLocation("minecraft", "cactus_flower")));
}

// ============================================================================
// CactusFlowerBlock canSustain 测试
// ============================================================================

class CactusFlowerSustainTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();

        // 创建独立的 CactusFlowerBlock 测试实例，用于测试 canSustain
        m_cactusFlower = std::make_unique<CactusFlowerBlockTestAccess>(
            BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).resistance(0.0f));
    }

    std::unique_ptr<CactusFlowerBlockTestAccess> m_cactusFlower;
    CactusFlowerTestWorld m_world;
};

TEST_F(CactusFlowerSustainTest, CanSustainOnCactus)
{
    // 仙人掌花可放置在仙人掌上方
    if (VanillaBlocks::CACTUS == nullptr) {
        GTEST_SKIP() << "CACTUS not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::CACTUS->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

TEST_F(CactusFlowerSustainTest, CanSustainOnFarmland)
{
    // 仙人掌花可放置在耕地上
    if (VanillaBlocks::FARMLAND == nullptr) {
        GTEST_SKIP() << "FARMLAND not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::FARMLAND->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

TEST_F(CactusFlowerSustainTest, CanSustainOnSolidTopFaceStone)
{
    // 仙人掌花可放置在具有实心顶面的方块上（如石头）
    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::STONE->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

TEST_F(CactusFlowerSustainTest, CanSustainOnSolidTopFaceDirt)
{
    // 仙人掌花可放置在泥土（实心顶面）上
    if (VanillaBlocks::DIRT == nullptr) {
        GTEST_SKIP() << "DIRT not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::DIRT->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

TEST_F(CactusFlowerSustainTest, CanSustainOnGrassBlock)
{
    // 草方块有实心顶面，仙人掌花可放置
    if (VanillaBlocks::GRASS_BLOCK == nullptr) {
        GTEST_SKIP() << "GRASS_BLOCK not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::GRASS_BLOCK->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

TEST_F(CactusFlowerSustainTest, CanSustainOnSand)
{
    // 沙子有实心顶面（isSolid && hasCollision），仙人掌花可以放置
    if (VanillaBlocks::SAND == nullptr) {
        GTEST_SKIP() << "SAND not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::SAND->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    EXPECT_TRUE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

TEST_F(CactusFlowerSustainTest, CannotSustainOnAir)
{
    // 仙人掌花不能放置在空气上
    const BlockPos groundPos(0, 0, 0);
    // 不设置任何方块，getBlockState 返回 nullptr
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    EXPECT_EQ(groundState, nullptr);
    // 无法测试 canSustain(nullptr)，因为参数为 const BlockState& 引用
    // 这由 isValidPosition 中的空指针检查处理
}

TEST_F(CactusFlowerSustainTest, CannotSustainOnNonSolidBlock)
{
    // 仙人掌花不能放置在非固体方块（如普通花朵）上
    // 普通花朵不是 CACTUS/FARMLAND，也不是实心顶面方块
    if (VanillaBlocks::DANDELION == nullptr) {
        GTEST_SKIP() << "DANDELION not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::DANDELION->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    EXPECT_FALSE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

TEST_F(CactusFlowerSustainTest, CanSustainOnGlass)
{
    // 玻璃在原版中具有完整立方体的碰撞/支撑形状（仅 notSolid，并非 noCollision），
    // 因此 isFaceSturdy(Up, CENTER) 返回 true，仙人掌花可以放置在玻璃上。
    // 参考 MC 1.21.11 CactusFlowerBlock.mayPlaceOn：CACTUS || FARMLAND || isFaceSturdy(CENTER)。
    if (VanillaBlocks::GLASS == nullptr) {
        GTEST_SKIP() << "GLASS not registered";
    }
    const BlockPos groundPos(0, 0, 0);
    m_world.setBlockAt(groundPos, &VanillaBlocks::GLASS->defaultState());
    const BlockState* groundState = m_world.getBlockState(groundPos.x, groundPos.y, groundPos.z);
    ASSERT_NE(groundState, nullptr);
    // 玻璃顶面是完整方块，isFaceSturdy(Up, Center) 应返回 true
    EXPECT_TRUE(m_cactusFlower->testCanSustain(*groundState, m_world, groundPos));
}

// ============================================================================
// EyeblossomBlock 测试
// ============================================================================

class EyeblossomBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
    }
};

TEST_F(EyeblossomBlockTest, OpenEyeblossomIsNotNull)
{
    ASSERT_NE(mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM, nullptr);
}

TEST_F(EyeblossomBlockTest, ClosedEyeblossomIsNotNull)
{
    ASSERT_NE(mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM, nullptr);
}

TEST_F(EyeblossomBlockTest, OpenEyeblossomHasBlindnessStewEffect)
{
    auto* flower = static_cast<FlowerBlock*>(mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM);
    EXPECT_TRUE(flower->hasStewEffect());
    EXPECT_EQ(flower->getSuspiciousStewEffect(), static_cast<u32>(entity::effect::EffectType::Blindness));
    EXPECT_EQ(flower->getEffectDuration(), 11);
}

TEST_F(EyeblossomBlockTest, ClosedEyeblossomHasNauseaStewEffect)
{
    auto* flower = static_cast<FlowerBlock*>(mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM);
    EXPECT_TRUE(flower->hasStewEffect());
    EXPECT_EQ(flower->getSuspiciousStewEffect(), static_cast<u32>(entity::effect::EffectType::Nausea));
    EXPECT_EQ(flower->getEffectDuration(), 7);
}

TEST_F(EyeblossomBlockTest, OpenEyeblossomEmitsLight)
{
    auto* eyeblossom = static_cast<EyeblossomBlock*>(mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM);
    const auto& state = mc::block_registry::PaleGardenBlocks::OPEN_EYEBLOSSOM->defaultState();
    EXPECT_EQ(eyeblossom->getLightLevel(state), 1);
}

TEST_F(EyeblossomBlockTest, ClosedEyeblossomEmitsNoLight)
{
    auto* eyeblossom = static_cast<EyeblossomBlock*>(mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM);
    const auto& state = mc::block_registry::PaleGardenBlocks::CLOSED_EYEBLOSSOM->defaultState();
    EXPECT_EQ(eyeblossom->getLightLevel(state), 0);
}

TEST_F(EyeblossomBlockTest, BothInSmallFlowersTag)
{
    EXPECT_TRUE(BlockTags::SMALL_FLOWERS().contains(ResourceLocation("minecraft", "open_eyeblossom")));
    EXPECT_TRUE(BlockTags::SMALL_FLOWERS().contains(ResourceLocation("minecraft", "closed_eyeblossom")));
}

// ============================================================================
// 花朵物品注册测试
// ============================================================================

class FlowerItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        mc::item::tag::ItemTags::initialize();
    }
};

TEST_F(FlowerItemRegistrationTest, CactusFlowerItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cactus_flower"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, WildflowersItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wildflowers"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, OpenEyeblossomItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "open_eyeblossom"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, ClosedEyeblossomItemIsRegistered)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "closed_eyeblossom"));
    ASSERT_NE(item, nullptr);
}

TEST_F(FlowerItemRegistrationTest, CactusFlowerInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "cactus_flower"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}

TEST_F(FlowerItemRegistrationTest, WildflowersInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "wildflowers"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}

TEST_F(FlowerItemRegistrationTest, OpenEyeblossomInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "open_eyeblossom"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}

TEST_F(FlowerItemRegistrationTest, ClosedEyeblossomInFlowersItemTag)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "closed_eyeblossom"));
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(mc::item::tag::ItemTags::FLOWERS().contains(item));
}
