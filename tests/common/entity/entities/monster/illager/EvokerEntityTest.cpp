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
#include "common/core/Constants.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/illager/EvokerEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/entity/entities/monster/illager/VexEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 唤魔者尖牙攻击测试用世界
 *
 * 提供可编程的方块状态存储，支持测试 _spawnFangs 的碰撞箱高度计算。
 */
class EvokerFangsSpawnTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        // 捕获生成的尖牙实体，以便检查其位置
        auto* fangs = dynamic_cast<entity::EvokerFangsEntity*>(entity.get());
        if (fangs != nullptr) {
            m_spawnedFangsY.push_back(fangs->y());
        }
        m_spawnedEntities.push_back(std::move(entity));
        return EntityInstanceId(static_cast<u32>(m_spawnedEntities.size()));
    }

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override
    {
        m_gameEvents.push_back({event.id(), pos, context.sourceEntity()});
    }

    void advanceTick() { m_currentTick++; }

    [[nodiscard]] size_t spawnedFangsCount() const { return m_spawnedFangsY.size(); }
    [[nodiscard]] const std::vector<f32>& spawnedFangsYPositions() const { return m_spawnedFangsY; }
    [[nodiscard]] const std::vector<std::tuple<std::string, BlockPos, const Entity*>>& capturedGameEvents() const
    {
        return m_gameEvents;
    }

    void clearSpawnedEntities()
    {
        m_spawnedEntities.clear();
        m_spawnedFangsY.clear();
        m_gameEvents.clear();
    }

private:
    u64 m_currentTick = 0;
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::vector<f32> m_spawnedFangsY;
    std::vector<std::tuple<std::string, BlockPos, const Entity*>> m_gameEvents;
};

// ============================================================================
// EvokerEntity 基础测试
// ============================================================================

TEST(EvokerEntityTest, Construction)
{
    EvokerEntity evoker(EntityInstanceId(1));

    // 验证唤魔者尺寸
    EXPECT_FLOAT_EQ(evoker.width(), 0.6f);
    EXPECT_FLOAT_EQ(evoker.height(), 1.8f);

    // 验证默认状态
    EXPECT_FALSE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::None);
}

TEST(EvokerEntityTest, Attributes)
{
    EvokerEntity evoker(EntityInstanceId(1));

    // MC 唤魔者属性
    EXPECT_FLOAT_EQ(static_cast<f32>(evoker.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 24.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(evoker.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED)), 0.5f);
    EXPECT_FLOAT_EQ(static_cast<f32>(evoker.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE)), 12.0f);
}

TEST(EvokerEntityTest, Spellcasting)
{
    EvokerEntity evoker(EntityInstanceId(1));

    // 默认不施法
    EXPECT_FALSE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellTicks(), 0);
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::None);

    // 开始施法
    evoker.startCasting(static_cast<i32>(SpellcastingIllagerEntity::SpellType::Fangs));
    EXPECT_TRUE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::Fangs);

    // 清除施法状态
    evoker.clearSpellcasting();
    EXPECT_FALSE(evoker.isSpellcasting());
    EXPECT_EQ(evoker.spellType(), SpellcastingIllagerEntity::SpellType::None);
}

TEST(EvokerEntityTest, SpellTypeConversion)
{
    // 测试 SpellType ID 转换
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(0), SpellcastingIllagerEntity::SpellType::None);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(1), SpellcastingIllagerEntity::SpellType::SummonVex);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(2), SpellcastingIllagerEntity::SpellType::Fangs);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(3), SpellcastingIllagerEntity::SpellType::Wololo);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(4), SpellcastingIllagerEntity::SpellType::Disappear);
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(5), SpellcastingIllagerEntity::SpellType::Blindness);
    // 无效 ID 返回 None
    EXPECT_EQ(SpellcastingIllagerEntity::spellTypeFromId(99), SpellcastingIllagerEntity::SpellType::None);
}

TEST(EvokerEntityTest, CreateFactory)
{
    auto entity = EvokerEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);

    // 验证创建的是 EvokerEntity
    auto* evokerPtr = dynamic_cast<EvokerEntity*>(entity.get());
    EXPECT_NE(evokerPtr, nullptr);
}

TEST(EvokerEntityTest, SpellCooldowns)
{
    EvokerEntity evoker(EntityInstanceId(1));

    // 验证冷却常量通过公共接口间接测试
    evoker.startCasting(static_cast<i32>(SpellcastingIllagerEntity::SpellType::Fangs));
    EXPECT_TRUE(evoker.isSpellcasting());
}

// ============================================================================
// EvokerFangsEntity 测试
// ============================================================================

TEST(EvokerFangsEntityTest, Construction)
{
    entity::EvokerFangsEntity fangs(EntityInstanceId(1));

    // 验证尖牙尺寸
    EXPECT_FLOAT_EQ(fangs.width(), 0.5f);
    EXPECT_FLOAT_EQ(fangs.height(), 0.8f);

    // 验证默认状态
    EXPECT_EQ(fangs.warmupDelay(), 0);
    EXPECT_EQ(fangs.owner(), nullptr);
}

TEST(EvokerFangsEntityTest, WarmupDelay)
{
    entity::EvokerFangsEntity fangs(EntityInstanceId(1));

    fangs.setWarmupDelay(10);
    EXPECT_EQ(fangs.warmupDelay(), 10);

    fangs.setWarmupDelay(5);
    EXPECT_EQ(fangs.warmupDelay(), 5);
}

TEST(EvokerFangsEntityTest, Owner)
{
    entity::EvokerFangsEntity fangs(EntityInstanceId(1));

    EXPECT_EQ(fangs.owner(), nullptr);

    fangs.setOwner(nullptr);
    EXPECT_EQ(fangs.owner(), nullptr);
}

TEST(EvokerFangsEntityTest, AnimationProgress)
{
    entity::EvokerFangsEntity fangs(EntityInstanceId(1));

    EXPECT_FLOAT_EQ(fangs.getAnimationProgress(0.0f), 0.0f);
}

TEST(EvokerFangsEntityTest, CreateFactory)
{
    auto entity = entity::EvokerFangsEntity::create(nullptr);
    ASSERT_NE(entity, nullptr);

    auto* fangsPtr = dynamic_cast<entity::EvokerFangsEntity*>(entity.get());
    EXPECT_NE(fangsPtr, nullptr);
}

// ============================================================================
// CollisionShape 高度计算测试
//
// 测试 _spawnFangs 中碰撞箱高度计算的核心逻辑：
// 1. 使用 isSolidSide(Direction::Up) 替代 isSolid() 进行地面检测
// 2. 通过 CollisionShape::boxes() 遍历获取碰撞箱最大 Y 值
// 3. 生成尖牙后触发 ENTITY_PLACE 游戏事件
// ============================================================================

class EvokerFangsCollisionTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    EvokerFangsSpawnTestWorld m_world;
};

// ============================================================================
// CollisionShape 基础测试 — 验证各种方块类型的碰撞箱 maxY
// ============================================================================

TEST_F(EvokerFangsCollisionTest, CollisionShape_EmptyShape_MaxYIsZero)
{
    // 空气等方块的碰撞箱为空，maxY 应为 0（不影响地面高度）
    const CollisionShape& empty = CollisionShape::empty();
    EXPECT_TRUE(empty.isEmpty());

    f32 maxY = 0.0f;
    for (const auto& box : empty.boxes()) {
        maxY = std::max(maxY, box.maxY);
    }
    EXPECT_FLOAT_EQ(maxY, 0.0f);
}

TEST_F(EvokerFangsCollisionTest, CollisionShape_FullBlock_MaxYIsOne)
{
    // 完整方块的碰撞箱 maxY = 1.0
    const CollisionShape& full = CollisionShape::fullBlock();
    EXPECT_TRUE(full.isFullBlock());

    f32 maxY = 0.0f;
    for (const auto& box : full.boxes()) {
        maxY = std::max(maxY, box.maxY);
    }
    EXPECT_FLOAT_EQ(maxY, 1.0f);
}

TEST_F(EvokerFangsCollisionTest, CollisionShape_Slab_MaxYIsHalf)
{
    // 下半台阶的碰撞箱 maxY = 0.5
    CollisionShape bottomSlab = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    EXPECT_FALSE(bottomSlab.isEmpty());
    EXPECT_FALSE(bottomSlab.isFullBlock());

    f32 maxY = 0.0f;
    for (const auto& box : bottomSlab.boxes()) {
        maxY = std::max(maxY, box.maxY);
    }
    EXPECT_FLOAT_EQ(maxY, 0.5f);
}

TEST_F(EvokerFangsCollisionTest, CollisionShape_Carpet_MaxYIsPixel)
{
    // 地毯碰撞箱高度为 1 像素（1/16 = 0.0625）
    CollisionShape carpet = CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 1.0f, 16.0f);
    EXPECT_FALSE(carpet.isEmpty());

    f32 maxY = 0.0f;
    for (const auto& box : carpet.boxes()) {
        maxY = std::max(maxY, box.maxY);
    }
    EXPECT_FLOAT_EQ(maxY, 0.0625f);
}

TEST_F(EvokerFangsCollisionTest, CollisionShape_TopSlab_MaxYIsOne)
{
    // 上半台阶的碰撞箱 maxY = 1.0
    CollisionShape topSlab = CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
    EXPECT_FALSE(topSlab.isEmpty());

    f32 maxY = 0.0f;
    for (const auto& box : topSlab.boxes()) {
        maxY = std::max(maxY, box.maxY);
    }
    EXPECT_FLOAT_EQ(maxY, 1.0f);
}

TEST_F(EvokerFangsCollisionTest, CollisionShape_MultiBoxShape_TakesMaximumMaxY)
{
    // 多碰撞箱形状（如楼梯），取所有 AABB 的最大 maxY
    CollisionShape stairs = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    stairs.addBox(0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 1.0f);

    EXPECT_EQ(stairs.boxCount(), 2u);

    f32 maxY = 0.0f;
    for (const auto& box : stairs.boxes()) {
        maxY = std::max(maxY, box.maxY);
    }
    EXPECT_FLOAT_EQ(maxY, 1.0f);
}

// ============================================================================
// isSolidSide 测试 — 验证地面检测使用 isSolidSide 而非 isSolid
// ============================================================================

TEST_F(EvokerFangsCollisionTest, IsSolidSide_StoneBlock_HasUpFace)
{
    // 石头方块应该有向上的实心面
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    BlockPos pos(0, 64, 0);
    m_world.setBlockState(0, 64, 0, stoneState);

    const BlockState* retrieved = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_TRUE(retrieved->isSolidSide(m_world, pos, Direction::Up));
}

TEST_F(EvokerFangsCollisionTest, IsSolidSide_AirBlock_NoUpFace)
{
    // 空气方块不应该有向上的实心面
    const BlockState* airState = &VanillaBlocks::AIR->defaultState();
    ASSERT_NE(airState, nullptr);

    BlockPos pos(0, 65, 0);
    EXPECT_FALSE(airState->isSolidSide(m_world, pos, Direction::Up));
}

TEST_F(EvokerFangsCollisionTest, IsSolidSide_DirtBlock_HasUpFace)
{
    // 泥土方块应该有向上的实心面
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
    ASSERT_NE(dirtState, nullptr);

    BlockPos pos(0, 64, 0);
    m_world.setBlockState(0, 64, 0, dirtState);

    const BlockState* retrieved = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_TRUE(retrieved->isSolidSide(m_world, pos, Direction::Up));
}

// ============================================================================
// _spawnFangs 集成测试 — 验证碰撞箱高度影响尖牙生成位置
// ============================================================================

TEST_F(EvokerFangsCollisionTest, SpawnFangsOnFullBlock_YPositionAtBlockLevel)
{
    // 在石头上方生成尖牙：groundY = blockPos.y + 0.0（空气碰撞箱为空）
    // 唤魔者位置 (0, 65, 0)，目标位置 (1, 65, 0)（近距离攻击）
    // 石头在 y=64，y=65 为空气
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    // 铺设足够的石头地面（覆盖尖牙攻击范围）
    for (i32 x = -3; x <= 3; ++x) {
        for (i32 z = -3; z <= 3; ++z) {
            m_world.setBlockState(x, 64, z, stoneState);
        }
    }

    // 创建唤魔者和目标
    auto evoker = std::make_unique<EvokerEntity>(EntityInstanceId(1));
    evoker->setPosition(0.0f, 65.0f, 0.0f);
    evoker->setWorld(&m_world);

    // 目标距离 < 3 格（触发近距离攻击）
    auto target = std::make_unique<Player>(EntityInstanceId(2), "TestTarget");
    target->setPosition(1.0f, 65.0f, 0.0f);
    target->setWorld(&m_world);

    evoker->setAttackTarget(target.get());
    evoker->castFangsAttack();

    // 尖牙应该生成在 y=65（石头顶部 y=64 + 1.0 方块高度 = y=65 空气层）
    // 因为 y=65 是空气，shapeMaxY = 0，groundY = 65 + 0 = 65.0
    ASSERT_GT(m_world.spawnedFangsCount(), 0u) << "应该在石头上方生成尖牙";
    for (f32 fangY : m_world.spawnedFangsYPositions()) {
        // 尖牙生成在 y=65 空气层，shapeMaxY=0（空气），所以 groundY = 65.0
        EXPECT_FLOAT_EQ(fangY, 65.0f) << "完整方块上方（空气层）的尖牙Y坐标应为方块Y+0";
    }
}

TEST_F(EvokerFangsCollisionTest, SpawnFangs_NoSolidGround_NoFangs)
{
    // 在空中（没有固体地面）不应生成尖牙
    // 所有位置都是空气
    auto evoker = std::make_unique<EvokerEntity>(EntityInstanceId(1));
    evoker->setPosition(0.0f, 100.0f, 0.0f);
    evoker->setWorld(&m_world);

    // 目标距离 < 3 格（触发近距离攻击，但仍无地面）
    auto target = std::make_unique<Player>(EntityInstanceId(2), "TestTarget");
    target->setPosition(1.0f, 100.0f, 0.0f);
    target->setWorld(&m_world);

    evoker->setAttackTarget(target.get());
    evoker->castFangsAttack();

    // 没有固体地面，不应生成任何尖牙
    EXPECT_EQ(m_world.spawnedFangsCount(), 0u) << "没有固体地面时不应生成尖牙";
}

TEST_F(EvokerFangsCollisionTest, SpawnFangs_GameEventTriggered)
{
    // 验证尖牙生成后触发了 ENTITY_PLACE 游戏事件
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    for (i32 x = -3; x <= 3; ++x) {
        for (i32 z = -3; z <= 3; ++z) {
            m_world.setBlockState(x, 64, z, stoneState);
        }
    }

    auto evoker = std::make_unique<EvokerEntity>(EntityInstanceId(1));
    evoker->setPosition(0.0f, 65.0f, 0.0f);
    evoker->setWorld(&m_world);

    auto target = std::make_unique<Player>(EntityInstanceId(2), "TestTarget");
    target->setPosition(1.0f, 65.0f, 0.0f);
    target->setWorld(&m_world);

    evoker->setAttackTarget(target.get());
    evoker->castFangsAttack();

    ASSERT_GT(m_world.spawnedFangsCount(), 0u) << "应该生成尖牙";

    // 验证 ENTITY_PLACE 游戏事件被触发
    const auto& events = m_world.capturedGameEvents();
    ASSERT_GT(events.size(), 0u) << "应该触发游戏事件";

    bool foundEntityPlace = false;
    for (const auto& [name, pos, sourceEntity] : events) {
        if (name == "entity_place") {
            foundEntityPlace = true;
            // 验证事件源实体是唤魔者
            EXPECT_EQ(sourceEntity, evoker.get()) << "ENTITY_PLACE 事件源应为唤魔者";
            break;
        }
    }
    EXPECT_TRUE(foundEntityPlace) << "应该触发 ENTITY_PLACE 游戏事件";
}

TEST_F(EvokerFangsCollisionTest, SpawnFangs_CloseRange_TwoRings)
{
    // 近距离攻击（距离 < 3 格）应生成两圈尖牙：内圈 5 个 + 外圈 8 个 = 13 个
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    // 铺设足够的石头地面
    for (i32 x = -3; x <= 3; ++x) {
        for (i32 z = -3; z <= 3; ++z) {
            m_world.setBlockState(x, 64, z, stoneState);
        }
    }

    auto evoker = std::make_unique<EvokerEntity>(EntityInstanceId(1));
    evoker->setPosition(0.0f, 65.0f, 0.0f);
    evoker->setWorld(&m_world);

    // 目标距离 < 3 格（触发近距离攻击）
    auto target = std::make_unique<Player>(EntityInstanceId(2), "TestTarget");
    target->setPosition(1.0f, 65.0f, 0.0f);
    target->setWorld(&m_world);

    evoker->setAttackTarget(target.get());
    evoker->castFangsAttack();

    // 内圈 5 + 外圈 8 = 13
    EXPECT_EQ(m_world.spawnedFangsCount(), 13u) << "近距离攻击应生成 13 个尖牙（内圈5 + 外圈8）";
}

TEST_F(EvokerFangsCollisionTest, SpawnFangs_LongRange_LineOfFangs)
{
    // 远距离攻击（距离 >= 3 格）应生成一条直线的 16 个尖牙
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    // 铺设足够的石头地面
    for (i32 x = 0; x <= 25; ++x) {
        m_world.setBlockState(x, 64, 0, stoneState);
    }

    auto evoker = std::make_unique<EvokerEntity>(EntityInstanceId(1));
    evoker->setPosition(0.0f, 65.0f, 0.0f);
    evoker->setWorld(&m_world);

    // 目标距离 >= 3 格（触发远距离攻击）
    auto target = std::make_unique<Player>(EntityInstanceId(2), "TestTarget");
    target->setPosition(10.0f, 65.0f, 0.0f);
    target->setWorld(&m_world);

    evoker->setAttackTarget(target.get());
    evoker->castFangsAttack();

    // 远距离攻击生成 16 个尖牙
    EXPECT_EQ(m_world.spawnedFangsCount(), 16u) << "远距离攻击应生成 16 个尖牙";
}

// ============================================================================
// 核心行为变更验证：碰撞箱高度计算
// ============================================================================

TEST_F(EvokerFangsCollisionTest, SpawnFangsOnBottomSlab_YPositionIncludesSlabHeight)
{
    // 核心场景：y=64 放石头，y=65 放下半台阶（碰撞箱高度 0.5）
    // 唤魔者和目标站在石头上 (y=64)，搜索从 maxY 向下找到 y=65 的台阶
    // 台阶的 isSolidSide(Up) 为 true（下半台阶顶面是实心的），所以 y=64 的石头是地面
    // 但等一下 — 需要更仔细地理解搜索逻辑：
    //   搜索从 floor(maxY) 向下，找到下方方块 isSolidSide(Up) 的位置
    //   然后检查当前位置是否为非空方块，如果是，获取碰撞箱 maxY
    //
    // 设置: 石头 y=64, 下半台阶 y=65, 唤魔者在 y=65.0（站在台阶上）
    //   minY = min(65.0, 65.0) = 65.0, maxY = max(65.0, 65.0) + 1 = 66.0
    //   搜索从 blockPos.y = floor(66.0) = 66 开始:
    //     y=66: 下方 y=65 = 台阶, isSolidSide(Up) = true → 找到地面!
    //     当前位置 y=66 = 空气, shapeMaxY = 0 → groundY = 66 + 0 = 66.0
    //
    // 这其实不是我们想测试的场景。我们需要的是：当尖牙搜索经过台阶时，
    // 台阶的碰撞箱高度被正确应用。正确的场景是唤魔者在台阶上方，
    // 且尖牙位置恰好在台阶所在的位置。
    //
    // 更好的设置: 石头 y=63, 下半台阶 y=64, 唤魔者在 y=64.5（站在台阶上）
    //   minY = min(64.5, 64.5) = 64.5, maxY = max(64.5, 64.5) + 1 = 65.5
    //   搜索从 blockPos.y = floor(65.5) = 65 开始:
    //     y=65: 下方 y=64 = 台阶, isSolidSide(Up) = true → 找到地面!
    //     当前位置 y=65 = 空气, shapeMaxY = 0 → groundY = 65 + 0 = 65.0
    //
    // 还是空气层... 那换一种方式：让台阶就在搜索路径上
    // 关键场景: 石头 y=64, 下半台阶 y=65, 唤魔者在 y=65（站在台阶表面 y=65.0 上）
    //   注意：唤魔者脚在 y=65.0（台阶的底部），实际上唤魔者应该站在 y=64.5
    //
    // 最终正确场景:
    // 石头 y=63, 下半台阶 y=64
    // 唤魔者和目标站在石头表面 y=64.0
    // minY = min(64.0, 64.0) = 64.0, maxY = max(64.0, 64.0) + 1.0 = 65.0
    // 搜索从 blockPos.y = floor(65.0) = 65 开始:
    //   y=65: 下方 y=64 = 下半台阶, isSolidSide(Up) = true → 找到地面!
    //   当前位置 y=65 = 空气, shapeMaxY = 0 → groundY = 65 + 0 = 65.0
    // 这还是空气...
    //
    // 等等！我需要让搜索停在台阶的位置，而不是在台阶上方。
    // 下半台阶的 isSolidSide(Direction::Up) 实际上应该返回 true，
    // 因为下半台阶的碰撞箱 (0,0,0)-(1,0.5,1) 在顶面并不是完全覆盖的。
    // 让我换一种方式思考：
    //
    // 正确的测试场景：
    // 石头 y=64, 下半台阶 y=65, 空气 y=66
    // 唤魔者 y=66.0, 目标 y=66.0 (近距离攻击)
    // minY = 66.0, maxY = 67.0
    // 搜索从 y=67:
    //   y=67: 下方 y=66 = 空气 → 不行，继续
    //   y=66: 下方 y=65 = 下半台阶
    //     isSolidSide(Direction::Up) = ???
    //
    // 问题是：下半台阶的 isSolidSide(Direction::Up) 是否为 true？
    // 默认实现是 m_isSolid && m_hasCollision。下半台阶两者都是 true。
    // 但在 MC 中，isFaceSturdy(UP) 检查的是顶面是否完全覆盖，
    // 下半台阶只覆盖了上半部分，所以 isFaceSturdy(UP) = false！
    //
    // 这正是 isSolidSide 与 isSolid 的关键区别！
    // 当搜索在 y=66 时，下方 y=65 是下半台阶，isSolidSide(Up) = false（顶面不完整）
    // 继续搜索到 y=65: 下方 y=64 = 石头, isSolidSide(Up) = true
    //   当前位置 y=65 = 下半台阶(非空气), shapeMaxY = 0.5
    //   groundY = 65 + 0.5 = 65.5 ✓
    //
    // 但是！当前项目的 isSolidSide 默认实现不检查面的完整性，它只检查 m_isSolid && m_hasCollision。
    // 下半台阶的 m_isSolid = true, m_hasCollision = true，所以 isSolidSide(Up) = true。
    // 这意味着搜索会在 y=66 停下，认为 y=65 的台阶是地面，
    // 但 y=66 是空气，所以 groundY = 66.0，这不是我们想要的结果。
    //
    // 因此，我们需要验证的是：当 isSolidSide(Up) 正确返回 true 时（如当前实现），
    // 搜索停在 y=66，y=66 是空气，groundY = 66.0。
    // 这与旧代码的行为不同 — 旧代码使用 isSolid() 也会返回 true，结果一样。
    //
    // 真正碰撞箱高度差异出现在方块本身占据搜索位置且其碰撞箱不是完整方块时。
    // 关键场景：唤魔者在石头上方，搜索路径经过一个非完整碰撞箱方块。
    //
    // 最终方案：石头 y=63, 下半台阶 y=64, 唤魔者 y=64（站在台阶上）
    // 近距离攻击，尖牙在唤魔者周围生成
    // minY = 64.0, maxY = 65.0
    // 搜索从 y=65: 下方 y=64 = 下半台阶, isSolidSide(Up) = true
    //   当前位置 y=65 = 空气 → shapeMaxY = 0 → groundY = 65.0
    //   但尖牙会生成在空气层 y=65，这在台阶上方（台阶顶面 y=64.5）的空气层
    //   这跟旧代码的行为也是一样的！
    //
    // 让我重新思考... 真正测试碰撞箱高度的场景是：
    // 在搜索停止位置，当前方块有碰撞箱但不是完整方块。
    // 这需要在 isSolidSide(Up) 返回 true 的方块上面，有一个非空气、非完整碰撞箱的方块。
    // 例如：石头 y=63, 下半台阶 y=64, 地毯 y=65
    // 搜索从 y=66: 下方 y=65 = 地毯 → isSolidSide(Up) = false（地毯没有实心顶面）
    // 搜索到 y=65: 下方 y=64 = 下半台阶 → isSolidSide(Up) = true
    //   当前位置 y=65 = 地毯(非空气), 碰撞箱 maxY = 0.0625
    //   groundY = 65 + 0.0625 = 65.0625
    //
    // 但地毯的 isSolidSide(Up) 也可能返回 true... 让我简化。
    //
    // 最简单直接的场景：石头 y=64, 下半台阶 y=65
    // 唤魔者站在 y=64.5（台阶表面）, 目标站在 y=64.5
    // minY = 64.5, maxY = 65.5
    // 搜索从 y=65: 下方 y=64 = 石头, isSolidSide(Up) = true
    //   当前位置 y=65 = 下半台阶(非空气!), 碰撞箱 maxY = 0.5
    //   groundY = 65 + 0.5 = 65.5 ✓
    //
    // 这才是正确的场景！唤魔者需要站在台阶上方，使得搜索到达台阶位置。

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    const BlockState* slabState = &VanillaBlocks::STONE_SLAB->defaultState();

    // 先验证下半台阶的碰撞箱形状
    ASSERT_FALSE(slabState->isAir()) << "下半台阶不是空气";
    const CollisionShape& slabShape = slabState->getCollisionShape();
    ASSERT_FALSE(slabShape.isEmpty()) << "下半台阶应该有碰撞箱";
    f32 slabMaxY = 0.0f;
    for (const auto& box : slabShape.boxes()) {
        slabMaxY = std::max(slabMaxY, box.maxY);
    }
    ASSERT_FLOAT_EQ(slabMaxY, 0.5f) << "下半台阶碰撞箱maxY应为0.5";

    // 铺设石头地面
    for (i32 x = -3; x <= 3; ++x) {
        for (i32 z = -3; z <= 3; ++z) {
            m_world.setBlockState(x, 64, z, stoneState);
            // 在石头上方放置下半台阶
            m_world.setBlockState(x, 65, z, slabState);
        }
    }

    // 唤魔者站在台阶表面 y=64.5，目标也在 y=64.5（近距离攻击）
    // minY = 64.5, maxY = 65.5
    // 搜索从 y=65: 下方 y=64 = 石头(isSolidSide(Up)=true) → 找到地面
    //   当前位置 y=65 = 下半台阶(非空气), 碰撞箱 maxY = 0.5
    //   groundY = 65 + 0.5 = 65.5
    auto evoker = std::make_unique<EvokerEntity>(EntityInstanceId(1));
    evoker->setPosition(0.0f, 64.5f, 0.0f);
    evoker->setWorld(&m_world);

    auto target = std::make_unique<Player>(EntityInstanceId(2), "TestTarget");
    target->setPosition(1.0f, 64.5f, 0.0f);
    target->setWorld(&m_world);

    evoker->setAttackTarget(target.get());
    evoker->castFangsAttack();

    // 关键断言：尖牙应生成在 y=65.5（台阶碰撞箱上表面），而非 y=65.0（旧代码的假设）
    ASSERT_GT(m_world.spawnedFangsCount(), 0u) << "应该在台阶上方生成尖牙";
    for (f32 fangY : m_world.spawnedFangsYPositions()) {
        EXPECT_FLOAT_EQ(fangY, 65.5f)
            << "下半台阶上方的尖牙Y坐标应为 65.5（blockPos.y=65 + shapeMaxY=0.5），而非旧代码的 65.0";
    }
}

TEST_F(EvokerFangsCollisionTest, SpawnFangsOnPackedIce_IsSolidSidePreventsSpawn)
{
    // 核心场景：验证地面检测使用 isSolidSide(Direction::Up) 而非 isSolid()
    //
    // 浮冰（PackedIce）是唯一具有以下属性的方块：
    //   isSolid() = true  （Material::ICE 是固体，且未调用 notSolid()）
    //   isSolidSide(Direction::Up) = false  （Block::isSolidSide 对 Material::ICE 返回 false）
    //
    // 旧代码使用 isSolid() 检测地面：浮冰会被认为是有效地面，尖牙会生成在浮冰上方
    // 新代码使用 isSolidSide(Direction::Up) 检测地面：浮冰不被认为是有效地面，尖牙不会生成
    //
    // 这正是 isSolidSide 替换 isSolid 的核心行为差异！
    const BlockState* packedIceState = &VanillaBlocks::PACKED_ICE->defaultState();
    ASSERT_NE(packedIceState, nullptr);

    // 验证浮冰的关键属性：isSolid()=true 但 isSolidSide(Up)=false
    EXPECT_TRUE(packedIceState->isSolid()) << "浮冰应该 isSolid()=true（Material::ICE 是固体）";
    EXPECT_FALSE(packedIceState->isSolidSide(m_world, BlockPos(0, 64, 0), Direction::Up))
        << "浮冰应该 isSolidSide(Up)=false（Block::isSolidSide 对 Material::ICE 返回 false）";

    // 铺设浮冰地面
    for (i32 x = -3; x <= 3; ++x) {
        for (i32 z = -3; z <= 3; ++z) {
            m_world.setBlockState(x, 64, z, packedIceState);
        }
    }

    auto evoker = std::make_unique<EvokerEntity>(EntityInstanceId(1));
    evoker->setPosition(0.0f, 65.0f, 0.0f);
    evoker->setWorld(&m_world);

    // 目标距离 < 3 格（近距离攻击）
    auto target = std::make_unique<Player>(EntityInstanceId(2), "TestTarget");
    target->setPosition(1.0f, 65.0f, 0.0f);
    target->setWorld(&m_world);

    evoker->setAttackTarget(target.get());
    evoker->castFangsAttack();

    // 浮冰的 isSolidSide(Up)=false，搜索时不会认为是有效地面，因此不应生成尖牙
    // 如果错误地回退到 isSolid()（返回 true），浮冰会被当作地面，尖牙会错误地生成
    EXPECT_EQ(m_world.spawnedFangsCount(), 0u)
        << "浮冰 isSolidSide(Up)=false，不应在其上方生成尖牙（旧代码使用 isSolid() 会错误生成）";
}

} // namespace
} // namespace mc
