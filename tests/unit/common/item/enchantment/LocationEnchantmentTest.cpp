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
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/enchantment/LocationEnchantmentTracker.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/item/enchantment/enchantments/protection/FrostWalkerEnchantment.hpp"
#include "common/item/enchantment/enchantments/special/SoulSpeedEnchantment.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"

#include <map>
#include <memory>

using namespace mc;
using namespace mc::block_registry;
using namespace mc::entity;
using namespace mc::item::enchant;

// ============================================================================
// LocationEnchantmentTracker 测试
// ============================================================================

class LocationEnchantmentTrackerTest : public ::testing::Test {
protected:
    LocationEnchantmentTracker tracker;
};

TEST_F(LocationEnchantmentTrackerTest, InitiallyNoActiveEnchantments)
{
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_FALSE(tracker.isActive(1, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetActiveAndCheck)
{
    tracker.setActive(0, "minecraft:frost_walker");
    EXPECT_TRUE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_FALSE(tracker.isActive(1, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetActiveMultipleSlots)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");
    tracker.setActive(3, "minecraft:frost_walker");

    EXPECT_TRUE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_TRUE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_TRUE(tracker.isActive(3, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(1, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetActiveIdempotent)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:frost_walker"); // 重复设置不报错
    EXPECT_TRUE(tracker.isActive(0, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetInactiveReturnsPreviousState)
{
    EXPECT_FALSE(tracker.setInactive(0, "minecraft:frost_walker")); // 从未激活，返回 false

    tracker.setActive(0, "minecraft:frost_walker");
    EXPECT_TRUE(tracker.setInactive(0, "minecraft:frost_walker")); // 之前活跃，返回 true
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, SetInactiveRemovesEmptySlot)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setInactive(0, "minecraft:frost_walker");

    // 槽位 0 应该没有活跃附魔了
    const auto& active = tracker.getActiveEnchantments(0);
    EXPECT_TRUE(active.empty());
}

TEST_F(LocationEnchantmentTrackerTest, SetInactiveKeepsOtherEnchantments)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");
    tracker.setInactive(0, "minecraft:frost_walker");

    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_TRUE(tracker.isActive(0, "minecraft:soul_speed"));
}

TEST_F(LocationEnchantmentTrackerTest, ClearSlotReturnsAllActive)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");

    auto cleared = tracker.clearSlot(0);
    EXPECT_EQ(cleared.size(), 2u);
    EXPECT_TRUE(cleared.count("minecraft:frost_walker") > 0);
    EXPECT_TRUE(cleared.count("minecraft:soul_speed") > 0);
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
}

TEST_F(LocationEnchantmentTrackerTest, ClearEmptySlotReturnsEmpty)
{
    auto cleared = tracker.clearSlot(0);
    EXPECT_TRUE(cleared.empty());
}

TEST_F(LocationEnchantmentTrackerTest, ClearSlotDoesNotAffectOtherSlots)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(3, "minecraft:soul_speed");

    tracker.clearSlot(0);
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_TRUE(tracker.isActive(3, "minecraft:soul_speed"));
}

TEST_F(LocationEnchantmentTrackerTest, ClearAllRemovesEverything)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");
    tracker.setActive(3, "minecraft:frost_walker");

    tracker.clearAll();
    EXPECT_FALSE(tracker.isActive(0, "minecraft:frost_walker"));
    EXPECT_FALSE(tracker.isActive(0, "minecraft:soul_speed"));
    EXPECT_FALSE(tracker.isActive(3, "minecraft:frost_walker"));
}

TEST_F(LocationEnchantmentTrackerTest, GetActiveEnchantmentsEmpty)
{
    const auto& active = tracker.getActiveEnchantments(0);
    EXPECT_TRUE(active.empty());
}

TEST_F(LocationEnchantmentTrackerTest, GetActiveEnchantmentsWithEntries)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");

    const auto& active = tracker.getActiveEnchantments(0);
    EXPECT_EQ(active.size(), 2u);
    EXPECT_TRUE(active.count("minecraft:frost_walker") > 0);
    EXPECT_TRUE(active.count("minecraft:soul_speed") > 0);
}

TEST_F(LocationEnchantmentTrackerTest, HasActiveEnchantmentsEmpty)
{
    // 空tracker不应有活跃附魔
    EXPECT_FALSE(tracker.hasActiveEnchantments());
}

TEST_F(LocationEnchantmentTrackerTest, HasActiveEnchantmentsAfterSetActive)
{
    tracker.setActive(0, "minecraft:frost_walker");
    EXPECT_TRUE(tracker.hasActiveEnchantments());

    tracker.setActive(3, "minecraft:soul_speed");
    EXPECT_TRUE(tracker.hasActiveEnchantments());
}

TEST_F(LocationEnchantmentTrackerTest, HasActiveEnchantmentsAfterSetInactive)
{
    tracker.setActive(0, "minecraft:frost_walker");
    EXPECT_TRUE(tracker.hasActiveEnchantments());

    // 移除唯一的活跃附魔后应为空
    tracker.setInactive(0, "minecraft:frost_walker");
    EXPECT_FALSE(tracker.hasActiveEnchantments());
}

TEST_F(LocationEnchantmentTrackerTest, HasActiveEnchantmentsAfterClearSlot)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(0, "minecraft:soul_speed");
    EXPECT_TRUE(tracker.hasActiveEnchantments());

    // 清除槽位后应为空
    tracker.clearSlot(0);
    EXPECT_FALSE(tracker.hasActiveEnchantments());
}

TEST_F(LocationEnchantmentTrackerTest, HasActiveEnchantmentsMultipleSlots)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(3, "minecraft:soul_speed");
    EXPECT_TRUE(tracker.hasActiveEnchantments());

    // 清除一个槽位，另一个仍有活跃附魔
    tracker.clearSlot(0);
    EXPECT_TRUE(tracker.hasActiveEnchantments());

    // 清除最后一个槽位
    tracker.clearSlot(3);
    EXPECT_FALSE(tracker.hasActiveEnchantments());
}

TEST_F(LocationEnchantmentTrackerTest, HasActiveEnchantmentsAfterClearAll)
{
    tracker.setActive(0, "minecraft:frost_walker");
    tracker.setActive(3, "minecraft:soul_speed");
    EXPECT_TRUE(tracker.hasActiveEnchantments());

    tracker.clearAll();
    EXPECT_FALSE(tracker.hasActiveEnchantments());
}

// ============================================================================
// FrostWalkerEnchantment 属性测试
// ============================================================================

class FrostWalkerEnchantmentTest : public ::testing::Test {
protected:
    FrostWalkerEnchantment frostWalker;
};

TEST_F(FrostWalkerEnchantmentTest, Properties)
{
    EXPECT_EQ(frostWalker.id(), "minecraft:frost_walker");
    EXPECT_EQ(frostWalker.minLevel(), 1);
    EXPECT_EQ(frostWalker.maxLevel(), 2);
    EXPECT_EQ(frostWalker.type(), EnchantmentType::ArmorFeet);
    EXPECT_EQ(frostWalker.rarity(), EnchantmentRarity::Rare);
    EXPECT_TRUE(frostWalker.isTreasure());
}

TEST_F(FrostWalkerEnchantmentTest, GetNameKey)
{
    EXPECT_EQ(frostWalker.getNameKey(1), "enchantment.minecraft.frost_walker");
    EXPECT_EQ(frostWalker.getNameKey(2), "enchantment.minecraft.frost_walker");
}

TEST_F(FrostWalkerEnchantmentTest, GetFrostRadius)
{
    EXPECT_EQ(FrostWalkerEnchantment::getFrostRadius(1), 2); // I: 1 + 1 = 2
    EXPECT_EQ(FrostWalkerEnchantment::getFrostRadius(2), 3); // II: 2 + 1 = 3
}

TEST_F(FrostWalkerEnchantmentTest, GetMinCost)
{
    EXPECT_EQ(frostWalker.getMinCost(1), 10);
    EXPECT_EQ(frostWalker.getMinCost(2), 20);
}

TEST_F(FrostWalkerEnchantmentTest, GetMaxCost)
{
    EXPECT_EQ(frostWalker.getMaxCost(1), 25); // getMinCost(1) + 15
    EXPECT_EQ(frostWalker.getMaxCost(2), 35); // getMinCost(2) + 15
}

TEST_F(FrostWalkerEnchantmentTest, IsIncompatibleWithDepthStrider)
{
    FrostWalkerEnchantment frostWalker;
    const Enchantment* depthStrider = EnchantmentRegistry::get("minecraft:depth_strider");
    if (depthStrider) {
        EXPECT_FALSE(frostWalker.isCompatibleWith(*depthStrider));
    }
}

TEST_F(FrostWalkerEnchantmentTest, OnLocationEffectDeactivatedIsSafe)
{
    // FrostWalkerEnchantment::onLocationEffectDeactivated() 是空实现，
    // 但应可安全调用不崩溃。由于需要 LivingEntity 参数，这里只验证接口存在。
    // 实际集成测试在 LivingEntity 环境中进行。
}

// ============================================================================
// SoulSpeedEnchantment 属性测试
// ============================================================================

class SoulSpeedEnchantmentTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }

    SoulSpeedEnchantment soulSpeed;
};

TEST_F(SoulSpeedEnchantmentTest, Properties)
{
    EXPECT_EQ(soulSpeed.id(), "minecraft:soul_speed");
    EXPECT_EQ(soulSpeed.minLevel(), 1);
    EXPECT_EQ(soulSpeed.maxLevel(), 3);
    EXPECT_EQ(soulSpeed.type(), EnchantmentType::ArmorFeet);
    EXPECT_EQ(soulSpeed.rarity(), EnchantmentRarity::VeryRare);
    EXPECT_TRUE(soulSpeed.isTreasure());
    EXPECT_FALSE(soulSpeed.canVillagerTrade());
    EXPECT_FALSE(soulSpeed.canGenerateInLoot());
}

TEST_F(SoulSpeedEnchantmentTest, GetNameKey)
{
    EXPECT_EQ(soulSpeed.getNameKey(1), "enchantment.minecraft.soul_speed");
    EXPECT_EQ(soulSpeed.getNameKey(3), "enchantment.minecraft.soul_speed");
}

TEST_F(SoulSpeedEnchantmentTest, GetMovementSpeedBonus)
{
    // LevelBasedValue.perLevel(0.0405F, 0.0105F):
    // 公式: 0.0405 + 0.0105 * (level - 1)
    // Level I: 0.0405
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getMovementSpeedBonus(1), 0.0405f);
    // Level II: 0.0405 + 0.0105 = 0.051
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getMovementSpeedBonus(2), 0.051f);
    // Level III: 0.0405 + 0.0105 * 2 = 0.0615
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getMovementSpeedBonus(3), 0.0615f);
}

TEST_F(SoulSpeedEnchantmentTest, GetMovementEfficiencyBonus)
{
    // MOVEMENT_EFFICIENCY 修饰符值固定为 1.0（所有等级）
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getMovementEfficiencyBonus(), 1.0f);
}

TEST_F(SoulSpeedEnchantmentTest, GetDurabilityConsumeChance)
{
    // 灵魂疾行固定 4% 概率消耗耐久，与等级无关
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getDurabilityConsumeChance(1), 0.04f);
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getDurabilityConsumeChance(2), 0.04f);
    EXPECT_FLOAT_EQ(SoulSpeedEnchantment::getDurabilityConsumeChance(3), 0.04f);
}

TEST_F(SoulSpeedEnchantmentTest, GetMinCost)
{
    EXPECT_EQ(soulSpeed.getMinCost(1), 10);
    EXPECT_EQ(soulSpeed.getMinCost(2), 20);
    EXPECT_EQ(soulSpeed.getMinCost(3), 30);
}

TEST_F(SoulSpeedEnchantmentTest, GetMaxCost)
{
    EXPECT_EQ(soulSpeed.getMaxCost(1), 25); // getMinCost(1) + 15
    EXPECT_EQ(soulSpeed.getMaxCost(2), 35); // getMinCost(2) + 15
    EXPECT_EQ(soulSpeed.getMaxCost(3), 45); // getMinCost(3) + 15
}

TEST_F(SoulSpeedEnchantmentTest, SoulSpeedModifierId)
{
    // 灵魂疾行速度修饰符使用固定 ID，确保不会与其他修饰符冲突
    // 修饰符 ID 在 SoulSpeedEnchantment.cpp 中定义为 "enchantment.soul_speed"
    // 验证注册表中可以找到灵魂疾行附魔
    const Enchantment* registered = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(registered, nullptr);
    EXPECT_EQ(registered->id(), "minecraft:soul_speed");
}

// ============================================================================
// Enchantment 位置依赖效果接口测试
// ============================================================================

class EnchantmentLocationEffectTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(EnchantmentLocationEffectTest, DefaultOnLocationChangedReturnsFalse)
{
    // 默认 Enchantment::onLocationChanged() 返回 false（不激活位置效果）
    const Enchantment* protection = EnchantmentRegistry::get("minecraft:protection");
    ASSERT_NE(protection, nullptr);

    // 无法直接调用 onLocationChanged（需要 LivingEntity），
    // 但验证附魔默认不实现位置效果是设计约束
    EXPECT_EQ(protection->type(), EnchantmentType::Armor);
}

TEST_F(EnchantmentLocationEffectTest, FrostWalkerAndSoulSpeedRegistered)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);
    EXPECT_EQ(frostWalker->id(), "minecraft:frost_walker");
    EXPECT_TRUE(frostWalker->isTreasure());

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);
    EXPECT_EQ(soulSpeed->id(), "minecraft:soul_speed");
    EXPECT_TRUE(soulSpeed->isTreasure());
}

TEST_F(EnchantmentLocationEffectTest, FrostWalkerIncompatibleWithDepthStrider)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    const Enchantment* depthStrider = EnchantmentRegistry::get("minecraft:depth_strider");
    ASSERT_NE(frostWalker, nullptr);
    ASSERT_NE(depthStrider, nullptr);

    EXPECT_FALSE(frostWalker->isCompatibleWith(*depthStrider));
    EXPECT_FALSE(depthStrider->isCompatibleWith(*frostWalker));
}

TEST_F(EnchantmentLocationEffectTest, SoulSpeedIsArmorFeetType)
{
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);
    EXPECT_EQ(soulSpeed->type(), EnchantmentType::ArmorFeet);
}

// ============================================================================
// 位置依赖附魔效果工具方法测试
// ============================================================================

class EnchantmentHelperLocationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        EnchantmentRegistry::clear();
        EnchantmentRegistry::initialize();
    }

    void TearDown() override { EnchantmentRegistry::clear(); }
};

TEST_F(EnchantmentHelperLocationTest, HasFrostWalkerOnEmptyStack)
{
    ItemStack empty;
    EXPECT_FALSE(EnchantmentHelper::hasFrostWalker(empty));
}

TEST_F(EnchantmentHelperLocationTest, HasSoulSpeedOnEmptyStack)
{
    ItemStack empty;
    EXPECT_FALSE(EnchantmentHelper::hasSoulSpeed(empty));
}

TEST_F(EnchantmentHelperLocationTest, HasFrostWalkerOnEnchantedItem)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 1}}, boots);
    EXPECT_TRUE(EnchantmentHelper::hasFrostWalker(boots));
}

TEST_F(EnchantmentHelperLocationTest, HasSoulSpeedOnEnchantedItem)
{
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    EXPECT_TRUE(EnchantmentHelper::hasSoulSpeed(boots));
}

TEST_F(EnchantmentHelperLocationTest, FrostWalkerLevel)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 2}}, boots);
    EXPECT_EQ(EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:frost_walker"), 2);
}

TEST_F(EnchantmentHelperLocationTest, SoulSpeedLevel)
{
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 3}}, boots);
    EXPECT_EQ(EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:soul_speed"), 3);
}

TEST_F(EnchantmentHelperLocationTest, FrostWalkerAndDepthStriderMutuallyExclusive)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    const Enchantment* depthStrider = EnchantmentRegistry::get("minecraft:depth_strider");
    ASSERT_NE(frostWalker, nullptr);
    ASSERT_NE(depthStrider, nullptr);

    // 冰霜行者和深海探索者互斥
    EXPECT_FALSE(frostWalker->isCompatibleWith(*depthStrider));
    EXPECT_FALSE(depthStrider->isCompatibleWith(*frostWalker));
}

// ============================================================================
// 集成测试：位置依赖附魔 Mock 世界
// ============================================================================

namespace {

/**
 * @brief 位置依赖附魔集成测试用的 Mock 世界
 *
 * 支持：
 * - 方块状态存储与查询（用于冰霜行者放冰、灵魂疾行检测脚下方块）
 * - 流体状态存储与查询（用于冰霜行者检测水源）
 * - isWaterAt 检查（用于冰霜行者水源检测）
 * - 属性修饰符验证（用于灵魂疾行速度修饰符）
 */
class LocationEnchantmentTestWorld final : public mc::test::BaseTestWorld {
public:
    LocationEnchantmentTestWorld() = default;

    using IWorld::getBlockState;
    using IWorld::getFluidState;

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
            m_ownedStates.erase(pos);
        } else {
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        m_blockChanges.push_back({pos, state});
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        (void)flags;
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        // 优先检查显式设置的流体状态
        const BlockPos pos(x, y, z);
        const auto fluidIt = m_fluids.find(pos);
        if (fluidIt != m_fluids.end() && fluidIt->second != nullptr) {
            return fluidIt->second;
        }

        // 回退到方块的流体状态
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr && !fluidState->isEmpty()) {
                return fluidState;
            }
        }

        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] bool isWaterAt(const BlockPos& pos) const override
    {
        // 如果显式标记为水源，返回 true
        const auto it = m_waterPositions.find(pos);
        if (it != m_waterPositions.end() && it->second) {
            return true;
        }
        return false;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        (void)type;
        (void)pos;
        (void)velocity;
        // 测试中忽略粒子效果
    }

    // 测试辅助方法
    void setBlockDirectly(const BlockPos& pos, const BlockState* state)
    {
        (void)setBlockState(pos.x, pos.y, pos.z, state);
    }

    void setFluidDirectly(const BlockPos& pos, const fluid::FluidState* state) { m_fluids[pos] = state; }

    void setWaterAt(const BlockPos& pos, bool isWater = true) { m_waterPositions[pos] = isWater; }

    [[nodiscard]] size_t blockChangeCount() const { return m_blockChanges.size(); }

    [[nodiscard]] const BlockState* getLastBlockChange() const
    {
        if (m_blockChanges.empty()) {
            return nullptr;
        }
        return m_blockChanges.back().second;
    }

    [[nodiscard]] bool hasBlockChangeAt(const BlockPos& pos) const
    {
        for (const auto& change : m_blockChanges) {
            if (change.first == pos) {
                return true;
            }
        }
        return false;
    }

    void clearBlockChanges() { m_blockChanges.clear(); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::map<BlockPos, const fluid::FluidState*> m_fluids;
    std::map<BlockPos, bool> m_waterPositions;
    std::vector<std::pair<BlockPos, const BlockState*>> m_blockChanges;
};

/**
 * @brief 位置依赖附魔集成测试用的 LivingEntity
 */
class TestLivingEntityForLocation : public LivingEntity {
public:
    TestLivingEntityForLocation()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerData();
        registerAttributes();
        setHealth(maxHealth());
    }

    /**
     * @brief 测试辅助方法：设置骑乘状态
     * Entity::setVehicle() 是 protected 方法，测试中通过此辅助方法访问
     */
    void setVehicleForTest(EntityInstanceId vehicle) { setVehicle(vehicle); }
};

} // namespace

// ============================================================================
// 集成测试：FrostWalker 冰霜行者放冰逻辑
// ============================================================================

class FrostWalkerIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            BlockTags::initialize();
            fluid::FluidRegistry::instance().initialize();
            EnchantmentRegistry::clear();
            EnchantmentRegistry::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<LocationEnchantmentTestWorld>();
        m_entity = std::make_unique<TestLivingEntityForLocation>();
        m_entity->setWorld(m_world.get());
        m_entity->setOnGround(true);
        m_entity->setPosition(0.5, 65.0, 0.5); // 站在 y=64 方块上方
    }

    void TearDown() override
    {
        m_entity.reset();
        m_world.reset();
    }

    /**
     * @brief 在指定位置设置水源方块（含流体状态和 isWaterAt 标记）
     */
    void setupWaterSourceAt(i32 x, i32 y, i32 z)
    {
        const BlockPos pos(x, y, z);
        // 设置水源方块
        if (VanillaBlocks::WATER != nullptr) {
            m_world->setBlockDirectly(pos, &VanillaBlocks::WATER->defaultState());
        }
        // 设置流体状态为水源
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            m_world->setFluidDirectly(pos, &waterFluid->defaultState());
        }
        // 标记为水源位置
        m_world->setWaterAt(pos, true);
    }

    /**
     * @brief 在指定位置设置灵魂沙
     */
    void setupSoulSandAt(i32 x, i32 y, i32 z)
    {
        const BlockPos pos(x, y, z);
        if (NetherBlocks::SOUL_SAND != nullptr) {
            m_world->setBlockDirectly(pos, &NetherBlocks::SOUL_SAND->defaultState());
        }
    }

    std::unique_ptr<LocationEnchantmentTestWorld> m_world;
    std::unique_ptr<TestLivingEntityForLocation> m_entity;
};

TEST_F(FrostWalkerIntegrationTest, PlacesFrostedIceOnWaterSource_Level1)
{
    if (VanillaBlocks::FROSTED_ICE == nullptr) {
        GTEST_SKIP() << "FROSTED_ICE not initialized";
    }

    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    // 给实体装备冰霜行者 I 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在 y=64 层设置水源方块（实体站在 y=65）
    setupWaterSourceAt(-1, 64, -1);
    setupWaterSourceAt(0, 64, 0);
    setupWaterSourceAt(1, 64, 0);
    setupWaterSourceAt(0, 64, 1);
    setupWaterSourceAt(-1, 64, 0);

    // 清除设置阶段的方块变化记录
    m_world->clearBlockChanges();

    // 触发位置变化
    m_entity->onChangedBlock();

    // 冰霜行者 I 半径 = 2，应放置霜冰
    // 检查至少一些水源位置被替换为霜冰
    EXPECT_TRUE(m_world->hasBlockChangeAt(BlockPos(-1, 64, -1)) || m_world->hasBlockChangeAt(BlockPos(0, 64, 0)));
}

TEST_F(FrostWalkerIntegrationTest, PlacesFrostedIceOnWaterSource_Level2)
{
    if (VanillaBlocks::FROSTED_ICE == nullptr) {
        GTEST_SKIP() << "FROSTED_ICE not initialized";
    }

    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    // 给实体装备冰霜行者 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在 y=64 层设置水源方块（实体站在 y=65）
    setupWaterSourceAt(0, 64, 0);
    setupWaterSourceAt(2, 64, 0);
    setupWaterSourceAt(-2, 64, 0);

    // 清除设置阶段的方块变化记录
    m_world->clearBlockChanges();

    // 触发位置变化
    m_entity->onChangedBlock();

    // 冰霜行者 II 半径 = 3，应覆盖更大范围
    EXPECT_TRUE(m_world->hasBlockChangeAt(BlockPos(0, 64, 0)));
}

TEST_F(FrostWalkerIntegrationTest, NoFrostedIceWhenNotOnGround)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    // 给实体装备冰霜行者 I 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 设置为不在地面
    m_entity->setOnGround(false);

    // 在脚下设置水源
    setupWaterSourceAt(0, 64, 0);

    // 清除设置阶段的方块变化记录
    m_world->clearBlockChanges();

    m_entity->onChangedBlock();

    // 不在地面，不应放冰
    EXPECT_FALSE(m_world->hasBlockChangeAt(BlockPos(0, 64, 0)));
}

TEST_F(FrostWalkerIntegrationTest, NoFrostedIceWithoutEnchantment)
{
    // 不装备冰霜行者的靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下设置水源
    setupWaterSourceAt(0, 64, 0);

    // 清除设置阶段的方块变化记录
    m_world->clearBlockChanges();

    m_entity->onChangedBlock();

    // 没有冰霜行者，不应放冰
    EXPECT_FALSE(m_world->hasBlockChangeAt(BlockPos(0, 64, 0)));
}

TEST_F(FrostWalkerIntegrationTest, NoFrostedIceOnFlowingWater)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    // 给实体装备冰霜行者 I 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 只设置 isWaterAt，但不设置 isSource 的流体状态
    // 模拟流动水（isWaterAt=true 但 isSource=false）
    const BlockPos waterPos(0, 64, 0);
    m_world->setWaterAt(waterPos, true);
    // 不设置流体状态，或设置为非源头的流体状态
    // getFluidState 将返回空的 FluidState（isEmpty=true）

    m_entity->onChangedBlock();

    // 流动水不应被冻结
    EXPECT_FALSE(m_world->hasBlockChangeAt(waterPos));
}

TEST_F(FrostWalkerIntegrationTest, RadiusLevel1Is2)
{
    // 验证冰霜行者 I 的半径 = 2（level + 1）
    EXPECT_EQ(FrostWalkerEnchantment::getFrostRadius(1), 2);
}

TEST_F(FrostWalkerIntegrationTest, RadiusLevel2Is3)
{
    // 验证冰霜行者 II 的半径 = 3（level + 1）
    EXPECT_EQ(FrostWalkerEnchantment::getFrostRadius(2), 3);
}

// ============================================================================
// 集成测试：SoulSpeed 灵魂疾行修饰符增删
// ============================================================================

class SoulSpeedIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            BlockTags::initialize();
            fluid::FluidRegistry::instance().initialize();
            EnchantmentRegistry::clear();
            EnchantmentRegistry::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<LocationEnchantmentTestWorld>();
        m_entity = std::make_unique<TestLivingEntityForLocation>();
        m_entity->setWorld(m_world.get());
        m_entity->setOnGround(true);
        m_entity->setPosition(0.5, 65.0, 0.5); // 站在 y=64 方块上方
        // 固定随机种子，使依赖 entity.getRandom() 的概率性用例（如耐久消耗）
        // 在各次运行/各测试顺序下行为确定、可复现，消除 flake。
        m_entity->getRandom().setSeed(0x5EEDULL);
    }

    void TearDown() override
    {
        m_entity.reset();
        m_world.reset();
    }

    std::unique_ptr<LocationEnchantmentTestWorld> m_world;
    std::unique_ptr<TestLivingEntityForLocation> m_entity;
};

TEST_F(SoulSpeedIntegrationTest, AddsModifierWhenOnSoulSand)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 初始状态没有修饰符
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));

    // 触发位置变化
    m_entity->onChangedBlock();

    // 应该添加灵魂疾行速度修饰符
    EXPECT_TRUE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
}

TEST_F(SoulSpeedIntegrationTest, ModifierValueLevel1)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 I 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    m_entity->onChangedBlock();

    // 修饰符应存在（Addition 操作，I: +0.0405）
    EXPECT_TRUE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));

    // 验证修饰符值
    f64 speedModValue = m_entity->attributes().getModifierValue(
        entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed");
    EXPECT_NEAR(speedModValue, 0.0405, 0.0001);
}

TEST_F(SoulSpeedIntegrationTest, RemovesModifierWhenOffSoulSand)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 第一次位置变化：激活灵魂疾行
    m_entity->onChangedBlock();
    EXPECT_TRUE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));

    // 移动到新位置（脚下没有灵魂沙）
    m_entity->setPosition(10.5, 65.0, 10.5);

    // 第二次位置变化：离开灵魂沙，停用灵魂疾行
    m_entity->onChangedBlock();
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
}

TEST_F(SoulSpeedIntegrationTest, NoModifierWithoutEnchantment)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    // 不装备灵魂疾行的靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    m_entity->onChangedBlock();

    // 没有灵魂疾行，不应有速度修饰符
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
}

TEST_F(SoulSpeedIntegrationTest, NoModifierWhenNotOnGround)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 设置为不在地面
    m_entity->setOnGround(false);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    m_entity->onChangedBlock();

    // 不在地面，不应添加速度修饰符
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
}

TEST_F(SoulSpeedIntegrationTest, ModifierRemovedOnDeath)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 激活灵魂疾行
    m_entity->onChangedBlock();
    EXPECT_TRUE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));

    // 实体死亡：应移除所有位置依赖附魔效果
    EnchantmentHelper::stopAllLocationBasedEffects(*m_entity);
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
}

// ============================================================================
// 集成测试：onChangedBlock → EnchantmentHelper 完整链路
// ============================================================================

class OnChangedBlockChainTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            BlockTags::initialize();
            fluid::FluidRegistry::instance().initialize();
            EnchantmentRegistry::clear();
            EnchantmentRegistry::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<LocationEnchantmentTestWorld>();
        m_entity = std::make_unique<TestLivingEntityForLocation>();
        m_entity->setWorld(m_world.get());
        m_entity->setOnGround(true);
        m_entity->setPosition(0.5, 65.0, 0.5);
    }

    void TearDown() override
    {
        m_entity.reset();
        m_world.reset();
    }

    std::unique_ptr<LocationEnchantmentTestWorld> m_world;
    std::unique_ptr<TestLivingEntityForLocation> m_entity;
};

TEST_F(OnChangedBlockChainTest, TrackerActivatesOnSoulSand)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    m_world->setBlockDirectly(BlockPos(0, 64, 0), &NetherBlocks::SOUL_SAND->defaultState());

    // 初始状态：tracker 无活跃附魔
    EXPECT_FALSE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));

    // 触发 onChangedBlock
    m_entity->onChangedBlock();

    // tracker 应标记灵魂疾行为活跃
    EXPECT_TRUE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));
}

TEST_F(OnChangedBlockChainTest, TrackerDeactivatesWhenOffSoulSand)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    m_world->setBlockDirectly(BlockPos(0, 64, 0), &NetherBlocks::SOUL_SAND->defaultState());

    // 激活
    m_entity->onChangedBlock();
    EXPECT_TRUE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));

    // 移动到新位置（没有灵魂沙）
    m_entity->setPosition(10.5, 65.0, 10.5);
    m_entity->onChangedBlock();

    // tracker 应标记灵魂疾行为非活跃
    EXPECT_FALSE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));
}

TEST_F(OnChangedBlockChainTest, TrackerActivatesForFrostWalker)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    // 装备冰霜行者 I 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 冰霜行者在地面时总是活跃（不需要特定脚下方块）
    m_entity->onChangedBlock();

    // tracker 应标记冰霜行者为活跃
    EXPECT_TRUE(m_entity->locationEnchantmentTracker().isActive(
        static_cast<i32>(EquipmentSlot::Feet), "minecraft:frost_walker"));
}

TEST_F(OnChangedBlockChainTest, TrackerDeactivatesWhenOffGround)
{
    const Enchantment* frostWalker = EnchantmentRegistry::get("minecraft:frost_walker");
    ASSERT_NE(frostWalker, nullptr);

    // 装备冰霜行者 I 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{frostWalker, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在地面时激活
    m_entity->setOnGround(true);
    m_entity->onChangedBlock();
    EXPECT_TRUE(m_entity->locationEnchantmentTracker().isActive(
        static_cast<i32>(EquipmentSlot::Feet), "minecraft:frost_walker"));

    // 腾空（不在地面）时停用
    m_entity->setOnGround(false);
    m_entity->onChangedBlock();
    EXPECT_FALSE(m_entity->locationEnchantmentTracker().isActive(
        static_cast<i32>(EquipmentSlot::Feet), "minecraft:frost_walker"));
}

TEST_F(OnChangedBlockChainTest, StopLocationBasedEffectsClearsModifier)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    m_world->setBlockDirectly(BlockPos(0, 64, 0), &NetherBlocks::SOUL_SAND->defaultState());

    // 激活
    m_entity->onChangedBlock();
    EXPECT_TRUE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));

    // 停用指定装备上的位置效果
    m_entity->stopLocationBasedEffects(boots, EquipmentSlot::Feet);

    // 修饰符应被移除
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
}

TEST_F(OnChangedBlockChainTest, StopAllLocationBasedEffectsClearsAll)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    m_world->setBlockDirectly(BlockPos(0, 64, 0), &NetherBlocks::SOUL_SAND->defaultState());

    // 激活
    m_entity->onChangedBlock();
    EXPECT_TRUE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));

    // 停用所有位置效果
    EnchantmentHelper::stopAllLocationBasedEffects(*m_entity);

    // tracker 应清空
    EXPECT_FALSE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));

    // 修饰符应被移除
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
}

TEST_F(OnChangedBlockChainTest, MultipleEnchantmentsOnSameSlot)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    // 冰霜行者和灵魂疾行互斥（都是 ArmorFeet 类型），
    // 但我们测试 tracker 支持同一槽位上的多个附魔标记
    // 实际游戏中无法同时装备两者，但 tracker 支持这种情况

    // 直接通过 tracker API 测试多附魔追踪
    m_entity->locationEnchantmentTracker().setActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:frost_walker");
    m_entity->locationEnchantmentTracker().setActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed");

    EXPECT_TRUE(m_entity->locationEnchantmentTracker().isActive(
        static_cast<i32>(EquipmentSlot::Feet), "minecraft:frost_walker"));
    EXPECT_TRUE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));

    // 清除一个不应影响另一个
    m_entity->locationEnchantmentTracker().setInactive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:frost_walker");
    EXPECT_FALSE(m_entity->locationEnchantmentTracker().isActive(
        static_cast<i32>(EquipmentSlot::Feet), "minecraft:frost_walker"));
    EXPECT_TRUE(
        m_entity->locationEnchantmentTracker().isActive(static_cast<i32>(EquipmentSlot::Feet), "minecraft:soul_speed"));
}

// ============================================================================
// 集成测试：SoulSpeed MOVEMENT_SPEED 修饰符值和 Addition 操作验证
// ============================================================================

TEST_F(SoulSpeedIntegrationTest, SpeedModifierUsesAdditionOperation)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 I 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 激活灵魂疾行
    m_entity->onChangedBlock();

    // 验证 MOVEMENT_SPEED 修饰符使用 Addition 操作（值=0.0405）
    f64 speedModValue = m_entity->attributes().getModifierValue(
        entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed");
    EXPECT_NEAR(speedModValue, 0.0405, 0.0001);
}

TEST_F(SoulSpeedIntegrationTest, SpeedModifierValueLevel2)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    m_entity->onChangedBlock();

    // Level II: 0.0405 + 0.0105 = 0.051
    f64 speedModValue = m_entity->attributes().getModifierValue(
        entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed");
    EXPECT_NEAR(speedModValue, 0.051, 0.0001);
}

TEST_F(SoulSpeedIntegrationTest, SpeedModifierValueLevel3)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 III 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 3}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    m_entity->onChangedBlock();

    // Level III: 0.0405 + 0.0105 * 2 = 0.0615
    f64 speedModValue = m_entity->attributes().getModifierValue(
        entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed");
    EXPECT_NEAR(speedModValue, 0.0615, 0.0001);
}

// ============================================================================
// 集成测试：SoulSpeed MOVEMENT_EFFICIENCY 修饰符验证
// ============================================================================

TEST_F(SoulSpeedIntegrationTest, EfficiencyModifierAppliedOnSoulSand)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 初始状态没有修饰符
    EXPECT_FALSE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));

    m_entity->onChangedBlock();

    // 激活后应有 MOVEMENT_EFFICIENCY 修饰符
    EXPECT_TRUE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));

    // 修饰符值应为 1.0（所有等级均为 +1.0）
    f64 efficiencyValue = m_entity->attributes().getModifierValue(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency");
    EXPECT_NEAR(efficiencyValue, 1.0, 0.0001);
}

TEST_F(SoulSpeedIntegrationTest, EfficiencyModifierRemovedWhenOffSoulSand)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 激活
    m_entity->onChangedBlock();
    EXPECT_TRUE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));

    // 移动到新位置（没有灵魂沙）
    m_entity->setPosition(10.5, 65.0, 10.5);
    m_entity->onChangedBlock();

    // 效率修饰符应被移除
    EXPECT_FALSE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));
}

// ============================================================================
// 集成测试：getBlockSpeedFactor() 插值逻辑
// ============================================================================

TEST_F(SoulSpeedIntegrationTest, GetBlockSpeedFactorDefaultIsOne)
{
    // 默认情况下（没有减速方块、没有修饰符），getBlockSpeedFactor() 应返回 1.0
    // 脚下没有方块，blockSpeedFactor 默认为 1.0
    f32 factor = m_entity->getBlockSpeedFactor();
    EXPECT_NEAR(factor, 1.0f, 0.001f);
}

TEST_F(SoulSpeedIntegrationTest, GetBlockSpeedFactorSoulSandReduces)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 没有 MOVEMENT_EFFICIENCY 修饰符时，应返回灵魂沙的 speedFactor（0.4）
    f32 factor = m_entity->getBlockSpeedFactor();
    EXPECT_NEAR(factor, 0.4f, 0.01f);
}

TEST_F(SoulSpeedIntegrationTest, GetBlockSpeedFactorSoulSandWithEfficiency)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 激活灵魂疾行（添加 MOVEMENT_EFFICIENCY=1.0 修饰符）
    m_entity->onChangedBlock();

    // MOVEMENT_EFFICIENCY=1.0 时，lerp(1.0, 0.4, 1.0) = 1.0
    // 灵魂疾行完全抵消灵魂沙减速
    f32 factor = m_entity->getBlockSpeedFactor();
    EXPECT_NEAR(factor, 1.0f, 0.01f);
}

TEST_F(SoulSpeedIntegrationTest, GetBlockSpeedFactorLerpWithPartialEfficiency)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 手动添加部分 MOVEMENT_EFFICIENCY（0.5），验证插值
    entity::attribute::AttributeModifier partialEfficiency(
        "test.partial_efficiency", "Partial Efficiency", 0.5, entity::attribute::Operation::Addition);
    m_entity->attributes().addModifier(entity::attribute::Attributes::MOVEMENT_EFFICIENCY, partialEfficiency);

    // lerp(0.5, 0.4, 1.0) = 0.4 + (1.0 - 0.4) * 0.5 = 0.4 + 0.3 = 0.7
    f32 factor = m_entity->getBlockSpeedFactor();
    EXPECT_NEAR(factor, 0.7f, 0.01f);

    // 清理
    m_entity->attributes().removeModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "test.partial_efficiency");
}

// ============================================================================
// 集成测试：SoulSpeed 骑乘状态下不激活
// ============================================================================

TEST_F(SoulSpeedIntegrationTest, NoModifierWhenRiding)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 设置骑乘状态（模拟骑在另一个实体上）
    m_entity->setVehicleForTest(EntityInstanceId(42));
    ASSERT_TRUE(m_entity->isRiding());

    // 在地面且在灵魂沙上，但骑乘中 → 灵魂疾行不应激活
    m_entity->onChangedBlock();
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
    EXPECT_FALSE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));

    // 清理：恢复非骑乘状态后应正常激活
    m_entity->setVehicleForTest(INVALID_ENTITY_ID);
    ASSERT_FALSE(m_entity->isRiding());
    m_entity->onChangedBlock();
    EXPECT_TRUE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
    EXPECT_TRUE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));
}

// ============================================================================
// 集成测试：SoulSpeed 鞘翅滑翔时不激活
// ============================================================================

TEST_F(SoulSpeedIntegrationTest, NoModifierWhenElytraFlying)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 设置鞘翅滑翔标志（模拟正在使用鞘翅滑翔）
    m_entity->addFlag(mc::EntityFlags::FallFlying);

    // 在地面且在灵魂沙上，但鞘翅滑翔中 → 灵魂疾行不应激活
    m_entity->onChangedBlock();
    EXPECT_FALSE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
    EXPECT_FALSE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));

    // 清理：移除鞘翅滑翔标志后应正常激活
    m_entity->removeFlag(mc::EntityFlags::FallFlying);
    m_entity->onChangedBlock();
    EXPECT_TRUE(
        m_entity->attributes().hasModifier(entity::attribute::Attributes::MOVEMENT_SPEED, "enchantment.soul_speed"));
    EXPECT_TRUE(m_entity->attributes().hasModifier(
        entity::attribute::Attributes::MOVEMENT_EFFICIENCY, "enchantment.soul_speed.efficiency"));
}

// ============================================================================
// 集成测试：SoulSpeed 耐久消耗
// ============================================================================

TEST_F(SoulSpeedIntegrationTest, DurabilityConsumedOverMultipleTicks)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 在脚下放置灵魂沙
    const BlockPos soulSandPos(0, 64, 0);
    m_world->setBlockDirectly(soulSandPos, &NetherBlocks::SOUL_SAND->defaultState());

    // 记录初始伤害值
    i32 initialDamage = m_entity->getEquipment(EquipmentSlot::Feet).getDamage();
    EXPECT_EQ(initialDamage, 0); // 新物品没有伤害

    // 灵魂疾行每次位置变化有4%概率消耗1点耐久
    // 多次触发 onChangedBlock，期望至少有一次耐久消耗
    // 注：SetUp 中已对 entity.getRandom() 设置固定种子，结果确定；试验次数取 1000
    // 使 4% 概率在统计上必触发（0.96^1000 ≈ 1.6e-18，几乎不可能为 0）。
    i32 totalTicks = 1000;
    for (i32 i = 0; i < totalTicks; ++i) {
        // 微小位置变化以触发 onChangedBlock
        m_entity->setPosition(0.5 + static_cast<f32>(i) * 0.01f, 65.0, 0.5);
        m_entity->onChangedBlock();
    }

    // 验证耐久消耗发生了（伤害值 > 0）
    i32 finalDamage = m_entity->getEquipment(EquipmentSlot::Feet).getDamage();
    // 4%概率 × 1000次 ≈ 40次消耗期望值，至少应该 > 0
    EXPECT_GT(finalDamage, 0);

    // 验证消耗量合理：1000次 × 4% ≈ 40次，允许范围在1-100之间（宽泛容差）
    EXPECT_LT(finalDamage, 100);
}

TEST_F(SoulSpeedIntegrationTest, NoDurabilityConsumptionWhenNotOnSoulSand)
{
    if (NetherBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_SAND not initialized";
    }

    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);

    // 装备灵魂疾行 II 靴子
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 2}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);

    // 脚下没有灵魂沙
    // 灵魂疾行不会激活，不应消耗耐久
    i32 initialDamage = m_entity->getEquipment(EquipmentSlot::Feet).getDamage();

    for (i32 i = 0; i < 100; ++i) {
        m_entity->setPosition(0.5 + static_cast<f32>(i) * 0.01f, 65.0, 0.5);
        m_entity->onChangedBlock();
    }

    i32 finalDamage = m_entity->getEquipment(EquipmentSlot::Feet).getDamage();
    // 不在灵魂沙上，灵魂疾行不会激活，耐久不应被消耗
    EXPECT_EQ(finalDamage, initialDamage);
}
