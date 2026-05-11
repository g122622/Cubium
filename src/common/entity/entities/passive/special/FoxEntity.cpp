#include "FoxEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

FoxEntity::FoxEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> FoxEntity::create(IWorld* /*world*/) {
    return std::make_unique<FoxEntity>(LegacyEntityType::Unknown, 0);
}

bool FoxEntity::trusts(u64 playerId) const {
    for (u64 trustedId : m_trustedPlayers) {
        if (trustedId == playerId) {
            return true;
        }
    }
    return false;
}

void FoxEntity::addTrustedPlayer(u64 playerId) {
    if (trusts(playerId)) {
        return;
    }

    if (m_trustedPlayers.size() < MAX_TRUSTED_PLAYERS) {
        m_trustedPlayers.push_back(playerId);
    } else {
        // 替换最早的信任
        m_trustedPlayers.erase(m_trustedPlayers.begin());
        m_trustedPlayers.push_back(playerId);
    }
}

void FoxEntity::removeTrustedPlayer(u64 playerId) {
    auto it = std::find(m_trustedPlayers.begin(), m_trustedPlayers.end(), playerId);
    if (it != m_trustedPlayers.end()) {
        m_trustedPlayers.erase(it);
    }
}

std::optional<u64> FoxEntity::getFirstTrustedPlayer() const {
    if (m_trustedPlayers.empty()) {
        return std::nullopt;
    }
    return m_trustedPlayers[0];
}

void FoxEntity::setSleeping(bool sleeping) {
    m_sleeping = sleeping;
    if (sleeping) {
        m_sleepTimer = 100 + (rand() % 100); // 5-10秒
    }
}

bool FoxEntity::isHoldingItem() const {
    return m_heldItem != nullptr && !m_heldItem->isEmpty();
}

void FoxEntity::setHeldItem(std::unique_ptr<ItemStack> item) {
    m_heldItem = std::move(item);
}

void FoxEntity::dropHeldItem() {
    // TODO: 在世界生成掉落物
    m_heldItem.reset();
}

bool FoxEntity::isBreedingItem(const ItemStack& itemStack) const {
    // MC 1.16.5: FoxEntity.isBreedingItem()
    // 只有甜浆果可以用来繁殖狐狸
    // 注意：发光浆果是 MC 1.17 添加的，MC 1.16.5 只有甜浆果
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::SWEET_BERRIES;
}

std::unique_ptr<AnimalEntity> FoxEntity::spawnBaby(AnimalEntity& partner) {
    // MC 1.16.5: FoxEntity.func_241840_a() (createChild)
    auto baby = std::make_unique<FoxEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // MC 1.16.5: 遗传皮肤类型
    // foxentity.setVariantType(this.rand.nextBoolean() ? this.getVariantType() : ((FoxEntity)p_241840_2_).getVariantType());
    // 50% 概率从任一父母继承皮肤类型
    FoxEntity* partnerFox = dynamic_cast<FoxEntity*>(&partner);
    math::Random rng = getRandom();
    if (rng.nextBoolean()) {
        baby->setFoxType(m_foxType);
    } else if (partnerFox != nullptr) {
        baby->setFoxType(partnerFox->getFoxType());
    } else {
        // 如果配偶不是狐狸（不应该发生），使用自己的类型
        baby->setFoxType(m_foxType);
    }

    // MC 1.16.5: 幼狐继承父母的信任玩家
    // 参考 onChildSpawnFromEgg(): ((FoxEntity)child).addTrustedUUID(playerIn.getUniqueID());
    // 在 BreedGoal 中，繁殖时幼狐应该信任喂食者
    // 这里我们从父母继承信任玩家
    for (u64 playerId : m_trustedPlayers) {
        baby->addTrustedPlayer(playerId);
    }
    if (partnerFox != nullptr) {
        for (u64 playerId : partnerFox->getTrustedPlayers()) {
            baby->addTrustedPlayer(playerId);
        }
    }

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

void FoxEntity::tick() {
    AnimalEntity::tick();

    // 睡眠计时器
    if (m_sleeping) {
        m_sleepTimer--;
        if (m_sleepTimer <= 0) {
            setSleeping(false);
        }
    }
}

void FoxEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 狐狸特有目标
    // 优先级 2: 逃离玩家（未信任的玩家）
    // TODO: 需要 AvoidEntityGoal 支持
    // m_goalSelector.addGoal(2, new entity::ai::goal::AvoidEntityGoal(this, Player.class, 16.0f, 1.6, 1.4));

    // 优先级 3: 食物诱惑（甜浆果）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isBerryPredicate));

    // TODO: 狐狸特有目标
    // - FoxPounceGoal: 扑击攻击
    // - FoxEatBerriesGoal: 吃浆果
    // - FoxHuntGoal: 狩猎小动物
    // - FoxSleepGoal: 睡觉
}

void FoxEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 狐狸的属性
    // 参考 MC 1.16.5 狐狸属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

std::optional<ResourceLocation> FoxEntity::getAmbientSound() const {
    // MC 1.16.5: 白狐使用 screech 音效
    if (m_foxType == FoxType::Snow) {
        return SoundEvents::ENTITY_FOX_SCREECH;
    }
    return SoundEvents::ENTITY_FOX_AMBIENT;
}

std::optional<ResourceLocation> FoxEntity::getHurtSound(DamageSource& /*source*/) const {
    return SoundEvents::ENTITY_FOX_HURT;
}

std::optional<ResourceLocation> FoxEntity::getDeathSound() const {
    return SoundEvents::ENTITY_FOX_DEATH;
}

void FoxEntity::playSleepSound() {
    playSound(SoundEvents::ENTITY_FOX_SLEEP, 1.0f, 1.0f);
}

void FoxEntity::playSniffSound() {
    playSound(SoundEvents::ENTITY_FOX_SNIFF, 1.0f, 1.0f);
}

void FoxEntity::playBiteSound() {
    playSound(SoundEvents::ENTITY_FOX_BITE, 1.0f, 1.0f);
}

void FoxEntity::playEatSound() {
    playSound(SoundEvents::ENTITY_FOX_EAT, 1.0f, 1.0f);
}

void FoxEntity::playSpitSound() {
    playSound(SoundEvents::ENTITY_FOX_SPIT, 1.0f, 1.0f);
}

} // namespace mc
