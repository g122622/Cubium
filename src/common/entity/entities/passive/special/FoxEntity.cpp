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

#include "FoxEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../utils/ItemDropHelper.hpp"

namespace mc {

FoxEntity::FoxEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> FoxEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FoxEntity>(LegacyEntityType::Unknown, 0);
}

bool FoxEntity::trusts(u64 playerId) const
{
    for (u64 trustedId : m_trustedPlayers) {
        if (trustedId == playerId) {
            return true;
        }
    }
    return false;
}

void FoxEntity::addTrustedPlayer(u64 playerId)
{
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

void FoxEntity::removeTrustedPlayer(u64 playerId)
{
    auto it = std::find(m_trustedPlayers.begin(), m_trustedPlayers.end(), playerId);
    if (it != m_trustedPlayers.end()) {
        m_trustedPlayers.erase(it);
    }
}

std::optional<u64> FoxEntity::getFirstTrustedPlayer() const
{
    if (m_trustedPlayers.empty()) {
        return std::nullopt;
    }
    return m_trustedPlayers[0];
}

void FoxEntity::setSleeping(bool sleeping)
{
    m_sleeping = sleeping;
    if (sleeping) {
        m_sleepTimer = 100 + (rand() % 100); // 5-10秒
    }
}

bool FoxEntity::isHoldingItem() const
{
    return m_heldItem != nullptr && !m_heldItem->isEmpty();
}

void FoxEntity::setHeldItem(std::unique_ptr<ItemStack> item)
{
    m_heldItem = std::move(item);
}

void FoxEntity::dropHeldItem()
{
    // [已完成] 在世界生成掉落物 - 2026/05/16
    // 参考 MC 1.16.5 FoxEntity.entityDropItem()
    if (m_heldItem == nullptr || m_heldItem->isEmpty()) {
        return;
    }

    IWorld* world = this->world();
    if (world == nullptr) {
        return;
    }

    // 使用 ItemDropHelper 在实体位置生成物品实体
    math::Random rng = getRandom();
    ItemDropHelper::spawnItemAtEntity(this, *m_heldItem, 0.0f, rng, 10);

    // 清空物品引用
    m_heldItem.reset();
}

bool FoxEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: FoxEntity.isBreedingItem()
    // 只有甜浆果可以用来繁殖狐狸
    // 注意：发光浆果是 MC 1.17 添加的，MC 1.16.5 只有甜浆果
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::SWEET_BERRIES;
}

std::unique_ptr<AnimalEntity> FoxEntity::spawnBaby(AnimalEntity& partner)
{
    // MC 1.16.5: FoxEntity.func_241840_a() (createChild)
    auto baby = std::make_unique<FoxEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // MC 1.16.5: 遗传皮肤类型
    // foxentity.setVariantType(this.rand.nextBoolean() ? this.getVariantType() :
    // ((FoxEntity)p_241840_2_).getVariantType()); 50% 概率从任一父母继承皮肤类型
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

void FoxEntity::tick()
{
    AnimalEntity::tick();

    // 睡眠计时器
    if (m_sleeping) {
        m_sleepTimer--;
        if (m_sleepTimer <= 0) {
            setSleeping(false);
        }
    }
}

void FoxEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // MC 1.16.5: 狐狸特有目标注册顺序
    // 注意：优先级数值越小，优先级越高

    // 优先级 4: 躲避玩家（未信任的玩家）
    // MC 1.16.5: new AvoidEntityGoal<>(this, PlayerEntity.class, 16.0F, 1.6D, 1.4D,
    //     (p_213497_1_) -> SHOULD_AVOID.test(p_213497_1_) && !this.isTrustedUUID(p_213497_1_.getUniqueID())
    //         && !this.isFoxAggroed());
    // SHOULD_AVOID = !isDiscrete() && CAN_AI_TARGET.test(this)
    // isDiscrete() 检查玩家是否隐形/旁观者等，CAN_AI_TARGET 检查是否可作为AI目标
    // [已完成] 实现 AvoidEntityGoal 躲避未信任的玩家 - 2026/05/16
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::AvoidEntityGoal>(
        this,
        16.0f, // 检测距离：16格
        1.6,   // 近距离逃跑速度（更快）
        1.4,   // 远距离逃跑速度
        [this](const LivingEntity* entity) -> bool {
            if (entity == nullptr) return false;
            // 只躲避玩家
            const Player* player = dynamic_cast<const Player*>(entity);
            if (player == nullptr) return false;
            // MC 1.16.5: SHOULD_AVOID 检查
            // isDiscrete() = isSpectator() || isInvisible() || ...
            // CAN_AI_TARGET = !isCreative() && !isSpectator() && isAlive()
            if (player->isSpectator() || player->isCreative()) return false;
            // 不躲避信任的玩家
            if (trusts(player->id())) return false;
            // 不躲避当狐狸处于攻击状态时（即 isFoxAggroed 为 false）
            // 当前简化实现：没有 isFoxAggroed 状态，始终躲避
            return true;
        }));

    // 优先级 3: 食物诱惑（甜浆果）
    // MC 1.16.5: new TemptGoal(this, 1.0D, Ingredient.fromItems(Items.SWEET_BERRIES), false)
    // [已完成] 实现 TemptGoal 甜浆果诱惑 - 2026/05/16
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::TemptGoal>(
        this,
        1.0,  // 跟随速度
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item == Items::SWEET_BERRIES;
        },
        false // 不被玩家移动吓跑
    ));

    // TODO: 狐狸特有目标（需要实现更多 Goal 类）
    // - FoxPounceGoal: 扑击攻击
    // - FoxEatBerriesGoal: 吃浆果
    // - FoxHuntGoal: 狩猎小动物
    // - FoxSleepGoal: 睡觉
}

void FoxEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 狐狸的属性
    // 参考 MC 1.16.5 狐狸属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

std::optional<ResourceLocation> FoxEntity::getAmbientSound() const
{
    // MC 1.16.5: 白狐使用 screech 音效
    if (m_foxType == FoxType::Snow) {
        return SoundEvents::ENTITY_FOX_SCREECH;
    }
    return SoundEvents::ENTITY_FOX_AMBIENT;
}

std::optional<ResourceLocation> FoxEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_FOX_HURT;
}

std::optional<ResourceLocation> FoxEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_FOX_DEATH;
}

void FoxEntity::playSleepSound()
{
    playSound(SoundEvents::ENTITY_FOX_SLEEP, 1.0f, 1.0f);
}

void FoxEntity::playSniffSound()
{
    playSound(SoundEvents::ENTITY_FOX_SNIFF, 1.0f, 1.0f);
}

void FoxEntity::playBiteSound()
{
    playSound(SoundEvents::ENTITY_FOX_BITE, 1.0f, 1.0f);
}

void FoxEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_FOX_EAT, 1.0f, 1.0f);
}

void FoxEntity::playSpitSound()
{
    playSound(SoundEvents::ENTITY_FOX_SPIT, 1.0f, 1.0f);
}

} // namespace mc
