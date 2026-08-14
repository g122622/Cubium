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

#include "SpellcastingIllagerEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/entities/monster/illager/AbstractIllagerEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>

namespace mc {

// ============================================================================
// 继承链标识（parent = AbstractIllagerEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& SpellcastingIllagerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"SpellcastingIllagerEntity", &AbstractIllagerEntity::classInfo()};
    return s_classInfo;
}

SpellcastingIllagerEntity::SpellcastingIllagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractIllagerEntity(id, registry)
{}

SpellcastingIllagerEntity::SpellType SpellcastingIllagerEntity::spellTypeFromId(i32 id) noexcept
{
    switch (id) {
        case 1:
            return SpellType::SummonVex;
        case 2:
            return SpellType::Fangs;
        case 3:
            return SpellType::Wololo;
        case 4:
            return SpellType::Disappear;
        case 5:
            return SpellType::Blindness;
        default:
            return SpellType::None;
    }
}

Vector3 SpellcastingIllagerEntity::getSpellParticleColor(SpellType type) noexcept
{
    // 粒子颜色通过速度参数传递给 ENTITY_EFFECT 粒子
    switch (type) {
        case SpellType::SummonVex:
            // 召唤恼鬼 - 淡蓝白色
            return Vector3(0.7f, 0.7f, 0.8f);
        case SpellType::Fangs:
            // 尖牙攻击 - 棕色
            return Vector3(0.4f, 0.3f, 0.35f);
        case SpellType::Wololo:
            // 唔噜噜法术（羊变色）- 橙黄色
            return Vector3(0.7f, 0.5f, 0.2f);
        case SpellType::Disappear:
            // 消失/镜像法术 - 蓝色
            return Vector3(0.3f, 0.3f, 0.8f);
        case SpellType::Blindness:
            // 失明法术 - 深蓝/深紫色
            return Vector3(0.1f, 0.1f, 0.2f);
        default:
            // 无施法 - 黑色（不显示粒子）
            return Vector3(0.0f, 0.0f, 0.0f);
    }
}

void SpellcastingIllagerEntity::tick()
{
    AbstractIllagerEntity::tick();

    if (m_spellTicks > 0) {
        --m_spellTicks;
    }

    // 客户端施法粒子效果
    if (m_world && m_world->isClientSide() && isSpellcasting()) {
        SpellType currentSpell = spellType();
        if (currentSpell != SpellType::None) {
            // 获取粒子颜色
            Vector3 particleColor = getSpellParticleColor(currentSpell);

            // 计算粒子位置的角度
            f32 angle =
                renderYawOffset() * math::DEG_TO_RAD + std::cos(static_cast<f32>(ticksExisted()) * 0.6662f) * 0.25f;
            f32 cosAngle = std::cos(angle);
            f32 sinAngle = std::sin(angle);

            // 在实体左右两侧各生成一个粒子
            constexpr f32 lateralOffset = 0.6f; // 左右偏移
            constexpr f32 heightOffset = 1.8f;  // 头部高度偏移

            Vector3 pos = position();
            Vector3 velocity = particleColor; // 颜色作为速度参数传递

            // 右侧粒子
            m_world->addParticle(particle::ParticleTypeId::EntityEffect,
                Vector3(pos.x + static_cast<f64>(cosAngle) * lateralOffset,
                    pos.y + heightOffset,
                    pos.z + static_cast<f64>(sinAngle) * lateralOffset),
                velocity);

            // 左侧粒子
            m_world->addParticle(particle::ParticleTypeId::EntityEffect,
                Vector3(pos.x - static_cast<f64>(cosAngle) * lateralOffset,
                    pos.y + heightOffset,
                    pos.z - static_cast<f64>(sinAngle) * lateralOffset),
                velocity);
        }
    }
}

} // namespace mc
