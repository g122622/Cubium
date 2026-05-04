#include "PigEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"  // 包含 LookRandomlyGoal
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
    m_boostHelper.saddledRaw = false;
    m_boostHelper.boostingTick = 0;
}

f32 PigEntity::getSteeringSpeed() const {
    // MC 1.16.5 PigEntity.getMountedSpeed():
    // return (float)this.getAttributeValue(Attributes.MOVEMENT_SPEED) * 0.225F;
    f32 baseSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    return baseSpeed * MOUNTED_SPEED_MULT;
}

bool PigEntity::boost() {
    // MC 1.16.5: 使用BoostHelper进行加速
    math::Random rng = getRandom();
    return m_boostHelper.boost(rng);
}

bool PigEntity::canBeSteered() const {
    // MC 1.16.5: 猪需要玩家手持胡萝卜钓竿才能控制
    // 暂时返回是否有鞍
    // TODO: 当 Player 和 Items::CARROT_ON_A_STICK 实现后检查玩家手持物品
    return hasSaddle();
}

void PigEntity::travelTowards(const Vector3& travelVec) {
    // MC 1.16.5: PigEntity.travelTowards() -> super.travel(travelVec)
    // 调用父类的travel方法处理移动
    AnimalEntity::travel(travelVec);
}

void PigEntity::travel(const Vector3& travelVec) {
    // MC 1.16.5: PigEntity.travel()
    // this.setAIMoveSpeed((float)this.getAttributeValue(Attributes.MOVEMENT_SPEED));
    // this.ride(this, this.field_234214_bx_, travelVec);

    // 设置 AI 移动速度
    f32 moveSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    // setAIMoveSpeed(moveSpeed);  // TODO: 当 MobEntity 实现 setAIMoveSpeed 后添加

    // 调用 IRideable::ride() 处理骑乘移动
    ride(*this, m_boostHelper, travelVec);
}

void PigEntity::tick() {
    // 调用父类的tick
    AnimalEntity::tick();

    // 更新加速计时
    m_boostHelper.tick();
}

void PigEntity::registerGoals() {
    // 调用父类方法（AgeableEntity 会调用 AnimalEntity，现在 AnimalEntity 不注册任何目标）
    AgeableEntity::registerGoals();

    // MC 1.16.5 PigEntity.registerGoals()
    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表
    // 注意：PigEntity 是多重继承（AnimalEntity + IRideable），需要显式转换为对应的基类类型

    // 优先级 0: 游泳（最高优先级）- SwimGoal 需要 MobEntity*
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害或着火时）- PanicGoal 需要 CreatureEntity*
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 2: 繁殖（当处于爱心状态时）- BreedGoal 需要 AnimalEntity*
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（胡萝卜、马铃薯、甜菜根）- TemptGoal 需要 CreatureEntity*
    // MC 1.16.5: TemptGoal 使用 TEMPTATION_ITEMS，速度 1.2
    m_goalSelector.addGoal(3, std::make_unique<::mc::entity::ai::goal::TemptGoal>(
        this, 1.2,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr
                && (item == Items::CARROT
                    || item == Items::POTATO
                    || item == Items::BEETROOT);
        },
        false));  // scaredByMovement = false

    // 优先级 4: 跟随父母（幼体行为）- FollowParentGoal 需要 AnimalEntity*
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 5: 随机漫步 - RandomWalkingGoal 需要 CreatureEntity*
    // MC 1.16.5: 使用 WaterAvoidingRandomWalkingGoal，这里先用 RandomWalkingGoal
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 6: 看向玩家 - LookAtGoal 需要 MobEntity*
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 7: 随机看向 - LookRandomlyGoal 需要 MobEntity*
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));
}

void PigEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MC 1.16.5 PigEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, PIG_SPEED);
}

} // namespace mc
