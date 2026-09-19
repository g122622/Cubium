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

// JavaAttributeIdMap 单测：锁定关键 registry id，防迁移或重排导致 wire 映射错位。

#include "common/entity/attribute/Attributes.hpp"
#include "common/network/backend/java/mappings/JavaAttributeIdMap.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>

using namespace mc;
using namespace mc::network::backend::java;

// Attributes 是命名空间（内为一组属性名常量），起个别名缩短引用。
namespace Attributes = mc::entity::attribute::Attributes;

class JavaAttributeIdMapTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto result = JavaAttributeIdMap::instance().initialize();
        ASSERT_TRUE(result.success()) << result.error().toString();
    }

    static const JavaAttributeIdMap& map() { return JavaAttributeIdMap::instance(); }
};

TEST_F(JavaAttributeIdMapTest, VanillaIdsMatchDeclarationOrder)
{
    // 顺序取自 vanilla 1.21.11 Attributes.java 的静态字段声明顺序（0-based）。
    // 这些 id 直接上线，任何偏移都会让客户端把属性值套到别的属性上。
    const std::pair<const char*, u32> expected[] = {
        {Attributes::ARMOR, 0},
        {Attributes::ARMOR_TOUGHNESS, 1},
        {Attributes::ATTACK_DAMAGE, 2},
        {Attributes::ATTACK_KNOCKBACK, 3},
        {Attributes::ATTACK_SPEED, 4},
        {Attributes::BLOCK_INTERACTION_RANGE, 6},
        {Attributes::BURNING_TIME, 7},
        {Attributes::EXPLOSION_KNOCKBACK_RESISTANCE, 9},
        {Attributes::ENTITY_INTERACTION_RANGE, 10},
        {Attributes::FALL_DAMAGE_MULTIPLIER, 11},
        {Attributes::FLYING_SPEED, 12},
        {Attributes::FOLLOW_RANGE, 13},
        {Attributes::ENTITY_GRAVITY, 14},
        {Attributes::JUMP_STRENGTH, 15},
        {Attributes::KNOCKBACK_RESISTANCE, 16},
        {Attributes::LUCK, 17},
        {Attributes::MAX_ABSORPTION, 18},
        {Attributes::MAX_HEALTH, 19},
        {Attributes::MOVEMENT_EFFICIENCY, 21},
        {Attributes::MOVEMENT_SPEED, 22},
        {Attributes::OXYGEN_BONUS, 23},
        {Attributes::SAFE_FALL_DISTANCE, 24},
        {Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, 27},
    };

    for (const auto& [name, expectedId] : expected) {
        const auto actual = map().toJavaRegistryId(name);
        ASSERT_TRUE(actual.has_value()) << "未映射的项目属性：" << name;
        EXPECT_EQ(*actual, expectedId) << "属性映射错位：" << name;
    }
}

TEST_F(JavaAttributeIdMapTest, ReverseLookupRoundTrips)
{
    const auto healthId = map().toJavaRegistryId(Attributes::MAX_HEALTH);
    ASSERT_TRUE(healthId.has_value());
    EXPECT_EQ(map().fromJavaRegistryId(*healthId), std::string(Attributes::MAX_HEALTH));

    // 未映射的 id 返回空串（而非悬垂引用）。5 是 vanilla 的 block_break_speed，项目未实现。
    EXPECT_TRUE(map().fromJavaRegistryId(5).empty());
    EXPECT_TRUE(map().fromJavaRegistryId(9999).empty());
}

TEST_F(JavaAttributeIdMapTest, UnimplementedAttributeHasNoId)
{
    // 项目自有的非 vanilla 属性没有对应的 registry id，不能上线。
    EXPECT_FALSE(map().toJavaRegistryId(Attributes::HORSE_JUMP_STRENGTH).has_value());
    EXPECT_FALSE(map().toJavaRegistryId(Attributes::BREATH_MAX).has_value());
    EXPECT_FALSE(map().isImplemented(Attributes::BREATH_MAX));
}

TEST_F(JavaAttributeIdMapTest, ClientSyncableMatchesVanilla)
{
    // vanilla 对 8 条属性调用了 setSyncable(false)，它们不应下发。
    const u32 notSyncable[] = {2, 3, 13, 16, 27, 31, 33, 34};
    for (u32 id : notSyncable) {
        EXPECT_FALSE(map().isClientSyncable(id)) << "registry id " << id << " 不应同步";
    }

    const u32 syncable[] = {0, 1, 4, 6, 19, 22};
    for (u32 id : syncable) {
        EXPECT_TRUE(map().isClientSyncable(id)) << "registry id " << id << " 应同步";
    }

    // 项目未实现的属性即使 vanilla 可同步也不下发（5 = block_break_speed）。
    EXPECT_FALSE(map().isClientSyncable(5));
}
