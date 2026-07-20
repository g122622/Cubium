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

#include "common/core/Types.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/util/math/Vector3.hpp"

#include <cmath>

using namespace mc;
using mc::entity::ProjectileEntity;

// ============================================================================
// ProjectileEntity::shoot 负 inaccuracy 行为测试
// ============================================================================
//
// 背景：
// MC 1.21.11 Shoot.tick() 中旋风人发射风弹使用公式 `5 - difficulty.getId() * 4`
// 计算散布（inaccuracy），该公式在 Normal(-3) 和 Hard(-7) 难度下会产生负值。
// 由于 MC 原版使用对称的 triangle 分布，负值与同绝对值的正值产生相同散布效果。
//
// 本项目的 ProjectileEntity::shoot 使用高斯分布，同样具有对称性，
// 因此对 inaccuracy 取绝对值（std::abs）即可保证负值与正值产生相同散布。
//
// 注意：shoot 实现中高斯偏移是在方向向量归一化之后加上的，且加偏移后不再重新归一化，
// 因此散布会使速度大小略大于 velocity 参数（这是 MC 原版行为，非 bug）。
//
// 本测试集验证的核心契约：
// - inaccuracy=0 时速度方向无偏移（精确射击，速度大小严格等于 velocity）
// - 负 inaccuracy 不会导致速度方向反转（方向向量保持原方向）
// - 负 inaccuracy 与同绝对值的正 inaccuracy 产生完全相同的散布（核心修复点）
// - 负 inaccuracy 不会跳过散布计算（即散布确实被应用）

namespace {

/// 测试用弹射物子类（ProjectileEntity 是抽象类，需要实现纯虚函数）
class TestProjectile : public ProjectileEntity {
public:
    explicit TestProjectile(EntityInstanceId id)
        : ProjectileEntity(id)
    {}

    [[nodiscard]] std::string getTypeId() const override { return "minecraft:test_projectile"; }
};

} // namespace

// ============================================================================
// 基础行为测试
// ============================================================================

TEST(ProjectileShootInaccuracyTest, ZeroInaccuracy_ProducesExactVelocity)
{
    // inaccuracy=0 时，速度方向应与输入方向一致，大小严格等于 velocity 参数
    TestProjectile p(EntityInstanceId(1));

    // 沿 +X 方向射击
    p.shoot(1.0f, 0.0f, 0.0f, 1.5f, 0.0f);

    // 速度应严格沿 +X 方向，大小为 1.5
    EXPECT_FLOAT_EQ(p.velocityX(), 1.5f);
    EXPECT_FLOAT_EQ(p.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(p.velocityZ(), 0.0f);
}

TEST(ProjectileShootInaccuracyTest, ZeroInaccuracy_NormalizedDirection)
{
    // 非单位方向向量应被归一化，速度大小仍等于 velocity 参数
    TestProjectile p(EntityInstanceId(1));

    // 方向向量 (2, 0, 0) 长度为 2，应归一化为 (1, 0, 0)
    p.shoot(2.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    EXPECT_FLOAT_EQ(p.velocityX(), 1.0f);
    EXPECT_FLOAT_EQ(p.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(p.velocityZ(), 0.0f);
}

// ============================================================================
// 负 inaccuracy 核心契约测试（核心修复点）
// ============================================================================

TEST(ProjectileShootInaccuracyTest, NegativeInaccuracy_MatchesPositiveAbsoluteValue)
{
    // 核心契约：负 inaccuracy 与同绝对值的正 inaccuracy 产生相同的散布效果
    //
    // 验证方法：使用相同的实体 ID 和 tick（通过不调用 tick 保持 ticksExisted=0），
    // 使 createRandomFromEntity 产生相同的随机种子。
    // 这样 +N 和 -N 应产生完全相同的高斯偏移（因为 std::abs 后传入的值相同）。
    //
    // 对应 MC 原版行为：triangle 分布对称，负 inaccuracy 与正 inaccuracy 等效。

    // 正 inaccuracy=7（对应 Hard 难度旋风人风弹散布绝对值）
    TestProjectile pPositive(EntityInstanceId(1));
    pPositive.shoot(1.0f, 0.0f, 0.0f, 1.0f, 7.0f);
    const Vector3 positiveVelocity = pPositive.velocity();

    // 负 inaccuracy=-7（对应 Hard 难度旋风人风弹散布原始值）
    // 使用相同的实体 ID，createRandomFromEntity 种子相同
    TestProjectile pNegative(EntityInstanceId(1));
    pNegative.shoot(1.0f, 0.0f, 0.0f, 1.0f, -7.0f);
    const Vector3 negativeVelocity = pNegative.velocity();

    // 两者速度应完全相同（因为 std::abs(-7) == std::abs(7)，且随机种子相同）
    EXPECT_FLOAT_EQ(negativeVelocity.x, positiveVelocity.x);
    EXPECT_FLOAT_EQ(negativeVelocity.y, positiveVelocity.y);
    EXPECT_FLOAT_EQ(negativeVelocity.z, positiveVelocity.z);
}

TEST(ProjectileShootInaccuracyTest, NegativeInaccuracy_MatchesPositive_ForAllBreezeValues)
{
    // 验证旋风人风弹所有难度下的 inaccuracy 值都满足"负值等效于同绝对值正值"的契约
    //
    // 各难度 inaccuracy（公式 5 - difficulty.getId() * 4）：
    // - Peaceful (id=0): 5（正值，无负值对照）
    // - Easy (id=1): 1（正值，无负值对照）
    // - Normal (id=2): -3（负值，对照 +3）
    // - Hard (id=3): -7（负值，对照 +7）

    struct TestCase {
        const char* name;
        f32 inaccuracy;    // 原始值（可能为负）
        f32 absInaccuracy; // 绝对值（正值对照）
    };

    const TestCase cases[] = {
        {"Normal", -3.0f, 3.0f},
        {"Hard", -7.0f, 7.0f},
    };

    for (const auto& tc : cases) {
        // 正值对照
        TestProjectile pPositive(EntityInstanceId(1));
        pPositive.shoot(1.0f, 0.0f, 0.0f, 0.7f, tc.absInaccuracy);
        const Vector3 positiveVelocity = pPositive.velocity();

        // 负值
        TestProjectile pNegative(EntityInstanceId(1));
        pNegative.shoot(1.0f, 0.0f, 0.0f, 0.7f, tc.inaccuracy);
        const Vector3 negativeVelocity = pNegative.velocity();

        EXPECT_FLOAT_EQ(negativeVelocity.x, positiveVelocity.x) << "难度 " << tc.name;
        EXPECT_FLOAT_EQ(negativeVelocity.y, positiveVelocity.y) << "难度 " << tc.name;
        EXPECT_FLOAT_EQ(negativeVelocity.z, positiveVelocity.z) << "难度 " << tc.name;
    }
}

// ============================================================================
// 负 inaccuracy 方向保持测试
// ============================================================================

TEST(ProjectileShootInaccuracyTest, NegativeInaccuracy_DoesNotReverseDirection)
{
    // 负 inaccuracy 不应导致方向向量反转
    // 即使 inaccuracy 为负，shoot 后的速度方向仍应与输入方向同向
    TestProjectile p(EntityInstanceId(1));

    // 沿 +X 方向射击，负 inaccuracy
    p.shoot(1.0f, 0.0f, 0.0f, 1.0f, -5.0f);

    // X 分量应仍为正（方向未反转）
    // inaccuracy=5 时高斯偏移最大约 5*0.0075*3σ ≈ 0.1125，远小于 1.0
    EXPECT_GT(p.velocityX(), 0.0f);
}

TEST(ProjectileShootInaccuracyTest, NegativeInaccuracy_DoesNotReverseDirection_ForHardDifficulty)
{
    // 验证 Hard 难度（inaccuracy=-7，绝对值最大）下方向也不反转
    TestProjectile p(EntityInstanceId(1));

    p.shoot(1.0f, 0.0f, 0.0f, 0.7f, -7.0f);

    // X 分量应仍为正
    EXPECT_GT(p.velocityX(), 0.0f);
}

TEST(ProjectileShootInaccuracyTest, NegativeInaccuracy_DoesNotReverseDirection_ForLargeInaccuracy)
{
    // 验证即使 inaccuracy 绝对值很大（如 -14，对应弓/弩 Peaceful 难度散布绝对值），
    // 方向也不会反转
    TestProjectile p(EntityInstanceId(1));

    p.shoot(1.0f, 0.0f, 0.0f, 1.0f, -14.0f);

    // X 分量应仍为正（虽然散布很大，但 14*0.0075*3σ ≈ 0.315 仍小于 1.0）
    EXPECT_GT(p.velocityX(), 0.0f);
}

// ============================================================================
// 负 inaccuracy 散布应用测试
// ============================================================================

TEST(ProjectileShootInaccuracyTest, NegativeInaccuracy_AppliesSpread)
{
    // 负 inaccuracy 应实际应用散布（而非跳过散布计算）
    // 验证方法：负 inaccuracy 后的速度与 inaccuracy=0 的速度不同
    // 由于高斯分布有理论概率产生接近 0 的偏移，使用较大 inaccuracy 确保偏移可见

    // inaccuracy=0 的基准速度
    TestProjectile pZero(EntityInstanceId(1));
    pZero.shoot(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    const Vector3 zeroVelocity = pZero.velocity();

    // inaccuracy=-7 的速度（Hard 难度旋风人风弹）
    TestProjectile pNegative(EntityInstanceId(1));
    pNegative.shoot(1.0f, 0.0f, 0.0f, 1.0f, -7.0f);
    const Vector3 negativeVelocity = pNegative.velocity();

    // 两者速度应不同（散布被应用）
    // 注意：理论上高斯分布可能恰好产生 0 偏移，但 inaccuracy=7 时概率极低
    const bool velocityDiffers = (std::abs(negativeVelocity.x - zeroVelocity.x) > 1e-6f) ||
        (std::abs(negativeVelocity.y - zeroVelocity.y) > 1e-6f) ||
        (std::abs(negativeVelocity.z - zeroVelocity.z) > 1e-6f);
    EXPECT_TRUE(velocityDiffers) << "负 inaccuracy 未应用散布（速度与 inaccuracy=0 相同）";
}

TEST(ProjectileShootInaccuracyTest, PositiveInaccuracy_AppliesSpread)
{
    // 对照测试：正 inaccuracy 同样应用散布
    TestProjectile pZero(EntityInstanceId(1));
    pZero.shoot(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    const Vector3 zeroVelocity = pZero.velocity();

    TestProjectile pPositive(EntityInstanceId(1));
    pPositive.shoot(1.0f, 0.0f, 0.0f, 1.0f, 7.0f);
    const Vector3 positiveVelocity = pPositive.velocity();

    const bool velocityDiffers = (std::abs(positiveVelocity.x - zeroVelocity.x) > 1e-6f) ||
        (std::abs(positiveVelocity.y - zeroVelocity.y) > 1e-6f) ||
        (std::abs(positiveVelocity.z - zeroVelocity.z) > 1e-6f);
    EXPECT_TRUE(velocityDiffers) << "正 inaccuracy 未应用散布";
}

// ============================================================================
// 旋风人风弹场景模拟测试
// ============================================================================

TEST(ProjectileShootInaccuracyTest, BreezeWindChargeScenario_AllDifficulties)
{
    // 模拟 MC 1.21.11 旋风人风弹在所有难度下的射击场景
    // 验证 shoot 能正确处理所有难度的 inaccuracy 值，不崩溃、不反转方向
    //
    // 各难度 inaccuracy（公式 5 - difficulty.getId() * 4）：
    // - Peaceful (id=0): 5
    // - Easy (id=1): 1
    // - Normal (id=2): -3
    // - Hard (id=3): -7

    struct DifficultyScenario {
        const char* name;
        f32 inaccuracy;
    };

    const DifficultyScenario scenarios[] = {
        {"Peaceful", 5.0f},
        {"Easy", 1.0f},
        {"Normal", -3.0f},
        {"Hard", -7.0f},
    };

    for (const auto& scenario : scenarios) {
        TestProjectile p(EntityInstanceId(1));
        // 模拟旋风人射击：速度 0.7，方向 +X
        p.shoot(1.0f, 0.0f, 0.0f, 0.7f, scenario.inaccuracy);

        // X 分量应为正（方向未反转）
        EXPECT_GT(p.velocityX(), 0.0f) << "难度 " << scenario.name << " 下方向反转";
    }
}

// ============================================================================
// shootFrom 一致性测试
// ============================================================================

TEST(ProjectileShootInaccuracyTest, ShootFrom_WithNegativeInaccuracy_DoesNotCrash)
{
    // 验证 shootFrom（封装 shoot 的辅助方法）也能正确处理负 inaccuracy
    // 使用一个简单的 Entity 作为 shooter
    class SimpleShooter : public Entity {
    public:
        explicit SimpleShooter(EntityInstanceId id)
            : Entity(id)
        {}
        [[nodiscard]] std::string getTypeId() const override { return "minecraft:test_shooter"; }
    };

    SimpleShooter shooter(EntityInstanceId(1));
    TestProjectile p(EntityInstanceId(2));

    // pitch=0, yaw=0, pitchOffset=0 → 方向 (0, 0, 1)
    // 负 inaccuracy 应正常处理
    p.shootFrom(shooter, 0.0f, 0.0f, 0.0f, 1.0f, -3.0f);

    // Z 分量应为正（方向 (0,0,1) 未反转）
    EXPECT_GT(p.velocityZ(), 0.0f);
}
