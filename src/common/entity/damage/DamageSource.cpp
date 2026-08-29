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

#include "DamageSource.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/Vector3.hpp"
#include <optional>

namespace mc {

// DamageSource::sourcePosition() 默认实现在头文件中（返回 nullopt）。
// 此处仅实现需要 Entity::position() 的实体来源子类，避免在 DamageSource.hpp
// 中引入完整 Entity 定义造成循环包含。

std::optional<math::Vector3f> EntityDamageSource::sourcePosition() const
{
    return (m_source != nullptr) ? std::optional<math::Vector3f>{m_source->position()} : std::nullopt;
}

std::optional<math::Vector3f> IndirectEntityDamageSource::sourcePosition() const
{
    // DamageSource.getSourcePosition：优先 directEntity.position()。
    return (m_directSource != nullptr) ? std::optional<math::Vector3f>{m_directSource->position()} : std::nullopt;
}

// isProjectile 查 DamageTypeTags::IS_PROJECTILE 标签（对齐 vanilla source.is(DamageTypeTags.IS_PROJECTILE)）。
// 此前两子类各自硬编码：EntityDamageSource 列 Arrow/Trident/MobProjectile/Fireball 四类型，
// IndirectEntityDamageSource 只查 m_isProjectile 标志位（依赖调用方 setProjectile）。
// 两实现都漏 IS_PROJECTILE 标签的其余成员（WitherSkull/Thrown/WindBurst/UnattributedFireball），
// 且 IndirectEntityDamageSource 在调用方漏 setProjectile 时（如箭矢 AbstractArrowEntity 手动构造
// 未走 arrow() 工厂、windBurst 工厂漏 setProjectile）直接返 false，致弹射物保护附魔 EPF 减伤
// 链路（applyPotionDamageCalculations 设 DamageFlags::PROJECTILE 位）失效。
//
// 修复：统一查 IS_PROJECTILE 标签，标签成员（Arrow/Trident/MobProjectile/Fireball/WitherSkull/
// Thrown/WindBurst/UnattributedFireball）自动正确，无需依赖调用方 setProjectile。IndirectEntityDamageSource
// 额外 OR m_isProjectile 标志位作保底（标签未初始化时回退，及未来非标签成员投射物的扩展点）。
// 标签未初始化（DamageTypeTags::isInitialized()==false，如部分单元测试夹具未调 initialize）时
// IS_PROJECTILE() 返回空标签，contains 返 false，此时 IndirectEntityDamageSource 仍可由 m_isProjectile
// 标志位判定（箭矢/风爆经工厂 setProjectile 设位），EntityDamageSource 无标志位则回退 false（其 type
// 实际不会是投射物，安全）。
bool EntityDamageSource::isProjectile() const
{
    return is(DamageTypeTags::IS_PROJECTILE());
}

bool IndirectEntityDamageSource::isProjectile() const
{
    return m_isProjectile || is(DamageTypeTags::IS_PROJECTILE());
}

// damageScaling 把 MC 1.21.11 数据包 damage_type 目录下各 JSON 的 "scaling" 值固化进代码。
// 对齐 vanilla DamageSource.scalesWithDifficulty()（DamageSource.java:90-96）所依据的 scaling 字段。
// 数据包统计：
//   - ALWAYS（无条件缩放）：explosion, player_explosion, sonic_boom, bad_respawn_point
//   - WHEN_CAUSED_BY_LIVING_NON_PLAYER（非玩家生物造成时缩放）：其余全部
//   - NEVER：vanilla 1.21.11 数据包无此取值
// 故此处 default 分支返回 WhenCausedByLivingNonPlayer（覆盖绝大多数伤害类型）。
DamageScaling damageScaling(DamageType type) noexcept
{
    switch (type) {
        // ALWAYS：床重生爆炸（"-intentional_game_design"）、末影龙等爆炸、监守者音爆、玩家爆炸
        case DamageType::Explosion:
        case DamageType::ExplosionPlayer:
        case DamageType::SonicBoom:
        case DamageType::BadRespawnPoint:
            return DamageScaling::Always;
        default:
            return DamageScaling::WhenCausedByLivingNonPlayer;
    }
}

// DamageSource::scalesWithDifficulty 对齐 vanilla DamageSource.scalesWithDifficulty()
// （DamageSource.java:90-96）。vanilla 依据伤害类型的 scaling 字段动态判定：
//   - NEVER  → false
//   - WHEN_CAUSED_BY_LIVING_NON_PLAYER → causingEntity instanceof LivingEntity
//                                        && !(causingEntity instanceof Player)
//   - ALWAYS → true
//
// Cubium DamageType 无数据驱动 scaling 字段加载（见任务 #352），故用 damageScaling(type())
// 把数据包值固化。getTrueSource() 对应 vanilla getEntity()（causingEntity 真凶）：
//   - EntityDamageSource::getTrueSource → m_source（攻击者本身）
//   - IndirectEntityDamageSource::getTrueSource → m_source（射击者，非投射物本身）
// 两者均返回"造成伤害的实体"，与 vanilla causingEntity 语义一致。
//
// 关键：WHEN_CAUSED_BY_LIVING_NON_PLAYER 须在运行时按 getTrueSource() 是否为非玩家 LivingEntity
// 判定。静态 flag 无法表达此语义——玩家射出的箭（mobProjectile 工厂 setDifficultyScaled 后 flag=true）
// 在 vanilla 应 false（causingEntity=Player），但 Cubium 旧 flag 机制会错误缩放。此实现修复该偏差。
bool DamageSource::scalesWithDifficulty() const
{
    const DamageScaling scaling = damageScaling(type());
    switch (scaling) {
        case DamageScaling::Never:
            return false;
        case DamageScaling::Always:
            return true;
        case DamageScaling::WhenCausedByLivingNonPlayer: {
            // vanilla：causingEntity instanceof LivingEntity && !(causingEntity instanceof Player)
            Entity* causingEntity = getTrueSource();
            if (causingEntity == nullptr) {
                return false; // 无造成者（纯环境伤害）→ 不缩放
            }
            // 必须先确认是 LivingEntity，再排除 Player（Player 也是 LivingEntity）
            LivingEntity* asLiving = dynamic_cast<LivingEntity*>(causingEntity);
            if (asLiving == nullptr) {
                return false; // 造成者非生物（如抛射物实体本身）→ 不缩放
            }
            Player* asPlayer = dynamic_cast<Player*>(causingEntity);
            return asPlayer == nullptr; // 非玩家生物造成 → 缩放
        }
    }
    return false; // 不可达，防御性返回
}

// DamageSource::isCreativePlayer 对齐 vanilla DamageSource.isCreativePlayer()
// （DamageSource.java:98-100）：`this.getEntity() instanceof Player player
// && player.getAbilities().instabuild`。
//
// getEntity() 对应 vanilla causingEntity（真凶/造成者）：
//   - EntityDamageSource::getEntity → m_source（攻击者本身）
//   - IndirectEntityDamageSource::getEntity → m_source（射击者，非投射物本身）
// 两者均返回"造成伤害的实体"，与 vanilla getEntity() 语义一致。
//
// 用 getEntity() 而非 getTrueSource()：vanilla isCreativePlayer 明确用 getEntity()，
// 基类 getTrueSource() 默认转发 getEntity()，二者在造成者语义上一致，但为精确对齐
// vanilla 方法名选用 getEntity()。
bool DamageSource::isCreativePlayer() const
{
    Entity* causingEntity = getEntity();
    if (causingEntity == nullptr) {
        return false; // 无造成者（纯环境伤害）→ 非创造玩家
    }
    Player* asPlayer = dynamic_cast<Player*>(causingEntity);
    if (asPlayer == nullptr) {
        return false; // 造成者非玩家 → false
    }
    return asPlayer->isCreative(); // 对齐 vanilla player.getAbilities().instabuild
}

} // namespace mc
