#include "SheepEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <memory>
#include <vector>

namespace mc {

std::unique_ptr<Entity> SheepEntity::create(IWorld* /*world*/) {
    // 使用临时ID 0，实际ID由 EntityManager 分配
    return std::make_unique<SheepEntity>(LegacyEntityType::Unknown, 0);
}

SheepEntity::SheepEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册属性
    registerAttributes();
    // 注册 AI 目标
    registerGoals();
}

std::optional<ResourceLocation> SheepEntity::getAmbientSound() const {
    // MC 1.16.5: entity.sheep.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> SheepEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: entity.sheep.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> SheepEntity::getDeathSound() const {
    // MC 1.16.5: entity.sheep.death
    return makeSoundEventId("death");
}

bool SheepEntity::isShearable() const {
    // MC 1.16.5: 只有活着、有羊毛、非幼羊的羊才能被剪
    return isAlive() && !m_sheared && !isChild();
}

std::vector<ItemStack> SheepEntity::shear(Player* /*player*/) {
    std::vector<ItemStack> drops;

    if (!isShearable()) {
        return drops;
    }

    // MC 1.16.5: 设置剪毛状态
    m_sheared = true;

    // MC 1.16.5: 播放剪毛音效
    playSound(*makeSoundEventId("shear"), 1.0f, 1.0f);

    // MC 1.16.5: 掉落 1-3 个对应颜色的羊毛
    math::Random rng(ticksExisted());
    i32 woolCount = 1 + rng.nextInt(3);

    // TODO: 使用颜色映射获取正确的羊毛物品
    // 当前暂时使用 Items::WHITE_WOOL
    // 实际应该根据 m_fleeceColor 获取对应颜色的羊毛
    for (i32 i = 0; i < woolCount; ++i) {
        // drops.emplace_back(Items::getWoolByColor(m_fleeceColor), 1);
        // 暂时跳过，等待 Items 定义羊毛颜色映射
    }

    return drops;
}

bool SheepEntity::isBreedingItem(const ItemStack& itemStack) const {
    // MC 1.16.5: 羊用小麦繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::WHEAT;
}

bool SheepEntity::canMateWith(const AnimalEntity& other) const {
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> SheepEntity::spawnBaby(AnimalEntity& partner) {
    // MC 1.16.5: 创建小羊
    auto baby = std::make_unique<SheepEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // MC 1.16.5: 颜色继承逻辑
    SheepEntity* partnerSheep = dynamic_cast<SheepEntity*>(&partner);
    if (partnerSheep != nullptr) {
        // TODO: 实现颜色混合逻辑
        // DyeColor mixedColor = getDyeColorMixFromParents(this, partnerSheep);
        // baby->setFleeceColor(mixedColor);
        baby->setFleeceColor(getFleeceColor());  // 暂时继承父体颜色
    } else {
        baby->setFleeceColor(getFleeceColor());
    }

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

void SheepEntity::eatGrassBonus() {
    // MC 1.16.5: 吃草奖励
    // 如果被剪过，重新长出羊毛
    if (m_sheared) {
        m_sheared = false;
    }

    // 如果是幼羊，加速成长
    if (isChild()) {
        addGrowingAge(60);  // 加速成长 60 ticks (3秒)
    }
}

DyeColor SheepEntity::getRandomSheepColor(math::Random& random) {
    // MC 1.16.5 SheepEntity.getRandomSheepColor()
    i32 i = random.nextInt(100);

    if (i < 5) {
        return DyeColor::Black;       // 5% 黑色
    } else if (i < 10) {
        return DyeColor::Gray;        // 5% 灰色
    } else if (i < 15) {
        return DyeColor::LightGray;   // 5% 浅灰色
    } else if (i < 18) {
        return DyeColor::Brown;       // 3% 棕色
    } else {
        // 82% 白色，其中 0.2% 粉色
        if (random.nextInt(500) == 0) {
            return DyeColor::Pink;    // ~0.2% 粉色
        }
        return DyeColor::White;       // ~81.8% 白色
    }
}

void SheepEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // MC 1.16.5 SheepEntity.registerGoals()
    // 优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: PanicGoal (父类已注册)
    // 2: BreedGoal (父类已注册)
    // 3: TemptGoal - 小麦诱惑
    // 4: FollowParentGoal (父类已注册)
    // 5: EatGrassGoal (需要单独实现)
    // 6: WaterAvoidingRandomWalkingGoal (父类使用 RandomWalkingGoal)
    // 7: LookAtGoal (玩家)
    // 8: LookRandomlyGoal

    // 优先级 3: 小麦诱惑 - MC 1.16.5 速度为 1.1
    m_goalSelector.addGoal(3, std::make_unique<::mc::entity::ai::goal::TemptGoal>(
        this, 1.1,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && item == Items::WHEAT;
        },
        false));  // scaredByMovement = false

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 6.0f, 0.02f,
        [](const LivingEntity* /*entity*/) -> bool {
            // 只看向玩家
            // TODO: 检查是否是玩家
            return true;
        }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // TODO: 优先级 5: EatGrassGoal - 需要实现
}

void SheepEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MC 1.16.5 SheepEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
}

void SheepEntity::tick() {
    // MC 1.16.5: 吃草动画计时器递减
    // 只在客户端递减
    if (m_eatAnimationTimer > 0) {
        --m_eatAnimationTimer;
    }

    AnimalEntity::tick();
}

} // namespace mc
