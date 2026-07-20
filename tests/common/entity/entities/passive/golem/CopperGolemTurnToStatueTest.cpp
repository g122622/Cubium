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
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/CopperGolemStatueBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::entity;

namespace mc::test {
class CopperGolemEntityTestAccessor; // 测试访问器，声明为 friend 以访问 private 成员
}

namespace mc::test {

// ============================================================================
// 测试访问器 - 用于调用 CopperGolemEntity 的私有方法 turnToStatue()
// ============================================================================
//
// CopperGolemEntity 已在 hpp 中声明 friend class test::CopperGolemEntityTestAccessor
// 这里提供具体实现，将私有方法 turnToStatue() 暴露给测试代码。

class CopperGolemEntityTestAccessor {
public:
    static void turnToStatue(CopperGolemEntity& golem) { golem.turnToStatue(); }
};

} // namespace mc::test

namespace {
// ============================================================================
// 测试用世界 - 支持方块状态存储、方块实体存储、音效/实体生成捕获
// ============================================================================
//
// turnToStatue() 需要：
// - setBlockState() 放置 oxidized_copper_golem_statue 方块
// - getBlockEntity() 获取方块实体并设置自定义名称
// - getGameRules() 检查 DO_ENTITY_DROPS 规则
// - playSound() 播放变雕像音效
// - spawnEntity() 由 dropLeash() -> ItemDropHelper 间接调用以掉落拴绳物品

class CopperGolemTurnToStatueTestWorld final : public test::BaseTestWorld {
public:
    CopperGolemTurnToStatueTestWorld()
    {
        m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        // 默认开启 DO_ENTITY_DROPS，便于测试掉落场景
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, true, nullptr);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr) {
            m_blocks.erase(pos);
            m_blockEntities.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);

            // 模拟真实 World::setBlock 行为：若方块拥有方块实体，则自动创建并存储
            // 注意：createBlockEntity 不是 const 方法，需要通过 getBlockMutable 获取可变引用
            Block& block = state->getBlockMutable();
            if (block.hasBlockEntity()) {
                auto be = block.createBlockEntity(pos);
                if (be != nullptr) {
                    be->setWorld(this);
                    m_blockEntities[pos] = std::move(be);
                }
            } else {
                m_blockEntities.erase(pos);
            }
        }
        return true;
    }

    // 2-arg 重载
    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const
    {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it != m_blockEntities.end() ? it->second.get() : nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity != nullptr) {
            entity->setWorld(this);
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    void playSound(const ResourceLocation& sound,
        sound::SoundCategory category,
        const Vector3& pos,
        f32 volume,
        f32 pitch) override
    {
        m_sounds.push_back({sound, category, pos, volume, pitch});
    }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override { return *m_tickManagerPtr; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override { return *m_tickManagerPtr; }

    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

    // ========== 测试辅助方法 ==========

    struct SoundRecord {
        ResourceLocation sound;
        sound::SoundCategory category;
        Vector3 pos;
        f32 volume;
        f32 pitch;
    };

    [[nodiscard]] const std::vector<SoundRecord>& sounds() const { return m_sounds; }
    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }
    void clearSounds() { m_sounds.clear(); }
    void clearSpawnedEntities() { m_spawnedEntities.clear(); }

    /// 显式启用/禁用 DO_ENTITY_DROPS 规则
    void setDoEntityDrops(bool enabled)
    {
        m_gameRules.setBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS, enabled, nullptr);
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<SoundRecord> m_sounds;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_seed = 0;
};

} // namespace

// ============================================================================
// 测试夹具
// ============================================================================

class CopperGolemTurnToStatueTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            VanillaBlocks::initialize();
            Items::initialize();
            VanillaEntities::registerAll();
            s_initialized = true;
        }
    }

    void SetUp() override { m_world = std::make_unique<CopperGolemTurnToStatueTestWorld>(); }

    /// 创建一个铜傀儡，放置在 (8, 64, 8) 位置（脚下为空气）
    std::unique_ptr<CopperGolemEntity> createGolemAtStatuePosition()
    {
        auto golem = std::make_unique<CopperGolemEntity>(EntityId(1));
        golem->setWorld(m_world.get());
        // 位置 (8.5, 64, 8.5) → blockPosition() = (8, 64, 8)，下方为空气
        golem->setPosition(8.5f, 64.0f, 8.5f);
        return golem;
    }

    /// 统计生成的 ItemEntity 数量（用于验证拴绳物品掉落）
    static std::size_t countItemEntities(const std::vector<std::unique_ptr<Entity>>& entities)
    {
        std::size_t count = 0;
        for (const auto& e : entities) {
            if (dynamic_cast<ItemEntity*>(e.get()) != nullptr) {
                ++count;
            }
        }
        return count;
    }

    /// 从生成的 ItemEntity 中取第一个并返回其 ItemStack（用于验证是否为拴绳）
    static const ItemStack* firstItemEntityStack(const std::vector<std::unique_ptr<Entity>>& entities)
    {
        for (const auto& e : entities) {
            auto* itemEntity = dynamic_cast<ItemEntity*>(e.get());
            if (itemEntity != nullptr) {
                return &itemEntity->getItemStack();
            }
        }
        return nullptr;
    }

    std::unique_ptr<CopperGolemTurnToStatueTestWorld> m_world;
};

// ============================================================================
// turnToStatue 基础行为测试
// ============================================================================

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_PlacesOxidizedStatueBlock)
{
    // 验证 turnToStatue 在实体位置放置 oxidized_copper_golem_statue 方块
    if (VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE == nullptr) {
        GTEST_SKIP() << "OXIDIZED_COPPER_GOLEM_STATUE not registered";
    }

    auto golem = createGolemAtStatuePosition();
    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    const BlockPos expectedPos(8, 64, 8);
    const BlockState* state = m_world->getBlockState(expectedPos);
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->owner().blockLocation(), VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE->blockLocation());
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_PlaysBecomeStatueSound)
{
    // 验证 turnToStatue 播放 BLOCK_COPPER_GOLEM_BECOME_STATUE 音效
    auto golem = createGolemAtStatuePosition();
    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    bool foundSound = false;
    for (const auto& s : m_world->sounds()) {
        if (s.sound == SoundEvents::BLOCK_COPPER_GOLEM_BECOME_STATUE) {
            foundSound = true;
            break;
        }
    }
    EXPECT_TRUE(foundSound);
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_DiscardsEntity)
{
    // 验证 turnToStatue 调用 discard() 标记实体为已移除
    auto golem = createGolemAtStatuePosition();
    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    EXPECT_TRUE(golem->isRemoved());
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_TransfersCustomNameToStatue)
{
    // 验证 turnToStatue 将铜傀儡的自定义名称转移到雕像方块实体
    auto golem = createGolemAtStatuePosition();
    golem->setCustomName("MyCopperBuddy");

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    const BlockPos pos(8, 64, 8);
    BlockEntity* be = m_world->getBlockEntity(pos);
    ASSERT_NE(be, nullptr);
    EXPECT_EQ(be->getCustomName(), "MyCopperBuddy");
}

// ============================================================================
// turnToStatue 拴绳掉落行为测试（核心：收敛 TODO 的目标）
// ============================================================================
//
// 对应 MC 1.21.11 CopperGolem.turnToStatue(ServerLevel) 中的拴绳处理逻辑：
//   if (this.isLeashed()) {
//       if (p_479343_.getGameRules().get(GameRules.ENTITY_DROPS)) {
//           this.dropLeash();
//       } else {
//           this.removeLeash();
//       }
//   }
//
// 本项目翻译：
//   - dropLeash() 在 DO_ENTITY_DROPS=true 时掉落拴绳物品并清除拴绳状态
//   - clearLeash() 对应 removeLeash()，仅清除拴绳状态（不掉落物品）

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenLeashedAndDropsEnabled_DropsLeadItem)
{
    // 场景：被拴住的铜傀儡变雕像，DO_ENTITY_DROPS=true → 应掉落拴绳物品并清除拴绳状态
    if (Items::LEAD == nullptr) {
        GTEST_SKIP() << "Items::LEAD not registered";
    }

    auto golem = createGolemAtStatuePosition();
    golem->setLeashedToEntity("test-player-uuid");
    ASSERT_TRUE(golem->isLeashed());

    m_world->clearSpawnedEntities();
    m_world->setDoEntityDrops(true);

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    // 拴绳状态应被清除
    EXPECT_FALSE(golem->isLeashed());

    // 应生成 1 个 ItemEntity（拴绳物品）
    EXPECT_EQ(countItemEntities(m_world->spawnedEntities()), 1u);

    // 验证掉落的物品确实是拴绳
    const ItemStack* stack = firstItemEntityStack(m_world->spawnedEntities());
    ASSERT_NE(stack, nullptr);
    EXPECT_EQ(stack->getItem(), Items::LEAD);
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenLeashedAndDropsDisabled_ClearsLeashWithoutDropping)
{
    // 场景：被拴住的铜傀儡变雕像，DO_ENTITY_DROPS=false → 应仅清除拴绳状态，不掉落物品
    if (Items::LEAD == nullptr) {
        GTEST_SKIP() << "Items::LEAD not registered";
    }

    auto golem = createGolemAtStatuePosition();
    golem->setLeashedToEntity("test-player-uuid");
    ASSERT_TRUE(golem->isLeashed());

    m_world->clearSpawnedEntities();
    m_world->setDoEntityDrops(false);

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    // 拴绳状态应被清除（对应 MC Java 的 removeLeash）
    EXPECT_FALSE(golem->isLeashed());

    // 不应掉落任何物品实体
    EXPECT_EQ(countItemEntities(m_world->spawnedEntities()), 0u);
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenNotLeashed_DoesNotDropAnyItem)
{
    // 场景：未被拴住的铜傀儡变雕像 → 不应掉落任何物品
    auto golem = createGolemAtStatuePosition();
    ASSERT_FALSE(golem->isLeashed());

    m_world->clearSpawnedEntities();
    m_world->setDoEntityDrops(true);

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    // 未被拴住时不应生成任何 ItemEntity
    EXPECT_EQ(countItemEntities(m_world->spawnedEntities()), 0u);
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenNotLeashed_DoesNotChangeLeashState)
{
    // 场景：未被拴住的铜傀儡变雕像后，拴绳状态应保持 false
    auto golem = createGolemAtStatuePosition();
    ASSERT_FALSE(golem->isLeashed());

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    EXPECT_FALSE(golem->isLeashed());
}

// ============================================================================
// turnToStatue 拴绳绑定到栅栏柱的场景
// ============================================================================

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenLeashedToFenceAndDropsEnabled_DropsLeadItem)
{
    // 场景：被拴在栅栏柱上的铜傀儡变雕像，DO_ENTITY_DROPS=true → 应掉落拴绳物品
    if (Items::LEAD == nullptr) {
        GTEST_SKIP() << "Items::LEAD not registered";
    }

    auto golem = createGolemAtStatuePosition();
    const BlockPos fencePos(10, 64, 10);
    golem->setLeashedToFence(fencePos);
    ASSERT_TRUE(golem->isLeashed());

    m_world->clearSpawnedEntities();
    m_world->setDoEntityDrops(true);

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    // 拴绳状态应被清除
    EXPECT_FALSE(golem->isLeashed());

    // 应掉落拴绳物品
    EXPECT_EQ(countItemEntities(m_world->spawnedEntities()), 1u);
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenLeashedToFenceAndDropsDisabled_ClearsLeashWithoutDropping)
{
    // 场景：被拴在栅栏柱上的铜傀儡变雕像，DO_ENTITY_DROPS=false → 仅清除拴绳状态
    if (Items::LEAD == nullptr) {
        GTEST_SKIP() << "Items::LEAD not registered";
    }

    auto golem = createGolemAtStatuePosition();
    const BlockPos fencePos(10, 64, 10);
    golem->setLeashedToFence(fencePos);
    ASSERT_TRUE(golem->isLeashed());

    m_world->clearSpawnedEntities();
    m_world->setDoEntityDrops(false);

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    // 拴绳状态应被清除
    EXPECT_FALSE(golem->isLeashed());

    // 不应掉落物品
    EXPECT_EQ(countItemEntities(m_world->spawnedEntities()), 0u);
}

// ============================================================================
// 边界场景
// ============================================================================

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WithCustomNameAndLeashed_PreservesBothNameAndLeadDrop)
{
    // 综合场景：铜傀儡有自定义名称 + 被拴住 + DO_ENTITY_DROPS=true
    // 验证自定义名称转移到雕像 + 拴绳物品掉落
    if (Items::LEAD == nullptr) {
        GTEST_SKIP() << "Items::LEAD not registered";
    }

    auto golem = createGolemAtStatuePosition();
    golem->setCustomName("NamedGolem");
    golem->setLeashedToEntity("test-player-uuid");
    ASSERT_TRUE(golem->isLeashed());

    m_world->clearSpawnedEntities();
    m_world->setDoEntityDrops(true);

    test::CopperGolemEntityTestAccessor::turnToStatue(*golem);

    // 自定义名称应转移到雕像方块实体
    const BlockPos pos(8, 64, 8);
    BlockEntity* be = m_world->getBlockEntity(pos);
    ASSERT_NE(be, nullptr);
    EXPECT_EQ(be->getCustomName(), "NamedGolem");

    // 拴绳状态应清除
    EXPECT_FALSE(golem->isLeashed());

    // 应掉落拴绳物品
    EXPECT_EQ(countItemEntities(m_world->spawnedEntities()), 1u);
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenWorldNull_DoesNotCrash)
{
    // 边界场景：实体未设置世界时调用 turnToStatue 应安全返回（不崩溃）
    auto golem = std::make_unique<CopperGolemEntity>(EntityId(1));
    // 不调用 setWorld，golem->world() 返回 nullptr

    // 应安全返回，不崩溃
    EXPECT_NO_FATAL_FAILURE(test::CopperGolemEntityTestAccessor::turnToStatue(*golem));

    // 实体不应被标记为已移除（因为函数提前返回）
    EXPECT_FALSE(golem->isRemoved());
}

TEST_F(CopperGolemTurnToStatueTest, TurnToStatue_WhenStatueBlockNull_DoesNotCrash)
{
    // 边界场景：VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE 未注册时调用 turnToStatue 应安全返回
    // 注意：此测试仅在方块未注册时有意义；正常情况下方块已注册，此测试为防御性覆盖
    auto golem = createGolemAtStatuePosition();

    if (VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE == nullptr) {
        EXPECT_NO_FATAL_FAILURE(test::CopperGolemEntityTestAccessor::turnToStatue(*golem));
        EXPECT_FALSE(golem->isRemoved());
    } else {
        // 方块已注册时，正常执行 turnToStatue
        test::CopperGolemEntityTestAccessor::turnToStatue(*golem);
        EXPECT_TRUE(golem->isRemoved());
    }
}
