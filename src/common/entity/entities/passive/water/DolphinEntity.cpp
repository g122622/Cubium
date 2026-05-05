#include "DolphinEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../sound/SoundEvents.hpp"

namespace mc {

DolphinEntity::DolphinEntity(LegacyEntityType type, EntityId id)
    : WaterMobEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> DolphinEntity::create(IWorld* /*world*/) {
    return std::make_unique<DolphinEntity>(LegacyEntityType::Unknown, 0);
}

bool DolphinEntity::canJumpOutOfWater() const {
    // TODO: 检查是否接近水面
    return isInWater();
}

void DolphinEntity::setTreasurePos(const BlockPos& pos) {
    m_treasurePos = pos;
    m_hasTreasure = true;
}

void DolphinEntity::clearTreasureTarget() {
    m_hasTreasure = false;
    m_guidingPlayer = false;
    m_guidedPlayerId = 0;
    m_guideTimer = 0;
}

void DolphinEntity::setGuidingPlayer(bool guiding, u64 playerId) {
    m_guidingPlayer = guiding;
    m_guidedPlayerId = playerId;
    if (guiding) {
        m_guideTimer = GUIDE_DURATION;
    }
}

bool DolphinEntity::isFoodItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是鱼
    // return itemStack.getItem() == Items::COD ||
    //        itemStack.getItem() == Items::SALMON ||
    //        itemStack.getItem() == Items::PUFFERFISH ||
    //        itemStack.getItem() == Items::TROPICAL_FISH;
    (void)itemStack;
    return false;
}

void DolphinEntity::onLeaveWater() {
    WaterMobEntity::onLeaveWater();
    playSound(SoundEvents::ENTITY_DOLPHIN_JUMP, 1.0f, 1.0f);
}

std::optional<ResourceLocation> DolphinEntity::getAmbientSound() const {
    if (isInWater()) {
        return SoundEvents::ENTITY_DOLPHIN_AMBIENT_WATER;
    }
    return SoundEvents::ENTITY_DOLPHIN_AMBIENT;
}

void DolphinEntity::playAttackSound(LivingEntity& /*target*/) {
    playSound(SoundEvents::ENTITY_DOLPHIN_ATTACK, 1.0f, 1.0f);
}

void DolphinEntity::tick() {
    WaterMobEntity::tick();

    // 更新引导计时器
    if (m_guidingPlayer && m_guideTimer > 0) {
        m_guideTimer--;
        if (m_guideTimer <= 0) {
            clearTreasureTarget();
        }
    }

    // 更新游泳行为
    if (isInWater()) {
        m_swimTimer++;

        // 随机跳跃
        if (m_swimTimer >= 200 && canJumpOutOfWater()) {
            math::Random rng = getRandom();
            if (rng.nextInt(1, 100) == 1) {
                m_jumping = true;
                m_swimTimer = 0;
            }
        }
    } else {
        m_jumping = false;
    }
}

void DolphinEntity::registerGoals() {
    // TODO: 海豚 AI 目标
    // - DolphinSwimGoal: 随机游泳
    // - DolphinJumpGoal: 跳出水
    // - DolphinPlayWithItemsGoal: 玩物品
    // - DolphinLocateTreasureGoal: 寻找宝藏
    // - DolphinGuidePlayerGoal: 引导玩家
}

void DolphinEntity::registerAttributes() {
    // 调用父类方法
    WaterMobEntity::registerAttributes();

    // 海豚的属性
    // 参考 MC 1.16.5 海豚属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, SWIM_SPEED);
}

} // namespace mc
