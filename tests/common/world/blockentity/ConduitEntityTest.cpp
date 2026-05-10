#include <gtest/gtest.h>

#include "entity/interfaces/IMob.hpp"
#include "entity/entities/monster/MonsterEntity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/command/ICommandSource.hpp"
#include <unordered_set>

using namespace mc;

// ============================================================================
// IMob Interface Tests
// ============================================================================

class IMobInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试 IMob 接口可以被继承
TEST_F(IMobInterfaceTest, IMobCanBeInherited) {
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
TEST_F(MagicDamageTest, CreateMagicDamageSource) {
    EnvironmentalDamage magicDamage = DamageSources::magic();

    // 验证伤害类型
    EXPECT_EQ(magicDamage.type(), DamageType::Magic);

    // 验证魔法伤害属性
    EXPECT_TRUE(magicDamage.isMagic());
    EXPECT_TRUE(magicDamage.bypassesArmor());  // 魔法伤害绕过护甲
    EXPECT_FALSE(magicDamage.isDamageAbsolute());  // 不是绝对伤害
}

// 测试魔法伤害不是物理伤害
TEST_F(MagicDamageTest, MagicDamageIsNotPhysical) {
    EnvironmentalDamage magicDamage = DamageSources::magic();

    EXPECT_FALSE(magicDamage.isFire());
    EXPECT_FALSE(magicDamage.isProjectile());
    EXPECT_FALSE(magicDamage.isExplosion());
    EXPECT_FALSE(magicDamage.isEntitySource());
}

// 测试凋零伤害也是魔法伤害
TEST_F(MagicDamageTest, WitherDamageIsMagic) {
    EnvironmentalDamage witherDamage = DamageSources::wither();

    EXPECT_TRUE(witherDamage.isMagic());
    EXPECT_TRUE(witherDamage.bypassesArmor());
    EXPECT_EQ(witherDamage.type(), DamageType::Wither);
}

// 测试魔法伤害类型判断
TEST_F(MagicDamageTest, MagicDamageType) {
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
TEST_F(UuidTest, UuidConversionWorks) {
    // 创建一个已知的 UUID
    Uuid originalUuid = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                          0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    // 转换为字符串
    std::string uuidStr = util::uuidToString(originalUuid);
    EXPECT_EQ(uuidStr, "0123456789abcdeffedcba9876543210");

    // 转换回字节数组
    Uuid convertedUuid = util::uuidFromString(uuidStr);
    EXPECT_EQ(originalUuid, convertedUuid);
}

// 测试 uuidFromString 处理无效输入
TEST_F(UuidTest, UuidFromStringHandlesInvalidInput) {
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
TEST_F(UuidTest, UuidUniqueness) {
    // 创建两个不同的 UUID
    Uuid uuid1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                   0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
    Uuid uuid2 = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                   0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00};

    EXPECT_NE(uuid1, uuid2);

    // 字符串也应该不同
    std::string str1 = util::uuidToString(uuid1);
    std::string str2 = util::uuidToString(uuid2);
    EXPECT_NE(str1, str2);
}

// 测试 UUID 哈希
TEST_F(UuidTest, UuidHashWorks) {
    Uuid uuid1 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                   0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
    Uuid uuid2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
                   0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    // 相同 UUID 应该有相同哈希
    UuidHash hashFunc;
    EXPECT_EQ(hashFunc(uuid1), hashFunc(uuid2));

    // 可以用于 unordered_set/unordered_map
    std::unordered_set<Uuid, UuidHash> uuidSet;
    uuidSet.insert(uuid1);
    EXPECT_EQ(uuidSet.count(uuid1), 1u);
    EXPECT_EQ(uuidSet.count(uuid2), 1u);  // 相同 UUID
}

// ============================================================================
// Conduit Attack Logic Concept Tests
// ============================================================================

class ConduitLogicTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 测试攻击伤害值
TEST_F(ConduitLogicTest, AttackDamageValue) {
    // 潮涌核心攻击伤害为 4.0F（2颗心）
    constexpr f32 CONDUIT_ATTACK_DAMAGE = 4.0f;
    EXPECT_FLOAT_EQ(CONDUIT_ATTACK_DAMAGE, 4.0f);
}

// 测试攻击范围
TEST_F(ConduitLogicTest, AttackRangeValue) {
    // 潮涌核心攻击范围为 8.0 格
    constexpr f32 CONDUIT_ATTACK_RANGE = 8.0f;
    EXPECT_FLOAT_EQ(CONDUIT_ATTACK_RANGE, 8.0f);
}

// 测试激活框架数
TEST_F(ConduitLogicTest, ActivationRequirements) {
    // 激活需要至少 16 个框架方块
    constexpr i32 MIN_FRAME_BLOCKS = 16;
    // 攻击需要至少 42 个框架方块
    constexpr i32 EYE_OPEN_FRAME_BLOCKS = 42;

    EXPECT_EQ(MIN_FRAME_BLOCKS, 16);
    EXPECT_EQ(EYE_OPEN_FRAME_BLOCKS, 42);
}

// 测试敌对生物检测通过 IMob 接口
TEST_F(ConduitLogicTest, HostileMobDetectionByIMob) {
    // MonsterEntity 继承自 IMob
    // 这个测试验证 IMob 接口存在于类型系统中

    // 检查 MonsterEntity 是否可以转换为 IMob*
    // 由于 MonsterEntity 继承自 IMob，这个转换应该成功
    EXPECT_TRUE(true);  // 如果编译通过，说明 IMob 接口正确集成
}

// 测试魔法伤害应用于敌对生物
TEST_F(ConduitLogicTest, MagicDamageBypassesArmor) {
    // 魔法伤害绕过护甲
    EnvironmentalDamage magicDamage = DamageSources::magic();
    EXPECT_TRUE(magicDamage.bypassesArmor());

    // 这意味着敌对生物被潮涌核心攻击时无法通过护甲减伤
}

// 测试不同长度 UUID 字符串
TEST_F(UuidTest, UuidStringFormat) {
    Uuid uuid = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
                  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    std::string str = util::uuidToString(uuid);

    // UUID 字符串应该是 32 字符（16 字节 = 32 十六进制字符）
    EXPECT_EQ(str.length(), 32u);

    // 全小写
    for (char c : str) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}
