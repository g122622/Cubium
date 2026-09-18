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

// IS_EXPLOSION 标签遗漏 Fireworks + ALWAYS_HURTS_ENDER_DRAGONS 硬编码代标签 修复测试。
//
// 验证两处关联缺陷修复（对齐 vanilla 1.21.11）：
//
// 1. EnvironmentalDamage::isExplosion()（DamageSource.hpp）此前硬编码仅含
//    Explosion/ExplosionPlayer/BadRespawnPoint，遗漏 Fireworks（注释明示应含 fireworks）。
//    DamageTypeTags::IS_EXPLOSION 标签成员含 fireworks（DamageTypeTags.cpp:585）。
//    修复：isExplosion() 补 DamageType::Fireworks。
//
// 2. EnderDragonEntity::attackEntityPartFrom（EnderDragonEntity.cpp:294）此前用
//    source.isExplosion() flag 代替 source.is(DamageTypeTags::ALWAYS_HURTS_ENDER_DRAGONS())
//    标签查询（vanilla EnderDragon.java:471）。"硬编码代标签"缺陷：数据包扩展标签无效。
//    修复：改查 ALWAYS_HURTS_ENDER_DRAGONS 标签（成员=#is_explosion，含 fireworks）。
//
// 连带偏差（修复前）：烟花爆炸伤害（DamageSources::fireworks()）既不被 isExplosion() 识别、
// 也不被末影龙接受（canHurt=false），导致：
//   - 烟花无法伤害末影龙（vanilla 烟花可伤龙，因 ALWAYS_HURTS_ENDER_DRAGONS 含 fireworks）
//   - 烟花不走爆炸保护附魔（LivingEntity.cpp:558 EXPLOSION flag）
//   - 末影水晶被烟花炸毁时误触二次爆炸（EffectEntities.cpp:277 !isExplosion 判定）
//
// 测试设计（3 例）：
//   - FireworksIsExplosionFlag：DamageSources::fireworks().isExplosion()==true（验证 isExplosion 补 Fireworks）
//   - FireworksInAlwaysHurtsEnderDragonsTag：fireworks.is(ALWAYS_HURTS_ENDER_DRAGONS())==true（标签成员含 fireworks）
//   - FireworksDamagesEnderDragon：烟花伤害经 attackEntityPartFrom 头部 → 龙受伤（修复前 canHurt=false 被拒）
//
// Ref: vanilla EnderDragon.java:471（ALWAYS_HURTS_ENDER_DRAGONS 标签查询）
// Ref: vanilla DamageTypeTagsProvider.java（IS_EXPLOSION 含 fireworks、ALWAYS_HURTS_ENDER_DRAGONS=#is_explosion）
// Ref: DamageSource.hpp（isExplosion 补 Fireworks）
// Ref: EnderDragonEntity.cpp:294（attackEntityPartFrom 改查标签）

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/boss/EnderDragonEntity.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

// 最小测试世界：末影龙 attackEntityPartFrom 仅依赖 isInvulnerableTo（BaseTestWorld 默认 false）
// 与 canHurt 判定，无需方块/物理引擎。复用 BaseTestWorld 默认实现即可。
class DragonExplosionTestWorld final : public mc::test::BaseTestWorld {
public:
    DragonExplosionTestWorld() = default;

    [[nodiscard]] world::tick::TickManager& tickManager() override { throw std::runtime_error("not implemented"); }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("not implemented");
    }
};

} // namespace

// 烟花伤害源的 isExplosion() 标志应为 true（验证 isExplosion 补 DamageType::Fireworks）。
TEST(EnderDragonExplosionTest, FireworksIsExplosionFlag)
{
    auto dmg = DamageSources::fireworks();
    EXPECT_TRUE(dmg.isExplosion());
}

// 烟花伤害源属于 ALWAYS_HURTS_ENDER_DRAGONS 标签（成员=#is_explosion 含 fireworks）。
TEST(EnderDragonExplosionTest, FireworksInAlwaysHurtsEnderDragonsTag)
{
    auto dmg = DamageSources::fireworks();
    EXPECT_TRUE(dmg.is(DamageTypeTags::ALWAYS_HURTS_ENDER_DRAGONS()));
}

// 烟花爆炸伤害应能伤害末影龙（修复前 isExplosion 漏 Fireworks + 用 flag 代标签致 canHurt=false）。
//
// vanilla EnderDragon.attackEntityPartFrom:471：source.is(ALWAYS_HURTS_ENDER_DRAGONS) 时可伤龙。
// 烟花在标签内 → 头部受 10.0 伤害应全额扣血。修复前 canHurt=false → attackEntityPartFrom 返 false、不扣血。
TEST(EnderDragonExplosionTest, FireworksDamagesEnderDragon)
{
    DragonExplosionTestWorld world;
    entity::EnderDragonEntity dragon(EntityInstanceId(1), mc::test::testEcsRegistry());
    dragon.setWorld(&world);
    dragon.setHealth(200.0f);

    entity::EnderDragonPartEntity headPart(EntityInstanceId(2), mc::test::testEcsRegistry());
    headPart.setPart(entity::EnderDragonPartEntity::Part::Head);

    auto fireworksDmg = DamageSources::fireworks();
    const f32 healthBefore = dragon.health();
    const bool result = dragon.attackEntityPartFrom(&headPart, fireworksDmg, 10.0f);

    // 烟花在 ALWAYS_HURTS_ENDER_DRAGONS 标签内 → canHurt=true → 受伤
    EXPECT_TRUE(result);
    // 头部伤害不减伤：扣 10.0
    EXPECT_NEAR(dragon.health(), healthBefore - 10.0f, 0.01f);
}

} // namespace mc
