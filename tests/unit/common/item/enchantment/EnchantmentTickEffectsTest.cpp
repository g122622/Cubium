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
 * @file EnchantmentTickEffectsTest.cpp
 * @brief 附魔每 tick 效果派发（EnchantmentHelper::runTickEffects）与灵魂疾行 TICK 组件
 *
 * 覆盖：
 * - runTickEffects 遍历全部装备槽位并对每个附魔调用一次 onTick；
 * - 空槽位与空附魔列表不派发；
 * - 基类 Enchantment::onTick 为无操作（不崩溃、不产生副作用）；
 * - 灵魂疾行 onTick 仅在周期性 tick、着地、非飞行、脚踩灵魂方块、水平移动时产出粒子；
 * - 灵魂疾行粒子位置取碰撞箱内水平采样、速度按 movement_scale(-0.2) 缩放且垂直固定 0.1；
 * - 灵魂疾行 onTick 只作用于脚部槽位。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/item/enchantment/enchantments/special/SoulSpeedEnchantment.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::block_registry;
using namespace mc::entity;
using namespace mc::item::enchant;

namespace {

/**
 * @brief 记录粒子与音效的最小测试世界
 *
 * 灵魂疾行只在服务端产出粒子/音效，故 isClientSide() 返回 false；
 * 脚下方块由测试显式设置，其余坐标为空气。
 */
class TickEffectsTestWorld final : public mc::test::BaseTestWorld {
public:
    struct ParticleRecord {
        particle::ParticleTypeId type;
        Vector3 position;
        Vector3 velocity;
    };

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] bool isClientSide() const override { return false; }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void setBlock(const BlockPos& pos, const BlockState* state) { m_blocks[pos] = state; }

    [[nodiscard]] const std::vector<ParticleRecord>& particles() const { return m_particles; }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::vector<ParticleRecord> m_particles;
};

/**
 * @brief 用于计数 onTick 派发的探针附魔
 */
class TickProbeEnchantment final : public Enchantment {
public:
    [[nodiscard]] std::string id() const override { return "test:tick_probe"; }
    [[nodiscard]] std::string getNameKey(i32) const override { return "enchantment.test.tick_probe"; }
    [[nodiscard]] EnchantmentType type() const override { return EnchantmentType::All; }
    [[nodiscard]] i32 minLevel() const noexcept override { return 1; }
    [[nodiscard]] i32 maxLevel() const noexcept override { return 1; }

    void onTick(LivingEntity& entity, const ItemStack& stack, EquipmentSlot slot, i32 level) const override
    {
        ++m_calls;
        m_lastEntity = &entity;
        m_lastSlot = slot;
        m_lastLevel = level;
        m_lastStackPresent = !stack.isEmpty();
    }

    mutable i32 m_calls = 0;
    mutable const LivingEntity* m_lastEntity = nullptr;
    mutable EquipmentSlot m_lastSlot = EquipmentSlot::Count;
    mutable i32 m_lastLevel = 0;
    mutable bool m_lastStackPresent = false;
};

/**
 * @brief 位置依赖附魔测试用实体
 */
class TickEffectsTestEntity final : public LivingEntity {
public:
    TickEffectsTestEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerData();
        registerAttributes();
        setHealth(maxHealth());
    }

    /// 测试辅助：直接设定已存在刻数（决定 TICK 组件的周期刻采样点）。
    void setTicksExistedForTest(u32 ticks) { m_ticksExisted = ticks; }
};

class EnchantmentTickEffectsTest : public ::testing::Test {
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
        m_world = std::make_unique<TickEffectsTestWorld>();
        m_entity = std::make_unique<TickEffectsTestEntity>();
        m_entity->setWorld(m_world.get());
        m_entity->setPosition(0.5, 65.0, 0.5);
        m_entity->setOnGround(true);
    }

    void TearDown() override
    {
        m_entity->setWorld(nullptr);
        m_entity.reset();
        m_world.reset();
    }

    /// 装备灵魂疾行靴子并站在灵魂沙上（不触发位置检测，活跃状态无关 TICK）。
    void equipSoulSpeedBoots(i32 level = 1)
    {
        const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
        ASSERT_NE(soulSpeed, nullptr);
        ItemStack boots(Items::DIAMOND_BOOTS, 1);
        EnchantmentHelper::setEnchantments({{soulSpeed, level}}, boots);
        m_entity->setEquipment(EquipmentSlot::Feet, boots);
        m_world->setBlock(BlockPos(0, 64, 0), &NetherBlocks::SOUL_SAND->defaultState());
    }

    std::unique_ptr<TickEffectsTestWorld> m_world;
    std::unique_ptr<TickEffectsTestEntity> m_entity;
};

TEST_F(EnchantmentTickEffectsTest, BaseOnTickIsNoOp)
{
    const Enchantment* sharpness = EnchantmentRegistry::get("minecraft:sharpness");
    ASSERT_NE(sharpness, nullptr);

    ItemStack sword(Items::DIAMOND_SWORD, 1);
    EnchantmentHelper::setEnchantments({{sharpness, 1}}, sword);
    m_entity->setEquipment(EquipmentSlot::MainHand, sword);

    // 基类默认实现为空操作：派发不应崩溃，也不产生粒子。
    EXPECT_NO_THROW(EnchantmentHelper::runTickEffects(*m_entity));
    EXPECT_TRUE(m_world->particles().empty());
}

TEST_F(EnchantmentTickEffectsTest, DispatchesToEveryEquippedEnchantment)
{
    auto ownedProbe = std::make_unique<TickProbeEnchantment>();
    TickProbeEnchantment* probe = ownedProbe.get();
    // 注册表按 id 反查附魔实例，getEnchantments 据此把物品上的附魔还原为探针对象。
    ASSERT_TRUE(EnchantmentRegistry::registerEnchantment(std::move(ownedProbe)));

    ItemStack sword(Items::DIAMOND_SWORD, 1);
    sword.addEnchantment(probe->id(), 1);
    m_entity->setEquipment(EquipmentSlot::MainHand, sword);

    EnchantmentHelper::runTickEffects(*m_entity);

    ASSERT_EQ(probe->m_calls, 1);
    EXPECT_EQ(probe->m_lastEntity, m_entity.get());
    EXPECT_EQ(probe->m_lastSlot, EquipmentSlot::MainHand);
    EXPECT_EQ(probe->m_lastLevel, 1);
    EXPECT_TRUE(probe->m_lastStackPresent);
}

TEST_F(EnchantmentTickEffectsTest, EmptyEquipmentDispatchesNothing)
{
    // 不装备任何物品：派发不崩溃且不产生粒子。
    EXPECT_NO_THROW(EnchantmentHelper::runTickEffects(*m_entity));
    EXPECT_TRUE(m_world->particles().empty());
}

TEST_F(EnchantmentTickEffectsTest, SoulSpeedSpawnsParticleOnPeriodicTick)
{
    equipSoulSpeedBoots();
    m_entity->setTicksExistedForTest(5);
    m_entity->setVelocity(0.2f, 0.0f, -0.1f);

    EnchantmentHelper::runTickEffects(*m_entity);

    const auto& particles = m_world->particles();
    ASSERT_EQ(particles.size(), 1u);
    EXPECT_EQ(particles[0].type, particle::ParticleTypeId::Soul);

    // 水平位置：实体位置 + (nextDouble()-0.5) * 碰撞箱宽度（宽度 0.6 → 偏移 ∈ [-0.3, 0.3)）。
    EXPECT_GE(particles[0].position.x, 0.5f - 0.3f);
    EXPECT_LT(particles[0].position.x, 0.5f + 0.3f);
    EXPECT_GE(particles[0].position.z, 0.5f - 0.3f);
    EXPECT_LT(particles[0].position.z, 0.5f + 0.3f);
    // 垂直位置：实体位置 + 0.1。
    EXPECT_NEAR(particles[0].position.y, 65.1f, 1e-5f);
    // 水平速度按 movement_scale(-0.2) 缩放，垂直速度固定 0.1。
    EXPECT_NEAR(particles[0].velocity.x, 0.2f * -0.2f, 1e-6f);
    EXPECT_NEAR(particles[0].velocity.y, 0.1f, 1e-6f);
    EXPECT_NEAR(particles[0].velocity.z, -0.1f * -0.2f, 1e-6f);
}

TEST_F(EnchantmentTickEffectsTest, SoulSpeedSkipsNonPeriodicTick)
{
    equipSoulSpeedBoots();
    m_entity->setTicksExistedForTest(6);
    m_entity->setVelocity(0.2f, 0.0f, 0.0f);

    EnchantmentHelper::runTickEffects(*m_entity);

    EXPECT_TRUE(m_world->particles().empty());
}

TEST_F(EnchantmentTickEffectsTest, SoulSpeedSkipsWhenAirborne)
{
    equipSoulSpeedBoots();
    m_entity->setTicksExistedForTest(5);
    m_entity->setVelocity(0.2f, 0.0f, 0.0f);
    m_entity->setOnGround(false);

    EnchantmentHelper::runTickEffects(*m_entity);

    EXPECT_TRUE(m_world->particles().empty());
}

TEST_F(EnchantmentTickEffectsTest, SoulSpeedSkipsWhenNotMoving)
{
    equipSoulSpeedBoots();
    m_entity->setTicksExistedForTest(5);
    m_entity->setVelocity(0.0f, 0.5f, 0.0f);

    EnchantmentHelper::runTickEffects(*m_entity);

    EXPECT_TRUE(m_world->particles().empty());
}

TEST_F(EnchantmentTickEffectsTest, SoulSpeedSkipsWhenNotOnSoulBlock)
{
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);
    ItemStack boots(Items::DIAMOND_BOOTS, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 1}}, boots);
    m_entity->setEquipment(EquipmentSlot::Feet, boots);
    m_world->setBlock(BlockPos(0, 64, 0), &VanillaBlocks::STONE->defaultState());

    m_entity->setTicksExistedForTest(5);
    m_entity->setVelocity(0.2f, 0.0f, 0.0f);

    EnchantmentHelper::runTickEffects(*m_entity);

    EXPECT_TRUE(m_world->particles().empty());
}

TEST_F(EnchantmentTickEffectsTest, SoulSpeedTickOnlyAppliesToFeetSlot)
{
    // 灵魂疾行残存于主手时，TICK 组件不应产出粒子（原版 TICK 仅作用于 feet 槽）。
    const Enchantment* soulSpeed = EnchantmentRegistry::get("minecraft:soul_speed");
    ASSERT_NE(soulSpeed, nullptr);
    ItemStack sword(Items::DIAMOND_SWORD, 1);
    EnchantmentHelper::setEnchantments({{soulSpeed, 1}}, sword);
    m_entity->setEquipment(EquipmentSlot::MainHand, sword);
    m_world->setBlock(BlockPos(0, 64, 0), &NetherBlocks::SOUL_SAND->defaultState());

    m_entity->setTicksExistedForTest(5);
    m_entity->setVelocity(0.2f, 0.0f, 0.0f);

    EnchantmentHelper::runTickEffects(*m_entity);

    EXPECT_TRUE(m_world->particles().empty());
}

} // namespace