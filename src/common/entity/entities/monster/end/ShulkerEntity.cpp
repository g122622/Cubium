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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ShulkerEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/special/ShulkerGoals.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/attribute/AttributeModifier.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/projectile/OtherProjectiles.hpp"
#include "entity/registry/VanillaEntityTypeKeys.hpp"
#include "sound/SoundEvents.hpp"
#include "util/Direction.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/WorldConstants.hpp"
#include "world/block/Block.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace mc {

// 常量定义
static const std::string COVERED_ARMOR_BONUS_ID = "shulker_covered_armor_bonus";

// ============================================================================
// ShulkerEntity 实现
// ============================================================================

ShulkerEntity::ShulkerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    // 潜影贝不移动
    setExperienceValue(5);

    // 注意：MonsterEntity 基类构造虽调用了 registerGoals()/registerAttributes()，但 C++ 基类构造期
    // 虚函数不派发到派生类（vtable 此时仍是 MonsterEntity 的），导致 ShulkerEntity 的 override 版本
    // 不会被调用——AI 目标（射弹反击等）与属性（MAX_HEALTH=30 等）将丢失。故在此显式补调，
    // 此时 vtable 已就绪。与 LivingEntity 生命值同步修复（commit 340f9c235）同源根因。
    registerAttributes();
    registerGoals();
}

std::unique_ptr<Entity> ShulkerEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ShulkerEntity>(EntityInstanceId(0), registry);
}

void ShulkerEntity::updatePeekTicks(i32 peekTicks)
{
    m_peekTicks = std::clamp(peekTicks, 0, 100);

    if (m_world != nullptr && !m_world->isClientSide()) {
        // 闭合时（peekTicks == 0）获得额外护甲
        if (m_peekTicks == 0) {
            // 添加护甲加成
            entity::attribute::AttributeModifier modifier(COVERED_ARMOR_BONUS_ID,
                "Shulker covered armor bonus",
                ARMOR_BONUS,
                entity::attribute::Operation::Addition);
            attributes().addModifier(entity::attribute::Attributes::ARMOR, modifier);
            playCloseSound();
        } else {
            // 移除护甲加成
            attributes().removeModifier(entity::attribute::Attributes::ARMOR, COVERED_ARMOR_BONUS_ID);
            playOpenSound();
        }
    }
}

void ShulkerEntity::openShell()
{
    if (m_shellState == ShellState::Closed) {
        m_shellState = ShellState::Opening;
        m_shellStateTime = OPEN_DURATION;
        updatePeekTicks(100); // 打开时设置 peek ticks
    }
}

void ShulkerEntity::closeShell()
{
    if (m_shellState == ShellState::Open) {
        m_shellState = ShellState::Closing;
        m_shellStateTime = CLOSE_DURATION;
        updatePeekTicks(0); // 关闭时清零 peek ticks
    }
}

bool ShulkerEntity::isImmuneToDamage() const
{
    // 贝壳完全闭合时免疫
    return m_shellState == ShellState::Closed && m_peekTicks == 0;
}

bool ShulkerEntity::teleport()
{
    return _tryTeleportToNewPosition();
}

bool ShulkerEntity::_tryTeleportToNewPosition()
{
    // 检查AI是否禁用
    if (!m_aiEnabled || !isAlive()) {
        return true;
    }

    // 获取当前位置
    BlockPos currentPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));

    for (i32 i = 0; i < TELEPORT_ATTEMPTS; ++i) {
        // 随机目标位置（±8格）
        math::Random& rng = m_world->getRandom();
        i32 targetX = currentPos.x + rng.nextInt(TELEPORT_RANGE * 2 + 1) - TELEPORT_RANGE;
        i32 targetY = currentPos.y + rng.nextInt(TELEPORT_RANGE * 2 + 1) - TELEPORT_RANGE;
        i32 targetZ = currentPos.z + rng.nextInt(TELEPORT_RANGE * 2 + 1) - TELEPORT_RANGE;
        BlockPos targetPos(targetX, targetY, targetZ);

        // 检查高度是否有效
        if (targetPos.y <= world::MIN_BUILD_HEIGHT) {
            continue;
        }

        // 检查是否是空气方块
        const BlockState* state = m_world->getBlockState(targetPos);
        if (state == nullptr || !state->isAir()) {
            continue;
        }

        // 检查碰撞
        AxisAlignedBB testBox(
            targetPos.x, targetPos.y, targetPos.z, targetPos.x + 1.0, targetPos.y + 1.0, targetPos.z + 1.0);
        if (m_world->hasBlockCollision(testBox)) {
            continue;
        }

        // 找到可以附着的方向
        std::optional<Direction> facing = _findValidFacing(targetPos);
        if (!facing.has_value()) {
            continue;
        }

        // 瞬移成功
        m_attachmentFacing = facing.value();
        playTeleportSound();
        setAttachmentPos(targetPos);
        updatePeekTicks(0);
        setAttackTarget(nullptr);
        return true;
    }

    return false;
}

bool ShulkerEntity::_canAttachAt(const BlockPos& pos, Direction facing) const
{
    if (m_world == nullptr) {
        return false;
    }

    // 检查附着方块
    BlockPos attachPos(
        pos.x + Directions::xOffset(facing), pos.y + Directions::yOffset(facing), pos.z + Directions::zOffset(facing));
    const BlockState* attachState = m_world->getBlockState(attachPos);
    if (attachState == nullptr) {
        return false;
    }

    // 检查方块是否是固体
    if (!attachState->blocksMovement()) {
        return false;
    }

    // 检查潜影贝位置是否没有碰撞
    AxisAlignedBB shulkerBox(pos.x, pos.y, pos.z, pos.x + 1.0, pos.y + 1.0, pos.z + 1.0);
    return !m_world->hasBlockCollision(shulkerBox);
}

std::optional<Direction> ShulkerEntity::_findValidFacing(const BlockPos& pos) const
{
    for (Direction dir : Directions::all()) {
        if (_canAttachAt(pos, dir)) {
            return dir;
        }
    }
    return std::nullopt;
}

void ShulkerEntity::shootBullet()
{
    if (m_attackCooldown > 0) {
        return;
    }

    LivingEntity* target = attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return;
    }

    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }

    // 创建潜影贝子弹
    auto bullet = std::make_unique<entity::ShulkerBulletEntity>(
        m_world, this, target, Directions::getAxis(m_attachmentFacing), *registry);
    bullet->setTypeId(entity::EntityTypeKeys::SHULKER_BULLET); // 工厂绕过补救：直接构造缺 typeId
    m_world->spawnEntity(std::move(bullet));

    // 设置攻击冷却
    math::Random& rng = m_world->getRandom();
    m_attackCooldown = ATTACK_COOLDOWN_MIN + rng.nextInt(ATTACK_COOLDOWN_RANDOM / 2);
    m_attacking = true;

    playShootSound();
}

void ShulkerEntity::_updateShellState()
{
    // 更新开壳动画
    f32 targetPeek = static_cast<f32>(m_peekTicks) * 0.01f;
    m_prevPeekAmount = m_peekAmount;

    if (m_peekAmount > targetPeek) {
        m_peekAmount = std::max(m_peekAmount - 0.05f, targetPeek);
    } else if (m_peekAmount < targetPeek) {
        m_peekAmount = std::min(m_peekAmount + 0.05f, targetPeek);
    }

    // 更新状态
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

    // 更新附着位置
    if (m_attachmentPos.x == 0 && m_attachmentPos.y == 0 && m_attachmentPos.z == 0) {
        // 初始化附着位置
        m_attachmentPos = BlockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    }

    // 更新贝壳状态
    _updateShellState();

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 更新瞬移冷却
    if (m_teleportCooldown > 0) {
        m_teleportCooldown--;
    }
}

bool ShulkerEntity::hurt(DamageSource& source, f32 amount)
{
    // 闭合时仅免疫箭矢类投射物（对齐 vanilla 1.21.11 Shulker.hurtServer:411-418：
    //   isClosed() 时仅对 instanceof AbstractArrow return false，其他投射物正常受伤）。
    // AbstractArrow 含普通箭/光灵箭/三叉戟（均继承 AbstractArrowEntity，对齐 vanilla 继承体系）。
    // 此前偏差：闭壳对所有投射物（火球/雪球/鸡蛋/末影珍珠/药水/羊驼唾沫/潜影弹）都免疫，
    //   范围过宽偏离 vanilla（vanilla 闭壳潜影贝受火球等非箭投射物伤害）。改为仅箭矢免疫。
    // 注意：DEFLECTS_PROJECTILES 标签此前误含 shulker 致箭被无条件偏转弹开（连 hurt 都不进），
    //   已从标签移除（EntityTypeTags 对齐 vanilla 仅 breeze），故箭现在进 hurt 由本分支判定。
    if (isShellClosed()) {
        Entity* attacker = source.directSource();
        if (attacker != nullptr && dynamic_cast<const entity::AbstractArrowEntity*>(attacker) != nullptr) {
            return false;
        }
    }

    // 调用父类受伤
    if (MonsterEntity::hurt(source, amount)) {
        // 血量低于一半时有概率瞬移
        if (health() < maxHealth() * 0.5f && m_world != nullptr) {
            math::Random& rng = m_world->getRandom();
            if (rng.nextInt(4) == 0) {
                _tryTeleportToNewPosition();
            }
        }
        return true;
    }
    return false;
}

void ShulkerEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 目标选择器优先级
    // 1: HurtByTargetGoal（被攻击时反击，呼唤同伴）
    // 2: AttackNearestGoal（攻击最近的玩家）
    // 3: DefenseAttackGoal（防御攻击IMob）

    // 行为目标优先级
    // 1: LookAtGoal（看向玩家）
    // 4: AttackGoal（发射子弹攻击）
    // 7: PeekGoal（空闲时开壳张望）
    // 8: LookRandomlyGoal（随机看向）

    // 目标选择器
    // MC 原版: HurtByTargetGoal(this, this.getClass()).setAlertOthers()
    // 潜影贝不会反击其他潜影贝，但会警醒同类
    targetSelector().addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            // MC 原版使用 this.getClass()，即 Shulker.class
            // 潜影贝不会反击同类
            return attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::SHULKER;
        }));
    // 优先级2: 攻击最近的玩家（和平难度下不执行）
    targetSelector().addGoal(2, std::make_unique<entity::ai::goal::ShulkerNearestAttackGoal>(this));
    // 优先级3: 防御攻击——当潜影贝处于队伍中时，攻击附近的敌对生物（IMob）
    targetSelector().addGoal(3, std::make_unique<entity::ai::goal::ShulkerDefenseAttackGoal>(this));

    // 行为目标
    // LookAtGoal: 看向玩家，距离8格，概率0.02
    goalSelector().addGoal(
        1, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return dynamic_cast<const Player*>(entity) != nullptr;
        }));
    // ShulkerAttackGoal: 发射子弹攻击
    goalSelector().addGoal(4, std::make_unique<entity::ai::goal::ShulkerAttackGoal>(this));
    // ShulkerPeekGoal: 空闲时开壳张望
    goalSelector().addGoal(7, std::make_unique<entity::ai::goal::ShulkerPeekGoal>(this));
    goalSelector().addGoal(8, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));
}

void ShulkerEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0f);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0f); // 不移动
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 18.0f);
}

std::optional<ResourceLocation> ShulkerEntity::getAmbientSound() const
{
    // MC 1.16.5: 只有贝壳打开时才播放环境音效
    if (isShellClosed()) {
        return std::nullopt;
    }
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
    playSound(SoundEvents::ENTITY_SHULKER_SHOOT, 2.0f, 1.0f);
}

void ShulkerEntity::playTeleportSound()
{
    playSound(SoundEvents::ENTITY_SHULKER_TELEPORT, 1.0f, 1.0f);
}

} // namespace mc
