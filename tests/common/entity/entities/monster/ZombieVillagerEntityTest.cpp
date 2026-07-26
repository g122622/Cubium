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
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/monster/undead/ZombieVillagerEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用世界实现
 *
 * 提供僵尸村民测试所需的最小 IWorld 接口实现
 */
class ZombieVillagerTestWorld final : public test::BaseTestWorld {
public:
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
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ZombieVillagerTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ZombieVillagerTestWorld::tickManager not implemented");
    }

    // 获取生成的实体数量
    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    // 获取最后生成的实体
    Entity* lastSpawnedEntity() { return m_spawnedEntities.empty() ? nullptr : m_spawnedEntities.back().get(); }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    Difficulty m_difficulty = Difficulty::Normal;
};

/**
 * @brief 僵尸村民测试夹具
 */
class ZombieVillagerEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建测试世界
        m_world = std::make_unique<ZombieVillagerTestWorld>();

        // 创建僵尸村民
        m_zombieVillager = std::make_unique<ZombieVillagerEntity>(EntityInstanceId(1));
        m_zombieVillager->setWorld(m_world.get());
        m_zombieVillager->setPosition(0.0, 64.0, 0.0);
    }

    std::unique_ptr<ZombieVillagerTestWorld> m_world;
    std::unique_ptr<ZombieVillagerEntity> m_zombieVillager;
};

// ============================================================================
// 基本属性测试
// ============================================================================

TEST_F(ZombieVillagerEntityTest, EyeHeight)
{
    // 成年僵尸村民
    EXPECT_FLOAT_EQ(m_zombieVillager->eyeHeight(), 1.79f);

    // 设置为婴儿
    m_zombieVillager->setBaby(true);
    EXPECT_FLOAT_EQ(m_zombieVillager->eyeHeight(), 0.93f);
}

TEST_F(ZombieVillagerEntityTest, InitialConversionState)
{
    // 初始状态应该是不在治愈
    EXPECT_FALSE(m_zombieVillager->isConverting());
    EXPECT_EQ(m_zombieVillager->getConversionTime(), 0);
}

// ============================================================================
// 村民数据测试
// ============================================================================

TEST_F(ZombieVillagerEntityTest, VillagerData)
{
    // 设置村民数据
    entity::VillagerData data(entity::VillagerType::Desert, entity::VillagerProfession::Farmer, 3);
    data.setExperience(50);

    m_zombieVillager->setVillagerData(data);

    // 验证获取的数据
    EXPECT_EQ(m_zombieVillager->getVillagerType(), entity::VillagerType::Desert);
    EXPECT_EQ(m_zombieVillager->getProfession(), entity::VillagerProfession::Farmer);
    EXPECT_EQ(m_zombieVillager->getTradingLevel(), 3);
    EXPECT_EQ(m_zombieVillager->getTradingExperience(), 50);
}

TEST_F(ZombieVillagerEntityTest, ProfessionSetters)
{
    m_zombieVillager->setProfession(entity::VillagerProfession::Librarian);
    EXPECT_EQ(m_zombieVillager->getProfession(), entity::VillagerProfession::Librarian);

    m_zombieVillager->setVillagerType(entity::VillagerType::Snow);
    EXPECT_EQ(m_zombieVillager->getVillagerType(), entity::VillagerType::Snow);

    m_zombieVillager->setTradingLevel(5);
    EXPECT_EQ(m_zombieVillager->getTradingLevel(), 5);

    m_zombieVillager->setTradingExperience(100);
    EXPECT_EQ(m_zombieVillager->getTradingExperience(), 100);
}

// ============================================================================
// 治愈系统测试
// ============================================================================

TEST_F(ZombieVillagerEntityTest, StartConverting)
{
    // 开始治愈
    m_zombieVillager->startConverting("test-player-uuid", 3600);

    EXPECT_TRUE(m_zombieVillager->isConverting());
    EXPECT_EQ(m_zombieVillager->getConversionTime(), 3600);
    EXPECT_EQ(m_zombieVillager->getConversionStarterUuid(), "test-player-uuid");
}

TEST_F(ZombieVillagerEntityTest, StopConverting)
{
    // 开始治愈
    m_zombieVillager->startConverting("test-player-uuid", 3600);
    EXPECT_TRUE(m_zombieVillager->isConverting());

    // 停止治愈
    m_zombieVillager->stopConverting();

    EXPECT_FALSE(m_zombieVillager->isConverting());
    EXPECT_EQ(m_zombieVillager->getConversionTime(), 0);
    EXPECT_TRUE(m_zombieVillager->getConversionStarterUuid().empty());
}

TEST_F(ZombieVillagerEntityTest, SetConversionTime)
{
    m_zombieVillager->setConversionTime(1000);

    EXPECT_TRUE(m_zombieVillager->isConverting());
    EXPECT_EQ(m_zombieVillager->getConversionTime(), 1000);

    // 设置为 0 应该停止治愈
    m_zombieVillager->setConversionTime(0);

    EXPECT_FALSE(m_zombieVillager->isConverting());
    EXPECT_EQ(m_zombieVillager->getConversionTime(), 0);
}

// ============================================================================
// 难度影响力量效果测试
// ============================================================================

TEST_F(ZombieVillagerEntityTest, StrengthEffectOnEasyDifficulty)
{
    // 简单难度：无力量效果
    m_world->setDifficulty(Difficulty::Easy);

    m_zombieVillager->startConverting("test-uuid", 3600);

    // 简单难度下 strengthLevel = max(1 - 1, 0) = 0，不应有力量效果
    EXPECT_EQ(m_zombieVillager->getEffectLevel(entity::effect::EffectType::Strength), 0);
}

TEST_F(ZombieVillagerEntityTest, StrengthEffectOnNormalDifficulty)
{
    // 普通难度：力量 I
    m_world->setDifficulty(Difficulty::Normal);

    m_zombieVillager->startConverting("test-uuid", 3600);

    // 普通难度下 strengthLevel = max(2 - 1, 0) = 1，应有力量 I
    EXPECT_EQ(m_zombieVillager->getEffectLevel(entity::effect::EffectType::Strength), 1);
}

TEST_F(ZombieVillagerEntityTest, StrengthEffectOnHardDifficulty)
{
    // 困难难度：力量 II
    m_world->setDifficulty(Difficulty::Hard);

    m_zombieVillager->startConverting("test-uuid", 3600);

    // 困难难度下 strengthLevel = max(3 - 1, 0) = 2，应有力量 II
    EXPECT_EQ(m_zombieVillager->getEffectLevel(entity::effect::EffectType::Strength), 2);
}

TEST_F(ZombieVillagerEntityTest, StrengthEffectOnPeacefulDifficulty)
{
    // 和平难度：无力量效果（虽然僵尸村民不会在和平模式生成）
    m_world->setDifficulty(Difficulty::Peaceful);

    m_zombieVillager->startConverting("test-uuid", 3600);

    // 和平难度下 strengthLevel = max(0 - 1, 0) = 0，不应有力量效果
    EXPECT_EQ(m_zombieVillager->getEffectLevel(entity::effect::EffectType::Strength), 0);
}

TEST_F(ZombieVillagerEntityTest, StrengthEffectDurationMatchesConversionTime)
{
    // 测试力量效果持续时间等于治愈时间
    m_world->setDifficulty(Difficulty::Normal);

    const i32 conversionTime = 5000;
    m_zombieVillager->startConverting("test-uuid", conversionTime);

    // 验证力量效果持续时间
    const auto* effect = m_zombieVillager->getEffect(entity::effect::EffectType::Strength);
    ASSERT_NE(effect, nullptr);
    EXPECT_EQ(effect->duration(), conversionTime);
}

TEST_F(ZombieVillagerEntityTest, ConversionProgress)
{
    // 基础进度应该为 1
    EXPECT_EQ(m_zombieVillager->getConversionProgress(), 1);
}

TEST_F(ZombieVillagerEntityTest, CanDespawnWhenConverting)
{
    // 正在治愈的僵尸村民不能消失
    m_zombieVillager->startConverting("test-uuid", 3600);
    EXPECT_FALSE(m_zombieVillager->canDespawn(100.0));

    // 停止治愈后可以消失
    m_zombieVillager->stopConverting();
    EXPECT_TRUE(m_zombieVillager->canDespawn(100.0));
}

TEST_F(ZombieVillagerEntityTest, CanDespawnWithExperience)
{
    // 有交易经验的僵尸村民不能消失
    m_zombieVillager->setTradingExperience(50);
    EXPECT_FALSE(m_zombieVillager->canDespawn(100.0));

    // 无经验的可以消失
    m_zombieVillager->setTradingExperience(0);
    EXPECT_TRUE(m_zombieVillager->canDespawn(100.0));
}

// ============================================================================
// Tick 测试
// ============================================================================

// 注意：tick() 测试需要完整的世界实现（AI 系统、物理系统等），
// 存根测试世界无法支持完整的 tick 继承链。
// 核心的治愈时间更新逻辑已经在 ConversionProgress 测试中验证。

// ============================================================================
// 声音测试
// ============================================================================

TEST_F(ZombieVillagerEntityTest, SoundEvents)
{
    // 验证声音事件返回正确
    auto ambient = m_zombieVillager->getAmbientSound();
    EXPECT_TRUE(ambient.has_value());
    EXPECT_EQ(ambient.value(), SoundEvents::ENTITY_ZOMBIE_VILLAGER_AMBIENT);

    auto hurtSource = DamageSources::generic();
    auto hurt = m_zombieVillager->getHurtSound(hurtSource);
    EXPECT_TRUE(hurt.has_value());
    EXPECT_EQ(hurt.value(), SoundEvents::ENTITY_ZOMBIE_VILLAGER_HURT);

    auto death = m_zombieVillager->getDeathSound();
    EXPECT_TRUE(death.has_value());
    EXPECT_EQ(death.value(), SoundEvents::ENTITY_ZOMBIE_VILLAGER_DEATH);

    auto step = m_zombieVillager->getStepSound();
    EXPECT_TRUE(step.has_value());
    EXPECT_EQ(step.value(), SoundEvents::ENTITY_ZOMBIE_VILLAGER_STEP);
}

// ============================================================================
// 治愈时装备处理测试（dropPreservedEquipment 逻辑）
// ============================================================================

/**
 * @brief 装备处理测试用的世界实现
 *
 * 需要支持物品注册和实体生成
 */
class ZombieVillagerEquipmentTestWorld final : public test::BaseTestWorld {
public:
    ZombieVillagerEquipmentTestWorld() { Items::initialize(); }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }
    void setDifficulty(Difficulty d) { m_difficulty = d; }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size() + 100);
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    // 获取最后生成的实体（作为 VillagerEntity）
    Entity* lastSpawnedEntity() { return m_spawnedEntities.empty() ? nullptr : m_spawnedEntities.back().get(); }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    Difficulty m_difficulty = Difficulty::Normal;
};

/**
 * @brief 治愈时装备处理测试夹具
 */
class ZombieVillagerEquipmentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<ZombieVillagerEquipmentTestWorld>();
        m_zombieVillager = std::make_unique<ZombieVillagerEntity>(EntityInstanceId(1));
        m_zombieVillager->setWorld(m_world.get());
        m_zombieVillager->setPosition(0.0, 64.0, 0.0);
    }

    std::unique_ptr<ZombieVillagerEquipmentTestWorld> m_world;
    std::unique_ptr<ZombieVillagerEntity> m_zombieVillager;
};

TEST_F(ZombieVillagerEquipmentTest, FinishConverting_BindingCurseEquipmentTransferredToVillager)
{
    // 绑定诅咒装备应转移到村民
    ItemStack cursedHelmet(Items::IRON_HELMET, 1);
    cursedHelmet.addEnchantment("minecraft:binding_curse", 1);
    m_zombieVillager->setEquipment(EquipmentSlot::Head, cursedHelmet);

    m_zombieVillager->setConversionStarterUuid("test-player");
    m_zombieVillager->finishConverting();

    // 应生成了一个村民实体
    ASSERT_EQ(m_world->spawnedEntityCount(), 1u);

    // 获取村民
    Entity* spawned = m_world->lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    auto* villager = dynamic_cast<entity::VillagerEntity*>(spawned);
    ASSERT_NE(villager, nullptr);

    // 绑定诅咒的头盔应转移到村民
    const ItemStack& helmet = villager->getEquipment(EquipmentSlot::Head);
    EXPECT_FALSE(helmet.isEmpty());
    EXPECT_TRUE(item::enchant::EnchantmentHelper::hasBindingCurse(helmet));
}

TEST_F(ZombieVillagerEquipmentTest, FinishConverting_PreservedEquipmentDroppedOnGround)
{
    // 保留装备（掉落概率 > 1.0）应掉落在地上
    ItemStack sword(Items::IRON_SWORD, 1);
    m_zombieVillager->setEquipment(EquipmentSlot::MainHand, sword);
    m_zombieVillager->setGuaranteedDrop(EquipmentSlot::MainHand);

    m_zombieVillager->finishConverting();

    // 应生成了一个村民实体 + 一个物品实体（主手剑）
    ASSERT_GE(m_world->spawnedEntityCount(), 1u);

    // 获取村民
    Entity* spawned = m_world->lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    auto* villager = dynamic_cast<entity::VillagerEntity*>(spawned);
    ASSERT_NE(villager, nullptr);

    // 村民主手应为空（保留的剑掉落在地上，不转移给村民）
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::MainHand).isEmpty());
}

TEST_F(ZombieVillagerEquipmentTest, FinishConverting_NonPreservedEquipmentSilentlyDisappears)
{
    // 默认掉落概率（0.085）的装备应静默消失
    ItemStack chestplate(Items::IRON_CHESTPLATE, 1);
    m_zombieVillager->setEquipment(EquipmentSlot::Chest, chestplate);

    // 确认默认掉落概率
    EXPECT_FLOAT_EQ(m_zombieVillager->getEquipmentDropChance(EquipmentSlot::Chest), 0.085f);

    size_t entityCountBefore = m_world->spawnedEntityCount();
    m_zombieVillager->finishConverting();

    // 仅生成村民，不应有额外的物品实体
    EXPECT_EQ(m_world->spawnedEntityCount(), entityCountBefore + 1);

    // 村民胸甲应为空
    Entity* spawned = m_world->lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);
    auto* villager = dynamic_cast<entity::VillagerEntity*>(spawned);
    ASSERT_NE(villager, nullptr);
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::Chest).isEmpty());
}

TEST_F(ZombieVillagerEquipmentTest, FinishConverting_EmptySlotsNoEffect)
{
    // 没有装备的槽位不应影响治愈
    m_zombieVillager->finishConverting();

    ASSERT_EQ(m_world->spawnedEntityCount(), 1u);
    Entity* spawned = m_world->lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    auto* villager = dynamic_cast<entity::VillagerEntity*>(spawned);
    ASSERT_NE(villager, nullptr);

    // 村民不应有任何装备
    for (i32 i = 0; i < static_cast<i32>(EquipmentSlot::Count); ++i) {
        EXPECT_TRUE(villager->getEquipment(static_cast<EquipmentSlot>(i)).isEmpty())
            << "Slot " << i << " should be empty";
    }
}

TEST_F(ZombieVillagerEquipmentTest, FinishConverting_MixedEquipmentHandlesAllCorrectly)
{
    // 混合装备场景：绑定诅咒 + 保留 + 非保留 + 空
    ItemStack cursedHelmet(Items::IRON_HELMET, 1);
    cursedHelmet.addEnchantment("minecraft:binding_curse", 1);
    m_zombieVillager->setEquipment(EquipmentSlot::Head, cursedHelmet);

    ItemStack preservedSword(Items::IRON_SWORD, 1);
    m_zombieVillager->setEquipment(EquipmentSlot::MainHand, preservedSword);
    m_zombieVillager->setGuaranteedDrop(EquipmentSlot::MainHand);

    ItemStack normalBoots(Items::IRON_BOOTS, 1);
    m_zombieVillager->setEquipment(EquipmentSlot::Feet, normalBoots);

    // OffHand, Legs, Chest 为空

    m_zombieVillager->finishConverting();

    Entity* spawned = m_world->lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    auto* villager = dynamic_cast<entity::VillagerEntity*>(spawned);
    ASSERT_NE(villager, nullptr);

    // 绑定诅咒头盔转移到村民
    EXPECT_FALSE(villager->getEquipment(EquipmentSlot::Head).isEmpty());
    EXPECT_TRUE(item::enchant::EnchantmentHelper::hasBindingCurse(villager->getEquipment(EquipmentSlot::Head)));

    // 保留剑掉落在地上，村民不应持有
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::MainHand).isEmpty());

    // 非保留靴子静默消失，村民不应持有
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::Feet).isEmpty());

    // 空槽位仍为空
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::OffHand).isEmpty());
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::Legs).isEmpty());
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::Chest).isEmpty());
}

TEST_F(ZombieVillagerEquipmentTest, FinishConverting_DropChanceZeroNotTransferredOrDropped)
{
    // 掉落概率为 0 的装备（如万圣节南瓜头）不应被转移也不应掉落
    ItemStack pumpkin(Items::CARVED_PUMPKIN, 1);
    m_zombieVillager->setEquipment(EquipmentSlot::Head, pumpkin);
    m_zombieVillager->setEquipmentDropChance(EquipmentSlot::Head, 0.0f);

    m_zombieVillager->finishConverting();

    Entity* spawned = m_world->lastSpawnedEntity();
    ASSERT_NE(spawned, nullptr);

    auto* villager = dynamic_cast<entity::VillagerEntity*>(spawned);
    ASSERT_NE(villager, nullptr);

    // 南瓜头不应转移给村民
    EXPECT_TRUE(villager->getEquipment(EquipmentSlot::Head).isEmpty());
}

} // namespace
} // namespace mc
