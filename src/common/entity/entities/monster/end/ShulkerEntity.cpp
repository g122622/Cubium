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

#include "ShulkerEntity.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
#include <memory>

namespace mc {

ShulkerEntity::ShulkerEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 潜影贝不移动
    // TODO: 禁用移动控制器
}

std::unique_ptr<Entity> ShulkerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ShulkerEntity>(LegacyEntityType::Unknown, 0);
}

void ShulkerEntity::openShell()
{
    if (m_shellState == ShellState::Closed) {
        m_shellState = ShellState::Opening;
        m_shellStateTime = OPEN_DURATION;
    }
}

void ShulkerEntity::closeShell()
{
    if (m_shellState == ShellState::Open) {
        m_shellState = ShellState::Closing;
        m_shellStateTime = CLOSE_DURATION;
    }
}

bool ShulkerEntity::isImmuneToDamage() const
{
    return m_shellState == ShellState::Closed;
}

void ShulkerEntity::teleport()
{
    // TODO: 实现瞬移逻辑
    // 随机选择附近的一个有效位置
}

void ShulkerEntity::shootBullet()
{
    if (m_attackCooldown > 0) {
        return;
    }

    // TODO: 生成潜影贝子弹
    // auto bullet = std::make_unique<ShulkerBulletEntity>(LegacyEntityType::Unknown, 0);
    // bullet->setOwner(this);
    // bullet->setTarget(getAttackTarget());
    // world().spawnEntity(std::move(bullet), position());

    m_attackCooldown = ATTACK_COOLDOWN;
    m_attacking = true;
}

void ShulkerEntity::updateShellState()
{
    if (m_shellStateTime > 0) {
        m_shellStateTime--;

        if (m_shellStateTime <= 0) {
            switch (m_shellState) {
                case ShellState::Opening:
                    m_shellState = ShellState::Open;
                    break;
                case ShellState::Closing:
                    m_shellState = ShellState::Closed;
                    break;
                default:
                    break;
            }
        }
    }
}

void ShulkerEntity::tick()
{
    MonsterEntity::tick();

    // 更新贝壳状态
    updateShellState();

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 如果闭合且受到伤害，瞬移
    // TODO: 检测伤害并瞬移
}

void ShulkerEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // TODO: 潜影贝特有 AI 目标
    // - ShulkerAttackGoal (发射子弹攻击)
    // - ShulkerDefenseGoal (闭合贝壳防御)
    // - ShulkerTeleportGoal (受伤瞬移)
}

void ShulkerEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    // 潜影贝属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0f); // 不移动
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 18.0f);
}

std::optional<ResourceLocation> ShulkerEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_SHULKER_AMBIENT;
}

std::optional<ResourceLocation> ShulkerEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: 贝壳闭合时使用不同的受伤音效
    if (isShellClosed()) {
        return SoundEvents::ENTITY_SHULKER_HURT_CLOSED;
    }
    return SoundEvents::ENTITY_SHULKER_HURT;
}

std::optional<ResourceLocation> ShulkerEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_SHULKER_DEATH;
}

void ShulkerEntity::playOpenSound()
{
    playSound(SoundEvents::ENTITY_SHULKER_OPEN, 1.0f, 1.0f);
}

void ShulkerEntity::playCloseSound()
{
    playSound(SoundEvents::ENTITY_SHULKER_CLOSE, 1.0f, 1.0f);
}

void ShulkerEntity::playShootSound()
{
    playSound(SoundEvents::ENTITY_SHULKER_SHOOT, 1.0f, 1.0f);
}

void ShulkerEntity::playTeleportSound()
{
    playSound(SoundEvents::ENTITY_SHULKER_TELEPORT, 1.0f, 1.0f);
}

} // namespace mc
