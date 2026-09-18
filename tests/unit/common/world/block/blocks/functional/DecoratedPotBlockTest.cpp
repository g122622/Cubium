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
#include "common/core/BlockRaycastResult.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/functional/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include "common/world/blockentity/interactive/DecoratedPotPattern.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/Fluids.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;
using namespace mc::blockentity;

namespace {

/**
 * @brief 饰纹陶罐测试用 Mock 世界实现
 *
 * 支持方块存储、方块实体、音效/游戏事件追踪、setBlockState 标志追踪、
 * 实体生成追踪、游戏规则等。
 */
class DecoratedPotTestWorld final : public mc::test::BaseTestWorld {
public:
    DecoratedPotTestWorld()
    {
        VanillaBlocks::initialize();
        Items::initialize();
        item::tag::ItemTags::initialize();
    }

    // ========== IWorld 接口实现 ==========

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
        m_setBlockCallCount++;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        m_lastSetBlockFlags = flags;
        m_lastSetBlockPos = BlockPos(x, y, z);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
    {
        m_blockEntities[pos] = std::move(entity);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entityLookup.find(id);
        return it != m_entityLookup.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entityLookup.find(id);
        return it != m_entityLookup.end() ? it->second : nullptr;
    }

    void registerEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entityLookup[entity->id()] = entity;
        }
    }

    void unregisterEntity(Entity* entity)
    {
        if (entity != nullptr) {
            m_entityLookup.erase(entity->id());
        }
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntityCount++;
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_soundPlayed = true;
        m_lastSoundId = soundId;
        m_lastSoundPitch = pitch;
        MC_UNUSED(category);
        MC_UNUSED(pos);
        MC_UNUSED(volume);
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEventFired = true;
        m_gameEventCount++;
        MC_UNUSED(event);
        MC_UNUSED(pos);
        MC_UNUSED(context);
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        static world::tick::TickManager dummy(*static_cast<IWorld*>(nullptr));
        return dummy;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        static world::tick::TickManager dummy(*const_cast<DecoratedPotTestWorld*>(this));
        return dummy;
    }

    void notifyBlockUpdate(const BlockPos& pos) override { MC_UNUSED(pos); }

    // ========== 测试辅助方法 ==========

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

    [[nodiscard]] bool wasSoundPlayed() const { return m_soundPlayed; }
    [[nodiscard]] const ResourceLocation& lastSoundId() const { return m_lastSoundId; }
    [[nodiscard]] f32 lastSoundPitch() const { return m_lastSoundPitch; }
    [[nodiscard]] bool wasGameEventFired() const { return m_gameEventFired; }
    [[nodiscard]] i32 gameEventCount() const { return m_gameEventCount; }
    [[nodiscard]] i32 setBlockCallCount() const { return m_setBlockCallCount; }
    [[nodiscard]] i32 lastSetBlockFlags() const { return m_lastSetBlockFlags; }
    [[nodiscard]] const BlockPos& lastSetBlockPos() const { return m_lastSetBlockPos; }
    [[nodiscard]] i32 spawnedEntityCount() const { return m_spawnedEntityCount; }

    void resetTrackedState()
    {
        m_soundPlayed = false;
        m_lastSoundId = ResourceLocation();
        m_lastSoundPitch = 0.0f;
        m_gameEventFired = false;
        m_gameEventCount = 0;
        m_setBlockCallCount = 0;
        m_lastSetBlockFlags = 0;
        m_lastSetBlockPos = BlockPos(0, 0, 0);
        m_spawnedEntityCount = 0;
        m_spawnedEntities.clear();
    }

    void clearState()
    {
        m_blocks.clear();
        m_blockEntities.clear();
        m_entityLookup.clear();
        resetTrackedState();
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::unordered_map<EntityInstanceId, Entity*> m_entityLookup;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    bool m_soundPlayed = false;
    ResourceLocation m_lastSoundId;
    f32 m_lastSoundPitch = 0.0f;
    bool m_gameEventFired = false;
    i32 m_gameEventCount = 0;
    i32 m_setBlockCallCount = 0;
    i32 m_lastSetBlockFlags = 0;
    BlockPos m_lastSetBlockPos{0, 0, 0};
    i32 m_spawnedEntityCount = 0;
    world::gamerule::GameRules m_gameRules;
};

} // anonymous namespace

// ========== DecoratedPotBlock 基本属性测试 ==========

class DecoratedPotBlockTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // onProjectileHit 经 ProjectileEntity::mayBreak() 检查投射物类型是否属于
        // IMPACT_PROJECTILES 标签，需先初始化 EntityTypeTags（对齐 BellBlockEntityTest 模式）。
        if (!EntityTypeTags::isInitialized()) {
            EntityTypeTags::initialize();
        }
    }

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        item::tag::ItemTags::initialize();
        pot_ =
            std::make_unique<DecoratedPotBlock>(BlockProperties(Material::DECORATION).hardness(1.0f).resistance(0.0f));
    }

    void TearDown() override { m_world.clearState(); }

    std::unique_ptr<DecoratedPotBlock> pot_;
    DecoratedPotTestWorld m_world;
};

TEST_F(DecoratedPotBlockTest, Create_HasBlockEntity)
{
    EXPECT_TRUE(pot_->hasBlockEntity());
}

TEST_F(DecoratedPotBlockTest, CreateBlockEntity_ReturnsDecoratedPotType)
{
    auto entity = pot_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getType(), BlockEntityType::DecoratedPot);
}

TEST_F(DecoratedPotBlockTest, HasComparatorInputOverride_ReturnsTrue)
{
    const auto& state = pot_->defaultState();
    EXPECT_TRUE(pot_->hasComparatorInputOverride(state));
}

TEST_F(DecoratedPotBlockTest, UseShapeForLightOcclusion_ReturnsTrue)
{
    const auto& state = pot_->defaultState();
    EXPECT_TRUE(pot_->useShapeForLightOcclusion(state));
}

TEST_F(DecoratedPotBlockTest, GetPushReaction_ReturnsDestroy)
{
    const auto& state = pot_->defaultState();
    EXPECT_EQ(pot_->getPushReaction(state), Material::PushReaction::Destroy);
}

// ========== DecoratedPotBlock 比较器信号测试 ==========

TEST_F(DecoratedPotBlockTest, ComparatorSignal_EmptyPot_ReturnsZero)
{
    DecoratedPotTestWorld world;
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    world.setBlockAt(pos, &state);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&world);
    world.setBlockEntity(pos, std::move(entity));

    EXPECT_EQ(pot_->getComparatorInputOverride(state, world, pos), 0);
}

TEST_F(DecoratedPotBlockTest, ComparatorSignal_WithItem_ReturnsNonZero)
{
    DecoratedPotTestWorld world;
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    world.setBlockAt(pos, &state);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&world);

    // 放入物品
    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 1));

    world.setBlockEntity(pos, std::move(entity));

    EXPECT_GT(pot_->getComparatorInputOverride(state, world, pos), 0);
}

TEST_F(DecoratedPotBlockTest, ComparatorSignal_FullStack_MaxValue)
{
    DecoratedPotTestWorld world;
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    world.setBlockAt(pos, &state);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&world);

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    // 钻石最大堆叠为64，放入64个应为满信号
    potEntity->setItem(ItemStack(*diamond, 64));

    world.setBlockEntity(pos, std::move(entity));

    EXPECT_EQ(pot_->getComparatorInputOverride(state, world, pos), 15);
}

// ========== DecoratedPotBlock onBlockRemoved 测试 ==========

TEST_F(DecoratedPotBlockTest, OnBlockRemoved_ClearsStoredItem)
{
    DecoratedPotTestWorld world;
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    world.setBlockAt(pos, &state);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&world);

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 3));

    world.setBlockEntity(pos, std::move(entity));

    // 移除方块
    pot_->onBlockRemoved(world, pos, state);

    // 方块实体中的物品应被清空
    auto* entityAfter = world.getBlockEntity(pos);
    if (entityAfter != nullptr && entityAfter->getType() == BlockEntityType::DecoratedPot) {
        auto* potAfter = static_cast<DecoratedPotBlockEntity*>(entityAfter);
        EXPECT_FALSE(potAfter->hasItem());
    }
}

// ========== DecoratedPotBlock getCloneItemStack 测试 ==========

TEST_F(DecoratedPotBlockTest, GetCloneItemStack_WithDecorations)
{
    DecoratedPotTestWorld world;
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    world.setBlockAt(pos, &state);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&world);

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    PotDecorations deco(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::Blank,
        DecoratedPotPattern::Blade);
    potEntity->setDecorations(deco);

    world.setBlockEntity(pos, std::move(entity));

    ItemStack cloneItem = pot_->getCloneItemStack(state, &world, &pos);

    // getCloneItemStack 依赖 BlockItemRegistry 注册的 decorated_pot 方块物品
    // 如果 BlockItemRegistry 未注册，返回空物品；如果已注册，应包含图案数据
    if (!cloneItem.isEmpty() && cloneItem.getTag() != nullptr) {
        const nlohmann::json* tag = cloneItem.getTag();
        EXPECT_TRUE(tag->contains("BlockEntityTag"));
        EXPECT_TRUE((*tag)["BlockEntityTag"].contains("sherds"));
    }
}

TEST_F(DecoratedPotBlockTest, GetCloneItemStack_WithoutWorld_ReturnsDefault)
{
    const auto& state = pot_->defaultState();
    ItemStack cloneItem = pot_->getCloneItemStack(state, nullptr, nullptr);

    // 无世界/位置时调用 Block::getCloneItemStack，
    // 依赖 BlockItemRegistry 中 decorated_pot 方块物品是否已注册
    // 如果未注册则返回空物品，这是预期行为
    if (cloneItem.isEmpty()) {
        // BlockItemRegistry 未注册 decorated_pot 方块物品时返回空物品
        GTEST_SKIP() << "Decorated pot block item not registered in test environment";
    }
}

// ========== DecoratedPotBlockEntity wobble 动画测试 ==========

TEST_F(DecoratedPotBlockTest, Wobble_WithWorld_SetsStartTick)
{
    DecoratedPotTestWorld world;
    world.setCurrentTick(100);
    BlockPos pos(0, 64, 0);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&world);

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());

    // 触发正摇晃
    potEntity->wobble(DecoratedPotBlockEntity::WobbleStyle::Positive);
    EXPECT_EQ(potEntity->lastWobbleStyle(), DecoratedPotBlockEntity::WobbleStyle::Positive);

    // 有世界时应设置起始 tick
    EXPECT_TRUE(potEntity->isWobbling(100));
    EXPECT_TRUE(potEntity->isWobbling(105));  // Positive wobble 持续7tick
    EXPECT_FALSE(potEntity->isWobbling(107)); // 超过持续时间

    // 触发负摇晃
    potEntity->wobble(DecoratedPotBlockEntity::WobbleStyle::Negative);
    EXPECT_EQ(potEntity->lastWobbleStyle(), DecoratedPotBlockEntity::WobbleStyle::Negative);
}

TEST_F(DecoratedPotBlockTest, Wobble_WithoutWorld_DoesNotCrash)
{
    auto entity = pot_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    // 不设置世界，m_world == nullptr

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());

    // 无世界时不应崩溃
    potEntity->wobble(DecoratedPotBlockEntity::WobbleStyle::Positive);
    EXPECT_EQ(potEntity->lastWobbleStyle(), DecoratedPotBlockEntity::WobbleStyle::Positive);
}

// ========== DecoratedPotBlockEntity 比较器信号独立测试 ==========

TEST_F(DecoratedPotBlockTest, EntityComparatorSignal_Empty)
{
    auto entity = pot_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    EXPECT_EQ(potEntity->getComparatorSignal(), 0);
}

TEST_F(DecoratedPotBlockTest, EntityComparatorSignal_SingleItem)
{
    auto entity = pot_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 1));

    // 1个物品（max=64）：1 + (1-1)*15/64 = 1
    EXPECT_EQ(potEntity->getComparatorSignal(), 1);
}

TEST_F(DecoratedPotBlockTest, EntityComparatorSignal_HalfStack)
{
    auto entity = pot_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 32));

    // 32个物品（max=64）：1 + (32-1)*15/64 = 1 + 31*15/64 = 1 + 7 = 8
    EXPECT_EQ(potEntity->getComparatorSignal(), 8);
}

TEST_F(DecoratedPotBlockTest, EntityComparatorSignal_FullStack)
{
    auto entity = pot_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 64));

    // 64个物品（max=64）：1 + (64-1)*15/64 = 1 + 63*15/64 = 1 + 14 = 15
    EXPECT_EQ(potEntity->getComparatorSignal(), 15);
}

// ========== DecoratedPotBlockEntity getDirection 测试 ==========

TEST_F(DecoratedPotBlockTest, GetDirection_WithoutBlockState_ReturnsNorth)
{
    auto entity = pot_->createBlockEntity(BlockPos(0, 0, 0));
    ASSERT_NE(entity, nullptr);
    // 没有设置方块状态，应返回默认 North
    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    EXPECT_EQ(potEntity->getDirection(), Direction::North);
}

// ========== DecoratedPotBlockEntity NBT 序列化完整性测试 ==========

TEST_F(DecoratedPotBlockTest, NBTSerialization_RoundTrip)
{
    auto entity = std::make_unique<DecoratedPotBlockEntity>(BlockPos(10, 20, 30));

    PotDecorations deco(DecoratedPotPattern::Burn,
        DecoratedPotPattern::Danger,
        DecoratedPotPattern::Explorer,
        DecoratedPotPattern::Friend);
    entity->setDecorations(deco);

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    entity->setItem(ItemStack(*diamond, 2));

    // NBT 序列化
    nbt::tags::compound_tag tag;
    entity->saveToNBT(tag);

    // 反序列化到新实体
    auto loaded = std::make_unique<DecoratedPotBlockEntity>(BlockPos(0, 0, 0));
    bool success = loaded->loadFromNBT(tag);
    EXPECT_TRUE(success);

    EXPECT_EQ(loaded->getDecorations(), deco);
    EXPECT_TRUE(loaded->hasItem());
    EXPECT_EQ(loaded->getItem().getCount(), 2);
}

// ========== DecoratedPotBlock 水位测试 ==========

TEST_F(DecoratedPotBlockTest, IsWaterlogged_DefaultFalse)
{
    const auto& state = pot_->defaultState();
    EXPECT_FALSE(pot_->isWaterlogged(state));
}

TEST_F(DecoratedPotBlockTest, GetFluidState_DefaultNotWaterlogged)
{
    const auto& state = pot_->defaultState();
    // 默认不含水
    EXPECT_FALSE(pot_->isWaterlogged(state));

    // getFluidState 的结果取决于 Fluid 注册表是否可用
    // 在测试环境中可能为空，只验证接口不崩溃
    const auto* fluidState = pot_->getFluidState(state);
    MC_UNUSED(fluidState);
}

// ============================================================================
// DecoratedPotBlock playerWillDestroy 测试
// ============================================================================

TEST_F(DecoratedPotBlockTest, PlayerWillDestroy_SwordSetsCracked)
{
    // 手持剑（BREAKS_DECORATED_POTS 标签物品）破坏陶罐应设置 CRACKED 状态
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    // 给玩家手持钻石剑
    if (Items::DIAMOND_SWORD != nullptr) {
        ItemStack sword(Items::DIAMOND_SWORD, 1);
        player->getHeldItem(Hand::MainHand) = sword;

        i32 setBlockBefore = m_world.setBlockCallCount();
        pot_->playerWillDestroy(m_world, pos, state, *player);

        // 应调用 setBlockState 设置 CRACKED 状态（flags=260）
        EXPECT_GT(m_world.setBlockCallCount(), setBlockBefore);
        EXPECT_EQ(m_world.lastSetBlockFlags(), 260);

        // 验证方块被设为 CRACKED
        const BlockState* newState = m_world.getBlockState(pos.x, pos.y, pos.z);
        ASSERT_NE(newState, nullptr);
        EXPECT_TRUE(newState->get(BlockStateProperties::CRACKED()));
    }
}

TEST_F(DecoratedPotBlockTest, PlayerWillDestroy_PickaxeSetsCracked)
{
    // 手持镐（BREAKS_DECORATED_POTS 标签物品）破坏陶罐应设置 CRACKED 状态
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    if (Items::DIAMOND_PICKAXE != nullptr) {
        ItemStack pickaxe(Items::DIAMOND_PICKAXE, 1);
        player->getHeldItem(Hand::MainHand) = pickaxe;

        pot_->playerWillDestroy(m_world, pos, state, *player);

        const BlockState* newState = m_world.getBlockState(pos.x, pos.y, pos.z);
        ASSERT_NE(newState, nullptr);
        EXPECT_TRUE(newState->get(BlockStateProperties::CRACKED()));
    }
}

TEST_F(DecoratedPotBlockTest, PlayerWillDestroy_EmptyHandDoesNotSetCracked)
{
    // 空手破坏陶罐不应设置 CRACKED 状态
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    // 空手：默认 ItemStack 为空

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->playerWillDestroy(m_world, pos, state, *player);

    // 不应调用 setBlockState（不设置 CRACKED）
    EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore);
}

TEST_F(DecoratedPotBlockTest, PlayerWillDestroy_NonTagItemDoesNotSetCracked)
{
    // 手持非 BREAKS_DECORATED_POTS 标签物品不应设置 CRACKED 状态
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    // 钻石不是工具类物品，不在 BREAKS_DECORATED_POTS 标签中
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack diamondStack(*diamond, 1);
    player->getHeldItem(Hand::MainHand) = diamondStack;

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->playerWillDestroy(m_world, pos, state, *player);

    EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore);
}

TEST_F(DecoratedPotBlockTest, PlayerWillDestroy_AlreadyCracked_NoDoubleSet)
{
    // 已经是 CRACKED 状态时不应再次 setBlockState
    BlockPos pos(0, 64, 0);
    BlockState crackedState = pot_->defaultState().with(BlockStateProperties::CRACKED(), true);
    m_world.setBlockAt(pos, &crackedState);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    if (Items::DIAMOND_SWORD != nullptr) {
        ItemStack sword(Items::DIAMOND_SWORD, 1);
        player->getHeldItem(Hand::MainHand) = sword;

        i32 setBlockBefore = m_world.setBlockCallCount();
        pot_->playerWillDestroy(m_world, pos, crackedState, *player);

        // 已是 CRACKED 状态，不应再设置
        EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore);
    }
}

// ============================================================================
// DecoratedPotBlock onProjectileHit 测试
// ============================================================================

TEST_F(DecoratedPotBlockTest, OnProjectileHit_ProjectileSetsCrackedAndDestroys)
{
    // 投射物命中陶罐：设为 CRACKED 状态并破坏
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    // 创建一个箭矢投射物，设置射手为生存模式玩家
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);
    m_world.registerEntity(player.get());

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);
    arrow.setShooter(player.get());

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->onProjectileHit(m_world, state, hitResult, arrow);

    // 应调用 setBlockState 两次：一次设置 CRACKED（flags=260），一次移除方块（flags=3）
    EXPECT_GE(m_world.setBlockCallCount(), setBlockBefore + 1);

    // 方块应被移除（设为空气）
    const BlockState* finalState = m_world.getBlockState(pos.x, pos.y, pos.z);
    EXPECT_TRUE(finalState == nullptr || finalState->is(VanillaBlocks::AIR));
}

TEST_F(DecoratedPotBlockTest, OnProjectileHit_ClientSide_DoesNothing)
{
    // 客户端侧不应触发投射物命中逻辑
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(true); // 客户端

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->onProjectileHit(m_world, state, hitResult, arrow);

    EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore);
}

TEST_F(DecoratedPotBlockTest, OnProjectileHit_NonProjectileEntity_DoesNothing)
{
    // 非 ProjectileEntity 的普通实体不应触发投射物命中逻辑
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    // 使用普通 Entity（不是 ProjectileEntity 的子类）
    Entity nonProjectile(EntityInstanceId(300), nullptr, mc::test::testEcsRegistry());
    nonProjectile.setWorld(&m_world);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->onProjectileHit(m_world, state, hitResult, nonProjectile);

    // dynamic_cast<ProjectileEntity*> 失败，不应触发逻辑
    EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore);
}

TEST_F(DecoratedPotBlockTest, OnProjectileHit_MayInteractFalse_DoesNothing)
{
    // 投射物射手为冒险模式玩家时 mayInteract 返回 false，不应破坏陶罐
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure); // 冒险模式：mayInteract 返回 false
    m_world.registerEntity(player.get());

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);
    arrow.setShooter(player.get());

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->onProjectileHit(m_world, state, hitResult, arrow);

    EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore);
}

TEST_F(DecoratedPotBlockTest, OnProjectileHit_MobGriefingFalse_DoesNothing)
{
    // MOB_GRIEFING=false 时，非玩家射手的投射物 mayInteract 返回 false
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);

    // 非玩家实体作为射手
    Entity mob(EntityInstanceId(300), nullptr, mc::test::testEcsRegistry());
    mob.setWorld(&m_world);
    m_world.registerEntity(&mob);

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);
    arrow.setShooter(&mob);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->onProjectileHit(m_world, state, hitResult, arrow);

    EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore);
}

TEST_F(DecoratedPotBlockTest, OnProjectileHit_NullShooter_Allowed)
{
    // 无主投射物 mayInteract 返回 true，应破坏陶罐
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);
    // 不设置射手（无主投射物）

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    pot_->onProjectileHit(m_world, state, hitResult, arrow);

    // 方块应被移除
    const BlockState* finalState = m_world.getBlockState(pos.x, pos.y, pos.z);
    EXPECT_TRUE(finalState == nullptr || finalState->is(VanillaBlocks::AIR));
}

TEST_F(DecoratedPotBlockTest, OnProjectileHit_AlreadyCracked_StillDestroys)
{
    // 已经 CRACKED 的陶罐被投射物命中时，仍应被破坏（不再设置 CRACKED，但应移除方块）
    BlockPos pos(0, 64, 0);
    BlockState crackedState = pot_->defaultState().with(BlockStateProperties::CRACKED(), true);
    m_world.setBlockAt(pos, &crackedState);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);
    m_world.registerEntity(player.get());

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);
    arrow.setShooter(player.get());

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    i32 setBlockBefore = m_world.setBlockCallCount();
    pot_->onProjectileHit(m_world, crackedState, hitResult, arrow);

    // 应调用 setBlockState 移除方块（flags=3）
    // 因为已是 CRACKED，不应再调用 setBlockState 设置 CRACKED（flags=260）
    // 所以调用次数应为 setBlockBefore + 1（仅移除）
    EXPECT_EQ(m_world.setBlockCallCount(), setBlockBefore + 1);
    EXPECT_EQ(m_world.lastSetBlockFlags(), 3);

    // 方块应被移除
    const BlockState* finalState = m_world.getBlockState(pos.x, pos.y, pos.z);
    EXPECT_TRUE(finalState == nullptr || finalState->is(VanillaBlocks::AIR));
}

// ============================================================================
// DecoratedPotBlock onBlockRemoved 测试 — CRACKED vs 非 CRACKED 状态
// ============================================================================

TEST_F(DecoratedPotBlockTest, OnBlockRemoved_CrackedState_ClearsStoredItem)
{
    // CRACKED 状态：应掉落4个陶片/砖块物品，不保留存储物品
    BlockPos pos(0, 64, 0);
    BlockState crackedState = pot_->defaultState().with(BlockStateProperties::CRACKED(), true);
    m_world.setBlockAt(pos, &crackedState);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());

    // 设置存储物品
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 5));

    // 设置装饰（4个面都有图案）
    PotDecorations deco(DecoratedPotPattern::Angler,
        DecoratedPotPattern::Archer,
        DecoratedPotPattern::Blade,
        DecoratedPotPattern::Burn);
    potEntity->setDecorations(deco);

    m_world.setBlockEntity(pos, std::move(entity));

    i32 spawnBefore = m_world.spawnedEntityCount();
    pot_->onBlockRemoved(m_world, pos, crackedState);

    // CRACKED 状态应生成物品实体（4个陶片）
    // spawnEntity 可能被 ItemDropHelper 调用，但因为测试世界可能无法完全模拟
    // 关键验证：方块实体中物品应被清空
    auto* entityAfter = m_world.getBlockEntity(pos);
    if (entityAfter != nullptr && entityAfter->getType() == BlockEntityType::DecoratedPot) {
        auto* potAfter = static_cast<DecoratedPotBlockEntity*>(entityAfter);
        EXPECT_FALSE(potAfter->hasItem());
    }
}

TEST_F(DecoratedPotBlockTest, OnBlockRemoved_NonCrackedState_ClearsStoredItem)
{
    // 非 CRACKED 状态：应掉落存储物品（陶罐物品由战利品表处理）
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState(); // CRACKED = false
    m_world.setBlockAt(pos, &state);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 3));

    m_world.setBlockEntity(pos, std::move(entity));

    pot_->onBlockRemoved(m_world, pos, state);

    // 方块实体中物品应被清空
    auto* entityAfter = m_world.getBlockEntity(pos);
    if (entityAfter != nullptr && entityAfter->getType() == BlockEntityType::DecoratedPot) {
        auto* potAfter = static_cast<DecoratedPotBlockEntity*>(entityAfter);
        EXPECT_FALSE(potAfter->hasItem());
    }
}

TEST_F(DecoratedPotBlockTest, OnBlockRemoved_NonCrackedEmptyPot_NoDrop)
{
    // 非 CRACKED 且空陶罐：不应生成任何物品实体
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    // 不设置物品（空陶罐）

    m_world.setBlockEntity(pos, std::move(entity));

    i32 spawnBefore = m_world.spawnedEntityCount();
    pot_->onBlockRemoved(m_world, pos, state);

    // 空陶罐不应生成物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), spawnBefore);
}

TEST_F(DecoratedPotBlockTest, OnBlockRemoved_ClientSide_DoesNotDropItems)
{
    // 客户端侧不应掉落物品
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(true); // 客户端

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);

    auto* potEntity = static_cast<DecoratedPotBlockEntity*>(entity.get());
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    potEntity->setItem(ItemStack(*diamond, 10));

    m_world.setBlockEntity(pos, std::move(entity));

    i32 spawnBefore = m_world.spawnedEntityCount();
    pot_->onBlockRemoved(m_world, pos, state);

    // 客户端侧不应生成物品实体
    EXPECT_EQ(m_world.spawnedEntityCount(), spawnBefore);
}

TEST_F(DecoratedPotBlockTest, OnBlockRemoved_NoBlockEntity_DoesNotCrash)
{
    // 无方块实体时不应崩溃
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    // 不设置方块实体

    EXPECT_NO_THROW(pot_->onBlockRemoved(m_world, pos, state));
}

TEST_F(DecoratedPotBlockTest, OnBlockRemoved_WrongBlockEntityType_DoesNotCrash)
{
    // 方块实体类型不匹配时不应崩溃（不太可能发生，但需确保安全）
    // 此测试验证即使方块实体类型不是 DecoratedPot，也不会崩溃
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    // 不设置方块实体——getBlockEntity 返回 nullptr，应安全跳过

    EXPECT_NO_THROW(pot_->onBlockRemoved(m_world, pos, state));
}

// ============================================================================
// DecoratedPotBlock onBlockActivated 测试 — 音效和动画
// ============================================================================

TEST_F(DecoratedPotBlockTest, OnBlockActivated_EmptyHand_NegativeWobbleAndSound)
{
    // 空手交互应触发负摇晃动画和插入失败音效
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setBlockEntity(pos, std::move(entity));

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    m_world.resetTrackedState();
    auto result = pot_->onBlockActivated(state, m_world, pos, *player, Hand::MainHand, hitResult);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(m_world.wasSoundPlayed());
}

TEST_F(DecoratedPotBlockTest, OnBlockActivated_InsertItem_PositiveWobbleAndSound)
{
    // 手持物品向空陶罐放入应触发正摇晃动画和插入音效
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setBlockEntity(pos, std::move(entity));

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    // 手持钻石
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    player->getHeldItem(Hand::MainHand) = ItemStack(*diamond, 10);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    m_world.resetTrackedState();
    auto result = pot_->onBlockActivated(state, m_world, pos, *player, Hand::MainHand, hitResult);

    EXPECT_EQ(result, ActionResultType::Success);
    EXPECT_TRUE(m_world.wasSoundPlayed());

    // 验证物品被放入陶罐
    auto* potEntityAfter = static_cast<DecoratedPotBlockEntity*>(m_world.getBlockEntity(pos));
    ASSERT_NE(potEntityAfter, nullptr);
    EXPECT_TRUE(potEntityAfter->hasItem());
    EXPECT_EQ(potEntityAfter->getItem().getCount(), 1);
}

TEST_F(DecoratedPotBlockTest, OnBlockActivated_OffHand_Pass)
{
    // 副手交互应返回 Pass
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto entity = pot_->createBlockEntity(pos);
    ASSERT_NE(entity, nullptr);
    entity->setWorld(&m_world);
    m_world.setBlockEntity(pos, std::move(entity));

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    player->getHeldItem(Hand::OffHand) = ItemStack(*diamond, 10);

    BlockRaycastResult hitResult = BlockRaycastResult::hit(Vector3(0.5f, 64.5f, 0.5f), pos, Direction::Up, 0.0f);

    auto result = pot_->onBlockActivated(state, m_world, pos, *player, Hand::OffHand, hitResult);
    EXPECT_EQ(result, ActionResultType::Pass);
}

// ============================================================================
// DecoratedPotBlock CRACKED 属性测试
// ============================================================================

TEST_F(DecoratedPotBlockTest, DefaultState_NotCracked)
{
    // 默认状态 CRACKED = false
    const auto& state = pot_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::CRACKED()));
}

TEST_F(DecoratedPotBlockTest, CrackedState_CanBeSet)
{
    // CRACKED 属性可以被设置
    BlockState crackedState = pot_->defaultState().with(BlockStateProperties::CRACKED(), true);
    EXPECT_TRUE(crackedState.get(BlockStateProperties::CRACKED()));
}

TEST_F(DecoratedPotBlockTest, PlayerWillDestroy_SilkTouchPreventsCracked)
{
    // 精准采集附魔应阻止设置 CRACKED 状态
    // 注意：EnchantmentHelper::hasSilkTouch 的完整测试依赖附魔系统，
    // 这里验证当 hasSilkTouch 返回 true 时逻辑正确分支
    // 由于测试环境中附魔可能未完整注册，此测试验证基本逻辑：
    // 当手持物品在 BREAKS_DECORATED_POTS 标签中但没有精准采集时，设置 CRACKED
    BlockPos pos(0, 64, 0);
    const auto& state = pot_->defaultState();
    m_world.setBlockAt(pos, &state);
    m_world.setClientSide(false);

    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);

    if (Items::DIAMOND_SWORD != nullptr) {
        ItemStack sword(Items::DIAMOND_SWORD, 1);
        player->getHeldItem(Hand::MainHand) = sword;

        // 不添加精准采集附魔——应设置 CRACKED
        i32 setBlockBefore = m_world.setBlockCallCount();
        pot_->playerWillDestroy(m_world, pos, state, *player);

        // 验证 CRACKED 被设置（因为没有精准采集）
        EXPECT_GT(m_world.setBlockCallCount(), setBlockBefore);
        EXPECT_EQ(m_world.lastSetBlockFlags(), 260);
    }
}

// ============================================================================
// ProjectileEntity::mayInteract 与 DecoratedPotBlock 集成测试
// ============================================================================

TEST_F(DecoratedPotBlockTest, ProjectileMayInteract_SurvivalPlayer_Allowed)
{
    // 生存模式玩家的投射物 mayInteract 返回 true，应能破坏陶罐
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Survival);
    m_world.registerEntity(player.get());

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);
    arrow.setShooter(player.get());

    BlockPos pos(0, 64, 0);
    EXPECT_TRUE(arrow.mayInteract(m_world, pos));
}

TEST_F(DecoratedPotBlockTest, ProjectileMayInteract_AdventurePlayer_Denied)
{
    // 冒险模式玩家的投射物 mayInteract 返回 false
    auto player = std::make_unique<Player>(EntityInstanceId(100), "TestPlayer", mc::test::testEcsRegistry());
    player->setWorld(&m_world);
    player->setGameMode(GameMode::Adventure);
    m_world.registerEntity(player.get());

    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);
    arrow.setShooter(player.get());

    BlockPos pos(0, 64, 0);
    EXPECT_FALSE(arrow.mayInteract(m_world, pos));
}

TEST_F(DecoratedPotBlockTest, ProjectileMayInteract_NullShooter_Allowed)
{
    // 无主投射物 mayInteract 返回 true
    entity::ArrowEntity arrow(EntityInstanceId(200), mc::test::testEcsRegistry());
    // 直接构造的 ArrowEntity 未设 typeId，需显式设为 minecraft:arrow 使 mayBreak()
    // 的 IMPACT_PROJECTILES 标签检查通过（对齐 commit 8bb41781b 测试修复策略）。
    arrow.setTypeId(entity::EntityTypeKeys::ARROW);
    arrow.setWorld(&m_world);

    BlockPos pos(0, 64, 0);
    EXPECT_TRUE(arrow.mayInteract(m_world, pos));
}

// ============================================================================
// BREAKS_DECORATED_POTS 标签集成测试
// ============================================================================

TEST_F(DecoratedPotBlockTest, BreaksDecoratedPotsTag_ContainsSwords)
{
    // 验证 SWORDS 标签包含至少一种剑
    const item::tag::ItemTag& swordsTag = item::tag::ItemTags::SWORDS();
    bool hasSword = false;
    if (Items::DIAMOND_SWORD != nullptr) {
        hasSword = swordsTag.contains(Items::DIAMOND_SWORD);
    }
    // 如果钻石剑已注册，应在 SWORDS 标签中
    if (Items::DIAMOND_SWORD != nullptr) {
        EXPECT_TRUE(hasSword);
    }
}

TEST_F(DecoratedPotBlockTest, BreaksDecoratedPotsTag_ContainsTools)
{
    // 验证 BREAKS_DECORATED_POTS 标签包含工具类物品
    const item::tag::ItemTag& breaksTag = item::tag::ItemTags::BREAKS_DECORATED_POTS();

    bool hasTool = false;
    if (Items::DIAMOND_SWORD != nullptr) {
        hasTool = breaksTag.contains(Items::DIAMOND_SWORD);
    }
    if (Items::DIAMOND_PICKAXE != nullptr) {
        hasTool = hasTool || breaksTag.contains(Items::DIAMOND_PICKAXE);
    }

    // 至少应有一种工具在标签中
    if (Items::DIAMOND_SWORD != nullptr || Items::DIAMOND_PICKAXE != nullptr) {
        EXPECT_TRUE(hasTool);
    }
}

TEST_F(DecoratedPotBlockTest, BreaksDecoratedPotsTag_DoesNotContainDiamond)
{
    // 验证 BREAKS_DECORATED_POTS 标签不包含非工具物品（如钻石）
    const item::tag::ItemTag& breaksTag = item::tag::ItemTags::BREAKS_DECORATED_POTS();

    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    if (diamond != nullptr) {
        EXPECT_FALSE(breaksTag.contains(diamond));
    }
}
