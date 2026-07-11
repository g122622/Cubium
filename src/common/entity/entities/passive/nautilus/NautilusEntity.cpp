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

#include "NautilusEntity.hpp"

#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"

#include <memory>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

NautilusEntity::NautilusEntity(EntityId id)
    : AbstractNautilusEntity(id)
{
    // 显式调用 registerData()，注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类
    // （Entity::Entity 内部调用 registerData() 时调用的是 Entity::registerData
    //   而非 NautilusEntity::registerData），必须在派生类构造函数中显式调用，
    // 参考 WolfEntity 模式。
    registerData();

    // 注册属性（父类构造函数未调用 registerAttributes）
    registerAttributes();

    // 父类构造函数已经调用 registerGoals()，但此时虚函数尚未派发到 NautilusEntity
    // 这里再次调用以注册 NautilusEntity 特有的 BreedGoal
    registerGoals();
}

std::unique_ptr<Entity> NautilusEntity::create(IWorld* /*world*/)
{
    // 使用临时 ID 0，实际 ID 由 EntityManager 分配
    return std::make_unique<NautilusEntity>(0);
}

// ============================================================================
// 繁殖系统
// ============================================================================

std::unique_ptr<AnimalEntity> NautilusEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 对应 MC 1.21.11 Nautilus.getBreedOffspring()
    auto baby = std::make_unique<NautilusEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置）
    baby->setPosition(x(), y(), z());

    // 对应 MC 1.21.11：若父体已驯服，幼体继承主人和驯服状态
    if (isTamed()) {
        baby->setTamed(true);
        baby->setOwnerId(getOwnerId().value_or(0));
    }

    return baby;
}

// ============================================================================
// 物品判断
// ============================================================================

bool NautilusEntity::isTamingItem(const ItemStack& itemStack) const
{
    // 对应 MC 1.21.11 ItemTags.NAUTILUS_TAMING_ITEMS
    // 简化实现：直接检查物品 ID
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    // 驯服物品：生鱼 + 桶装鱼
    return item == Items::COD || item == Items::SALMON || item == Items::PUFFERFISH || item == Items::TROPICAL_FISH ||
        item == Items::COD_BUCKET || item == Items::SALMON_BUCKET || item == Items::PUFFERFISH_BUCKET ||
        item == Items::TROPICAL_FISH_BUCKET;
}

bool NautilusEntity::isNautilusFood(const ItemStack& itemStack) const
{
    // 对应 MC 1.21.11 ItemTags.NAUTILUS_FOOD
    // 简化实现：直接检查物品 ID
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    // 食物：生鱼（非桶装）
    return item == Items::COD || item == Items::SALMON || item == Items::PUFFERFISH || item == Items::TROPICAL_FISH;
}

bool NautilusEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 对应 MC 1.21.11 AbstractNautilus.isFood()：
    // 未驯服 → NAUTILUS_TAMING_ITEMS；已驯服 → NAUTILUS_FOOD
    if (!isTamed()) {
        return isTamingItem(itemStack);
    }
    return isNautilusFood(itemStack);
}

// ============================================================================
// 音效
// ============================================================================

std::optional<ResourceLocation> NautilusEntity::getAmbientSound() const
{
    // 4 路分支：幼体/成体 × 水下/陆地
    if (isChild()) {
        return isInWater() ? SoundEvents::ENTITY_BABY_NAUTILUS_AMBIENT
                           : SoundEvents::ENTITY_BABY_NAUTILUS_AMBIENT_ON_LAND;
    }
    return isInWater() ? SoundEvents::ENTITY_NAUTILUS_AMBIENT : SoundEvents::ENTITY_NAUTILUS_AMBIENT_ON_LAND;
}

std::optional<ResourceLocation> NautilusEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 4 路分支：幼体/成体 × 水下/陆地
    if (isChild()) {
        return isInWater() ? SoundEvents::ENTITY_BABY_NAUTILUS_HURT : SoundEvents::ENTITY_BABY_NAUTILUS_HURT_ON_LAND;
    }
    return isInWater() ? SoundEvents::ENTITY_NAUTILUS_HURT : SoundEvents::ENTITY_NAUTILUS_HURT_ON_LAND;
}

std::optional<ResourceLocation> NautilusEntity::getDeathSound() const
{
    // 4 路分支：幼体/成体 × 水下/陆地
    if (isChild()) {
        return isInWater() ? SoundEvents::ENTITY_BABY_NAUTILUS_DEATH : SoundEvents::ENTITY_BABY_NAUTILUS_DEATH_ON_LAND;
    }
    return isInWater() ? SoundEvents::ENTITY_NAUTILUS_DEATH : SoundEvents::ENTITY_NAUTILUS_DEATH_ON_LAND;
}

std::optional<ResourceLocation> NautilusEntity::getDashSound() const
{
    // 2 路分支：水下/陆地（无幼体变体）
    return isInWater() ? SoundEvents::ENTITY_NAUTILUS_DASH : SoundEvents::ENTITY_NAUTILUS_DASH_ON_LAND;
}

std::optional<ResourceLocation> NautilusEntity::getDashReadySound() const
{
    // 2 路分支：水下/陆地（无幼体变体）
    return isInWater() ? SoundEvents::ENTITY_NAUTILUS_DASH_READY : SoundEvents::ENTITY_NAUTILUS_DASH_READY_ON_LAND;
}

std::optional<ResourceLocation> NautilusEntity::getEatSound() const
{
    // 2 路分支：幼体/成体
    return isChild() ? SoundEvents::ENTITY_BABY_NAUTILUS_EAT : SoundEvents::ENTITY_NAUTILUS_EAT;
}

// ============================================================================
// 空气供应
// ============================================================================

void NautilusEntity::updateAirSupply()
{
    // 对应 MC 1.21.11 Nautilus.handleAirSupply()
    if (!isAlive()) {
        return;
    }

    if (!isInWater()) {
        // 不在水中：空气 -1，到 -20 时清零并承受 2 点干涸伤害
        i32 newAir = air() - 1;
        setAir(newAir);
        if (newAir <= -20) {
            setAir(0);
            // 干涸伤害（对应 MC damageSources().dryOut()）
            auto dryOutSource = DamageSources::dryout();
            hurt(dryOutSource, 2.0f);
        }
    } else {
        // 在水中：空气恢复到 300
        setAir(maxAir());
    }
}

// ============================================================================
// AI 目标注册
// ============================================================================

void NautilusEntity::registerGoals()
{
    // 调用父类方法注册通用鹦鹉螺 AI 目标
    AbstractNautilusEntity::registerGoals();

    // 优先级 2: 繁殖（仅 NautilusEntity 可繁殖，ZombieNautilusEntity 不注册此目标）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 0.4));
}

void NautilusEntity::registerAttributes()
{
    // 调用父类方法注册基础属性
    AbstractNautilusEntity::registerAttributes();

    // NautilusEntity 使用父类默认属性（MAX_HEALTH=15, MOVEMENT_SPEED=1.0）
    // 不需要额外设置
}

} // namespace mc
