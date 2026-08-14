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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/interfaces/IMob.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/world/entity/EntityTracker.hpp"

using namespace mc;
using namespace mc::server;

// ============================================================================
// Test Helper: Minimal Entity for testing
// ============================================================================

class TestEntity : public Entity {
public:
    TestEntity(EntityInstanceId id)
        : Entity(id, nullptr, mc::test::testEcsRegistry())
    {
        setPosition(0.0f, 64.0f, 0.0f);
    }

    void tick() override {}
};

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void tick() override {}
};

class TestMobEntity : public MobEntity {
public:
    TestMobEntity(EntityInstanceId id)
        : MobEntity(id, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

// 测试用敌对生物实体（实现 IMob）
class TestMonsterEntity : public MonsterEntity {
public:
    TestMonsterEntity(EntityInstanceId id)
        : MonsterEntity(id, mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void tick() override {}
};

// ============================================================================
// EntityTracker UUID Tests
// ============================================================================

class EntityTrackerUuidTest : public ::testing::Test {
protected:
    void SetUp() override { tracker = std::make_unique<EntityTracker>(); }

    void TearDown() override { tracker.reset(); }

    std::unique_ptr<EntityTracker> tracker;
};

// 测试 Entity 构造时生成有效的 UUID
TEST_F(EntityTrackerUuidTest, EntityGeneratesValidUuid)
{
    TestEntity entity(1);

    // UUID 不应为空
    const std::string& uuid = entity.uuid();
    EXPECT_FALSE(uuid.empty());
    EXPECT_EQ(uuid.length(), 32u); // 32 字符十六进制字符串
}

// 测试 UUID 字符串与字节数组转换
TEST_F(EntityTrackerUuidTest, UuidConversionWorks)
{
    TestEntity entity(1);
    const std::string& uuidStr = entity.uuid();

    // 转换为字节数组
    Uuid uuidBytes = util::uuidFromString(uuidStr);

    // 不应全为零（除非转换失败）
    bool allZero = true;
    for (u8 byte : uuidBytes) {
        if (byte != 0) {
            allZero = false;
            break;
        }
    }
    EXPECT_FALSE(allZero) << "UUID should not be all zeros after conversion";

    // 转换回字符串应该一致
    std::string convertedBack = util::uuidToString(uuidBytes);
    EXPECT_EQ(uuidStr, convertedBack);
}

// 测试不同 Entity 有不同的 UUID
TEST_F(EntityTrackerUuidTest, DifferentEntitiesHaveDifferentUuids)
{
    TestEntity entity1(1);
    TestEntity entity2(2);

    EXPECT_NE(entity1.uuid(), entity2.uuid()) << "Different entities should have different UUIDs";
}

// 测试 MonsterEntity 继承 IMob 接口
TEST_F(EntityTrackerUuidTest, MonsterEntityImplementsIMob)
{
    TestMonsterEntity monster(1);

    // 应该能够 dynamic_cast 到 IMob
    entity::IMob* imob = dynamic_cast<entity::IMob*>(&monster);
    EXPECT_NE(imob, nullptr) << "MonsterEntity should implement IMob interface";
}

// 测试普通生物不继承 IMob 接口
TEST_F(EntityTrackerUuidTest, PassiveMobDoesNotImplementIMob)
{
    TestMobEntity passiveMob(1);

    // 不应该能够 dynamic_cast 到 IMob
    entity::IMob* imob = dynamic_cast<entity::IMob*>(&passiveMob);
    EXPECT_EQ(imob, nullptr) << "Passive MobEntity should not implement IMob interface";
}

// 测试 LivingEntity 不继承 IMob 接口
TEST_F(EntityTrackerUuidTest, LivingEntityDoesNotImplementIMob)
{
    TestLivingEntity living(1);

    entity::IMob* imob = dynamic_cast<entity::IMob*>(&living);
    EXPECT_EQ(imob, nullptr) << "LivingEntity should not implement IMob interface";
}

// 测试 Entity 不继承 IMob 接口
TEST_F(EntityTrackerUuidTest, EntityDoesNotImplementIMob)
{
    TestEntity entity(1);

    entity::IMob* imob = dynamic_cast<entity::IMob*>(&entity);
    EXPECT_EQ(imob, nullptr) << "Entity should not implement IMob interface";
}

// 测试 UUID 格式正确性
TEST_F(EntityTrackerUuidTest, UuidFormatIsValid)
{
    TestEntity entity(1);
    const std::string& uuid = entity.uuid();

    // 应该是 32 字符的十六进制字符串
    EXPECT_EQ(uuid.length(), 32u);

    // 所有字符应该是十六进制数字
    for (char c : uuid) {
        bool isHexDigit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        EXPECT_TRUE(isHexDigit) << "UUID should only contain hex digits, found: " << c;
    }
}

// 测试 uuidFromString 处理无效输入
TEST_F(EntityTrackerUuidTest, UuidFromStringHandlesInvalidInput)
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

// 测试 uuidToString 和 uuidFromString 的往返转换
TEST_F(EntityTrackerUuidTest, UuidRoundTrip)
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
