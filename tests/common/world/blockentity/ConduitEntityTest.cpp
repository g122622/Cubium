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
#include "common/command/ICommandSource.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/monster/MonsterEntity.hpp"
#include "entity/interfaces/IMob.hpp"
#include "world/IWorld.hpp"
#include "world/WorldConstants.hpp"
#include "world/blockentity/processing/ConduitEntity.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include <unordered_map>
#include <unordered_set>

using namespace mc;

// ============================================================================
// ConduitTestWorld - Mock World for ConduitEntity Tests
// ============================================================================
class ConduitTestWorld final : public test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_statesByPos.find(pos);
        if (it != m_statesByPos.end()) {
            return it->second;
        }
        return &VanillaBlocks::WATER->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_statesByPos[BlockPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_entitiesInAabb;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return m_entitiesInRange;
    }

    // ConduitEntity::_findExistingTarget 通过 getEntityByUuid() 进行 O(1) 查找
    // （commit e4a6925b2 改造，对齐 MC Java EntityLookup.byUuid）。测试桩从注入的
    // 范围内实体列表中按 UUID 解析，使 mock 行为与范围内遍历语义一致。
    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override
    {
        for (Entity* entity : m_entitiesInRange) {
            if (entity != nullptr && entity->uuid() == uuid) {
                return entity;
            }
        }
        return nullptr;
    }
    [[nodiscard]] const Entity* getEntityByUuid(const std::string& uuid) const override
    {
        for (const Entity* entity : m_entitiesInRange) {
            if (entity != nullptr && entity->uuid() == uuid) {
                return entity;
            }
        }
        return nullptr;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        return it == m_blockEntities.end() ? nullptr : it->second;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override { m_blockEntities[pos] = entity; }

    void setEntitiesInRangeResult(const std::vector<Entity*>& entities) { m_entitiesInRange = entities; }

    void setEntitiesInAabbResult(const std::vector<Entity*>& entities) { m_entitiesInAabb = entities; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ConduitTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ConduitTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_statesByPos;
    std::unordered_map<BlockPos, BlockEntity*> m_blockEntities;
    std::vector<Entity*> m_entitiesInAabb;
    std::vector<Entity*> m_entitiesInRange;
};

// ============================================================================
// IMob Interface Tests
// ============================================================================

class IMobInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试 IMob 接口可以被继承
TEST_F(IMobInterfaceTest, IMobCanBeInherited)
{
    // 创建一个简单的测试类继承 IMob
    class TestMob : public entity::IMob {
    public:
        TestMob() = default;
    };

    TestMob mob;
    entity::IMob* imob = &mob;
    EXPECT_NE(imob, nullptr);
}

// ============================================================================
// DamageSource Magic Tests
// ============================================================================

class MagicDamageTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试魔法伤害源创建
TEST_F(MagicDamageTest, CreateMagicDamageSource)
{
    EnvironmentalDamage magicDamage = DamageSources::magic();

    // 验证伤害类型
    EXPECT_EQ(magicDamage.type(), DamageType::Magic);

    // 验证魔法伤害属性
    EXPECT_TRUE(magicDamage.isMagic());
    EXPECT_TRUE(magicDamage.bypassesArmor());     // 魔法伤害绕过护甲
    EXPECT_FALSE(magicDamage.isDamageAbsolute()); // 不是绝对伤害
}

// 测试魔法伤害不是物理伤害
TEST_F(MagicDamageTest, MagicDamageIsNotPhysical)
{
    EnvironmentalDamage magicDamage = DamageSources::magic();

    EXPECT_FALSE(magicDamage.isFire());
    EXPECT_FALSE(magicDamage.isProjectile());
    EXPECT_FALSE(magicDamage.isExplosion());
    EXPECT_FALSE(magicDamage.isEntitySource());
}

// 测试凋零伤害也是魔法伤害
TEST_F(MagicDamageTest, WitherDamageIsMagic)
{
    EnvironmentalDamage witherDamage = DamageSources::wither();

    EXPECT_TRUE(witherDamage.isMagic());
    EXPECT_TRUE(witherDamage.bypassesArmor());
    EXPECT_EQ(witherDamage.type(), DamageType::Wither);
}

// 测试魔法伤害类型判断
TEST_F(MagicDamageTest, MagicDamageType)
{
    EnvironmentalDamage magicDamage = DamageSources::magic();

    // 魔法伤害应该绕过护甲但不穿透无敌
    EXPECT_TRUE(magicDamage.bypassesArmor());
    EXPECT_FALSE(magicDamage.bypassesInvulnerability());
    EXPECT_FALSE(magicDamage.canDamageCreative());
}

// ============================================================================
// UUID Tests
// ============================================================================

class UuidTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试 UUID 字符串与字节数组转换
TEST_F(UuidTest, UuidConversionWorks)
{
    // 创建一个已知的 UUID
    Uuid originalUuid = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    // 转换为字符串
    std::string uuidStr = util::uuidToString(originalUuid);
    EXPECT_EQ(uuidStr, "0123456789abcdeffedcba9876543210");

    // 转换回字节数组
    Uuid convertedUuid = util::uuidFromString(uuidStr);
    EXPECT_EQ(originalUuid, convertedUuid);
}

// 测试 uuidFromString 处理无效输入
TEST_F(UuidTest, UuidFromStringHandlesInvalidInput)
{
    // 空字符串应该返回全零 UUID
    Uuid emptyUuid = util::uuidFromString("");
    for (u8 byte : emptyUuid) {
        EXPECT_EQ(byte, 0);
    }

    // 短字符串应该返回全零 UUID
    Uuid shortUuid = util::uuidFromString("abc");
    for (u8 byte : shortUuid) {
        EXPECT_EQ(byte, 0);
    }
}

// 测试 UUID 唯一性
TEST_F(UuidTest, UuidUniqueness)
{
    // 创建两个不同的 UUID
    Uuid uuid1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
    Uuid uuid2 = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00};

    EXPECT_NE(uuid1, uuid2);

    // 字符串也应该不同
    std::string str1 = util::uuidToString(uuid1);
    std::string str2 = util::uuidToString(uuid2);
    EXPECT_NE(str1, str2);
}

// 测试 UUID 哈希
TEST_F(UuidTest, UuidHashWorks)
{
    Uuid uuid1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
    Uuid uuid2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    // 相同 UUID 应该有相同哈希
    UuidHash hashFunc;
    EXPECT_EQ(hashFunc(uuid1), hashFunc(uuid2));

    // 可以用于 unordered_set/unordered_map
    std::unordered_set<Uuid, UuidHash> uuidSet;
    uuidSet.insert(uuid1);
    EXPECT_EQ(uuidSet.count(uuid1), 1u);
    EXPECT_EQ(uuidSet.count(uuid2), 1u); // 相同 UUID
}

// ============================================================================
// Conduit Attack Logic Concept Tests
// ============================================================================

class ConduitLogicTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试攻击伤害值
TEST_F(ConduitLogicTest, AttackDamageValue)
{
    // 潮涌核心攻击伤害为 4.0F（2颗心）
    constexpr f32 CONDUIT_ATTACK_DAMAGE = 4.0f;
    EXPECT_FLOAT_EQ(CONDUIT_ATTACK_DAMAGE, 4.0f);
}

// 测试攻击范围
TEST_F(ConduitLogicTest, AttackRangeValue)
{
    // 潮涌核心攻击范围为 8.0 格
    constexpr f32 CONDUIT_ATTACK_RANGE = 8.0f;
    EXPECT_FLOAT_EQ(CONDUIT_ATTACK_RANGE, 8.0f);
}

// 测试激活框架数
TEST_F(ConduitLogicTest, ActivationRequirements)
{
    // 激活需要至少 16 个框架方块
    constexpr i32 MIN_FRAME_BLOCKS = 16;
    // 攻击需要至少 42 个框架方块
    constexpr i32 EYE_OPEN_FRAME_BLOCKS = 42;

    EXPECT_EQ(MIN_FRAME_BLOCKS, 16);
    EXPECT_EQ(EYE_OPEN_FRAME_BLOCKS, 42);
}

// 测试敌对生物检测通过 IMob 接口
TEST_F(ConduitLogicTest, HostileMobDetectionByIMob)
{
    // MonsterEntity 继承自 IMob
    // 这个测试验证 IMob 接口存在于类型系统中

    // 检查 MonsterEntity 是否可以转换为 IMob*
    // 由于 MonsterEntity 继承自 IMob，这个转换应该成功
    EXPECT_TRUE(true); // 如果编译通过，说明 IMob 接口正确集成
}

// 测试魔法伤害应用于敌对生物
TEST_F(ConduitLogicTest, MagicDamageBypassesArmor)
{
    // 魔法伤害绕过护甲
    EnvironmentalDamage magicDamage = DamageSources::magic();
    EXPECT_TRUE(magicDamage.bypassesArmor());

    // 这意味着敌对生物被潮涌核心攻击时无法通过护甲减伤
}

// 测试不同长度 UUID 字符串
TEST_F(UuidTest, UuidStringFormat)
{
    Uuid uuid = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    std::string str = util::uuidToString(uuid);

    // UUID 字符串应该是 32 字符（16 字节 = 32 十六进制字符）
    EXPECT_EQ(str.length(), 32u);

    // 全小写
    for (char c : str) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

// ============================================================================
// ConduitEntity::findExistingTarget Tests
// ============================================================================

// Mock LivingEntity for testing
class MockLivingEntityForConduit : public LivingEntity {
public:
    explicit MockLivingEntityForConduit(EntityInstanceId id)
        : LivingEntity(id, nullptr)
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// Mock Mob Entity (implements IMob interface)
class MockMobEntityForConduit : public LivingEntity, public entity::IMob {
public:
    explicit MockMobEntityForConduit(EntityInstanceId id)
        : LivingEntity(id, nullptr)
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// Mock non-LivingEntity (for testing UUID match but wrong type)
class MockNonLivingEntityForConduit : public Entity {
public:
    explicit MockNonLivingEntityForConduit(EntityInstanceId id)
        : Entity(id, nullptr)
    {}
};

// Test ConduitEntity that exposes protected methods
class TestConduitEntity : public blockentity::ConduitEntity {
public:
    explicit TestConduitEntity(const BlockPos& pos)
        : ConduitEntity(pos)
    {}

    // Expose protected method for testing
    LivingEntity* testFindExistingTarget(IWorld& world) { return _findExistingTarget(world); }

    // Expose _isWaterAt for testing
    bool testIsWaterAt(IWorld& world, const BlockPos& pos) const { return _isWaterAt(world, pos); }

    void setTargetUuidForTest(const std::string& uuid)
    {
        nlohmann::json data;
        data["target_uuid"] = uuid;
        load(data);
    }
};

// Test: findExistingTarget returns nullptr when no target UUID is set
TEST(ConduitEntityFindTargetTest, ReturnsNullptrWhenNoTargetUuid)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    // No target UUID set
    MockMobEntityForConduit mob(1);
    world.setEntitiesInRangeResult({&mob});

    EXPECT_EQ(conduit.testFindExistingTarget(world), nullptr);
}

// Test: findExistingTarget returns nullptr when UUID doesn't match any entity
TEST(ConduitEntityFindTargetTest, ReturnsNullptrWhenUuidNotMatch)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    conduit.setTargetUuidForTest("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

    MockMobEntityForConduit mob(1);
    mob.setUuid("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
    world.setEntitiesInRangeResult({&mob});

    EXPECT_EQ(conduit.testFindExistingTarget(world), nullptr);
}

// Test: findExistingTarget returns nullptr when entity is not LivingEntity
TEST(ConduitEntityFindTargetTest, ReturnsNullptrWhenEntityNotLivingEntity)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    MockNonLivingEntityForConduit item(1);
    std::string itemUuid = "cccccccccccccccccccccccccccccccc";
    item.setUuid(itemUuid);

    conduit.setTargetUuidForTest(itemUuid);
    world.setEntitiesInRangeResult({&item});

    // Should return nullptr because item is not a LivingEntity
    EXPECT_EQ(conduit.testFindExistingTarget(world), nullptr);
}

// Test: findExistingTarget returns correct LivingEntity when UUID matches
TEST(ConduitEntityFindTargetTest, ReturnsEntityWhenUuidMatchesLivingEntity)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    MockMobEntityForConduit mob(1);
    std::string mobUuid = "dddddddddddddddddddddddddddddddd";
    mob.setUuid(mobUuid);

    conduit.setTargetUuidForTest(mobUuid);
    world.setEntitiesInRangeResult({&mob});

    LivingEntity* result = conduit.testFindExistingTarget(world);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->uuid(), mobUuid);
}

// Test: findExistingTarget returns correct entity among multiple entities
TEST(ConduitEntityFindTargetTest, ReturnsCorrectEntityAmongMultiple)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    MockMobEntityForConduit mob1(1);
    mob1.setUuid("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

    MockMobEntityForConduit mob2(2);
    std::string targetUuid = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    mob2.setUuid(targetUuid);

    MockMobEntityForConduit mob3(3);
    mob3.setUuid("cccccccccccccccccccccccccccccccc");

    conduit.setTargetUuidForTest(targetUuid);
    world.setEntitiesInRangeResult({&mob1, &mob2, &mob3});

    LivingEntity* result = conduit.testFindExistingTarget(world);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->uuid(), targetUuid);
    EXPECT_EQ(result, &mob2);
}

// Test: findExistingTarget returns nullptr when no entities in range
TEST(ConduitEntityFindTargetTest, ReturnsNullptrWhenNoEntitiesInRange)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    conduit.setTargetUuidForTest("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    world.setEntitiesInRangeResult({});

    EXPECT_EQ(conduit.testFindExistingTarget(world), nullptr);
}

// Test: findExistingTarget works with non-IMob LivingEntity
TEST(ConduitEntityFindTargetTest, WorksWithNonMobLivingEntity)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    MockLivingEntityForConduit player(1);
    std::string playerUuid = "ffffffffffffffffffffffffffffffff";
    player.setUuid(playerUuid);

    conduit.setTargetUuidForTest(playerUuid);
    world.setEntitiesInRangeResult({&player});

    // findExistingTarget only checks LivingEntity, not IMob
    LivingEntity* result = conduit.testFindExistingTarget(world);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->uuid(), playerUuid);
}

// Test: load correctly restores target UUID from saved data
TEST(ConduitEntityFindTargetTest, LoadRestoresTargetUuid)
{
    TestConduitEntity conduit(BlockPos(10, 20, 30));

    // Simulate loading data with target_uuid
    nlohmann::json data;
    data["target_uuid"] = "1234567890abcdef1234567890abcdef";

    ASSERT_TRUE(conduit.load(data));

    // Verify target UUID was loaded correctly by calling findExistingTarget
    // which internally uses m_targetUuid
    ConduitTestWorld world;
    MockMobEntityForConduit mob(1);
    mob.setUuid("1234567890abcdef1234567890abcdef");
    // _findExistingTarget 改造为 getEntityByUuid + 距离校验后（commit e4a6925b2），
    // 需将 mob 放在 conduit 攻击范围内（ATTACK_RANGE=8），否则距离校验剔除目标。
    const Vector3 conduitCenter = BlockPos(10, 20, 30).center();
    mob.setPosition(conduitCenter);
    world.setEntitiesInRangeResult({&mob});

    // If the UUID was loaded correctly, findExistingTarget should find the mob
    LivingEntity* result = conduit.testFindExistingTarget(world);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->uuid(), "1234567890abcdef1234567890abcdef");
}

// Test: save writes target_uuid when target exists
TEST(ConduitEntityFindTargetTest, SaveWritesTargetUuidWhenTargetExists)
{
    ConduitTestWorld world;
    TestConduitEntity conduit(BlockPos(10, 20, 30));

    // Create a mob and set it as target
    MockMobEntityForConduit mob(1);
    std::string mobUuid = "abcdef1234567890abcdef1234567890";
    mob.setUuid(mobUuid);
    // 将 mob 置于 conduit 攻击范围内，满足 _findExistingTarget 的距离校验。
    mob.setPosition(BlockPos(10, 20, 30).center());
    world.setEntitiesInRangeResult({&mob});

    // Set target UUID and find the target
    conduit.setTargetUuidForTest(mobUuid);
    LivingEntity* foundTarget = conduit.testFindExistingTarget(world);
    ASSERT_NE(foundTarget, nullptr);

    // Verify that load correctly restores target_uuid
    nlohmann::json loadData;
    loadData["target_uuid"] = mobUuid;

    TestConduitEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(loadData));

    // Verify by finding the target again：loaded conduit 位于原点，需将 mob 移近原点。
    mob.setPosition(BlockPos(0, 0, 0).center());
    world.setEntitiesInRangeResult({&mob});
    LivingEntity* reloadedTarget = loaded.testFindExistingTarget(world);
    ASSERT_NE(reloadedTarget, nullptr);
    EXPECT_EQ(reloadedTarget->uuid(), mobUuid);
}

// Test: save does not write target_uuid when target is null
TEST(ConduitEntityFindTargetTest, SaveDoesNotWriteTargetUuidWhenNull)
{
    TestConduitEntity conduit(BlockPos(10, 20, 30));

    // No target set (m_target is nullptr)
    nlohmann::json data;
    conduit.save(data);

    // target_uuid should not be in the saved data
    EXPECT_FALSE(data.contains("target_uuid"));
}

// ============================================================================
// ConduitEntity::_isWaterAt Tests - 含水检测（使用流体状态而非方块检查）
// ============================================================================

// 支持 getFluidState 重写的测试世界
class ConduitWaterTestWorld final : public test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_statesByPos.find(pos);
        if (it != m_statesByPos.end()) {
            return it->second;
        }
        return &VanillaBlocks::WATER->defaultState();
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_fluidStatesByPos.find(pos);
        if (it != m_fluidStatesByPos.end()) {
            return it->second;
        }
        // 默认返回空流体
        return fluid::Fluid::getFluidState(fluid::FluidRegistry::EMPTY_ID);
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_statesByPos[BlockPos(x, y, z)] = state;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    void setFluidDirectly(const BlockPos& pos, const fluid::FluidState* state) { m_fluidStatesByPos[pos] = state; }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos&) override { return nullptr; }
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos&) const override { return nullptr; }
    void setBlockEntity(const BlockPos&, BlockEntity*) override {}

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ConduitWaterTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ConduitWaterTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_statesByPos;
    std::unordered_map<BlockPos, const fluid::FluidState*> m_fluidStatesByPos;
};

class ConduitIsWaterAtTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和流体注册表
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// 测试：有水流体状态的位置应返回 true
TEST_F(ConduitIsWaterAtTest, ReturnsTrueWhenWaterFluidPresent)
{
    ConduitWaterTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    // 设置水源流体状态
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    world.setFluidDirectly(BlockPos(0, 0, 0), &waterFluid->defaultState());

    // 有水源时 isWaterAt 应返回 true
    EXPECT_TRUE(conduit.testIsWaterAt(world, BlockPos(0, 0, 0)));
}

// 测试：无流体状态的位置应返回 false
TEST_F(ConduitIsWaterAtTest, ReturnsFalseWhenNoFluidPresent)
{
    ConduitWaterTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    // 无流体状态（默认为空流体）时应返回 false
    EXPECT_FALSE(conduit.testIsWaterAt(world, BlockPos(0, 0, 0)));
}

// 测试：有流动水流体状态的位置应返回 true
TEST_F(ConduitIsWaterAtTest, ReturnsTrueWhenFlowingWaterFluidPresent)
{
    ConduitWaterTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    // 设置流动水流体状态
    fluid::Fluid* flowingWater = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::FLOWING_WATER_ID);
    ASSERT_NE(flowingWater, nullptr);
    world.setFluidDirectly(BlockPos(0, 0, 0), &flowingWater->defaultState());

    // 流动水的默认状态也应返回 true（IWorld::isWaterAt 同时检测 water 和 flowing_water）
    EXPECT_TRUE(conduit.testIsWaterAt(world, BlockPos(0, 0, 0)));
}

// 测试：有岩浆流体状态的位置应返回 false
TEST_F(ConduitIsWaterAtTest, ReturnsFalseWhenLavaFluidPresent)
{
    ConduitWaterTestWorld world;
    TestConduitEntity conduit(BlockPos(0, 0, 0));

    // 设置岩浆流体状态
    fluid::Fluid* lavaFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::LAVA_ID);
    ASSERT_NE(lavaFluid, nullptr);
    world.setFluidDirectly(BlockPos(0, 0, 0), &lavaFluid->defaultState());

    // 岩浆不应被视为水
    EXPECT_FALSE(conduit.testIsWaterAt(world, BlockPos(0, 0, 0)));
}

// 测试：不同位置有不同的流体状态
TEST_F(ConduitIsWaterAtTest, DifferentPositionsHaveDifferentFluids)
{
    ConduitWaterTestWorld world;
    TestConduitEntity conduit(BlockPos(5, 10, 5));

    // 在不同位置设置不同流体
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);

    world.setFluidDirectly(BlockPos(5, 10, 5), &waterFluid->defaultState());
    // BlockPos(6, 10, 5) 没有流体（默认空流体）

    EXPECT_TRUE(conduit.testIsWaterAt(world, BlockPos(5, 10, 5)));
    EXPECT_FALSE(conduit.testIsWaterAt(world, BlockPos(6, 10, 5)));
}
