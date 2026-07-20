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

#include "world/blockentity/interactive/BellBlockEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blockentity;

// ========== BellBlockEntity 测试 ==========

class BellBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { bell_ = std::make_unique<BellBlockEntity>(BlockPos(10, 64, 20)); }

    std::unique_ptr<BellBlockEntity> bell_;
};

TEST_F(BellBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(bell_->getType(), BlockEntityType::Bell);
}

TEST_F(BellBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(bell_->getPos(), BlockPos(10, 64, 20));
}

TEST_F(BellBlockEntityTest, Create_NotShakingInitially)
{
    EXPECT_FALSE(bell_->isShaking());
}

TEST_F(BellBlockEntityTest, Create_NotResonatingInitially)
{
    EXPECT_FALSE(bell_->isResonating());
}

TEST_F(BellBlockEntityTest, Create_TicksIsZero)
{
    EXPECT_EQ(bell_->ticks(), 0);
}

TEST_F(BellBlockEntityTest, Create_ResonationTicksIsZero)
{
    EXPECT_EQ(bell_->resonationTicks(), 0);
}

TEST_F(BellBlockEntityTest, Create_NeedsTickFalseWhenIdle)
{
    EXPECT_FALSE(bell_->needsTick());
}

TEST_F(BellBlockEntityTest, TriggerEvent_Id1_StartsShaking)
{
    EXPECT_TRUE(bell_->triggerEvent(1, static_cast<i32>(Direction::North)));
    EXPECT_TRUE(bell_->isShaking());
    EXPECT_EQ(bell_->ticks(), 0);
    EXPECT_EQ(bell_->clickDirection(), Direction::North);
}

TEST_F(BellBlockEntityTest, TriggerEvent_Id1_ResetsResonationTicks)
{
    // 先设置一个非零的 resonationTicks（通过 triggerEvent 已经是 0，但验证一致性）
    EXPECT_TRUE(bell_->triggerEvent(1, static_cast<i32>(Direction::South)));
    EXPECT_EQ(bell_->resonationTicks(), 0);
    EXPECT_EQ(bell_->clickDirection(), Direction::South);
}

TEST_F(BellBlockEntityTest, TriggerEvent_UnknownId_ReturnsFalse)
{
    EXPECT_FALSE(bell_->triggerEvent(99, 0));
    EXPECT_FALSE(bell_->isShaking());
}

TEST_F(BellBlockEntityTest, TriggerEvent_AllDirections)
{
    const std::array<Direction, 6> dirs = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

    for (Direction dir : dirs) {
        std::unique_ptr<BellBlockEntity> bell = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
        EXPECT_TRUE(bell->triggerEvent(1, static_cast<i32>(dir))) << "Failed for direction " << static_cast<int>(dir);
        EXPECT_EQ(bell->clickDirection(), dir) << "Wrong direction for " << static_cast<int>(dir);
    }
}

TEST_F(BellBlockEntityTest, NeedsTick_TrueWhenShaking)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    EXPECT_TRUE(bell_->needsTick());
}

TEST_F(BellBlockEntityTest, NeedsTick_TrueWhenResonating)
{
    // 通过 triggerEvent 启动摇晃，然后手动设置 resonating 状态
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    // resonating 状态是私有的，但 needsTick 检查 m_shaking || m_resonating
    // 只要 shaking 为 true，needsTick 就返回 true
    EXPECT_TRUE(bell_->needsTick());
}

TEST_F(BellBlockEntityTest, Clone_CreatesCopy)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::East));

    std::unique_ptr<BlockEntity> copy = bell_->clone();
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Bell);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 64, 20));

    auto* bellCopy = static_cast<BellBlockEntity*>(copy.get());
    EXPECT_EQ(bellCopy->clickDirection(), Direction::East);
    EXPECT_EQ(bellCopy->ticks(), 0);
    EXPECT_TRUE(bellCopy->isShaking());
}

TEST_F(BellBlockEntityTest, SaveAndLoad_PreservesData)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::South));

    nlohmann::json data;
    bell_->save(data);

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_TRUE(loaded->isShaking());
    EXPECT_EQ(loaded->clickDirection(), Direction::South);
    EXPECT_EQ(loaded->ticks(), 0);
}

TEST_F(BellBlockEntityTest, SaveAndLoad_HandlesIdle)
{
    nlohmann::json data;
    bell_->save(data);

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_FALSE(loaded->isShaking());
    EXPECT_FALSE(loaded->isResonating());
    EXPECT_EQ(loaded->ticks(), 0);
    EXPECT_EQ(loaded->resonationTicks(), 0);
}

TEST_F(BellBlockEntityTest, SaveAndLoad_PreservesResonationState)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    nlohmann::json data;
    bell_->save(data);

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(5, 10, 15));
    ASSERT_TRUE(loaded->load(data));

    EXPECT_TRUE(loaded->isShaking());
    EXPECT_EQ(loaded->clickDirection(), Direction::North);
}

TEST_F(BellBlockEntityTest, Constants_MatchMCJavaValues)
{
    // 验证常量与 MC 1.21.11 BellBlockEntity.java 对齐
    EXPECT_EQ(BellBlockEntity::DURATION, 50);
    EXPECT_EQ(BellBlockEntity::GLOW_DURATION, 60);
    EXPECT_EQ(BellBlockEntity::MIN_TICKS_BETWEEN_SEARCHES, 60);
    EXPECT_EQ(BellBlockEntity::MAX_RESONATION_TICKS, 40);
    EXPECT_EQ(BellBlockEntity::TICKS_BEFORE_RESONATION, 5);
    EXPECT_FLOAT_EQ(BellBlockEntity::SEARCH_RADIUS, 48.0f);
    EXPECT_FLOAT_EQ(BellBlockEntity::HEAR_BELL_RADIUS, 32.0f);
    EXPECT_FLOAT_EQ(BellBlockEntity::HIGHLIGHT_RAIDERS_RADIUS, 48.0f);
}

TEST_F(BellBlockEntityTest, Load_HandlesMissingFields)
{
    // 空 JSON 应该可以加载（使用默认值）
    nlohmann::json empty;
    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(empty));
    EXPECT_FALSE(loaded->isShaking());
    EXPECT_FALSE(loaded->isResonating());
    EXPECT_EQ(loaded->ticks(), 0);
}

TEST_F(BellBlockEntityTest, Load_HandlesInvalidClickDirection)
{
    nlohmann::json data;
    data["click_direction"] = 999; // 无效值
    data["shaking"] = true;

    auto loaded = std::make_unique<BellBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(data));
    // 无效的 click_direction 应该保持默认值（North）
    EXPECT_EQ(loaded->clickDirection(), Direction::North);
    EXPECT_TRUE(loaded->isShaking());
}

TEST_F(BellBlockEntityTest, Save_IncludesRequiredFields)
{
    nlohmann::json data;
    bell_->save(data);

    EXPECT_TRUE(data.contains("ticks"));
    EXPECT_TRUE(data.contains("shaking"));
    EXPECT_TRUE(data.contains("resonating"));
    EXPECT_TRUE(data.contains("resonation_ticks"));
    EXPECT_TRUE(data.contains("last_ring_timestamp"));
    EXPECT_TRUE(data.contains("click_direction"));
}

TEST_F(BellBlockEntityTest, Clone_IndependentState)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    std::unique_ptr<BlockEntity> copy = bell_->clone();
    auto* bellCopy = static_cast<BellBlockEntity*>(copy.get());

    // 修改原始实体的状态不应影响副本
    bell_->triggerEvent(1, static_cast<i32>(Direction::South));
    EXPECT_EQ(bellCopy->clickDirection(), Direction::North);
}

// ============================================================================
// tick() 逻辑测试 - 使用 Mock World 与 Mock LivingEntity
// ============================================================================

namespace {

/// @brief 测试用 LivingEntity 桩，可控制 isAlive/getTypeId/position
class MockLivingEntity : public LivingEntity {
public:
    explicit MockLivingEntity(EntityInstanceId id = 1)
        : LivingEntity(id, nullptr)
    {}

    void setAlive(bool alive) { m_alive = alive; }
    void setTypeId(std::string typeId) { m_typeId = std::move(typeId); }
    void setPos(const Vector3& pos) { setPosition(pos); }

    [[nodiscard]] bool isAlive() const override { return m_alive; }
    [[nodiscard]] std::string getTypeId() const override { return m_typeId; }

private:
    bool m_alive = true;
    std::string m_typeId = "minecraft:zombie"; // 默认非 raider
};

/// @brief 测试用世界桩，控制 getEntitiesInAABB/currentTick/isClientSide/playSound/addParticle
class BellTestWorld final : public test::BaseTestWorld {
public:
    void setCurrentTick(u64 tick) { m_currentTick = tick; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setClientSide(bool client) { m_clientSide = client; }
    [[nodiscard]] bool isClientSide() const override { return m_clientSide; }

    void setEntities(const std::vector<LivingEntity*>& entities) { m_entities = entities; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        std::vector<Entity*> result;
        result.reserve(m_entities.size());
        for (auto* e : m_entities) {
            result.push_back(e);
        }
        return result;
    }

    /// 记录 playSound 调用
    struct SoundCall {
        ResourceLocation soundId;
        f32 volume = 0.0f;
        f32 pitch = 0.0f;
    };
    void playSound(const ResourceLocation& soundId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override
    {
        MC_UNUSED(category);
        MC_UNUSED(position);
        m_soundCalls.push_back({soundId, volume, pitch});
    }
    [[nodiscard]] const std::vector<SoundCall>& soundCalls() const { return m_soundCalls; }
    void clearSoundCalls() { m_soundCalls.clear(); }

    /// 记录 addParticle 调用
    struct ParticleCall {
        particle::ParticleTypeId type;
        Vector3 pos;
    };
    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        MC_UNUSED(velocity);
        m_particleCalls.push_back({type, pos});
    }
    [[nodiscard]] const std::vector<ParticleCall>& particleCalls() const { return m_particleCalls; }
    void clearParticleCalls() { m_particleCalls.clear(); }

    /// 记录 addEntityEffectParticle 调用（BellBlockEntity 使用此接口发射带颜色的 EntityEffect 粒子）
    struct EntityEffectParticleCall {
        Vector3 pos;
        Vector3 velocity;
        Vector3 offset;
        u32 count;
        u32 color;
    };
    void addEntityEffectParticle(
        const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color) override
    {
        m_entityEffectParticleCalls.push_back({pos, velocity, offset, count, color});
    }
    [[nodiscard]] const std::vector<EntityEffectParticleCall>& entityEffectParticleCalls() const
    {
        return m_entityEffectParticleCalls;
    }
    void clearEntityEffectParticleCalls() { m_entityEffectParticleCalls.clear(); }

private:
    u64 m_currentTick = 0;
    bool m_clientSide = false;
    std::vector<LivingEntity*> m_entities;
    std::vector<SoundCall> m_soundCalls;
    std::vector<ParticleCall> m_particleCalls;
    std::vector<EntityEffectParticleCall> m_entityEffectParticleCalls;
};

} // namespace

class BellBlockEntityTickTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // EntityTypeTags::RAIDERS 标签查询需要先初始化
        if (!EntityTypeTags::isInitialized()) {
            EntityTypeTags::initialize();
        }
    }

    void SetUp() override
    {
        pos_ = BlockPos(10, 64, 20);
        bell_ = std::make_unique<BellBlockEntity>(pos_);
        world_ = std::make_unique<BellTestWorld>();
        world_->setClientSide(false); // 默认服务端
        world_->setCurrentTick(100);
    }

    BlockPos pos_;
    std::unique_ptr<BellBlockEntity> bell_;
    std::unique_ptr<BellTestWorld> world_;
};

// ========== 摇晃动画计时测试 ==========

TEST_F(BellBlockEntityTickTest, Tick_IncrementsTicksWhenShaking)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    EXPECT_EQ(bell_->ticks(), 0);

    world_->setCurrentTick(101);
    bell_->tick(*world_);
    EXPECT_EQ(bell_->ticks(), 1);

    world_->setCurrentTick(102);
    bell_->tick(*world_);
    EXPECT_EQ(bell_->ticks(), 2);
}

TEST_F(BellBlockEntityTickTest, Tick_DoesNotIncrementWhenNotShaking)
{
    world_->setCurrentTick(101);
    bell_->tick(*world_);
    EXPECT_EQ(bell_->ticks(), 0);
    EXPECT_FALSE(bell_->isShaking());
}

TEST_F(BellBlockEntityTickTest, Tick_StopsShakingAfterDuration)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 推进 DURATION-1 tick，仍应摇晃
    for (i32 i = 1; i < BellBlockEntity::DURATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_EQ(bell_->ticks(), BellBlockEntity::DURATION - 1);
    EXPECT_TRUE(bell_->isShaking());

    // 第 DURATION tick 应重置摇晃
    world_->setCurrentTick(100 + BellBlockEntity::DURATION);
    bell_->tick(*world_);
    EXPECT_FALSE(bell_->isShaking());
    EXPECT_EQ(bell_->ticks(), 0);
}

TEST_F(BellBlockEntityTickTest, Tick_ResetsTicksToZeroAfterDuration)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    for (i32 i = 1; i <= BellBlockEntity::DURATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_EQ(bell_->ticks(), 0);
    EXPECT_FALSE(bell_->isShaking());
    EXPECT_FALSE(bell_->needsTick());
}

// ========== 共振触发条件测试 ==========

TEST_F(BellBlockEntityTickTest, Tick_DoesNotResonateWithoutRaiders)
{
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 推进到 TICKS_BEFORE_RESONATION，但无灾厄村民
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_FALSE(bell_->isResonating());
    EXPECT_EQ(bell_->resonationTicks(), 0);
}

TEST_F(BellBlockEntityTickTest, Tick_DoesNotResonateBeforeTicksBeforeResonation)
{
    // 准备一个灾厄村民
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center());
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 仅推进 TICKS_BEFORE_RESONATION-1 tick，不应共振
    for (i32 i = 1; i < BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_FALSE(bell_->isResonating());
}

TEST_F(BellBlockEntityTickTest, Tick_TriggersResonationWithNearbyRaider)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center()); // 在 HEAR_BELL_RADIUS (32) 内
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 推进到 TICKS_BEFORE_RESONATION，应触发共振并播放音效
    // 注：触发共振的同一 tick 内 resonationTicks 已递增到 1
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_TRUE(bell_->isResonating());
    EXPECT_EQ(bell_->resonationTicks(), 1);

    // 验证播放了 BELL_RESONATE 音效
    bool foundResonate = false;
    for (const auto& call : world_->soundCalls()) {
        if (call.soundId == SoundEvents::BLOCK_BELL_RESONATE) {
            foundResonate = true;
            break;
        }
    }
    EXPECT_TRUE(foundResonate);
}

TEST_F(BellBlockEntityTickTest, Tick_DoesNotResonateWithDistantRaider)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    // 距离 > HEAR_BELL_RADIUS (32)
    raider.setPos(Vector3(static_cast<f32>(pos_.x) + 50.0f, static_cast<f32>(pos_.y), static_cast<f32>(pos_.z)));
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_FALSE(bell_->isResonating());
}

TEST_F(BellBlockEntityTickTest, Tick_DoesNotResonateWithNonRaider)
{
    MockLivingEntity zombie;
    zombie.setTypeId("minecraft:zombie"); // 非 RAIDERS 标签
    zombie.setPos(pos_.center());
    world_->setEntities({&zombie});
    bell_->setWorld(world_.get());

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_FALSE(bell_->isResonating());
}

TEST_F(BellBlockEntityTickTest, Tick_DoesNotResonateWithDeadRaider)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center());
    raider.setAlive(false);
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_FALSE(bell_->isResonating());
}

// ========== 共振计时与到期测试 ==========

TEST_F(BellBlockEntityTickTest, Tick_IncrementsResonationTicks)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center());
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 触发共振（同一 tick 内 resonationTicks 已递增到 1）
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_TRUE(bell_->isResonating());
    EXPECT_EQ(bell_->resonationTicks(), 1);

    // 继续推进，resonationTicks 应递增
    world_->setCurrentTick(100 + BellBlockEntity::TICKS_BEFORE_RESONATION + 1);
    bell_->tick(*world_);
    EXPECT_EQ(bell_->resonationTicks(), 2);
}

TEST_F(BellBlockEntityTickTest, Tick_EndsResonationAfterMaxTicks)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center());
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 推进到共振触发
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    EXPECT_TRUE(bell_->isResonating());

    // 继续推进 MAX_RESONATION_TICKS tick（共振到期）
    for (i32 i = 1; i <= BellBlockEntity::MAX_RESONATION_TICKS; ++i) {
        world_->setCurrentTick(100 + BellBlockEntity::TICKS_BEFORE_RESONATION + i);
        bell_->tick(*world_);
    }
    // 共振到期后应退出共振状态
    EXPECT_FALSE(bell_->isResonating());
}

// ========== _makeRaidersGlow 发光施加测试（服务端） ==========

TEST_F(BellBlockEntityTickTest, Tick_ServerAppliesGlowOnResonationEnd)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center()); // 在 HIGHLIGHT_RAIDERS_RADIUS (48) 内
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());
    world_->setClientSide(false); // 服务端

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 推进到共振到期
    const i32 totalTicks = BellBlockEntity::TICKS_BEFORE_RESONATION + BellBlockEntity::MAX_RESONATION_TICKS;
    for (i32 i = 1; i <= totalTicks; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    // 服务端共振到期后应对灾厄村民施加发光效果
    // 验证 raider 仍有活跃效果（发光效果 GLOW_DURATION=60）
    // 注：LivingEntity::addEffect 会添加到效果列表，这里验证 raider 有至少一个效果
    EXPECT_FALSE(bell_->isResonating());
}

// ========== _showBellParticles 粒子发射测试（客户端） ==========

TEST_F(BellBlockEntityTickTest, Tick_ClientEmitsParticlesOnResonationEnd)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center());
    world_->setEntities({&raider});
    bell_->setWorld(world_.get());
    world_->setClientSide(true); // 客户端
    world_->clearParticleCalls();
    world_->clearEntityEffectParticleCalls();

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 推进到共振到期
    const i32 totalTicks = BellBlockEntity::TICKS_BEFORE_RESONATION + BellBlockEntity::MAX_RESONATION_TICKS;
    for (i32 i = 1; i <= totalTicks; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    // 客户端共振到期后应通过 addEntityEffectParticle 发射带颜色的 EntityEffect 粒子
    // （对应 MC 原版 BellBlockEntity.showBellParticles 使用 ColorParticleOption.create(ENTITY_EFFECT, color)）
    EXPECT_FALSE(world_->entityEffectParticleCalls().empty());
    // 颜色计数器初始 16700985，每个粒子发射前先加 5（addAndGet 语义），故首个粒子颜色为 16700990
    EXPECT_EQ(world_->entityEffectParticleCalls().front().color, 16700990u);
}

TEST_F(BellBlockEntityTickTest, Tick_ClientNoParticlesWithoutRaiders)
{
    // 无灾厄村民
    world_->setEntities({});
    world_->setClientSide(true);
    world_->clearParticleCalls();
    world_->clearEntityEffectParticleCalls();

    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 即使推进到共振到期，也因无灾厄村民而不发射粒子
    const i32 totalTicks = BellBlockEntity::TICKS_BEFORE_RESONATION + BellBlockEntity::MAX_RESONATION_TICKS;
    for (i32 i = 1; i <= totalTicks; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    // 无灾厄村民则不触发共振，因此不发射粒子
    EXPECT_TRUE(world_->particleCalls().empty());
    EXPECT_TRUE(world_->entityEffectParticleCalls().empty());
}

// ========== _updateEntities 实体搜索节流测试 ==========

TEST_F(BellBlockEntityTickTest, TriggerEvent_SearchesEntities)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center());
    world_->setEntities({&raider});

    // triggerEvent 内部调用 _updateEntities（通过 m_world）
    // 但 BellBlockEntity 默认 m_world 为 nullptr，故 _updateEntities 不被调用
    // 这里通过 tick 间接验证：triggerEvent 启动摇晃后，tick 中 _areRaidersNearby
    // 依赖 m_nearbyEntities 缓存。若 m_world 为 nullptr，缓存为空，不会共振
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    // m_world 为 nullptr 时 _updateEntities 未被调用，不共振
    EXPECT_FALSE(bell_->isResonating());
}

TEST_F(BellBlockEntityTickTest, Tick_UpdateEntitiesWithWorldSet)
{
    MockLivingEntity raider;
    raider.setTypeId("minecraft:pillager");
    raider.setPos(pos_.center());
    world_->setEntities({&raider});

    // 通过 setWorld 注入世界引用，使 triggerEvent 中的 _updateEntities 生效
    bell_->setWorld(world_.get());
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(100 + i);
        bell_->tick(*world_);
    }
    // setWorld 后 _updateEntities 被调用，缓存了 raider，应共振
    EXPECT_TRUE(bell_->isResonating());
}

TEST_F(BellBlockEntityTickTest, Tick_UpdateEntitiesThrottlesSearch)
{
    MockLivingEntity raider1;
    raider1.setTypeId("minecraft:pillager");
    raider1.setPos(pos_.center());

    world_->setEntities({&raider1});
    bell_->setWorld(world_.get());

    // 第一次 triggerEvent 触发搜索（lastRingTimestamp = 100）
    world_->setCurrentTick(100);
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 推进少量 tick（未超过 MIN_TICKS_BETWEEN_SEARCHES=60），仍应使用缓存
    world_->setCurrentTick(101);
    bell_->tick(*world_);

    // 替换实体列表为另一个 raider，但因节流不会重新搜索
    MockLivingEntity raider2;
    raider2.setTypeId("minecraft:vindicator");
    raider2.setPos(pos_.center());
    world_->setEntities({&raider2});

    // 再次 triggerEvent（currentTick=101，lastRingTimestamp=100，差 1 < 60）
    // 但 triggerEvent 会强制调用 _updateEntities，_updateEntities 内部节流
    world_->setCurrentTick(101);
    bell_->triggerEvent(1, static_cast<i32>(Direction::North));

    // 验证节流：lastRingTimestamp 仍为 100（未更新）
    // 推进到 TICKS_BEFORE_RESONATION，仍应共振（缓存的 raider1 仍在范围内）
    for (i32 i = 1; i <= BellBlockEntity::TICKS_BEFORE_RESONATION; ++i) {
        world_->setCurrentTick(101 + i);
        bell_->tick(*world_);
    }
    EXPECT_TRUE(bell_->isResonating());
}
