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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/cave/CaveVinesBlock.hpp"
#include "common/world/block/blocks/cave/CaveVinesPlantBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/BlockRaycastResult.hpp"
#include "core/Constants.hpp"
#include "util/Direction.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/Fluids.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 洞穴藤蔓测试用 Mock 世界实现
 */
class CaveVinesTestWorld final : public mc::test::BaseTestWorld {
public:
    CaveVinesTestWorld() { VanillaBlocks::initialize(); }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        if (VanillaBlocks::AIR) {
            return &VanillaBlocks::AIR->defaultState();
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(BlockPos(x, y, z));
        } else {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        }
        m_lastSetBlockFlags = m_currentSetBlockFlags;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        m_currentSetBlockFlags = flags;
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        Entity* rawPtr = entity.get();
        EntityInstanceId id = static_cast<EntityInstanceId>(m_spawnedEntities.size() + 1);
        m_spawnedEntities.push_back(std::move(entity));
        m_spawnedEntityPtrs.push_back(rawPtr);
        return id;
    }

    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_soundPlayed = true;
        m_lastSoundId = soundId;
        m_lastSoundPos = pos;
        m_lastSoundVolume = volume;
        m_lastSoundPitch = pitch;
        MC_UNUSED(category);
    }

    // TickManager 接口（测试存根）
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        static world::tick::TickManager dummy(*static_cast<IWorld*>(nullptr));
        return dummy;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        static world::tick::TickManager dummy(*const_cast<CaveVinesTestWorld*>(this));
        return dummy;
    }

    // 测试辅助方法
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }
    [[nodiscard]] Entity* getSpawnedEntity(size_t index) const
    {
        if (index < m_spawnedEntityPtrs.size()) {
            return m_spawnedEntityPtrs[index];
        }
        return nullptr;
    }

    [[nodiscard]] bool wasSoundPlayed() const { return m_soundPlayed; }
    [[nodiscard]] const ResourceLocation& lastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] f32 lastSoundVolume() const { return m_lastSoundVolume; }
    [[nodiscard]] f32 lastSoundPitch() const { return m_lastSoundPitch; }
    [[nodiscard]] i32 lastSetBlockFlags() const { return m_lastSetBlockFlags; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<Entity*> m_spawnedEntityPtrs;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    bool m_soundPlayed = false;
    ResourceLocation m_lastSoundId;
    Vector3 m_lastSoundPos{0.0f, 0.0f, 0.0f};
    f32 m_lastSoundVolume = 1.0f;
    f32 m_lastSoundPitch = 1.0f;
    i32 m_currentSetBlockFlags = 0;
    i32 m_lastSetBlockFlags = 0;
};

} // anonymous namespace

// ============================================================================
// CaveVinesBlock 基础属性测试
// ============================================================================

class CaveVinesBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();

        m_vines =
            std::make_unique<CaveVinesBlock>(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    }

    void TearDown() override { m_vines.reset(); }

    std::unique_ptr<CaveVinesBlock> m_vines;
};

TEST_F(CaveVinesBlockTest, DefaultStateHasNoBerries)
{
    const BlockState& state = m_vines->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::BERRIES()));
}

TEST_F(CaveVinesBlockTest, DefaultStateAgeIsZero)
{
    const BlockState& state = m_vines->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::AGE_0_25()), 0);
}

TEST_F(CaveVinesBlockTest, LightLevelWithBerries)
{
    const BlockState& withBerries = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    const BlockState& withoutBerries = m_vines->defaultState().with(BlockStateProperties::BERRIES(), false);

    EXPECT_EQ(m_vines->getLightLevel(withBerries), 14);
    EXPECT_EQ(m_vines->getLightLevel(withoutBerries), 0);
}

TEST_F(CaveVinesBlockTest, GetCloneItemStackReturnsGlowBerries)
{
    const BlockState& state = m_vines->defaultState();
    ItemStack stack = m_vines->getCloneItemStack(state);

    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem()->itemId(), Items::GLOW_BERRIES->itemId());
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(CaveVinesBlockTest, GetCloneItemStackWithBerries)
{
    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    ItemStack stack = m_vines->getCloneItemStack(state);

    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem()->itemId(), Items::GLOW_BERRIES->itemId());
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// CaveVinesBlock 交互测试
// ============================================================================

TEST_F(CaveVinesBlockTest, OnBlockActivated_WithBerries_ReturnsSuccess)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(0, 64, 0);

    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = m_vines->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
}

TEST_F(CaveVinesBlockTest, OnBlockActivated_WithBerries_SetsBerriesFalse)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(0, 64, 0);

    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    m_vines->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    // 方块状态应该变为 BERRIES=false
    const BlockState* currentState = world.getBlockState(0, 64, 0);
    ASSERT_NE(currentState, nullptr);
    EXPECT_FALSE(currentState->get(BlockStateProperties::BERRIES()));
}

TEST_F(CaveVinesBlockTest, OnBlockActivated_WithBerries_UsesFlag2)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(0, 64, 0);

    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    m_vines->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    // MC 1.21.11: setBlockState 使用 flag=2
    EXPECT_EQ(world.lastSetBlockFlags(), 2);
}

TEST_F(CaveVinesBlockTest, OnBlockActivated_WithBerries_DropsGlowBerries)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(10, 64, 10);

    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(10.5f, 64.5f, 10.5f), pos, Direction::Up, 1.0f);
    m_vines->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    // 应该生成1个物品实体
    EXPECT_EQ(world.spawnedEntityCount(), 1u);

    // 验证是发光浆果物品实体
    Entity* entity = world.getSpawnedEntity(0);
    ASSERT_NE(entity, nullptr);
    ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
    ASSERT_NE(itemEntity, nullptr);

    const ItemStack& itemStack = itemEntity->getItemStack();
    EXPECT_EQ(itemStack.getCount(), 1);
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_EQ(itemStack.getItem(), Items::GLOW_BERRIES);
}

TEST_F(CaveVinesBlockTest, OnBlockActivated_WithBerries_PlaysPickSound)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(20, 64, 20);

    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(20.5f, 64.5f, 20.5f), pos, Direction::Up, 1.0f);
    m_vines->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    // 应该播放采摘音效
    EXPECT_TRUE(world.wasSoundPlayed());
    EXPECT_EQ(world.lastSoundId(), SoundEvents::BLOCK_CAVE_VINES_PICK_BERRIES);
    EXPECT_FLOAT_EQ(world.lastSoundVolume(), 1.0f);
    // 音调应在0.8-1.2之间
    EXPECT_GE(world.lastSoundPitch(), 0.8f);
    EXPECT_LE(world.lastSoundPitch(), 1.2f);
}

TEST_F(CaveVinesBlockTest, OnBlockActivated_WithBerries_ClientSide_NoDropNoSound)
{
    CaveVinesTestWorld world;
    world.setClientSide(true); // 客户端
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(30, 64, 30);

    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(30.5f, 64.5f, 30.5f), pos, Direction::Up, 1.0f);
    auto result = m_vines->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    // 客户端仍应返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 客户端不应该掉落物品
    EXPECT_EQ(world.spawnedEntityCount(), 0u);

    // 客户端不应该播放音效
    EXPECT_FALSE(world.wasSoundPlayed());
}

TEST_F(CaveVinesBlockTest, OnBlockActivated_WithoutBerries_ReturnsPass)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(40, 64, 40);

    const BlockState& state = m_vines->defaultState().with(BlockStateProperties::BERRIES(), false);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(40.5f, 64.5f, 40.5f), pos, Direction::Up, 1.0f);
    auto result = m_vines->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Pass);

    // 不应该掉落物品
    EXPECT_EQ(world.spawnedEntityCount(), 0u);

    // 不应该播放音效
    EXPECT_FALSE(world.wasSoundPlayed());
}

// ============================================================================
// CaveVinesPlantBlock 基础属性测试
// ============================================================================

class CaveVinesPlantBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();

        m_plant = std::make_unique<CaveVinesPlantBlock>(
            BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());
    }

    void TearDown() override { m_plant.reset(); }

    std::unique_ptr<CaveVinesPlantBlock> m_plant;
};

TEST_F(CaveVinesPlantBlockTest, DefaultStateHasNoBerries)
{
    const BlockState& state = m_plant->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::BERRIES()));
}

TEST_F(CaveVinesPlantBlockTest, LightLevelWithBerries)
{
    const BlockState& withBerries = m_plant->defaultState().with(BlockStateProperties::BERRIES(), true);
    const BlockState& withoutBerries = m_plant->defaultState().with(BlockStateProperties::BERRIES(), false);

    EXPECT_EQ(m_plant->getLightLevel(withBerries), 14);
    EXPECT_EQ(m_plant->getLightLevel(withoutBerries), 0);
}

TEST_F(CaveVinesPlantBlockTest, GetCloneItemStackReturnsGlowBerries)
{
    const BlockState& state = m_plant->defaultState();
    ItemStack stack = m_plant->getCloneItemStack(state);

    ASSERT_NE(stack.getItem(), nullptr);
    EXPECT_EQ(stack.getItem()->itemId(), Items::GLOW_BERRIES->itemId());
    EXPECT_EQ(stack.getCount(), 1);
}

// ============================================================================
// CaveVinesPlantBlock 交互测试
// ============================================================================

TEST_F(CaveVinesPlantBlockTest, OnBlockActivated_WithBerries_ReturnsSuccess)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(0, 64, 0);

    const BlockState& state = m_plant->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    auto result = m_plant->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);
}

TEST_F(CaveVinesPlantBlockTest, OnBlockActivated_WithBerries_SetsBerriesFalse)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(0, 64, 0);

    const BlockState& state = m_plant->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 1.0f);
    m_plant->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    const BlockState* currentState = world.getBlockState(0, 64, 0);
    ASSERT_NE(currentState, nullptr);
    EXPECT_FALSE(currentState->get(BlockStateProperties::BERRIES()));
}

TEST_F(CaveVinesPlantBlockTest, OnBlockActivated_WithBerries_DropsGlowBerries)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(10, 64, 10);

    const BlockState& state = m_plant->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(10.5f, 64.5f, 10.5f), pos, Direction::Up, 1.0f);
    m_plant->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(world.spawnedEntityCount(), 1u);

    Entity* entity = world.getSpawnedEntity(0);
    ASSERT_NE(entity, nullptr);
    ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
    ASSERT_NE(itemEntity, nullptr);

    const ItemStack& itemStack = itemEntity->getItemStack();
    EXPECT_EQ(itemStack.getCount(), 1);
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_EQ(itemStack.getItem(), Items::GLOW_BERRIES);
}

TEST_F(CaveVinesPlantBlockTest, OnBlockActivated_WithBerries_PlaysPickSound)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(20, 64, 20);

    const BlockState& state = m_plant->defaultState().with(BlockStateProperties::BERRIES(), true);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(20.5f, 64.5f, 20.5f), pos, Direction::Up, 1.0f);
    m_plant->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    EXPECT_TRUE(world.wasSoundPlayed());
    EXPECT_EQ(world.lastSoundId(), SoundEvents::BLOCK_CAVE_VINES_PICK_BERRIES);
    EXPECT_FLOAT_EQ(world.lastSoundVolume(), 1.0f);
    EXPECT_GE(world.lastSoundPitch(), 0.8f);
    EXPECT_LE(world.lastSoundPitch(), 1.2f);
}

TEST_F(CaveVinesPlantBlockTest, OnBlockActivated_WithoutBerries_ReturnsPass)
{
    CaveVinesTestWorld world;
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    BlockPos pos(40, 64, 40);

    const BlockState& state = m_plant->defaultState().with(BlockStateProperties::BERRIES(), false);
    world.setBlockAt(pos, &state);

    BlockRaycastResult hit(Vector3(40.5f, 64.5f, 40.5f), pos, Direction::Up, 1.0f);
    auto result = m_plant->onBlockActivated(state, world, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Pass);
    EXPECT_EQ(world.spawnedEntityCount(), 0u);
    EXPECT_FALSE(world.wasSoundPlayed());
}

// ============================================================================
// Glow Berries 物品注册测试
// ============================================================================

class GlowBerriesItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(GlowBerriesItemTest, GlowBerriesIsRegistered)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
}

TEST_F(GlowBerriesItemTest, GlowBerriesHasNonZeroItemId)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_GT(Items::GLOW_BERRIES->itemId(), 0u);
}

TEST_F(GlowBerriesItemTest, GlowBerriesIsFood)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_TRUE(Items::GLOW_BERRIES->isFood());
}

TEST_F(GlowBerriesItemTest, GlowBerriesMaxStackSize)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    EXPECT_EQ(Items::GLOW_BERRIES->maxStackSize(), 64);
}

TEST_F(GlowBerriesItemTest, GlowBerriesItemStackCreation)
{
    ASSERT_NE(Items::GLOW_BERRIES, nullptr);
    ItemStack stack(*Items::GLOW_BERRIES, 1);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getCount(), 1);
    EXPECT_EQ(stack.getItem(), Items::GLOW_BERRIES);
}
