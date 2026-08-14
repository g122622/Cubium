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
 * @file NautilusEntityTest.cpp
 * @brief 鹦鹉螺实体测试
 *
 * 测试覆盖：
 * 1. NBT 序列化/反序列化（Items 列表 + Slot 索引模式）
 *    - 空物品栏不写入 Items 键
 *    - 鞍/铠甲完整 ItemStack 往返
 *    - DashCooldown 往返
 *    - 旧格式 SaddleItem 布尔标记向后兼容
 * 2. spawnBubbles() 气泡粒子生成
 *    - 速度为零时不生成（概率下限 0.15）
 *    - 高速度时按概率生成
 *    - 粒子类型为 Bubble
 *    - 粒子位置在实体后方
 *    - 世界为 null 时不崩溃
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/passive/nautilus/NautilusEntity.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::entity::serialization;

namespace {

/**
 * @brief 测试记录粒子数据的结构
 */
struct ParticleRecord {
    particle::ParticleTypeId type;
    Vector3 position;
    Vector3 velocity;
};

/**
 * @brief 鹦鹉螺测试用世界桩
 *
 * 支持粒子记录，覆写 addParticle 两个重载。
 */
class NautilusTestWorld final : public mc::test::BaseTestWorld {
public:
    std::vector<ParticleRecord>& particles() { return m_particles; }
    void clearParticles() { m_particles.clear(); }

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    void addParticle(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3&, u32) override
    {
        m_particles.push_back({type, pos, velocity});
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId) override { return nullptr; }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override { return nullptr; }
    EntityInstanceId spawnEntity(std::unique_ptr<Entity>) override { return EntityInstanceId(1); }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(const BlockPos&) const override { return 15; }
    [[nodiscard]] u8 getBlockLight(const BlockPos&) const override { return 0; }
    [[nodiscard]] bool canSeeSky(const BlockPos&) const override { return true; }
    [[nodiscard]] f32 getBrightness(const BlockPos&) const override { return 1.0f; }
    [[nodiscard]] u8 getLightSubtracted(const BlockPos&, u32) const override { return 15; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool isThundering() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }

private:
    std::vector<ParticleRecord> m_particles;
};

/**
 * @brief 将 NautilusEntity 序列化到 NBT
 */
std::unique_ptr<nbt::tags::compound_tag> saveToNbt(const NautilusEntity& entity)
{
    auto tag = std::make_unique<nbt::tags::compound_tag>();
    entity.addAdditionalSaveData(*tag);
    return tag;
}

/**
 * @brief 从 NBT 反序列化创建新的 NautilusEntity
 */
std::unique_ptr<NautilusEntity> loadFromNbt(const nbt::tags::compound_tag& tag)
{
    auto entity = std::make_unique<NautilusEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    auto result = entity->readAdditionalSaveData(tag);
    EXPECT_TRUE(result.success()) << "readAdditionalSaveData should succeed";
    return entity;
}

/**
 * @brief 测试用 NautilusEntity 子类，将 protected spawnBubbles() 暴露为 public
 *
 * spawnBubbles() 在 AbstractNautilusEntity 中是 protected 方法（仅在 tick() 中被调用），
 * 为了直接测试其行为，通过子类将其暴露为 public。
 */
class TestableNautilusEntity : public NautilusEntity {
public:
    explicit TestableNautilusEntity(EntityInstanceId id)
        : NautilusEntity(id, mc::test::testEcsRegistry())
    {}

    using NautilusEntity::spawnBubbles;
};

} // namespace

// ============================================================================
// NBT 序列化/反序列化测试
// ============================================================================

class NautilusNbtTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(NautilusNbtTest, EmptyInventory_DoesNotWriteItemsKey)
{
    NautilusEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    auto tag = saveToNbt(original);

    // 空物品栏不应写入 Items 键
    const auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    EXPECT_EQ(itemsList, nullptr) << "Empty inventory should not write Items key";
}

TEST_F(NautilusNbtTest, SaddleOnly_RoundTrip)
{
    NautilusEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    ASSERT_NE(Items::SADDLE, nullptr);
    original.setEquipment(0, ItemStack(*Items::SADDLE, 1));

    auto tag = saveToNbt(original);

    // 验证 Items 键存在
    const auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    ASSERT_NE(itemsList, nullptr);
    EXPECT_EQ(itemsList->element_id(), nbt::TagId::Compound);

    // 应有 1 个物品
    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
    EXPECT_EQ(compoundList.value.size(), 1u);

    auto loaded = loadFromNbt(*tag);

    // 验证鞍已加载
    EXPECT_TRUE(loaded->isSaddled());
    EXPECT_FALSE(loaded->getEquipment(0).isEmpty());
    EXPECT_EQ(loaded->getEquipment(0).getCount(), 1);

    // 铠甲槽应为空
    EXPECT_TRUE(loaded->getEquipment(1).isEmpty());
}

TEST_F(NautilusNbtTest, ArmorOnly_RoundTrip)
{
    NautilusEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    ASSERT_NE(Items::IRON_NAUTILUS_ARMOR, nullptr);
    original.setEquipment(1, ItemStack(*Items::IRON_NAUTILUS_ARMOR, 1));

    auto tag = saveToNbt(original);

    const auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    ASSERT_NE(itemsList, nullptr);

    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
    EXPECT_EQ(compoundList.value.size(), 1u);

    auto loaded = loadFromNbt(*tag);

    EXPECT_FALSE(loaded->isSaddled());
    EXPECT_TRUE(loaded->getEquipment(0).isEmpty());
    EXPECT_FALSE(loaded->getEquipment(1).isEmpty());
}

TEST_F(NautilusNbtTest, SaddleAndArmor_RoundTrip)
{
    NautilusEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    ASSERT_NE(Items::SADDLE, nullptr);
    ASSERT_NE(Items::DIAMOND_NAUTILUS_ARMOR, nullptr);
    original.setEquipment(0, ItemStack(*Items::SADDLE, 1));
    original.setEquipment(1, ItemStack(*Items::DIAMOND_NAUTILUS_ARMOR, 1));

    auto tag = saveToNbt(original);

    const auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    ASSERT_NE(itemsList, nullptr);

    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
    EXPECT_EQ(compoundList.value.size(), 2u);

    auto loaded = loadFromNbt(*tag);

    EXPECT_TRUE(loaded->isSaddled());
    EXPECT_FALSE(loaded->getEquipment(0).isEmpty());
    EXPECT_FALSE(loaded->getEquipment(1).isEmpty());
}

TEST_F(NautilusNbtTest, DashCooldown_RoundTrip)
{
    // 构造旧 NBT，包含 DashCooldown=20
    nbt::tags::compound_tag originalTag;
    originalTag.put("DashCooldown", 20);

    auto loaded = loadFromNbt(originalTag);

    // 验证加载后的冷却值
    EXPECT_EQ(loaded->getDashCooldown(), 20);

    // 重新序列化，验证 DashCooldown 键被正确写入
    auto reTag = saveToNbt(*loaded);
    auto cooldownVal = nbt_helper::tryGetInt(*reTag, "DashCooldown");
    ASSERT_TRUE(cooldownVal.has_value());
    EXPECT_EQ(*cooldownVal, 20);
}

TEST_F(NautilusNbtTest, DashCooldown_NotSerializedWhenZero)
{
    NautilusEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 冷却为 0 时不应写入
    auto tag = saveToNbt(original);

    auto cooldownVal = nbt_helper::tryGetInt(*tag, "DashCooldown");
    EXPECT_FALSE(cooldownVal.has_value()) << "DashCooldown should not be serialized when 0";
}

TEST_F(NautilusNbtTest, ItemsKey_UsesCompoundList)
{
    NautilusEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    ASSERT_NE(Items::SADDLE, nullptr);
    original.setEquipment(0, ItemStack(*Items::SADDLE, 1));

    auto tag = saveToNbt(original);

    const auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    ASSERT_NE(itemsList, nullptr);
    EXPECT_EQ(itemsList->element_id(), nbt::TagId::Compound) << "Items list should be a compound list (not end tag)";
}

TEST_F(NautilusNbtTest, SlotIndex_PreservedInNbt)
{
    NautilusEntity original(EntityInstanceId(1), mc::test::testEcsRegistry());

    ASSERT_NE(Items::SADDLE, nullptr);
    ASSERT_NE(Items::IRON_NAUTILUS_ARMOR, nullptr);
    original.setEquipment(0, ItemStack(*Items::SADDLE, 1));
    original.setEquipment(1, ItemStack(*Items::IRON_NAUTILUS_ARMOR, 1));

    auto tag = saveToNbt(original);

    const auto* itemsList = nbt_helper::tryGetList(*tag, nbt_keys::ITEMS);
    ASSERT_NE(itemsList, nullptr);
    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);

    bool foundSlot0 = false;
    bool foundSlot1 = false;
    for (const auto& itemTag : compoundList.value) {
        auto slotOpt = nbt_helper::tryGetByte(itemTag, "Slot");
        if (slotOpt.has_value()) {
            if (*slotOpt == 0) {
                foundSlot0 = true;
            } else if (*slotOpt == 1) {
                foundSlot1 = true;
            }
        }
    }
    EXPECT_TRUE(foundSlot0) << "Slot 0 (saddle) should be in NBT";
    EXPECT_TRUE(foundSlot1) << "Slot 1 (armor) should be in NBT";
}

TEST_F(NautilusNbtTest, OldSaddleItemBoolFormat_BackwardCompatible)
{
    // 模拟旧格式：仅有 SaddleItem 布尔标记（底层为 byte=1），无 Items 列表
    nbt::tags::compound_tag tag;
    tag.put("SaddleItem", static_cast<i8>(1));

    ASSERT_NE(Items::SADDLE, nullptr);
    auto loaded = loadFromNbt(tag);

    // 旧格式应恢复为默认鞍物品
    EXPECT_TRUE(loaded->isSaddled());
    EXPECT_FALSE(loaded->getEquipment(0).isEmpty());
    EXPECT_EQ(loaded->getEquipment(0).getItem(), Items::SADDLE);
}

TEST_F(NautilusNbtTest, OldSaddleItemFalse_BackwardCompatible)
{
    // 模拟旧格式：SaddleItem = 0（false）
    nbt::tags::compound_tag tag;
    tag.put("SaddleItem", static_cast<i8>(0));

    auto loaded = loadFromNbt(tag);

    EXPECT_FALSE(loaded->isSaddled());
    EXPECT_TRUE(loaded->getEquipment(0).isEmpty());
}

// ============================================================================
// spawnBubbles() 气泡粒子生成测试
// ============================================================================

class NautilusBubbleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        world = std::make_unique<NautilusTestWorld>();
        nautilus = std::make_unique<TestableNautilusEntity>(EntityInstanceId(1));
        nautilus->setWorld(world.get());
    }

    void TearDown() override
    {
        nautilus.reset();
        world.reset();
    }

    std::unique_ptr<NautilusTestWorld> world;
    std::unique_ptr<TestableNautilusEntity> nautilus;
};

TEST_F(NautilusBubbleTest, HighSpeed_MayGenerateBubbles)
{
    // 设置较高速度（speed * 2 > 1.0 → 概率上限 1.0）
    nautilus->setVelocity(2.0f, 0.0f, 0.0f);
    nautilus->setPosition(0.0f, 0.0f, 0.0f);
    world->clearParticles();

    // 跑 200 次，至少应该有 1 次生成气泡（概率下限 0.15）
    bool anyBubble = false;
    for (int i = 0; i < 200; ++i) {
        nautilus->spawnBubbles();
        if (!world->particles().empty()) {
            anyBubble = true;
            break;
        }
    }
    EXPECT_TRUE(anyBubble) << "With high speed, bubbles should eventually be generated";
}

TEST_F(NautilusBubbleTest, ParticleType_IsBubble)
{
    // 高速度确保概率上限为 1.0
    nautilus->setVelocity(10.0f, 0.0f, 0.0f);
    nautilus->setPosition(0.0f, 0.0f, 0.0f);
    world->clearParticles();

    // 跑到至少产生一个粒子
    for (int i = 0; i < 50; ++i) {
        nautilus->spawnBubbles();
        if (!world->particles().empty()) {
            break;
        }
    }

    ASSERT_FALSE(world->particles().empty()) << "Should have generated at least one particle";
    EXPECT_EQ(world->particles().back().type, particle::ParticleTypeId::Bubble);
}

TEST_F(NautilusBubbleTest, NoCrash_WhenWorldIsNull)
{
    // 设置世界为 null，spawnBubbles 不应崩溃
    nautilus->setWorld(nullptr);
    nautilus->setVelocity(10.0f, 0.0f, 0.0f);

    EXPECT_NO_FATAL_FAILURE({
        for (int i = 0; i < 100; ++i) {
            nautilus->spawnBubbles();
        }
    });
}

TEST_F(NautilusBubbleTest, ParticlePosition_BehindEntity)
{
    // 设置高速度确保概率上限为 1.0
    nautilus->setVelocity(10.0f, 0.0f, 0.0f);
    // 设置位置和朝向：yaw=0 时视线方向为 (0, 0, 1)
    // 粒子应在 (x, y+0.25, z-1.1)
    nautilus->setPosition(100.0f, 50.0f, 200.0f);
    nautilus->setRotation(0.0f, 0.0f); // yaw=0, pitch=0
    world->clearParticles();

    // 跑到至少产生一个粒子
    for (int i = 0; i < 50; ++i) {
        nautilus->spawnBubbles();
        if (!world->particles().empty()) {
            break;
        }
    }

    ASSERT_FALSE(world->particles().empty());

    const auto& p = world->particles().back();
    // 视线方向 (viewX, viewY, viewZ) = (-sin(0)*cos(0), -sin(0), cos(0)*cos(0)) = (0, 0, 1)
    // 粒子位置 = (x - viewX*1.1, y - viewY + 0.25, z - viewZ*1.1) = (100, 50.25, 198.9)
    EXPECT_FLOAT_EQ(p.position.x, 100.0f);
    EXPECT_FLOAT_EQ(p.position.y, 50.25f);
    EXPECT_NEAR(p.position.z, 198.9f, 0.001f);
}

TEST_F(NautilusBubbleTest, ParticlePosition_Yaw180_BehindEntity)
{
    nautilus->setVelocity(10.0f, 0.0f, 0.0f);
    nautilus->setPosition(0.0f, 0.0f, 0.0f);
    // yaw=180: 视线方向 = (-sin(180)*cos(0), -sin(0), cos(180)*cos(0)) = (0, 0, -1)
    nautilus->setRotation(180.0f, 0.0f);
    world->clearParticles();

    for (int i = 0; i < 50; ++i) {
        nautilus->spawnBubbles();
        if (!world->particles().empty()) {
            break;
        }
    }

    ASSERT_FALSE(world->particles().empty());

    const auto& p = world->particles().back();
    // 粒子位置 z = 0 - (-1) * 1.1 = 1.1
    EXPECT_NEAR(p.position.z, 1.1f, 0.001f);
}

TEST_F(NautilusBubbleTest, ParticleVelocity_BoundedBySpread)
{
    nautilus->setVelocity(1.0f, 0.0f, 0.0f);
    nautilus->setPosition(0.0f, 0.0f, 0.0f);
    nautilus->setRotation(0.0f, 0.0f);
    world->clearParticles();

    // 跑多次，收集粒子速度
    for (int i = 0; i < 500; ++i) {
        nautilus->spawnBubbles();
    }

    ASSERT_FALSE(world->particles().empty());

    // spread = nextDouble() * 0.8 * (1 + speed) = nextDouble() * 0.8 * 2.0 = [0, 1.6]
    // 每个分量 = (nextFloat() - 0.5) * spread = [-0.8, 0.8]
    for (const auto& p : world->particles()) {
        EXPECT_LE(p.velocity.x, 0.8f + 0.001f);
        EXPECT_GE(p.velocity.x, -0.8f - 0.001f);
        EXPECT_LE(p.velocity.y, 0.8f + 0.001f);
        EXPECT_GE(p.velocity.y, -0.8f - 0.001f);
        EXPECT_LE(p.velocity.z, 0.8f + 0.001f);
        EXPECT_GE(p.velocity.z, -0.8f - 0.001f);
    }
}

// ============================================================================
// 装备查询测试
// ============================================================================

class NautilusEquipmentTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(NautilusEquipmentTest, IsSaddled_FalseByDefault)
{
    NautilusEntity n(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(n.isSaddled());
}

TEST_F(NautilusEquipmentTest, IsSaddled_TrueAfterEquipSaddle)
{
    NautilusEntity n(EntityInstanceId(1), mc::test::testEcsRegistry());
    ASSERT_NE(Items::SADDLE, nullptr);
    n.setEquipment(0, ItemStack(*Items::SADDLE, 1));
    EXPECT_TRUE(n.isSaddled());
}

TEST_F(NautilusEquipmentTest, GetEquipmentSlotCount_ReturnsTwo)
{
    NautilusEntity n(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(n.getEquipmentSlotCount(), 2);
}

TEST_F(NautilusEquipmentTest, SetEquipment_SaddleSlot)
{
    NautilusEntity n(EntityInstanceId(1), mc::test::testEcsRegistry());
    ASSERT_NE(Items::SADDLE, nullptr);
    n.setEquipment(0, ItemStack(*Items::SADDLE, 1));

    const auto& stack = n.getEquipment(0);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::SADDLE);
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(NautilusEquipmentTest, SetEquipment_ArmorSlot)
{
    NautilusEntity n(EntityInstanceId(1), mc::test::testEcsRegistry());
    ASSERT_NE(Items::IRON_NAUTILUS_ARMOR, nullptr);
    n.setEquipment(1, ItemStack(*Items::IRON_NAUTILUS_ARMOR, 1));

    const auto& stack = n.getEquipment(1);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::IRON_NAUTILUS_ARMOR);
}
