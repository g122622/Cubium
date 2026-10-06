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
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/items/block/WallOrFloorItem.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/blocks/decorative/SkullBlock.hpp"
#include "common/world/block/blocks/decorative/TorchBlock.hpp"
#include "common/world/block/blocks/decorative/WallTorchBlock.hpp"
#include "common/world/block/blocks/mob/DriedGhastBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SkullBlockEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/sound/SoundEvents.hpp"

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 新增方块测试用世界
 *
 * 提供内存方块存储、流体状态、计划刻记录与声音记录，供 DriedGhastBlock / SkullBlock /
 * 铜火把等方块的放置、更新、tick 行为测试使用。
 */
class NewContentTestWorld final : public mc::test::BaseTestWorld {
public:
    NewContentTestWorld()
        : m_tickManager(std::make_unique<mc::test::DummyTickManager>())
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] mc::world::chunk::IChunkManager* chunkManager() override { return &m_stubChunks; }
    [[nodiscard]] const mc::world::chunk::IChunkManager* chunkManager() const override { return &m_stubChunks; }
    mc::test::StubChunkManager m_stubChunks{nullptr, true};

    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override { return *m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return *m_tickManager; }

    void playSound(const ResourceLocation& soundId, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        m_playedSounds.push_back(soundId);
    }

    // 测试辅助
    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[pos] = std::make_unique<BlockState>(*state);
    }

    void clearBlockAt(const BlockPos& pos) { m_blocks.erase(pos); }

    [[nodiscard]] bool hasPlayedSound(const ResourceLocation& soundId) const
    {
        for (const auto& s : m_playedSounds) {
            if (s == soundId) {
                return true;
            }
        }
        return false;
    }

    void clearSounds() { m_playedSounds.clear(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<ResourceLocation> m_playedSounds;
    std::unique_ptr<world::tick::TickManager> m_tickManager;
};

} // namespace

// ============================================================================
// 注册完备性测试：新方块与物品必须全链路注册
// ============================================================================

class NewContentRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

TEST_F(NewContentRegistrationTest, MaterialItems_Registered)
{
    // 材料 / 矿物 / 染料物品指针非空
    ASSERT_NE(Items::NETHER_BRICK, nullptr);
    ASSERT_NE(Items::CLAY_BALL, nullptr);
    ASSERT_NE(Items::GOLD_NUGGET, nullptr);
    ASSERT_NE(Items::IRON_NUGGET, nullptr);
    ASSERT_NE(Items::COPPER_NUGGET, nullptr);
    ASSERT_NE(Items::PRISMARINE_SHARD, nullptr);
    ASSERT_NE(Items::PRISMARINE_CRYSTALS, nullptr);
    ASSERT_NE(Items::SHULKER_SHELL, nullptr);
    ASSERT_NE(Items::POPPED_CHORUS_FRUIT, nullptr);
    ASSERT_NE(Items::ECHO_SHARD, nullptr);
    ASSERT_NE(Items::GLOW_INK_SAC, nullptr);
    ASSERT_NE(Items::DISC_FRAGMENT_5, nullptr);

    // 染料补齐 16 色
    ASSERT_NE(Items::BLACK_DYE, nullptr);
    ASSERT_NE(Items::BLUE_DYE, nullptr);
    ASSERT_NE(Items::BROWN_DYE, nullptr);
}

TEST_F(NewContentRegistrationTest, MaterialItems_LookupByName)
{
    // 物品注册表可按 id 反查（验证注册链路完整，非仅静态指针）
    auto& registry = ItemRegistry::instance();
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:nether_brick")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:copper_nugget")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:echo_shard")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:black_dye")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:blue_dye")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:brown_dye")), nullptr);
}

TEST_F(NewContentRegistrationTest, BannerPatterns_Registered)
{
    ASSERT_NE(Items::FIELD_MASONED_BANNER_PATTERN, nullptr);
    ASSERT_NE(Items::BORDURE_INDENTED_BANNER_PATTERN, nullptr);

    auto& registry = ItemRegistry::instance();
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:field_masoned_banner_pattern")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:bordure_indented_banner_pattern")), nullptr);
}

TEST_F(NewContentRegistrationTest, NewBlocks_PointersNotNull)
{
    ASSERT_NE(VanillaBlocks::HONEYCOMB_BLOCK, nullptr);
    ASSERT_NE(VanillaBlocks::DRIED_GHAST, nullptr);
    ASSERT_NE(VanillaBlocks::COPPER_TORCH, nullptr);
    ASSERT_NE(VanillaBlocks::COPPER_WALL_TORCH, nullptr);
}

TEST_F(NewContentRegistrationTest, SkullBlocks_AllVariantsRegistered)
{
    // 站立 + 墙挂变体共 14 个
    ASSERT_NE(VanillaBlocks::SKELETON_SKULL, nullptr);
    ASSERT_NE(VanillaBlocks::SKELETON_WALL_SKULL, nullptr);
    ASSERT_NE(VanillaBlocks::WITHER_SKELETON_SKULL, nullptr);
    ASSERT_NE(VanillaBlocks::WITHER_SKELETON_WALL_SKULL, nullptr);
    ASSERT_NE(VanillaBlocks::PLAYER_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::PLAYER_WALL_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::ZOMBIE_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::ZOMBIE_WALL_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::CREEPER_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::CREEPER_WALL_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::DRAGON_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::DRAGON_WALL_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::PIGLIN_HEAD, nullptr);
    ASSERT_NE(VanillaBlocks::PIGLIN_WALL_HEAD, nullptr);

    // 方块注册表反查
    auto& registry = BlockRegistry::instance();
    EXPECT_NE(registry.getBlock(ResourceLocation("minecraft:skeleton_skull")), nullptr);
    EXPECT_NE(registry.getBlock(ResourceLocation("minecraft:skeleton_wall_skull")), nullptr);
    EXPECT_NE(registry.getBlock(ResourceLocation("minecraft:dragon_wall_head")), nullptr);
    EXPECT_NE(registry.getBlock(ResourceLocation("minecraft:piglin_wall_head")), nullptr);
}

TEST_F(NewContentRegistrationTest, SkullItems_AreWallOrFloorItems)
{
    // 头颅物品必须升级为 WallOrFloorItem（支持地板/墙壁双向放置）
    auto& registry = ItemRegistry::instance();
    for (const char* name : {"skeleton_skull",
             "wither_skeleton_skull",
             "player_head",
             "zombie_head",
             "creeper_head",
             "dragon_head",
             "piglin_head"}) {
        Item* item = registry.getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "missing skull item: " << name;
        EXPECT_NE(dynamic_cast<WallOrFloorItem*>(item), nullptr) << "skull item not WallOrFloorItem: " << name;
    }
}

TEST_F(NewContentRegistrationTest, BlockItems_RegisteredForNewBlocks)
{
    auto& registry = ItemRegistry::instance();
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:honeycomb_block")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:dried_ghast")), nullptr);
    EXPECT_NE(registry.getItem(ResourceLocation("minecraft:copper_torch")), nullptr);

    // 墙挂变体映射到同一物品
    BlockItemRegistry& blockItems = BlockItemRegistry::instance();
    const BlockItem* wallTorchItem = blockItems.getBlockItem(*VanillaBlocks::COPPER_WALL_TORCH);
    ASSERT_NE(wallTorchItem, nullptr);
    EXPECT_EQ(wallTorchItem->itemId(), Items::COPPER_TORCH->itemId());
}

TEST_F(NewContentRegistrationTest, SkullWallVariants_MapToSameItem)
{
    BlockItemRegistry& blockItems = BlockItemRegistry::instance();
    const BlockItem* wallSkullItem = blockItems.getBlockItem(*VanillaBlocks::SKELETON_WALL_SKULL);
    ASSERT_NE(wallSkullItem, nullptr);
    EXPECT_EQ(wallSkullItem->itemId(), Items::SKELETON_SKULL->itemId());

    const BlockItem* playerWallHeadItem = blockItems.getBlockItem(*VanillaBlocks::PLAYER_WALL_HEAD);
    ASSERT_NE(playerWallHeadItem, nullptr);
    EXPECT_EQ(playerWallHeadItem->itemId(), Items::PLAYER_HEAD->itemId());
}

// ============================================================================
// 铜火把：属性与注册形态
// ============================================================================

class CopperTorchTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(CopperTorchTest, TorchBlock_IsTorchTypeWithLight14)
{
    ASSERT_NE(VanillaBlocks::COPPER_TORCH, nullptr);
    // 必须是 TorchBlock（而非误注册为 SimpleBlock，否则 animateTick/支撑校验失效）
    EXPECT_NE(dynamic_cast<TorchBlock*>(VanillaBlocks::COPPER_TORCH), nullptr);
    EXPECT_EQ(VanillaBlocks::COPPER_TORCH->lightLevel(), 14);
    // 墙挂变体必须是 WallTorchBlock
    ASSERT_NE(VanillaBlocks::COPPER_WALL_TORCH, nullptr);
    EXPECT_NE(dynamic_cast<WallTorchBlock*>(VanillaBlocks::COPPER_WALL_TORCH), nullptr);
    EXPECT_EQ(VanillaBlocks::COPPER_WALL_TORCH->lightLevel(), 14);
}

TEST_F(CopperTorchTest, TorchBlock_RequiresSupportBelow)
{
    NewContentTestWorld world;
    const BlockPos torchPos(4, 1, 4);

    // 无下方支撑：不可存活
    EXPECT_FALSE(VanillaBlocks::COPPER_TORCH->isValidPosition(
        VanillaBlocks::COPPER_TORCH->defaultState(), world, torchPos));

    // 下方放置石头：可存活
    world.setBlockAt(torchPos.down(), &VanillaBlocks::STONE->defaultState());
    EXPECT_TRUE(VanillaBlocks::COPPER_TORCH->isValidPosition(
        VanillaBlocks::COPPER_TORCH->defaultState(), world, torchPos));
}

TEST_F(CopperTorchTest, TorchBlock_RemovedWhenSupportLost)
{
    NewContentTestWorld world;
    const BlockPos torchPos(4, 1, 4);
    world.setBlockAt(torchPos, &VanillaBlocks::COPPER_TORCH->defaultState());
    // 无支撑时下方更新触发自毁
    BlockState result = VanillaBlocks::COPPER_TORCH->updatePostPlacement(VanillaBlocks::COPPER_TORCH->defaultState(),
        Direction::Down,
        VanillaBlocks::AIR->defaultState(),
        world,
        torchPos,
        torchPos.down());
    EXPECT_TRUE(result.isAir());
}

// ============================================================================
// 蜜脾块：基本属性
// ============================================================================

TEST_F(NewContentRegistrationTest, HoneycombBlock_Properties)
{
    ASSERT_NE(VanillaBlocks::HONEYCOMB_BLOCK, nullptr);
    EXPECT_FLOAT_EQ(VanillaBlocks::HONEYCOMB_BLOCK->hardness(), 0.6f);
    // 无状态属性
    EXPECT_FALSE(VanillaBlocks::HONEYCOMB_BLOCK->hasBlockEntity());
}

// ============================================================================
// 干燥恶魂：湿润等级 / 孵化 / 含水
// ============================================================================

class DriedGhastBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        m_block = dynamic_cast<DriedGhastBlock*>(VanillaBlocks::DRIED_GHAST);
        ASSERT_NE(m_block, nullptr);
    }

    DriedGhastBlock* m_block = nullptr;
    NewContentTestWorld world;
};

TEST_F(DriedGhastBlockTest, DefaultState_HydrationZeroNotWaterlogged)
{
    const BlockState& state = m_block->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS()), 0);
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(DriedGhastBlockTest, GetHydrationLevel_ReadsProperty)
{
    BlockState state = m_block->defaultState().with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), 2);
    EXPECT_EQ(m_block->getHydrationLevel(state), 2);
}

TEST_F(DriedGhastBlockTest, IsWaterlogged_ReadsProperty)
{
    EXPECT_TRUE(m_block->isWaterlogged(m_block->defaultState().with(BlockStateProperties::WATERLOGGED(), true)));
    EXPECT_FALSE(m_block->isWaterlogged(m_block->defaultState().with(BlockStateProperties::WATERLOGGED(), false)));
}

TEST_F(DriedGhastBlockTest, TicksRandomly_True)
{
    EXPECT_TRUE(m_block->ticksRandomly());
}

TEST_F(DriedGhastBlockTest, Tick_NotWaterlogged_DecrementsHydration)
{
    const BlockPos pos(2, 1, 2);
    BlockState state = m_block->defaultState().with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), 2);
    world.setBlockAt(pos, &state);

    math::Random rng(12345);
    m_block->tick(world, pos, state, rng);

    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_EQ(after->get(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS()), 1);
}

TEST_F(DriedGhastBlockTest, Tick_NotWaterlogged_ZeroHydration_Unchanged)
{
    const BlockPos pos(2, 1, 2);
    BlockState state = m_block->defaultState().with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), 0);
    world.setBlockAt(pos, &state);

    math::Random rng(12345);
    m_block->tick(world, pos, state, rng);

    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_EQ(after->get(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS()), 0);
}

TEST_F(DriedGhastBlockTest, Tick_Waterlogged_IncrementsHydrationAndPlaysTransitionSound)
{
    const BlockPos pos(2, 1, 2);
    BlockState state = m_block->defaultState()
                           .with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), 0)
                           .with(BlockStateProperties::WATERLOGGED(), true);
    world.setBlockAt(pos, &state);
    world.clearSounds();

    math::Random rng(12345);
    m_block->tick(world, pos, state, rng);

    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_EQ(after->get(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS()), 1);
    EXPECT_TRUE(world.hasPlayedSound(SoundEvents::DRIED_GHAST_TRANSITION));
}

TEST_F(DriedGhastBlockTest, Tick_Waterlogged_ReadyToSpawn_RemovesBlock)
{
    const BlockPos pos(2, 1, 2);
    BlockState state = m_block->defaultState()
                           .with(BlockStateProperties::DRIED_GHAST_HYDRATION_LEVELS(), 3)
                           .with(BlockStateProperties::WATERLOGGED(), true);
    world.setBlockAt(pos, &state);
    world.clearSounds();

    math::Random rng(12345);
    m_block->tick(world, pos, state, rng);

    // 满级含水 → 方块被移除（孵化为幼年恶魂）
    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->isAir());
    // 播放幼年恶魂生成音效
    EXPECT_TRUE(world.hasPlayedSound(SoundEvents::GHASTLING_SPAWN));
}

TEST_F(DriedGhastBlockTest, ReceiveFluid_SetsWaterloggedAndPlaysSound)
{
    const BlockPos pos(2, 1, 2);
    BlockState state = m_block->defaultState();
    world.setBlockAt(pos, &state);
    world.clearSounds();

    const fluid::FluidState& water = fluid::Fluids::WATER()->defaultState();
    const bool received = m_block->receiveFluid(world, pos, state, water);

    EXPECT_TRUE(received);
    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->get(BlockStateProperties::WATERLOGGED()));
    EXPECT_TRUE(world.hasPlayedSound(SoundEvents::DRIED_GHAST_PLACE_IN_WATER));
}

TEST_F(DriedGhastBlockTest, ReceiveFluid_AlreadyWaterlogged_Rejected)
{
    const BlockPos pos(2, 1, 2);
    BlockState state = m_block->defaultState().with(BlockStateProperties::WATERLOGGED(), true);
    world.setBlockAt(pos, &state);

    const fluid::FluidState& water = fluid::Fluids::WATER()->defaultState();
    EXPECT_FALSE(m_block->receiveFluid(world, pos, state, water));
}

TEST_F(DriedGhastBlockTest, HasBlockEntity_False)
{
    EXPECT_FALSE(m_block->hasBlockEntity());
}

// ============================================================================
// 生物头颅：属性 / 形状 / 红石 / 方块实体
// ============================================================================

class SkullBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

TEST_F(SkullBlockTest, StandingSkull_HasRotationAndPowered)
{
    const BlockState& state = VanillaBlocks::SKELETON_SKULL->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::ROTATION_0_15()), 0);
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(SkullBlockTest, WallSkull_HasFacingAndPowered)
{
    const BlockState& state = VanillaBlocks::SKELETON_WALL_SKULL->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
    EXPECT_FALSE(state.get(BlockStateProperties::POWERED()));
}

TEST_F(SkullBlockTest, AllSkulls_HaveBlockEntity)
{
    for (Block* block : {VanillaBlocks::SKELETON_SKULL,
             VanillaBlocks::SKELETON_WALL_SKULL,
             VanillaBlocks::WITHER_SKELETON_SKULL,
             VanillaBlocks::PLAYER_HEAD,
             VanillaBlocks::PLAYER_WALL_HEAD,
             VanillaBlocks::ZOMBIE_HEAD,
             VanillaBlocks::CREEPER_HEAD,
             VanillaBlocks::DRAGON_HEAD,
             VanillaBlocks::PIGLIN_HEAD,
             VanillaBlocks::PIGLIN_WALL_HEAD}) {
        ASSERT_NE(block, nullptr);
        EXPECT_TRUE(block->hasBlockEntity());
    }
}

TEST_F(SkullBlockTest, CreateBlockEntity_ReturnsSkullEntity)
{
    auto entity = VanillaBlocks::PLAYER_HEAD->createBlockEntity(BlockPos(1, 2, 3));
    ASSERT_NE(entity, nullptr);
    EXPECT_NE(dynamic_cast<blockentity::SkullBlockEntity*>(entity.get()), nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::Skull);
}

TEST_F(SkullBlockTest, Rotate_StandingSkull_UpdatesRotation)
{
    const BlockState& state = VanillaBlocks::SKELETON_SKULL->defaultState();
    const BlockState& rotated = VanillaBlocks::SKELETON_SKULL->rotate(state, Rotation::Clockwise90);
    // 旋转 90 度 → rotation 加 4（16 向，每 90 度 4 步）
    EXPECT_EQ(rotated.get(BlockStateProperties::ROTATION_0_15()), 4);
}

TEST_F(SkullBlockTest, Rotate_WallSkull_UpdatesFacing)
{
    const BlockState& state = VanillaBlocks::SKELETON_WALL_SKULL->defaultState();
    const BlockState& rotated = VanillaBlocks::SKELETON_WALL_SKULL->rotate(state, Rotation::Clockwise90);
    EXPECT_EQ(rotated.get(BlockStateProperties::HORIZONTAL_FACING()), Directions::rotateY(Direction::North));
}

TEST_F(SkullBlockTest, WallSkull_IsValidPosition_RequiresSturdyWall)
{
    NewContentTestWorld world;
    const BlockPos skullPos(4, 2, 4);
    // FACING 指向背离墙的一侧，故 North 朝向的墙位于南侧（opposite(North)）
    BlockState state = VanillaBlocks::SKELETON_WALL_SKULL->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    // 南面无方块：不可存活
    EXPECT_FALSE(VanillaBlocks::SKELETON_WALL_SKULL->isValidPosition(state, world, skullPos));

    // 南面放置石头：可存活
    world.setBlockAt(skullPos.offset(Direction::South), &VanillaBlocks::STONE->defaultState());
    EXPECT_TRUE(VanillaBlocks::SKELETON_WALL_SKULL->isValidPosition(state, world, skullPos));
}

TEST_F(SkullBlockTest, WallSkull_UpdatePostPlacement_RemovedWhenWallGone)
{
    NewContentTestWorld world;
    const BlockPos skullPos(4, 2, 4);
    BlockState state = VanillaBlocks::SKELETON_WALL_SKULL->defaultState().with(
        BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    // 墙面位于 opposite(FACING) = South，故南侧变化触发自毁
    BlockState result = VanillaBlocks::SKELETON_WALL_SKULL->updatePostPlacement(state,
        Direction::South,
        VanillaBlocks::AIR->defaultState(),
        world,
        skullPos,
        skullPos.offset(Direction::South));
    EXPECT_TRUE(result.isAir());
}

TEST_F(SkullBlockTest, PiglinStandingHead_HasLargerShape)
{
    const CollisionShape& normalShape = VanillaBlocks::SKELETON_SKULL->getShape(
        VanillaBlocks::SKELETON_SKULL->defaultState());
    const CollisionShape& piglinShape = VanillaBlocks::PIGLIN_HEAD->getShape(
        VanillaBlocks::PIGLIN_HEAD->defaultState());
    // 猪灵头碰撞箱更宽，两者不应为同一对象
    EXPECT_NE(&normalShape, &piglinShape);
}

TEST_F(SkullBlockTest, SkullEntity_OwnerProfileRoundTrip)
{
    blockentity::SkullBlockEntity entity(BlockPos(0, 0, 0));
    EXPECT_EQ(entity.getOwnerProfile(), nullptr);

    skin::GameProfile profile;
    profile.setName("Notch");
    profile.setUUID(skin::GameProfile::parseUUID("069a79f444e94726a5befca90e38aaf5"));
    entity.setOwnerProfile(profile);

    ASSERT_NE(entity.getOwnerProfile(), nullptr);
    EXPECT_EQ(entity.getOwnerProfile()->name(), "Notch");

    // JSON 往返
    nlohmann::json data;
    entity.save(data);
    blockentity::SkullBlockEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));
    ASSERT_NE(loaded.getOwnerProfile(), nullptr);
    EXPECT_EQ(loaded.getOwnerProfile()->name(), "Notch");
}

TEST_F(SkullBlockTest, SkullEntity_CustomNameRoundTrip)
{
    blockentity::SkullBlockEntity entity(BlockPos(0, 0, 0));
    entity.setCustomName("My Skull");
    EXPECT_EQ(entity.getCustomName(), "My Skull");

    nlohmann::json data;
    entity.save(data);
    blockentity::SkullBlockEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));
    EXPECT_EQ(loaded.getCustomName(), "My Skull");
}

TEST_F(SkullBlockTest, SkullEntity_AnimationAccumulatesWhenPowered)
{
    NewContentTestWorld world;
    const BlockPos pos(3, 3, 3);

    blockentity::SkullBlockEntity entity(pos);
    entity.setWorld(&world);

    // 未激活：动画计数不增长
    world.setBlockAt(pos, &VanillaBlocks::DRAGON_HEAD->defaultState());
    entity.tick(world);
    const f32 before = entity.getAnimation(0.0f);

    // 激活：动画计数增长
    world.setBlockAt(pos,
        &VanillaBlocks::DRAGON_HEAD->defaultState().with(BlockStateProperties::POWERED(), true));
    entity.tick(world);
    EXPECT_GT(entity.getAnimation(0.0f), before);
}
