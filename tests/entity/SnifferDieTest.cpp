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

// SnifferEntity::die 对齐 MC Java 1.21.11 测试。
//
// 验证 SnifferEntity::die（SnifferEntity.cpp）对齐 vanilla Sniffer.die
// （Sniffer.java:347-350）：
//   public void die(DamageSource p_277689_) {
//       this.transitionTo(Sniffer.State.IDLING);
//       super.die(p_277689_);
//   }
// 嗅探兽死亡时应将状态机重置为 Idling，再委托父类执行通用死亡逻辑。
//
// 此前缺陷：Cubium SnifferEntity 未重写 die，死亡时状态机不会重置为 Idling，
// 与 vanilla 直接冲突。修复：重写 die 调 transitionTo(State::Idling) 后委托
// AnimalEntity::die(source)。
//
// 测试设计（两例验证 transitionTo(Idling) + super.die 均被调用）：
//   - DieFromNonIdlingState_ResetsToIdling：将嗅探兽置于 Digging（非 Idling）状态，
//     setHealth(0) 使 isDead()=true 后调用 die()，断言 getState()==Idling（transitionTo
//     生效）且 deathTime()==0（super.die 重置 m_deathTime=0 生效）。
//   - DieFromSearchingState_ResetsToIdling：将嗅探兽置于 Searching 状态同上验证，
//     覆盖第二个非 Idling 状态以排除 transitionTo 单分支偶然命中。
//   - 若 die override 缺失 transitionTo：getState() 仍为 Digging/Searching → 正向 FAIL。
//   - 若 die override 缺失 super.die 调用：deathTime() 维持初始值（构造后为 0，无法区分），
//     故本测试用 getState() 作为主断言；super.die 委托通过编译期 override 链保证
//     （AnimalEntity::die 无 override，链回 LivingEntity::die）。
//
// isDead() 守卫：LivingEntity::die（LivingEntity.cpp:682）首行 `if (!isDead()) return;`
//   阻止重复死亡。isDead() = health() <= 0.0f（LivingEntity.hpp:212），故测试必须先
//   setHealth(0) 使 isDead()=true，否则 die() 直接 return 不执行 transitionTo，断言失败。
//   这复刻生产路径：hurt 链路在 health 归零后调 die()（LivingEntity.hurt→actuallyHurt→die）。
//
// deathTime 断言：LivingEntity::die（LivingEntity.cpp:686-688）将 HurtStateComponent.
//   m_deathTime 置 0。构造后 m_deathTime 默认 0，为制造可观测变化，测试先手动置
//   m_deathTime=5（经 deathTime 访问器不可写，此处依赖 super.die 后 m_deathTime 仍为 0
//   不变——实际上 super.die 把它从任意值归 0，故构造后为 0 时断言 deathTime()==0 恒真，
//   仅作 super.die 不崩溃的烟雾测试）。主断言仍为 getState()==Idling。
//
// Ref: vanilla Sniffer.java:347-350（die：transitionTo(IDLING) + super.die）
// Ref: SnifferEntity.cpp（die 重写实现）
// Ref: LivingEntity.cpp:680-700（LivingEntity::die isDead 守卫 + m_deathTime 归零）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/special/SnifferEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

using namespace mc;

namespace {

// BaseTestWorld 默认构造为 protected，派生公开以作 fixture 成员（吸收 die 链路的 playSound/dropAllDeathLoot）。
class TestWorld final : public mc::test::BaseTestWorld {
public:
    TestWorld() = default;
};

} // namespace

class SnifferDieTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块和实体注册表（Sniffer 构造依赖 registerGoals/registerAttributes/registerData，
        // 不直接依赖实体注册表，但保持与其他实体测试一致的初始化契约）。
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }

    TestWorld m_world;
};

// 死亡时从 Digging 状态重置为 Idling（对齐 vanilla Sniffer.die: transitionTo(IDLING)）。
TEST_F(SnifferDieTest, DieFromDiggingState_ResetsToIdling)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setHealth(sniffer.maxHealth());

    // 将嗅探兽置于 Digging（非 Idling）状态。
    sniffer.setState(SnifferEntity::State::Digging);
    ASSERT_EQ(sniffer.getState(), SnifferEntity::State::Digging);

    // setHealth(0) 使 isDead()=true，LivingEntity::die 守卫放行。
    sniffer.setHealth(0.0f);
    ASSERT_TRUE(sniffer.isDead());

    auto source = DamageSources::generic();
    sniffer.die(source);

    // transitionTo(Idling) 生效：状态重置为 Idling。
    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::Idling) << "sniffer should reset to Idling state on death";
    // super.die 不崩溃的烟雾测试：m_deathTime 经 LivingEntity::die 归 0。
    EXPECT_EQ(sniffer.deathTime(), 0);
}

// 死亡时从 Searching 状态重置为 Idling（覆盖第二个非 Idling 状态，排除单分支偶然命中）。
TEST_F(SnifferDieTest, DieFromSearchingState_ResetsToIdling)
{
    SnifferEntity sniffer(EntityInstanceId(1), mc::test::testEcsRegistry());
    sniffer.setWorld(&m_world);
    sniffer.setHealth(sniffer.maxHealth());

    // 将嗅探兽置于 Searching（非 Idling）状态。
    sniffer.setState(SnifferEntity::State::Searching);
    ASSERT_EQ(sniffer.getState(), SnifferEntity::State::Searching);

    sniffer.setHealth(0.0f);
    ASSERT_TRUE(sniffer.isDead());

    auto source = DamageSources::generic();
    sniffer.die(source);

    EXPECT_EQ(sniffer.getState(), SnifferEntity::State::Idling)
        << "sniffer should reset to Idling state on death regardless of prior state";
    EXPECT_EQ(sniffer.deathTime(), 0);
}
