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
 * @file BrushItemTest.cpp
 * @brief BrushItem 刷子物品单元测试
 *
 * 测试刷子的核心逻辑：
 * - 物品注册和基本属性
 * - 使用动作和持续时长
 * - onUseTick 刷扫触发时机（仅在第5、15、25... tick触发）
 * - onUseTick 非玩家实体停止使用
 * - onUseTick 视线未对准方块时停止使用（对齐 MC BrushItem.onUseTick）
 * - onUseTick 命中方块时生成 Block 粒子并播放音效
 * - 命中 BrushableBlock（可疑沙）时使用方块专属刷扫音效
 * - 命中普通方块时使用 BRUSH_GENERIC 音效
 * - updateActiveItem 中 stopActiveHand 后不触发 onItemUseFinish
 * - itemInteractionForEntity 空实现
 * - DustParticlesDelta.fromDirection 各方向偏移
 * - 常量验证
 *
 * MC 1.21.11 参考：net.minecraft.world.item.BrushItem
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/special/BrushItem.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/functional/TrailsBlocks.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <unordered_map>

namespace mc {
namespace {

// ============================================================================
// 测试用实体
// ============================================================================

/**
 * @brief 测试用 LivingEntity（非玩家）
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/**
 * @brief 测试用 Player
 */
class TestPlayer : public Player {
public:
    explicit TestPlayer(IWorld* world = nullptr)
        : Player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }
};

// ============================================================================
// 测试用世界桩
// ============================================================================

/**
 * @brief 刷子测试用世界
 *
 * 提供方块状态查询、方块粒子生成、音效播放的捕获能力，
 * 用于验证 BrushItem::onUseTick 中的射线检测、粒子生成和音效播放逻辑。
 */
class BrushTestWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    // ========== 方块状态存储 ==========

    /**
     * @brief 在指定坐标放置方块状态（用于射线检测）
     */
    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[blockKey(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blockStates.find(blockKey(x, y, z));
        return it != m_blockStates.end() ? it->second : nullptr;
    }

    // ========== 粒子捕获 ==========

    struct BlockParticleCall {
        particle::ParticleTypeId typeId;
        Vector3 position;
        Vector3 velocity;
        const BlockState* blockState;
    };

    struct GenericParticleCall {
        particle::ParticleTypeId typeId;
        Vector3 position;
        Vector3 velocity;
    };

    void addParticle(particle::ParticleTypeId typeId, const Vector3& pos, const Vector3& velocity) override
    {
        m_genericParticles.push_back(GenericParticleCall{typeId, pos, velocity});
    }

    void addBlockParticle(particle::ParticleTypeId typeId,
        const Vector3& pos,
        const Vector3& velocity,
        const BlockState& blockState) override
    {
        m_blockParticles.push_back(BlockParticleCall{typeId, pos, velocity, &blockState});
    }

    [[nodiscard]] const std::vector<BlockParticleCall>& blockParticles() const { return m_blockParticles; }
    [[nodiscard]] const std::vector<GenericParticleCall>& genericParticles() const { return m_genericParticles; }
    [[nodiscard]] i32 blockParticleCount() const { return static_cast<i32>(m_blockParticles.size()); }
    [[nodiscard]] i32 genericParticleCount() const { return static_cast<i32>(m_genericParticles.size()); }
    void clearParticles()
    {
        m_blockParticles.clear();
        m_genericParticles.clear();
    }

    // ========== 音效捕获 ==========

    struct SoundCall {
        ResourceLocation soundId;
        sound::SoundCategory category;
        Vector3 position;
        f32 volume;
        f32 pitch;
    };

    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        m_soundCalls.push_back(SoundCall{soundId, category, position, volume, pitch});
    }

    [[nodiscard]] const std::vector<SoundCall>& soundCalls() const { return m_soundCalls; }
    [[nodiscard]] i32 soundCount() const { return static_cast<i32>(m_soundCalls.size()); }
    void clearSoundCalls() { m_soundCalls.clear(); }

    // ========== TickManager 接口（桩实现） ==========

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BrushTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BrushTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<i64, const BlockState*> m_blockStates;
    std::vector<BlockParticleCall> m_blockParticles;
    std::vector<GenericParticleCall> m_genericParticles;
    std::vector<SoundCall> m_soundCalls;

    static i64 blockKey(i32 x, i32 y, i32 z)
    {
        return static_cast<i64>(x) | (static_cast<i64>(y) << 16) | (static_cast<i64>(z) << 32);
    }
};

// ============================================================================
// 测试基类
// ============================================================================

class BrushItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        Items::initialize();
        if (!VanillaBlocks::STONE) {
            VanillaBlocks::initialize();
        }
    }

    void SetUp() override { m_world = std::make_unique<BrushTestWorld>(); }

    /**
     * @brief 配置一个直视下方并命中目标方块的玩家
     *
     * 玩家位置 (0.5, 5.0, 0.5)，pitch=90（直视下方），
     * 眼睛位置 (0.5, 6.62, 0.5)。
     * 在 (0, 4, 0) 放置方块后，方块顶面在 y=5.0，
     * 射线沿 (0, -1, 0) 向下在 (0.5, 5.0, 0.5) 命中 Up 面。
     */
    void setupPlayerLookingDownAtBlock(TestPlayer& player, const BlockState* blockState, BlockPos blockPos)
    {
        // 设置玩家位置与朝向
        player.setPosition(0.5f, 5.0f, 0.5f);
        player.setRotation(0.0f, 90.0f); // yaw=0, pitch=90 → 视线 (0, -1, 0)

        // 在指定位置放置方块
        m_world->setBlockStateAt(blockPos.x, blockPos.y, blockPos.z, blockState);
    }

    std::unique_ptr<BrushTestWorld> m_world;
};

// ============================================================================
// 物品注册测试
// ============================================================================

TEST_F(BrushItemTest, BrushIsRegistered)
{
    ASSERT_NE(Items::BRUSH, nullptr) << "BRUSH should be registered";
}

TEST_F(BrushItemTest, ArmadilloScuteIsRegistered)
{
    ASSERT_NE(Items::ARMADILLO_SCUTE, nullptr) << "ARMADILLO_SCUTE should be registered";
}

TEST_F(BrushItemTest, BrushItemLocation)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->itemLocation(), ResourceLocation("minecraft:brush"));
}

TEST_F(BrushItemTest, ArmadilloScuteItemLocation)
{
    ASSERT_NE(Items::ARMADILLO_SCUTE, nullptr);
    EXPECT_EQ(Items::ARMADILLO_SCUTE->itemLocation(), ResourceLocation("minecraft:armadillo_scute"));
}

// ============================================================================
// 物品属性测试
// ============================================================================

TEST_F(BrushItemTest, MaxDamage)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->maxDamage(), item::BrushItem::MAX_DURABILITY);
    EXPECT_EQ(Items::BRUSH->maxDamage(), 64);
}

TEST_F(BrushItemTest, MaxStackSize)
{
    // 有耐久度的物品堆叠数为1
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->maxStackSize(), 1);
}

TEST_F(BrushItemTest, IsDamageable)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_TRUE(Items::BRUSH->isDamageable());
}

TEST_F(BrushItemTest, GetUseDuration)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_EQ(Items::BRUSH->getUseDuration(stack), item::BrushItem::USE_DURATION);
    EXPECT_EQ(Items::BRUSH->getUseDuration(stack), 200);
}

TEST_F(BrushItemTest, GetUseAction)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_EQ(Items::BRUSH->getUseAction(stack), UseAction::Brush);
}

TEST_F(BrushItemTest, GetItemEnchantability)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->getItemEnchantability(), 1);
}

TEST_F(BrushItemTest, IsFood)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_FALSE(Items::BRUSH->isFood());
}

TEST_F(BrushItemTest, IsMusicDisc)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_FALSE(Items::BRUSH->isMusicDisc());
}

// ============================================================================
// 常量验证测试
// ============================================================================

TEST_F(BrushItemTest, ConstantValuesMatchMC)
{
    // MC 1.21.11: BrushItem 常量
    EXPECT_EQ(item::BrushItem::MAX_DURABILITY, 64);
    EXPECT_EQ(item::BrushItem::USE_DURATION, 200);
    EXPECT_EQ(item::BrushItem::ANIMATION_DURATION, 10);
    EXPECT_EQ(item::BrushItem::BRUSH_TICK_IN_CYCLE, 4);
    EXPECT_EQ(item::BrushItem::ARMADILLO_DURABILITY_COST, 16);
    // 注：BLOCK_INTERACTION_RANGE 已移除，改用 Player::blockInteractionRange()
    // 该值由 generic.block_interaction_range 属性决定（生存/冒险 4.5，创造 5.0），
    // 由 PlayerInteractionRange 测试覆盖
}

// ============================================================================
// 类型转换测试
// ============================================================================

TEST_F(BrushItemTest, DynamicCastToBrushItem)
{
    Item* baseItem = Items::BRUSH;
    ASSERT_NE(baseItem, nullptr);
    auto* brushItem = dynamic_cast<item::BrushItem*>(baseItem);
    ASSERT_NE(brushItem, nullptr) << "BRUSH should be castable to BrushItem";
}

// ============================================================================
// ItemStack 测试
// ============================================================================

TEST_F(BrushItemTest, ItemStackCreation)
{
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::BRUSH);
    EXPECT_EQ(stack.getCount(), 1);
    EXPECT_EQ(stack.getDamage(), 0) << "New brush should have 0 damage";
}

TEST_F(BrushItemTest, ItemStackDurabilityTracking)
{
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_EQ(stack.getMaxDamage(), 64);

    // 模拟耐久度消耗
    stack.attemptDamageItem(1, nullptr);
    EXPECT_EQ(stack.getDamage(), 1);

    // 累积消耗
    stack.attemptDamageItem(9, nullptr);
    EXPECT_EQ(stack.getDamage(), 10);
}

// ============================================================================
// DustParticlesDelta::fromDirection 测试
// ============================================================================

/**
 * @brief 验证 DustParticlesDelta::fromDirection 在各方向上的偏移
 *
 * 对齐 MC 1.21.11 BrushItem.DustParticlesDelta.fromDirection：
 * - Down/Up：偏移取视线向量的 (z, 0, -x)
 * - North：(1, 0, -0.1)
 * - South：(-1, 0, 0.1)
 * - West：(-0.1, 0, -1)
 * - East：(0.1, 0, 1)
 */
TEST_F(BrushItemTest, DustParticlesDelta_DownUsesViewVectorZAndNegX)
{
    const Vector3 view(0.3f, 0.0f, 0.7f);
    const auto delta = item::BrushItem::DustParticlesDelta::fromDirection(view, Direction::Down);
    // viewVector 为 f32，转 f64 后保留 f32 精度，用 EXPECT_NEAR 比较
    EXPECT_NEAR(delta.xd, 0.7, 1.0e-6);
    EXPECT_DOUBLE_EQ(delta.yd, 0.0);
    EXPECT_NEAR(delta.zd, -0.3, 1.0e-6);
}

TEST_F(BrushItemTest, DustParticlesDelta_UpUsesViewVectorZAndNegX)
{
    const Vector3 view(0.4f, 0.0f, -0.2f);
    const auto delta = item::BrushItem::DustParticlesDelta::fromDirection(view, Direction::Up);
    EXPECT_NEAR(delta.xd, -0.2, 1.0e-6);
    EXPECT_DOUBLE_EQ(delta.yd, 0.0);
    EXPECT_NEAR(delta.zd, -0.4, 1.0e-6);
}

TEST_F(BrushItemTest, DustParticlesDelta_North)
{
    const Vector3 view(0.0f, 0.0f, 1.0f);
    const auto delta = item::BrushItem::DustParticlesDelta::fromDirection(view, Direction::North);
    EXPECT_DOUBLE_EQ(delta.xd, 1.0);
    EXPECT_DOUBLE_EQ(delta.yd, 0.0);
    EXPECT_DOUBLE_EQ(delta.zd, -0.1);
}

TEST_F(BrushItemTest, DustParticlesDelta_South)
{
    const Vector3 view(0.0f, 0.0f, 1.0f);
    const auto delta = item::BrushItem::DustParticlesDelta::fromDirection(view, Direction::South);
    EXPECT_DOUBLE_EQ(delta.xd, -1.0);
    EXPECT_DOUBLE_EQ(delta.yd, 0.0);
    EXPECT_DOUBLE_EQ(delta.zd, 0.1);
}

TEST_F(BrushItemTest, DustParticlesDelta_West)
{
    const Vector3 view(0.0f, 0.0f, 1.0f);
    const auto delta = item::BrushItem::DustParticlesDelta::fromDirection(view, Direction::West);
    EXPECT_DOUBLE_EQ(delta.xd, -0.1);
    EXPECT_DOUBLE_EQ(delta.yd, 0.0);
    EXPECT_DOUBLE_EQ(delta.zd, -1.0);
}

TEST_F(BrushItemTest, DustParticlesDelta_East)
{
    const Vector3 view(0.0f, 0.0f, 1.0f);
    const auto delta = item::BrushItem::DustParticlesDelta::fromDirection(view, Direction::East);
    EXPECT_DOUBLE_EQ(delta.xd, 0.1);
    EXPECT_DOUBLE_EQ(delta.yd, 0.0);
    EXPECT_DOUBLE_EQ(delta.zd, 1.0);
}

// ============================================================================
// onUseTick 刷扫触发时机测试
// ============================================================================

/**
 * @brief 验证刷扫触发时机计算
 *
 * MC原版逻辑：elapsedTicks 从1开始，
 * 当 (elapsedTicks % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1) 时触发刷扫。
 * 即在 elapsedTicks = 5, 15, 25, 35, ... 时触发。
 */
TEST_F(BrushItemTest, OnUseTick_BrushTriggerTiming)
{
    constexpr i32 ANIMATION_DURATION = item::BrushItem::ANIMATION_DURATION;   // 10
    constexpr i32 BRUSH_TICK_IN_CYCLE = item::BrushItem::BRUSH_TICK_IN_CYCLE; // 4

    // 应该触发刷扫的 elapsedTicks 值
    // (elapsedTicks % 10 == 4 + 1) => (elapsedTicks % 10 == 5)
    // 即: 5, 15, 25, 35, 45, 55, 65, 75, 85, 95, 105, ...
    for (i32 tick = 1; tick <= 200; ++tick) {
        const bool shouldBrush = (tick % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1);
        if (tick % 10 == 5) {
            EXPECT_TRUE(shouldBrush) << "Tick " << tick << " should trigger brush";
        } else {
            EXPECT_FALSE(shouldBrush) << "Tick " << tick << " should NOT trigger brush";
        }
    }
}

/**
 * @brief 验证刷扫触发tick上命中普通方块时生成 Block 粒子并播放 BRUSH_GENERIC 音效
 *
 * 玩家直视下方的 STONE 方块，在第5个tick（elapsedTicks=5）应：
 * 1. 生成 Block 粒子（数量7~11，由 spawnDustParticles 内部随机决定）
 * 2. 播放 BRUSH_GENERIC 音效（因为 STONE 不是 BrushableBlock）
 */
TEST_F(BrushItemTest, OnUseTick_HitStoneGeneratesBlockParticlesAndGenericSound)
{
    TestPlayer player(m_world.get());
    setupPlayerLookingDownAtBlock(player, &VanillaBlocks::STONE->defaultState(), BlockPos(0, 4, 0));

    // 设置玩家手持刷子
    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();
    m_world->clearSoundCalls();

    // 模拟5个tick: elapsedTicks 1,2,3,4,5
    // 第5个tick（elapsedTicks=5）应触发刷扫：生成 Block 粒子 + BRUSH_GENERIC 音效
    for (i32 i = 0; i < 5; ++i) {
        player.updateActiveItem();
    }

    // 验证粒子：Block 类型，数量在 [7, 11] 范围内（对齐 MC random.nextInt(7, 12)）
    EXPECT_GE(m_world->blockParticleCount(), 7) << "Should generate at least 7 block particles at tick 5";
    EXPECT_LE(m_world->blockParticleCount(), 11) << "Should generate at most 11 block particles at tick 5";
    EXPECT_EQ(m_world->genericParticleCount(), 0) << "Should not call generic addParticle";
    for (const auto& call : m_world->blockParticles()) {
        EXPECT_EQ(call.typeId, particle::ParticleTypeId::Block);
        EXPECT_EQ(call.blockState, &VanillaBlocks::STONE->defaultState());
    }

    // 验证音效：BRUSH_GENERIC
    ASSERT_EQ(m_world->soundCount(), 1) << "Should play one sound at tick 5";
    EXPECT_EQ(m_world->soundCalls()[0].soundId, SoundEvents::BRUSH_GENERIC);
    EXPECT_EQ(m_world->soundCalls()[0].category, sound::SoundCategory::Blocks);
    EXPECT_FLOAT_EQ(m_world->soundCalls()[0].volume, 1.0f);
    EXPECT_FLOAT_EQ(m_world->soundCalls()[0].pitch, 1.0f);
}

/**
 * @brief 验证刷扫触发tick上命中可疑沙时使用 BRUSH_SAND 音效
 *
 * 可疑沙是 BrushableBlock，应使用其绑定的 BRUSH_SAND 音效而非 BRUSH_GENERIC。
 */
TEST_F(BrushItemTest, OnUseTick_HitSuspiciousSandUsesBrushSandSound)
{
    ASSERT_NE(block_registry::TrailsBlocks::SUSPICIOUS_SAND, nullptr) << "SUSPICIOUS_SAND should be registered";

    TestPlayer player(m_world.get());
    setupPlayerLookingDownAtBlock(
        player, &block_registry::TrailsBlocks::SUSPICIOUS_SAND->defaultState(), BlockPos(0, 4, 0));

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();
    m_world->clearSoundCalls();

    // 第5个tick触发刷扫
    for (i32 i = 0; i < 5; ++i) {
        player.updateActiveItem();
    }

    // 验证粒子：Block 类型，数量在 [7, 11] 范围内
    EXPECT_GE(m_world->blockParticleCount(), 7);
    EXPECT_LE(m_world->blockParticleCount(), 11);

    // 验证音效：BRUSH_SAND（可疑沙专属）
    ASSERT_EQ(m_world->soundCount(), 1);
    EXPECT_EQ(m_world->soundCalls()[0].soundId, SoundEvents::BRUSH_SAND);
    EXPECT_EQ(m_world->soundCalls()[0].category, sound::SoundCategory::Blocks);
}

/**
 * @brief 验证刷扫触发tick上命中可疑沙砾时使用 BRUSH_GRAVEL 音效
 */
TEST_F(BrushItemTest, OnUseTick_HitSuspiciousGravelUsesBrushGravelSound)
{
    ASSERT_NE(block_registry::TrailsBlocks::SUSPICIOUS_GRAVEL, nullptr) << "SUSPICIOUS_GRAVEL should be registered";

    TestPlayer player(m_world.get());
    setupPlayerLookingDownAtBlock(
        player, &block_registry::TrailsBlocks::SUSPICIOUS_GRAVEL->defaultState(), BlockPos(0, 4, 0));

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();
    m_world->clearSoundCalls();

    for (i32 i = 0; i < 5; ++i) {
        player.updateActiveItem();
    }

    EXPECT_GE(m_world->blockParticleCount(), 7);
    EXPECT_LE(m_world->blockParticleCount(), 11);
    ASSERT_EQ(m_world->soundCount(), 1);
    EXPECT_EQ(m_world->soundCalls()[0].soundId, SoundEvents::BRUSH_GRAVEL);
}

/**
 * @brief 验证非刷扫tick不生成粒子也不播放音效
 *
 * elapsedTicks = 1,2,3,4 不应触发任何粒子或音效。
 */
TEST_F(BrushItemTest, OnUseTick_NoParticlesOrSoundsOnNonBrushTick)
{
    TestPlayer player(m_world.get());
    setupPlayerLookingDownAtBlock(player, &VanillaBlocks::STONE->defaultState(), BlockPos(0, 4, 0));

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();
    m_world->clearSoundCalls();

    // 前4个tick不应生成粒子或音效（elapsedTicks = 1,2,3,4）
    for (i32 i = 0; i < 4; ++i) {
        player.updateActiveItem();
    }

    EXPECT_EQ(m_world->blockParticleCount(), 0) << "Should not generate particles on non-brush ticks";
    EXPECT_EQ(m_world->genericParticleCount(), 0);
    EXPECT_EQ(m_world->soundCount(), 0) << "Should not play sounds on non-brush ticks";
}

/**
 * @brief 验证多个刷扫周期
 *
 * 在前25个tick中，应该在 elapsedTicks=5, 15, 25 触发3次刷扫。
 * 每次刷扫应生成 7~11 个 Block 粒子和1次 playSound 调用。
 * 总粒子数应在 [21, 33] 范围内（3 × [7, 11]）。
 */
TEST_F(BrushItemTest, OnUseTick_MultipleBrushCycles)
{
    TestPlayer player(m_world.get());
    setupPlayerLookingDownAtBlock(player, &VanillaBlocks::STONE->defaultState(), BlockPos(0, 4, 0));

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();
    m_world->clearSoundCalls();

    // tick 25次，elapsedTicks 1-25
    // 刷扫触发在 5, 15, 25
    for (i32 i = 0; i < 25; ++i) {
        player.updateActiveItem();
    }

    // 3次刷扫 × 每次 [7, 11] 个粒子 = [21, 33] 个粒子
    EXPECT_GE(m_world->blockParticleCount(), 21) << "Should generate at least 21 particles in 3 brush cycles";
    EXPECT_LE(m_world->blockParticleCount(), 33) << "Should generate at most 33 particles in 3 brush cycles";
    EXPECT_EQ(m_world->soundCount(), 3) << "Should play 3 sounds in 25 ticks";
}

// ============================================================================
// onUseTick 视线未对准方块测试
// ============================================================================

/**
 * @brief 验证刷扫触发tick上视线未对准方块时停止使用
 *
 * 对齐 MC 1.21.11 BrushItem.onUseTick：
 * 当 shouldBrush 为 true 但 calculateHitResult 未命中方块时，
 * 调用 stopActiveHand() 取消使用。
 */
TEST_F(BrushItemTest, OnUseTick_RaycastMissStopsActiveHand)
{
    TestPlayer player(m_world.get());
    // 玩家直视下方，但下方没有任何方块 → 射线将 miss
    player.setPosition(0.5f, 5.0f, 0.5f);
    player.setRotation(0.0f, 90.0f);

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();
    m_world->clearSoundCalls();

    // 第5个tick应触发刷扫，但射线未命中 → stopActiveHand
    for (i32 i = 0; i < 5; ++i) {
        player.updateActiveItem();
    }

    EXPECT_FALSE(player.isUsingItem()) << "Should stop using brush when raycast misses on brush tick";
    EXPECT_EQ(m_world->blockParticleCount(), 0) << "Should not generate particles on raycast miss";
    EXPECT_EQ(m_world->soundCount(), 0) << "Should not play sounds on raycast miss";
}

/**
 * @brief 验证刷扫触发tick上视线超出方块交互距离时停止使用
 *
 * 玩家在 y=20 高处直视下方，下方 y=4 的方块距离 16 格，
 * 超出 Player::blockInteractionRange()（生存模式 4.5），射线应 miss。
 */
TEST_F(BrushItemTest, OnUseTick_RaycastOutOfRangeStopsActiveHand)
{
    TestPlayer player(m_world.get());
    // 玩家在 y=20，眼睛在 y=21.62，直视下方
    player.setPosition(0.5f, 20.0f, 0.5f);
    player.setRotation(0.0f, 90.0f);

    // 在 (0, 4, 0) 放置方块，距离眼睛 21.62 - 5.0 = 16.62 格
    // 超出 Player::blockInteractionRange()（生存模式 4.5）→ 射线 miss
    m_world->setBlockStateAt(0, 4, 0, &VanillaBlocks::STONE->defaultState());

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();
    m_world->clearSoundCalls();

    for (i32 i = 0; i < 5; ++i) {
        player.updateActiveItem();
    }

    EXPECT_FALSE(player.isUsingItem()) << "Should stop using when block is out of range";
    EXPECT_EQ(m_world->blockParticleCount(), 0);
    EXPECT_EQ(m_world->soundCount(), 0);
}

// ============================================================================
// onUseTick 非玩家实体停止使用测试
// ============================================================================

/**
 * @brief 验证非玩家实体使用刷子时被停止
 *
 * BrushItem::onUseTick 检查实体是否为玩家，
 * 非玩家实体调用 stopActiveHand()。
 */
TEST_F(BrushItemTest, OnUseTick_NonPlayerEntityStopsUsing)
{
    // 创建一个非玩家 LivingEntity
    TestLivingEntity entity;
    entity.setWorld(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    entity.setEquipment(EquipmentSlot::MainHand, brushStack);
    entity.setActiveHand(Hand::MainHand);

    // 确认已开始使用
    ASSERT_TRUE(entity.isUsingItem());

    // 模拟一个tick - onUseTick 会检测到非玩家并调用 stopActiveHand
    entity.updateActiveItem();

    // 非玩家应该已被停止使用
    EXPECT_FALSE(entity.isUsingItem()) << "Non-player entity should stop using brush after onUseTick";
}

// ============================================================================
// onUseTick 中 stopActiveHand 后不触发 onItemUseFinish 测试
// ============================================================================

/**
 * @brief 验证 onUseTick 内部调用 stopActiveHand 后不会触发 onItemUseFinish
 *
 * 当 onUseTick 调用 stopActiveHand()（如非玩家实体使用刷子），
 * updateActiveItem 不应在后续逻辑中调用 onItemUseFinish。
 * 这验证了 LivingEntity::updateActiveItem 中的 isUsingItem() 检查。
 */
TEST_F(BrushItemTest, OnUseTick_StopActiveHandPreventsFinish)
{
    TestLivingEntity entity;
    entity.setWorld(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    entity.setEquipment(EquipmentSlot::MainHand, brushStack);
    entity.setActiveHand(Hand::MainHand);

    ASSERT_TRUE(entity.isUsingItem());
    EXPECT_EQ(entity.getItemInUseCount(), item::BrushItem::USE_DURATION);

    // 单次tick后，非玩家实体应被停止使用
    entity.updateActiveItem();

    // 验证使用状态已完全重置（stopActiveHand 被调用）
    EXPECT_FALSE(entity.isUsingItem());
    EXPECT_EQ(entity.getItemInUseCount(), 0);
    EXPECT_TRUE(entity.getActiveItem().isEmpty());
}

// ============================================================================
// itemInteractionForEntity 测试
// ============================================================================

TEST_F(BrushItemTest, ItemInteractionForEntity_ReturnsFalse)
{
    // 当前 itemInteractionForEntity 返回 false（犰狳交互尚未实现）
    auto* brushItem = dynamic_cast<item::BrushItem*>(Items::BRUSH);
    ASSERT_NE(brushItem, nullptr);

    TestPlayer player(m_world.get());

    TestLivingEntity target;
    target.setWorld(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    EXPECT_FALSE(brushItem->itemInteractionForEntity(brushStack, player, target, Hand::MainHand));
}

// ============================================================================
// UseAction::Brush 枚举值测试
// ============================================================================

TEST_F(BrushItemTest, UseActionBrushValue)
{
    EXPECT_EQ(static_cast<u8>(UseAction::Brush), 9);
    EXPECT_STREQ(toString(UseAction::Brush), "brush");
}

// ============================================================================
// 默认 Item::onUseTick 测试
// ============================================================================

/**
 * @brief 验证默认 Item::onUseTick 不做任何事情
 *
 * 基类 Item 的 onUseTick 默认实现是空操作，
 * 不应抛出异常或改变任何状态。
 */
TEST_F(BrushItemTest, DefaultOnUseTickIsNoOp)
{
    // 使用一个没有重写 onUseTick 的普通物品
    ItemStack stickStack(Items::STICK, 1);
    Item* stickItem = Items::STICK;
    ASSERT_NE(stickItem, nullptr);

    // 创建一个测试实体
    TestLivingEntity entity;
    entity.setWorld(m_world.get());

    // 默认 onUseTick 不应抛出异常
    EXPECT_NO_THROW(stickItem->onUseTick(stickStack, *m_world, entity, 1));
    EXPECT_NO_THROW(stickItem->onUseTick(stickStack, *m_world, entity, 100));
}

// ============================================================================
// 耐久度消耗估算测试
// ============================================================================

/**
 * @brief 验证刷子耐久度消耗与使用次数的关系
 *
 * 刷子耐久度64，每次刷扫消耗1耐久（BrushableBlock场景），
 * 使用时长200 ticks，每10 ticks触发一次刷扫（第5 tick），
 * 因此一次完整使用最多触发 floor(200/10) = 20 次刷扫。
 *
 * 刷子可完整使用 floor(64/1) = 64 次刷扫（在BrushableBlock场景下），
 * 但每次完整使用最多触发20次刷扫，所以完全消耗耐久需要
 * ceil(64/20) = 4 次完整使用（64耐久 = 3*20 + 4）。
 *
 * 注意：当前 BrushableBlockEntity.brush() 尚未实现，刷扫不消耗耐久。
 * 待 BrushableBlockEntity 实现后，本测试仍应成立。
 */
TEST_F(BrushItemTest, DurabilityAndBrushCountEstimate)
{
    constexpr i32 maxDamage = item::BrushItem::MAX_DURABILITY;             // 64
    constexpr i32 useDuration = item::BrushItem::USE_DURATION;             // 200
    constexpr i32 animationDuration = item::BrushItem::ANIMATION_DURATION; // 10

    // 每次完整使用最多触发多少次刷扫
    const i32 brushCountPerUse = useDuration / animationDuration; // 200 / 10 = 20
    EXPECT_EQ(brushCountPerUse, 20);

    // 总耐久度支持多少次刷扫（每次消耗1耐久）
    const i32 totalBrushCount = maxDamage; // 64
    EXPECT_EQ(totalBrushCount, 64);
}

// ============================================================================
// BrushableBlock 音效绑定测试
// ============================================================================

/**
 * @brief 验证可疑沙的刷扫音效绑定
 */
TEST_F(BrushItemTest, SuspiciousSandHasCorrectBrushSound)
{
    ASSERT_NE(block_registry::TrailsBlocks::SUSPICIOUS_SAND, nullptr);
    const auto* block = dynamic_cast<const blocks::BrushableBlock*>(block_registry::TrailsBlocks::SUSPICIOUS_SAND);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getBrushSound(), SoundEvents::BRUSH_SAND);
    EXPECT_EQ(block->getBrushCompletedSound(), SoundEvents::BRUSH_SAND_COMPLETED);
}

/**
 * @brief 验证可疑沙砾的刷扫音效绑定
 */
TEST_F(BrushItemTest, SuspiciousGravelHasCorrectBrushSound)
{
    ASSERT_NE(block_registry::TrailsBlocks::SUSPICIOUS_GRAVEL, nullptr);
    const auto* block = dynamic_cast<const blocks::BrushableBlock*>(block_registry::TrailsBlocks::SUSPICIOUS_GRAVEL);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getBrushSound(), SoundEvents::BRUSH_GRAVEL);
    EXPECT_EQ(block->getBrushCompletedSound(), SoundEvents::BRUSH_GRAVEL_COMPLETED);
}

} // namespace
} // namespace mc
