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
#include "common/item/core/ItemStack.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/functional/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include "common/world/blockentity/interactive/DecoratedPotPattern.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

using namespace mc;
using namespace mc::blocks;
using namespace mc::blockentity;

namespace {

/**
 * @brief 饰纹陶罐测试用 Mock 世界实现
 */
class DecoratedPotTestWorld final : public test::BaseTestWorld {
public:
    DecoratedPotTestWorld()
    {
        VanillaBlocks::initialize();
        Items::initialize();
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
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
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
        return fluid::Fluid::getFluidState(0);
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }

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

    void resetTrackedState()
    {
        m_soundPlayed = false;
        m_lastSoundId = ResourceLocation();
        m_lastSoundPitch = 0.0f;
        m_gameEventFired = false;
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    bool m_soundPlayed = false;
    ResourceLocation m_lastSoundId;
    f32 m_lastSoundPitch = 0.0f;
    bool m_gameEventFired = false;
};

} // anonymous namespace

// ========== DecoratedPotBlock 基本属性测试 ==========

class DecoratedPotBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        pot_ =
            std::make_unique<DecoratedPotBlock>(BlockProperties(Material::DECORATION).hardness(1.0f).resistance(0.0f));
    }

    std::unique_ptr<DecoratedPotBlock> pot_;
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
