#include "SnowGolemEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

SnowGolemEntity::SnowGolemEntity(LegacyEntityType type, EntityId id)
    : GolemEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SnowGolemEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SnowGolemEntity>(LegacyEntityType::Unknown, 0);
}

std::vector<ItemStack> SnowGolemEntity::shear(Player* /*player*/)
{
    std::vector<ItemStack> drops;
    if (m_hasPumpkin) {
        m_hasPumpkin = false;
        // TODO: 返回南瓜物品
        // drops.emplace_back(Items::CARVED_PUMPKIN, 1);
    }
    return drops;
}

bool SnowGolemEntity::willMelt() const
{
    // TODO: 检查温度
    // 在热带、沙漠、下界等高温生物群系会融化
    // 在水中也会融化
    return false;
}

void SnowGolemEntity::tick()
{
    GolemEntity::tick();

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 更新雪层放置冷却
    if (m_snowPlaceCooldown > 0) {
        m_snowPlaceCooldown--;
    }

    // 检查融化
    if (willMelt()) {
        m_meltTimer++;
        if (m_meltTimer >= MELT_DAMAGE_INTERVAL) {
            m_meltTimer = 0;
            // TODO: 造成融化伤害
            // damage(DamageSource::ON_FIRE, MELT_DAMAGE);
        }
    } else {
        m_meltTimer = 0;
    }

    // 在寒冷生物群系行走时放置雪层
    // TODO: 检查是否在寒冷生物群系
    // if (isInColdBiome() && m_snowPlaceCooldown <= 0) {
    //     placeSnowLayer();
    //     m_snowPlaceCooldown = SNOW_PLACE_INTERVAL;
    // }
}

void SnowGolemEntity::registerGoals()
{
    // 调用父类方法
    GolemEntity::registerGoals();

    // TODO: 雪傀儡 AI 目标
    // - SnowGolemAttackGoal: 投掷雪球攻击
    // - SnowGolemFollowOwnerGoal: 跟随创造者（如果有）
}

void SnowGolemEntity::registerAttributes()
{
    // 调用父类方法
    GolemEntity::registerAttributes();

    // 雪傀儡的属性
    // 参考 MC 1.16.5 雪傀儡属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 4.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
}

} // namespace mc
