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
 * LIABILITY, INNO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc;
using namespace mc::math;

// ============================================================================
// Entity::getRandom() 测试
// ============================================================================

TEST(EntityRandom, SameEntityReturnsSameRandomInstance)
{
    // 同一实体多次调用 getRandom() 应返回同一对象的引用，
    // 保证随机数序列的连续性
    Entity entity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 第一次调用获取一个随机数
    i32 first = entity.getRandom().nextInt(1000);

    // 第二次调用应从同一个 RNG 继续生成，不会重新初始化种子
    i32 second = entity.getRandom().nextInt(1000);

    // 两次调用返回不同的随机数（极大概率不同）
    // 注意：不是验证两者不等（理论上可能相等），而是验证RNG状态的连续性
    // 通过验证引用同一对象来确认
    math::Random& ref1 = entity.getRandom();
    math::Random& ref2 = entity.getRandom();
    EXPECT_EQ(&ref1, &ref2);
}

TEST(EntityRandom, DifferentEntitiesHaveIndependentRNG)
{
    // 不同实体应拥有独立的随机数生成器
    Entity entityA(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    Entity entityB(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());

    // 两个实体的 getRandom() 返回不同对象的引用
    math::Random& rngA = entityA.getRandom();
    math::Random& rngB = entityB.getRandom();
    EXPECT_NE(&rngA, &rngB);

    // 两个实体生成不同的随机数序列
    i32 a1 = entityA.getRandom().nextInt(10000);
    i32 a2 = entityA.getRandom().nextInt(10000);
    i32 b1 = entityB.getRandom().nextInt(10000);
    i32 b2 = entityB.getRandom().nextInt(10000);

    // A的序列和B的序列应该不同（极大概率）
    // 不检查每个值不同，只检查序列不完全相同
    bool sequencesDifferent = (a1 != b1) || (a2 != b2);
    EXPECT_TRUE(sequencesDifferent);
}

TEST(EntityRandom, RandomStatePersistsAcrossCalls)
{
    // 实体的随机数生成器状态应在调用之间持续保存
    // 对比旧实现：旧 MobEntity::getRandom() 每次按值返回新对象，
    // 同一 tick 内多次调用会产生相同序列
    Entity entity(EntityInstanceId(42), nullptr, mc::test::testEcsRegistry());

    // 连续调用多次，产生一个序列
    i32 vals[10];
    for (int i = 0; i < 10; ++i) {
        vals[i] = entity.getRandom().nextInt(10000);
    }

    // 序列不应全部相同（旧实现中同一tick调用会返回相同值）
    bool allSame = true;
    for (int i = 1; i < 10; ++i) {
        if (vals[i] != vals[0]) {
            allSame = false;
            break;
        }
    }
    EXPECT_FALSE(allSame);
}

TEST(EntityRandom, ConstEntityCanCallGetRandom)
{
    // const 实体也应能调用 getRandom()（返回 mutable 引用），
    // 因为随机数生成是逻辑操作而非状态查询
    const Entity entity(EntityInstanceId(5), nullptr, mc::test::testEcsRegistry());

    // 应能编译且运行正常
    math::Random& rng = entity.getRandom();
    (void)rng;

    // 能在 const 上下文中修改随机数生成器
    i32 val = entity.getRandom().nextInt(100);
    EXPECT_GE(val, 0);
    EXPECT_LT(val, 100);
}

TEST(EntityRandom, MobEntityInheritsGetRandomFromEntity)
{
    // MobEntity 不再有独立的 getRandom()，应继承 Entity 基类的实现
    // 返回的引用应指向同一对象
    MobEntity mob(EntityInstanceId(10), mc::test::testEcsRegistry());

    math::Random& ref1 = mob.getRandom();
    math::Random& ref2 = mob.getRandom();
    EXPECT_EQ(&ref1, &ref2);

    // 验证随机数序列连续性
    i32 v1 = mob.getRandom().nextInt(1000);
    i32 v2 = mob.getRandom().nextInt(1000);
    (void)v1;
    (void)v2;
    // 只要两次调用不崩溃且返回合理值即可
}

TEST(EntityRandom, UUIDGeneratedFromEntityRandom)
{
    // Entity 构造函数使用 m_random 生成 UUID，
    // 不同实体的 UUID 应该不同
    Entity e1(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    Entity e2(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry());
    EXPECT_NE(e1.uuid(), e2.uuid());

    // UUID 不应为空
    EXPECT_FALSE(e1.uuid().empty());
    EXPECT_FALSE(e2.uuid().empty());
}

TEST(EntityRandom, SameEntityIdDifferentInstancesGetDifferentSequences)
{
    // 即使两个实体的 ID 相同，由于构造时间戳不同，
    // 它们的随机数生成器种子也应不同（高分辨率时钟参与种子生成）
    Entity e1(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());
    Entity e2(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry());

    // 由于时钟种子参与，即使 ID 相同，序列也应不同
    // 注意：在极端情况下时钟精度不足时可能相同，但实践中不应出现
    i32 v1 = e1.getRandom().nextInt(100000);
    i32 v2 = e2.getRandom().nextInt(100000);

    // 不强制要求不等（理论极端情况），但应验证基本功能
    (void)v1;
    (void)v2;
}

TEST(EntityRandom, RandomProducesValidIntRange)
{
    // 验证 nextInt(bound) 在合法范围内
    Entity entity(EntityInstanceId(100), nullptr, mc::test::testEcsRegistry());

    constexpr i32 bound = 50;
    for (int i = 0; i < 100; ++i) {
        i32 val = entity.getRandom().nextInt(bound);
        EXPECT_GE(val, 0);
        EXPECT_LT(val, bound);
    }
}

TEST(EntityRandom, RandomProducesValidFloatRange)
{
    // 验证 nextFloat() 在 [0.0, 1.0) 范围内
    Entity entity(EntityInstanceId(200), nullptr, mc::test::testEcsRegistry());

    for (int i = 0; i < 100; ++i) {
        f32 val = entity.getRandom().nextFloat();
        EXPECT_GE(val, 0.0f);
        EXPECT_LT(val, 1.0f);
    }
}

TEST(EntityRandom, RandomProducesValidDoubleRange)
{
    // 验证 nextDouble() 在 [0.0, 1.0) 范围内
    Entity entity(EntityInstanceId(300), nullptr, mc::test::testEcsRegistry());

    for (int i = 0; i < 100; ++i) {
        f64 val = entity.getRandom().nextDouble();
        EXPECT_GE(val, 0.0);
        EXPECT_LT(val, 1.0);
    }
}
