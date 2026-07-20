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
 * @file PumpkinCarveTest.cpp
 * @brief 南瓜雕刻功能单元测试
 *
 * 测试覆盖：
 * - 剪刀雕刻南瓜成功
 * - 非剪刀物品返回 Pass
 * - 雕刻南瓜朝向计算
 * - 南瓜种子掉落
 * - 剪刀耐久度消耗
 */

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/BlockRaycastResult.hpp"
#include "core/Constants.hpp"
#include "entity/entities/item/ItemEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/registry/VanillaEntities.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "sound/SoundEvents.hpp"
#include "util/Direction.hpp"
#include "util/math/random/Random.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/WorldEvents.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/agricultural/MelonPumpkinBlocks.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用 Mock 世界实现
 *
 * 提供 PumpkinCarve 测试所需的最小 IWorld 接口实现
 */
class PumpkinCarveTestWorld final : public test::BaseTestWorld {
public:
    PumpkinCarveTestWorld()
    {
        // 初始化 VanillaBlocks
        VanillaBlocks::initialize();
        // 初始化 VanillaEntities
        entity::VanillaEntities::registerAll();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        // 返回空气状态
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
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }
    [[nodiscard]] bool isThundering() const override { return false; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        Entity* rawPtr = entity.get();
        EntityId id = static_cast<EntityId>(m_spawnedEntities.size() + 1);
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

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override
    {
        m_lastEventId = eventId;
        m_lastEventPos = pos;
        m_lastEventData = data;
        m_eventCount++;
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        static world::tick::TickManager dummy(*static_cast<IWorld*>(nullptr));
        return dummy;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        static world::tick::TickManager dummy(*const_cast<PumpkinCarveTestWorld*>(this));
        return dummy;
    }

    // 测试辅助方法
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }
    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
    }

    void clearBlocks() { m_blocks.clear(); }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] Entity* getSpawnedEntity(size_t index) const
    {
        if (index < m_spawnedEntityPtrs.size()) {
            return m_spawnedEntityPtrs[index];
        }
        return nullptr;
    }

    void clearSpawnedEntities()
    {
        m_spawnedEntities.clear();
        m_spawnedEntityPtrs.clear();
    }

    [[nodiscard]] bool wasSoundPlayed() const { return m_soundPlayed; }
    [[nodiscard]] const ResourceLocation& lastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] const Vector3& lastSoundPos() const { return m_lastSoundPos; }
    [[nodiscard]] f32 lastSoundVolume() const { return m_lastSoundVolume; }
    [[nodiscard]] f32 lastSoundPitch() const { return m_lastSoundPitch; }
    [[nodiscard]] i32 getLastEventId() const { return m_lastEventId; }
    [[nodiscard]] i32 getEventCount() const { return m_eventCount; }
    void resetEventCount()
    {
        m_eventCount = 0;
        m_soundPlayed = false;
    }

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
    i32 m_lastEventId = 0;
    i32 m_lastEventData = 0;
    i32 m_eventCount = 0;
    BlockPos m_lastEventPos{0, 0, 0};
};

} // anonymous namespace

// ============================================================================
// PumpkinBlock 雕刻测试
// ============================================================================

class PumpkinCarveTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化 Items
        Items::initialize();

        // 创建测试世界
        world_ = std::make_unique<PumpkinCarveTestWorld>();

        // 创建南瓜方块（需要先注册雕刻南瓜）
        carvedPumpkin_ = std::make_unique<CarvedPumpkinBlock>(BlockProperties(Material::EARTH).hardness(1.0f));

        pumpkin_ = std::make_unique<PumpkinBlock>(nullptr, // stem
            nullptr,                                       // attachedStem
            carvedPumpkin_.get(),                          // carvedPumpkin
            BlockProperties(Material::EARTH).hardness(1.0f));
    }

    std::unique_ptr<PumpkinCarveTestWorld> world_;
    std::unique_ptr<PumpkinBlock> pumpkin_;
    std::unique_ptr<CarvedPumpkinBlock> carvedPumpkin_;
};

// ============================================================================
// 基础功能测试
// ============================================================================

TEST_F(PumpkinCarveTest, OnBlockActivated_WithoutShears_ReturnsPass)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 设置南瓜方块
    BlockPos pos(0, 64, 0);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 创建射线检测结果
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 1.0f), // hitPos
        pos,
        Direction::South, // face
        1.0f);

    // 执行交互（空手）
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    // 应该返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);

    // 方块应该没有变化
    const BlockState* currentState = world_->getBlockState(0, 64, 0);
    ASSERT_NE(currentState, nullptr);
    EXPECT_FALSE(currentState->isAir());
}

TEST_F(PumpkinCarveTest, OnBlockActivated_WithNonShearsItem_ReturnsPass)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 给玩家一个非剪刀物品（钻石剑）
    if (Items::DIAMOND_SWORD != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::DIAMOND_SWORD, 1);
    }

    // 设置南瓜方块
    BlockPos pos(0, 64, 0);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 创建射线检测结果
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 1.0f), pos, Direction::South, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    // 应该返回 Pass
    EXPECT_EQ(result, ActionResultType::Pass);
}

TEST_F(PumpkinCarveTest, OnBlockActivated_WithShears_CarvesPumpkin)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(10, 64, 10);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 创建射线检测结果（从南面点击）
    BlockRaycastResult hit(Vector3(10.5f, 64.5f, 11.0f), pos, Direction::South, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    // 应该返回 Success
    EXPECT_EQ(result, ActionResultType::Success);

    // 方块应该变成雕刻南瓜
    const BlockState* currentState = world_->getBlockState(10, 64, 10);
    ASSERT_NE(currentState, nullptr);
    EXPECT_EQ(&currentState->getBlock(), carvedPumpkin_.get());

    // 雕刻南瓜应该有正确的朝向
    std::optional<Direction> facing = currentState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_TRUE(facing.has_value());
    EXPECT_EQ(facing.value(), Direction::South);

    // 应该播放雕刻音效
    EXPECT_TRUE(world_->wasSoundPlayed());
    EXPECT_EQ(world_->lastSoundId(), SoundEvents::BLOCK_PUMPKIN_CARVE);
}

TEST_F(PumpkinCarveTest, OnBlockActivated_WithShears_DropsPumpkinSeeds)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(20, 64, 20);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 创建射线检测结果
    BlockRaycastResult hit(Vector3(20.5f, 64.5f, 21.0f), pos, Direction::South, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);

    // 应该生成一个物品实体
    EXPECT_EQ(world_->spawnedEntityCount(), 1u);

    // 验证是物品实体
    Entity* entity = world_->getSpawnedEntity(0);
    ASSERT_NE(entity, nullptr);
    ItemEntity* itemEntity = dynamic_cast<ItemEntity*>(entity);
    ASSERT_NE(itemEntity, nullptr);

    // 验证物品是南瓜种子，数量为4
    const ItemStack& itemStack = itemEntity->getItemStack();
    EXPECT_EQ(itemStack.getCount(), 4);
    if (Items::PUMPKIN_SEEDS != nullptr) {
        EXPECT_EQ(itemStack.getItem(), Items::PUMPKIN_SEEDS);
    }
}

TEST_F(PumpkinCarveTest, OnBlockActivated_WithShears_DamagesShears)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 记录剪刀耐久度
    i32 initialDamage = 0;
    if (Items::SHEARS != nullptr) {
        initialDamage = player.inventory().getSelectedStack().getDamage();
    }

    // 设置南瓜方块
    BlockPos pos(30, 64, 30);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 创建射线检测结果
    BlockRaycastResult hit(Vector3(30.5f, 64.5f, 31.0f), pos, Direction::South, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);

    // 验证剪刀耐久度消耗了1点
    if (Items::SHEARS != nullptr) {
        i32 newDamage = player.inventory().getSelectedStack().getDamage();
        EXPECT_EQ(newDamage, initialDamage + 1);
    }
}

// ============================================================================
// 朝向计算测试
// ============================================================================

TEST_F(PumpkinCarveTest, OnBlockActivated_SideClick_SetsCorrectFacing)
{
    // 创建玩家
    Player player(1, "TestPlayer");
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setRotation(0.0f, 0.0f); // 面向南方（yaw=0）

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(5, 64, 5);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 从北面点击（玩家在南面，看向北面）
    BlockRaycastResult hit(Vector3(5.5f, 64.5f, 4.0f), pos, Direction::North, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);

    // 验证雕刻南瓜朝向北面
    const BlockState* currentState = world_->getBlockState(5, 64, 5);
    ASSERT_NE(currentState, nullptr);
    std::optional<Direction> facing = currentState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    ASSERT_TRUE(facing.has_value());
    EXPECT_EQ(facing.value(), Direction::North);
}

TEST_F(PumpkinCarveTest, OnBlockActivated_TopClick_UsesPlayerFacing)
{
    // 创建玩家
    Player player(1, "TestPlayer");
    player.setPosition(0.0f, 65.0f, 0.0f);
    player.setRotation(90.0f, 0.0f); // 面向西方（yaw=90）

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(0, 64, 0);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 从顶面点击
    BlockRaycastResult hit(Vector3(0.5f, 65.0f, 0.5f), pos, Direction::Up, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);

    // 验证雕刻南瓜朝向东面（玩家朝向西，相反方向）
    const BlockState* currentState = world_->getBlockState(0, 64, 0);
    ASSERT_NE(currentState, nullptr);
    std::optional<Direction> facing = currentState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    ASSERT_TRUE(facing.has_value());
    // yaw=90 对应 West，相反方向是 East
    EXPECT_EQ(facing.value(), Direction::East);
}

TEST_F(PumpkinCarveTest, OnBlockActivated_BottomClick_UsesPlayerFacing)
{
    // 创建玩家
    Player player(1, "TestPlayer");
    player.setPosition(0.0f, 63.0f, 0.0f);
    player.setRotation(180.0f, 0.0f); // 面向北方（yaw=180）

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(0, 64, 0);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 从底面点击
    BlockRaycastResult hit(Vector3(0.5f, 64.0f, 0.5f), pos, Direction::Down, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);

    // 验证雕刻南瓜朝向南面（玩家朝向北，相反方向）
    const BlockState* currentState = world_->getBlockState(0, 64, 0);
    ASSERT_NE(currentState, nullptr);
    std::optional<Direction> facing = currentState->getOptional(BlockStateProperties::HORIZONTAL_FACING());
    ASSERT_TRUE(facing.has_value());
    // yaw=180 对应 North，相反方向是 South
    EXPECT_EQ(facing.value(), Direction::South);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(PumpkinCarveTest, OnBlockActivated_NullCarvedPumpkin_ReturnsPass)
{
    // 创建没有雕刻南瓜引用的南瓜
    auto pumpkinWithoutCarved = std::make_unique<PumpkinBlock>(nullptr, // stem
        nullptr,                                                        // attachedStem
        nullptr,                                                        // carvedPumpkin 为 nullptr
        BlockProperties(Material::EARTH).hardness(1.0f));

    // 创建玩家
    Player player(1, "TestPlayer");

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(0, 64, 0);
    world_->setBlockAt(pos, &pumpkinWithoutCarved->defaultState());

    // 创建射线检测结果
    BlockRaycastResult hit(Vector3(0.5f, 64.5f, 1.0f), pos, Direction::South, 1.0f);

    // 执行交互
    const auto& state = pumpkinWithoutCarved->defaultState();
    auto result = pumpkinWithoutCarved->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    // 应该返回 Pass（因为没有雕刻南瓜）
    EXPECT_EQ(result, ActionResultType::Pass);

    // 方块不应该改变
    const BlockState* currentState = world_->getBlockState(0, 64, 0);
    ASSERT_NE(currentState, nullptr);
    EXPECT_EQ(&currentState->getBlock(), pumpkinWithoutCarved.get());
}

TEST_F(PumpkinCarveTest, OnBlockActivated_OffHandShears_Works)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 把剪刀放在副手
    if (Items::SHEARS != nullptr) {
        player.inventory().setOffhandItem(ItemStack(*Items::SHEARS, 1));
    }

    // 设置南瓜方块
    BlockPos pos(40, 64, 40);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 创建射线检测结果
    BlockRaycastResult hit(Vector3(40.5f, 64.5f, 41.0f), pos, Direction::South, 1.0f);

    // 使用副手交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::OffHand, hit);

    // 应该成功
    EXPECT_EQ(result, ActionResultType::Success);

    // 方块应该变成雕刻南瓜
    const BlockState* currentState = world_->getBlockState(40, 64, 40);
    ASSERT_NE(currentState, nullptr);
    EXPECT_EQ(&currentState->getBlock(), carvedPumpkin_.get());
}

// ============================================================================
// 种子掉落位置测试
// ============================================================================

TEST_F(PumpkinCarveTest, OnBlockActivated_SeedsSpawnAtCorrectPosition)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(50, 64, 50);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 从东面点击
    BlockRaycastResult hit(Vector3(51.0f, 64.5f, 50.5f), pos, Direction::East, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);

    // 验证种子实体位置
    Entity* entity = world_->getSpawnedEntity(0);
    ASSERT_NE(entity, nullptr);

    // 种子应该在雕刻面方向偏移
    Vector3 seedPos = entity->position();
    // 期望位置：南瓜位置中心 + 朝向偏移
    // pos.x + 0.5 + xOffset * 0.65
    // 东面 xOffset = 1，所以应该是 50 + 0.5 + 0.65 = 51.15
    EXPECT_NEAR(seedPos.x, 51.15, 0.1);
    EXPECT_NEAR(seedPos.y, 64.1, 0.1); // y = 64 + 0.1
    EXPECT_NEAR(seedPos.z, 50.5, 0.1);
}

// ============================================================================
// 音效测试
// ============================================================================

TEST_F(PumpkinCarveTest, OnBlockActivated_PlaysCorrectSound)
{
    // 创建玩家
    Player player(1, "TestPlayer");

    // 给玩家剪刀
    if (Items::SHEARS != nullptr) {
        player.inventory().getSelectedStackRef() = ItemStack(*Items::SHEARS, 1);
    }

    // 设置南瓜方块
    BlockPos pos(60, 64, 60);
    world_->setBlockAt(pos, &pumpkin_->defaultState());

    // 创建射线检测结果
    BlockRaycastResult hit(Vector3(60.5f, 64.5f, 61.0f), pos, Direction::South, 1.0f);

    // 执行交互
    const auto& state = pumpkin_->defaultState();
    auto result = pumpkin_->onBlockActivated(state, *world_, pos, player, Hand::MainHand, hit);

    EXPECT_EQ(result, ActionResultType::Success);

    // 验证音效
    EXPECT_TRUE(world_->wasSoundPlayed());
    EXPECT_EQ(world_->lastSoundId(), SoundEvents::BLOCK_PUMPKIN_CARVE);

    // 验证音效位置
    Vector3 expectedSoundPos = pos.center();
    EXPECT_FLOAT_EQ(world_->lastSoundPos().x, expectedSoundPos.x);
    EXPECT_FLOAT_EQ(world_->lastSoundPos().y, expectedSoundPos.y);
    EXPECT_FLOAT_EQ(world_->lastSoundPos().z, expectedSoundPos.z);

    // 验证音量和音调
    EXPECT_FLOAT_EQ(world_->lastSoundVolume(), 1.0f);
    EXPECT_FLOAT_EQ(world_->lastSoundPitch(), 1.0f);
}
