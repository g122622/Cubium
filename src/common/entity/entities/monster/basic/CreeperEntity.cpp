#include "CreeperEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/explosion/ExplosionMode.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <memory>

namespace mc {

CreeperEntity::CreeperEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // MC 1.16.5: 苦力怕不在阳光下燃烧
    setBurnsInDaylight(false);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CreeperEntity::create(IWorld* /*world*/) {
    return std::make_unique<CreeperEntity>(LegacyEntityType::Unknown, 0);
}

std::optional<ResourceLocation> CreeperEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: entity.creeper.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> CreeperEntity::getDeathSound() const {
    // MC 1.16.5: entity.creeper.death
    return makeSoundEventId("death");
}

i32 CreeperEntity::getCreeperState() const {
    // MC 1.16.5: -1 = idle, 1 = fusing
    if (m_timeSinceIgnited > 0) {
        return 1;
    }
    return -1;
}

void CreeperEntity::setCreeperState(i32 state) {
    // MC 1.16.5: 设置状态
    if (state > 0) {
        ignite();
    }
}

void CreeperEntity::ignite() {
    // MC 1.16.5: 点燃苦力怕
    m_ignited = true;
}

bool CreeperEntity::ableToCauseSkullDrop() const {
    // MC 1.16.5: 只有高压苦力怕且还没掉过头颅才能导致头颅掉落
    return m_powered && m_droppedSkulls < 1;
}

void CreeperEntity::explode() {
    // MC 1.16.5 CreeperEntity.explode()
    // 只在服务端爆炸
    if (isDead()) return;

    IWorld* worldPtr = world();
    if (!worldPtr) {
        remove();
        return;
    }

    // 计算爆炸威力（高压翻倍）
    f32 radius = static_cast<f32>(m_explosionRadius);
    if (m_powered) {
        radius *= 2.0f;
    }

    // 苦力怕爆炸模式：DESTROY（破坏方块并掉落物品）
    // TODO: 检查游戏规则 mobGriefing，如果为 false 则使用 NONE
    world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy;

    // 创建爆炸
    worldPtr->createExplosion(
        position(),
        radius,
        mode,
        false,  // 不生成火焰
        this    // 爆炸源实体
    );

    // 移除实体
    remove();

    // 生成滞留药水云（如果有药水效果）
    spawnLingeringCloud();
}

void CreeperEntity::spawnLingeringCloud() {
    // MC 1.16.5 CreeperEntity.spawnLingeringCloud()
    // TODO: 当苦力怕有药水效果时，死亡后生成滞留药水云
    // 需要实现 AreaEffectCloudEntity
}

void CreeperEntity::tick() {
    // MC 1.16.5 CreeperEntity.tick()
    if (isAlive()) {
        m_lastActiveTime = m_timeSinceIgnited;

        // 如果被点燃，设置状态为膨胀
        if (hasIgnited()) {
            setCreeperState(1);
        }

        i32 state = getCreeperState();
        if (state > 0 && m_timeSinceIgnited == 0) {
            // MC 1.16.5: 开始膨胀时播放音效
            playSound(*makeSoundEventId("primed"), 1.0f, 0.5f);
        }

        m_timeSinceIgnited += state;

        if (m_timeSinceIgnited < 0) {
            m_timeSinceIgnited = 0;
        }

        // 达到点燃时间后爆炸
        if (m_timeSinceIgnited >= m_fuseTime) {
            m_timeSinceIgnited = m_fuseTime;
            explode();
            return;
        }
    }

    MonsterEntity::tick();
}

void CreeperEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // MC 1.16.5 CreeperEntity.registerGoals()
    // 优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: PanicGoal (父类已注册，但苦力怕不使用)
    // 2: CreeperSwellGoal - 膨胀爆炸
    // 3: AvoidEntityGoal<OcelotEntity> - 避开豹猫
    // 3: AvoidEntityGoal<CatEntity> - 避开猫
    // 4: MeleeAttackGoal - 近战攻击（实际不造成伤害，用于接近玩家）
    // 5: WaterAvoidingRandomWalkingGoal - 避水随机行走
    // 6: LookAtGoal<PlayerEntity> - 看向玩家
    // 6: LookRandomlyGoal - 随机看向
    //
    // 目标选择器：
    // 1: NearestAttackableTargetGoal<PlayerEntity> - 攻击玩家
    // 2: HurtByTargetGoal (父类已注册)

    // 优先级 2: 膨胀爆炸
    // TODO: 实现 CreeperSwellGoal
    // m_goalSelector.addGoal(2, new entity::ai::goal::CreeperSwellGoal(this));

    // 优先级 3: 避开猫/豹猫
    // TODO: 实现 AvoidEntityGoal
    // m_goalSelector.addGoal(3, new entity::ai::goal::AvoidEntityGoal<CatEntity>(this, 6.0f, 1.0, 1.2));
    // m_goalSelector.addGoal(3, new entity::ai::goal::AvoidEntityGoal<OcelotEntity>(this, 6.0f, 1.0, 1.2));

    // 优先级 4: 近战攻击（用于接近玩家）
    m_goalSelector.addGoal(4, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));

    // 优先级 5: 避水随机行走
    // TODO: 实现 WaterAvoidingRandomWalkingGoal
    // m_goalSelector.addGoal(5, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 0.8));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f,
        [](const LivingEntity* /*entity*/) -> bool {
            // 只看向玩家
            // TODO: 检查是否是玩家
            return true;
        }));

    // 优先级 6: 随机看向
    m_goalSelector.addGoal(6, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择器：攻击玩家
    // TODO: 实现 NearestAttackableTargetGoal
    // m_targetSelector.addGoal(1, new entity::ai::goal::NearestAttackableTargetGoal<PlayerEntity>(this, true));
}

void CreeperEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // MC 1.16.5 CreeperEntity 属性
    // 继承自 MonsterEntity: MAX_HEALTH = 20.0
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
