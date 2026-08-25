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

} // namespace mc
