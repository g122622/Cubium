#include "PigEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../damage/DamageSource.hpp"
#include <memory>

namespace mc {

std::unique_ptr<Entity> PigEntity::create(IWorld* /*world*/) {
    // 使用临时ID 0，实际ID由 EntityManager 分配
    return std::make_unique<PigEntity>(LegacyEntityType::Unknown, 0);
}

PigEntity::PigEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
}

std::optional<ResourceLocation> PigEntity::getAmbientSound() const {
    // MC 1.16.5: entity.pig.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> PigEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: entity.pig.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> PigEntity::getDeathSound() const {
    // MC 1.16.5: entity.pig.death
    return makeSoundEventId("death");
}

bool PigEntity::isBreedingItem(const ItemStack& itemStack) const {
    // MC 1.16.5: 猪用胡萝卜、马铃薯、甜菜根繁殖
    // PigEntity.TEMPTATION_ITEMS = Ingredient.fromItems(Items.CARROT, Items.POTATO, Items.BEETROOT)
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::CARROT
        || item == Items::POTATO
        || item == Items::BEETROOT;
}

bool PigEntity::canMateWith(const AnimalEntity& other) const {
    // MC 1.16.5: 检查是否是猪
    return dynamic_cast<const PigEntity*>(&other) != nullptr
        && AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> PigEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // MC 1.16.5: 创建小猪
    auto baby = std::make_unique<PigEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

// ========== IRideable 接口实现 ==========

void PigEntity::onPlayerStartRiding(Player* /*player*/) {
    // MC 1.16.5: 当玩家开始骑乘时
    // 可以添加骑乘音效或动画触发
}

void PigEntity::onPlayerStopRiding(Player* /*player*/) {
    // MC 1.16.5: 当玩家停止骑乘时
    // 重置加速状态
    m_boostTime = 0;
    m_boostSpeed = 0.0f;
}

f32 PigEntity::getSteeringSpeed() const {
    // MC 1.16.5 PigEntity.getMountedSpeed():
    // return (float)this.getAttributeValue(Attributes.MOVEMENT_SPEED) * 0.225F;
    f32 baseSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    return baseSpeed * MOUNTED_SPEED_MULT;
}

bool PigEntity::boost() {
    // MC 1.16.5: 只有装备了鞍才能加速
    if (!m_hasSaddle) {
        return false;
    }

    // 如果已经在加速中，不重复触发
    if (m_boostTime > 0) {
        return false;
    }

    // 设置加速时间和速度
    m_boostTime = MAX_BOOST_TIME;
    m_boostSpeed = BOOST_SPEED;

    return true;
}

void PigEntity::tick() {
    // 调用父类的tick
    AnimalEntity::tick();

    // 更新加速计时
    if (m_boostTime > 0) {
        m_boostTime--;

        // 加速结束，重置速度
        if (m_boostTime <= 0) {
            m_boostSpeed = 0.0f;
        }
    }
}

void PigEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // MC 1.16.5 PigEntity.registerGoals()
    // 优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: PanicGoal (父类已注册)
    // 2: BreedGoal (父类已注册)
    // 4: TemptGoal (胡萝卜、马铃薯、甜菜根) - 注意：优先级4，不是3
    // 5: FollowParentGoal (父类已注册)
    // 6: WaterAvoidingRandomWalkingGoal (父类使用 RandomWalkingGoal)
    // 7: LookAtGoal (玩家)
    // 8: LookRandomlyGoal

    // 优先级 4: 食物诱惑
    // MC 1.16.5: TemptGoal 使用 TEMPTATION_ITEMS (胡萝卜、马铃薯、甜菜根)，速度 1.2
    m_goalSelector.addGoal(4, std::make_unique<::mc::entity::ai::goal::TemptGoal>(
        this, 1.2,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr
                && (item == Items::CARROT
                    || item == Items::POTATO
                    || item == Items::BEETROOT);
        },
        false));  // scaredByMovement = false

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f,
        [](const LivingEntity* /*entity*/) -> bool {
            // 只看向玩家
            // TODO: 检查是否是玩家
            return true;
        }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));
}

void PigEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MC 1.16.5 PigEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, PIG_SPEED);
}

} // namespace mc
