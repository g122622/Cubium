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
 * @file GetLootTableIdTest.cpp
 * @brief Entity::getLootTableId() 及其子类覆写的单元测试
 *
 * 测试内容：
 * - Entity 基类 getLootTableId() 从 typeId 推导默认战利品表路径
 * - MobEntity 覆写：优先使用 NBT 自定义 DeathLootTable，否则回退到基类
 * - 无战利品表实体覆写：TNTEntity、MinecartEntity 等返回空字符串
 * - 边界情况：空 typeId、无命名空间 typeId、带命名空间 typeId
 * - 多态调用：通过基类指针调用 getLootTableId() 时正确分派到子类覆写
 */

#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// 测试辅助：访问 ProjectileEntity 的 protected 构造函数
// ============================================================================

class TestProjectileEntity : public ProjectileEntity {
public:
    explicit TestProjectileEntity(EntityInstanceId id)
        : ProjectileEntity(id)
    {}
};

// ============================================================================
// Entity 基类 getLootTableId() 测试
// ============================================================================

TEST(GetLootTableIdTest, EmptyTypeIdReturnsEmpty)
{
    // Entity 构造时 m_typeId 为空，getLootTableId() 检查内部 m_typeId，
    // 为空时返回空字符串。getTypeId() 对未设类型实体也返回空串。
    Entity entity(EntityInstanceId(1));
    EXPECT_TRUE(entity.getLootTableId().empty());
}

TEST(GetLootTableIdTest, NamespacedTypeIdDerivesLootTablePath)
{
    // 带命名空间的 typeId（如 "minecraft:pig"）应生成 "minecraft:entities/pig"
    // 对齐 MC Java: EntityType.Builder.withPrefix("entities/")
    Entity entity(EntityInstanceId(1));
    entity.setTypeId("minecraft:pig");
    EXPECT_EQ(entity.getLootTableId(), "minecraft:entities/pig");
}

TEST(GetLootTableIdTest, NamespacedTypeIdCustomNamespace)
{
    // 自定义命名空间的 typeId
    Entity entity(EntityInstanceId(1));
    entity.setTypeId("mymod:custom_mob");
    EXPECT_EQ(entity.getLootTableId(), "mymod:entities/custom_mob");
}

TEST(GetLootTableIdTest, NamespacedTypeIdMultiComponentPath)
{
    // 多段路径的 typeId
    Entity entity(EntityInstanceId(1));
    entity.setTypeId("minecraft:zombie_villager");
    EXPECT_EQ(entity.getLootTableId(), "minecraft:entities/zombie_villager");
}

TEST(GetLootTableIdTest, NoNamespaceTypeIdDefaultsToMinecraft)
{
    // 不带命名空间的 typeId 应被当作无命名空间前缀处理
    // 当前实现：直接加 "minecraft:entities/" 前缀
    Entity entity(EntityInstanceId(1));
    entity.setTypeId("pig");
    EXPECT_EQ(entity.getLootTableId(), "minecraft:entities/pig");
}

// ============================================================================
// MobEntity 覆写 getLootTableId() 测试
// ============================================================================

TEST(GetLootTableIdTest, MobEntityWithoutDeathLootTableFallsBackToBase)
{
    // MobEntity 没有设置 DeathLootTable 时，应回退到基类的实现
    MobEntity mob(EntityInstanceId(10));
    mob.setTypeId("minecraft:zombie");
    EXPECT_EQ(mob.getLootTableId(), "minecraft:entities/zombie");
}

TEST(GetLootTableIdTest, MobEntityWithDeathLootTableOverridesBase)
{
    // MobEntity 设置了 DeathLootTable 后，应优先使用自定义掉落表
    // 对齐 MC Java: this.lootTable.isPresent() ? this.lootTable : super.getLootTable()
    MobEntity mob(EntityInstanceId(11));
    mob.setTypeId("minecraft:zombie");
    mob.setDeathLootTable("minecraft:entities/custom_zombie");
    EXPECT_EQ(mob.getLootTableId(), "minecraft:entities/custom_zombie");
}

TEST(GetLootTableIdTest, MobEntityWithEmptyDeathLootTableFallsBackToBase)
{
    // DeathLootTable 设为空字符串时应回退到基类实现
    MobEntity mob(EntityInstanceId(12));
    mob.setTypeId("minecraft:skeleton");
    mob.setDeathLootTable("");
    EXPECT_EQ(mob.getLootTableId(), "minecraft:entities/skeleton");
}

TEST(GetLootTableIdTest, MobEntityWithNulloptDeathLootTableFallsBackToBase)
{
    // DeathLootTable 设为 nullopt 时应回退到基类实现
    MobEntity mob(EntityInstanceId(13));
    mob.setTypeId("minecraft:creeper");
    mob.setDeathLootTable(std::nullopt);
    EXPECT_EQ(mob.getLootTableId(), "minecraft:entities/creeper");
}

// ============================================================================
// 无战利品表实体覆写 getLootTableId() 测试
//
// 以下实体的 getLootTableId() 覆写应返回空字符串，
// 对齐 MC Java 中 EntityType.Builder.noLootTable() 的行为。
// ============================================================================

TEST(GetLootTableIdTest, ProjectileEntityReturnsEmpty)
{
    // ProjectileEntity 覆写返回空，覆盖所有弹射物子类
    // 对齐 MC Java: 大量弹射物实体调用 noLootTable()
    TestProjectileEntity proj(EntityInstanceId(20));
    EXPECT_TRUE(proj.getLootTableId().empty());
}

TEST(GetLootTableIdTest, FishingBobberEntityReturnsEmpty)
{
    // FishingBobberEntity 覆写返回空
    // 对齐 MC Java: noLootTable() for FishingBobber
    FishingBobberEntity bobber(EntityInstanceId(21));
    EXPECT_TRUE(bobber.getLootTableId().empty());
}

TEST(GetLootTableIdTest, EvokerFangsEntityReturnsEmpty)
{
    // EvokerFangsEntity 覆写返回空
    // 对齐 MC Java: noLootTable() for EvokerFangs
    EvokerFangsEntity fangs(EntityInstanceId(22));
    EXPECT_TRUE(fangs.getLootTableId().empty());
}

TEST(GetLootTableIdTest, EyeOfEnderEntityReturnsEmpty)
{
    // EyeOfEnderEntity 覆写返回空
    // 对齐 MC Java: noLootTable() for EyeOfEnder
    EyeOfEnderEntity eye(EntityInstanceId(23));
    EXPECT_TRUE(eye.getLootTableId().empty());
}

TEST(GetLootTableIdTest, FireworkRocketEntityReturnsEmpty)
{
    // FireworkRocketEntity 覆写返回空
    // 对齐 MC Java: noLootTable() for FireworkRocket
    FireworkRocketEntity rocket(EntityInstanceId(24));
    EXPECT_TRUE(rocket.getLootTableId().empty());
}

TEST(GetLootTableIdTest, TNTEntityReturnsEmpty)
{
    // TNTEntity 覆写返回空
    // 对齐 MC Java: noLootTable() for TNT
    TNTEntity tnt(EntityInstanceId(25));
    EXPECT_TRUE(tnt.getLootTableId().empty());
}

TEST(GetLootTableIdTest, MinecartEntityReturnsEmpty)
{
    // RideableMinecartEntity 继承自 AbstractMinecartEntity，
    // 后者覆写 getLootTableId() 返回空，覆盖所有矿车子类
    // 对齐 MC Java: 所有 Minecart 子类型都调用 noLootTable()
    RideableMinecartEntity minecart(EntityInstanceId(26));
    EXPECT_TRUE(minecart.getLootTableId().empty());
}

// ============================================================================
// 覆写优先级和一致性测试
// ============================================================================

TEST(GetLootTableIdTest, MobEntityTypeIdDerivesCorrectPath)
{
    // MobEntity 继承 Entity 的 getLootTableId() 路径推导逻辑
    MobEntity mob(EntityInstanceId(30));
    mob.setTypeId("minecraft:armor_stand");
    EXPECT_EQ(mob.getLootTableId(), "minecraft:entities/armor_stand");
}

TEST(GetLootTableIdTest, ConsistencyWithMCJavaFormat)
{
    // 验证生成的战利品表ID格式对齐 MC Java
    // MC Java 中 EntityType.Builder.withPrefix("entities/") 生成的格式：
    // "minecraft:pig" -> "minecraft:entities/pig"
    Entity entity(EntityInstanceId(40));

    entity.setTypeId("minecraft:cow");
    EXPECT_EQ(entity.getLootTableId(), "minecraft:entities/cow");

    entity.setTypeId("minecraft:wither_skeleton");
    EXPECT_EQ(entity.getLootTableId(), "minecraft:entities/wither_skeleton");

    entity.setTypeId("minecraft:evoker");
    EXPECT_EQ(entity.getLootTableId(), "minecraft:entities/evoker");
}

TEST(GetLootTableIdTest, PolymorphicCallResolvesCorrectly)
{
    // 通过基类指针调用 getLootTableId() 时应正确分派到子类覆写
    TestProjectileEntity proj(EntityInstanceId(50));
    Entity* basePtr = &proj;
    EXPECT_TRUE(basePtr->getLootTableId().empty());

    // MobEntity 通过基类指针调用应使用 MobEntity 覆写
    MobEntity mob(EntityInstanceId(51));
    mob.setTypeId("minecraft:blaze");
    basePtr = &mob;
    EXPECT_EQ(basePtr->getLootTableId(), "minecraft:entities/blaze");

    // MobEntity 设置自定义掉落表后，通过基类指针也能正确获取
    mob.setDeathLootTable("minecraft:entities/custom");
    EXPECT_EQ(basePtr->getLootTableId(), "minecraft:entities/custom");
}
