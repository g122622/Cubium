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
#include "entity/damage/tag/DamageTypeTags.hpp"
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
        // 对齐 vanilla 1.21.11 Shulker.teleportSomewhere:388-394：unRide() → setAttachFace →
        // playSound → setPos(blockpos1.getX()+0.5, blockpos1.getY(), blockpos1.getZ()+0.5) →
        // gameEvent(TELEPORT) → entityData.set(PEEK,0) → setTarget(null)。Cubium 此前仅 setAttachmentPos
        // 而未 setPosition，致瞬移后实体视觉/逻辑位置不变（仅记录附着点），与 vanilla 偏离且使
        // 受伤逃脱（hurt 半血分支）实际无效。补 setPosition 真正移动实体。
        m_attachmentFacing = facing.value();
        playTeleportSound();
        setPosition(Vector3(
            static_cast<f32>(targetPos.x) + 0.5f, static_cast<f32>(targetPos.y), static_cast<f32>(targetPos.z) + 0.5f));
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
        // 对齐 vanilla 1.21.11 Shulker.hurtServer:423-430：父类扣血成功后，二选一：
        //   a) 血量低于最大值一半且 1/4 概率 → teleportSomewhere（受伤逃脱）
        //   b) 否则若伤害来源是 IS_PROJECTILE 且直接来源实体为 SHULKER_BULLET → hitByShulkerBullet
        //      （被同类潜影弹命中：瞬移到新位置并在原位繁殖一只同色潜影贝）
        // 注：vanilla 是 if/else if，即半血逃脱优先，未触发逃脱时才检查潜影弹命中回调。
        if (health() < maxHealth() * 0.5f && m_world != nullptr) {
            math::Random& rng = m_world->getRandom();
            if (rng.nextInt(4) == 0) {
                _tryTeleportToNewPosition();
            }
        } else if (source.is(DamageTypeTags::IS_PROJECTILE())) {
            // 仅潜影弹命中触发瞬移+繁殖（vanilla :426-429 getDirectEntity().getType()==SHULKER_BULLET）。
            // directSource 对 IndirectEntityDamageSource 返回投射物本身（即潜影弹实体）。
            // 用 dynamic_cast<ShulkerBulletEntity*> 判定直接来源是否潜影弹（ShulkerBulletEntity 无派生类，
            // 与 vanilla getType()==SHULKER_BULLET 精确类型相等语义一致；与闭壳分支 dynamic_cast<AbstractArrowEntity*>
            // 风格统一）。
            Entity* directEntity = source.directSource();
            if (dynamic_cast<const entity::ShulkerBulletEntity*>(directEntity) != nullptr) {
                // 记录受击前位置，hitByShulkerBullet 在瞬移成功后在原位繁殖新潜影贝。
                _hitByShulkerBullet(position());
            }
        }
        return true;
    }
    return false;
}

void ShulkerEntity::_hitByShulkerBullet(const Vector3& originalPos)
{
    // 对齐 vanilla 1.21.11 Shulker.hitByShulkerBullet（Shulker.java:440-455）：
    //   Vec3 vec3 = this.position(); AABB aabb = this.getBoundingBox();
    //   if (!isClosed() && teleportSomewhere()) {
    //       int i = level.getEntities(SHULKER, aabb.inflate(8.0), isAlive).size();
    //       float f = (i - 1) / 5.0F;
    //       if (!(random.nextFloat() < f)) {
    //           Shulker shulker = EntityType.SHULKER.create(level, BREEDING);
    //           shulker.setVariant(this.getVariant());
    //           shulker.snapTo(vec3);
    //           level.addFreshEntity(shulker);
    //       }
    //   }
    // 即：仅开壳潜影贝被潜影弹命中、且成功瞬移到新位置后，在原位置周围 8 格范围内统计存活
    // 潜影贝数量 i，繁殖概率为 1-f（f=(i-1)/5）。潜影越密集越不易繁殖（i>=6 时 f>=1 概率为 0
    // 必不繁殖），从而抑制潜影贝无限增殖。
    //
    // 注意：vanilla 在 teleportSomewhere 之前取 vec3=position() 与 aabb=getBoundingBox()，统计用
    // 旧碰撞箱外扩 8 格、新潜影贝生成在旧位置。故此处 originalPos 由调用方传入（瞬移前 position），
    // searchBox 也在瞬移前记录，避免瞬移后实体已离开原位导致统计区域偏移。
    if (m_world == nullptr) {
        return;
    }

    // a) 开壳且瞬移成功才进入繁殖判定（闭壳或瞬移失败则什么都不做）
    if (isShellClosed()) {
        return;
    }
    // 瞬移前记录原碰撞箱（vanilla :442 aabb = this.getBoundingBox() 在 teleport 之前）。
    const AxisAlignedBB originalBox = boundingBox();
    if (!_tryTeleportToNewPosition()) {
        return;
    }

    // b) 统计原碰撞箱外扩 8 格范围内的存活潜影贝数量（vanilla getEntities(SHULKER, aabb.inflate(8.0))）
    const AxisAlignedBB searchBox = originalBox.grow(8.0f);
    i32 shulkerCount = 0;
    const std::vector<Entity*> nearby = m_world->getEntitiesInAABB(searchBox, this);
    for (const Entity* entity : nearby) {
        if (entity != nullptr && entity->isAlive() && entity->entityType() == entity::VanillaEntityTypeKeys::SHULKER) {
            ++shulkerCount;
        }
    }

    // c) 繁殖概率判定：f=(i-1)/5，random.nextFloat() >= f 时繁殖。i<=1 时 f<=0 必繁殖，
    //    i>=6 时 f>=1 必不繁殖。注意本实体已瞬移离开原位，故统计中不含自身（getEntitiesInAABB
    //    已排除 this）。
    const f32 threshold = static_cast<f32>(shulkerCount - 1) / 5.0f;
    math::Random& rng = m_world->getRandom();
    if (rng.nextFloat() < threshold) {
        return; // 概率未通过，不繁殖
    }

    // d) 在原位置生成一只同色新潜影贝（vanilla create(BREEDING)+setVariant+snapTo+addFreshEntity）
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }
    auto baby = std::make_unique<ShulkerEntity>(EntityInstanceId(0), *registry);
    baby->setTypeId(entity::EntityTypeKeys::SHULKER); // 显式置 typeId（工厂绕过补救，对齐 shootBullet）
    baby->setColor(getColor());                       // 继承颜色（variant）
    baby->setPosition(originalPos);                   // 在原位置生成（vanilla snapTo(vec3)）
    m_world->spawnEntity(std::move(baby));
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
