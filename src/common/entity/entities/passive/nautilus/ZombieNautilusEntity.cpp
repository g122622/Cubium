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

#include "ZombieNautilusEntity.hpp"

#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySpawnPlacementRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#include <cmath>
#include <memory>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ZombieNautilusEntity::ZombieNautilusEntity(EntityId id)
    : AbstractNautilusEntity(id)
{
    // 显式调用 registerData()，注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类
    // （Entity::Entity 内部调用 registerData() 时调用的是 Entity::registerData
    //   而非 ZombieNautilusEntity::registerData），必须在派生类构造函数中显式调用，
    // 参考 WolfEntity 模式。
    registerData();

    // 注册属性（父类构造函数未调用 registerAttributes）
    registerAttributes();

    // 父类构造函数已经调用 registerGoals()，但此时虚函数尚未派发到 ZombieNautilusEntity
    // 这里再次调用以确保 ZombieNautilusEntity 的虚函数版本被正确注册
    registerGoals();
}

std::unique_ptr<Entity> ZombieNautilusEntity::create(IWorld* /*world*/)
{
    // 使用临时 ID 0，实际 ID 由 EntityManager 分配
    return std::make_unique<ZombieNautilusEntity>(0);
}

// ============================================================================
// 音效
// ============================================================================

std::optional<ResourceLocation> ZombieNautilusEntity::getAmbientSound() const
{
    // 2 路分支：水下/陆地（无幼体变体，僵尸鹦鹉螺恒为成体）
    return isInWater() ? SoundEvents::ENTITY_ZOMBIE_NAUTILUS_AMBIENT
                       : SoundEvents::ENTITY_ZOMBIE_NAUTILUS_AMBIENT_ON_LAND;
}

std::optional<ResourceLocation> ZombieNautilusEntity::getHurtSound(DamageSource& /*source*/) const
{
    // 2 路分支：水下/陆地
    return isInWater() ? SoundEvents::ENTITY_ZOMBIE_NAUTILUS_HURT : SoundEvents::ENTITY_ZOMBIE_NAUTILUS_HURT_ON_LAND;
}

std::optional<ResourceLocation> ZombieNautilusEntity::getDeathSound() const
{
    // 2 路分支：水下/陆地
    return isInWater() ? SoundEvents::ENTITY_ZOMBIE_NAUTILUS_DEATH : SoundEvents::ENTITY_ZOMBIE_NAUTILUS_DEATH_ON_LAND;
}

std::optional<ResourceLocation> ZombieNautilusEntity::getDashSound() const
{
    // 2 路分支：水下/陆地
    return isInWater() ? SoundEvents::ENTITY_ZOMBIE_NAUTILUS_DASH : SoundEvents::ENTITY_ZOMBIE_NAUTILUS_DASH_ON_LAND;
}

std::optional<ResourceLocation> ZombieNautilusEntity::getDashReadySound() const
{
    // 2 路分支：水下/陆地
    return isInWater() ? SoundEvents::ENTITY_ZOMBIE_NAUTILUS_DASH_READY
                       : SoundEvents::ENTITY_ZOMBIE_NAUTILUS_DASH_READY_ON_LAND;
}

std::optional<ResourceLocation> ZombieNautilusEntity::getEatSound() const
{
    // 僵尸鹦鹉螺无幼体变体
    return SoundEvents::ENTITY_ZOMBIE_NAUTILUS_EAT;
}

// ============================================================================
// 生命周期
// ============================================================================

void ZombieNautilusEntity::tick()
{
    // 调用父类 tick 处理通用逻辑
    AbstractNautilusEntity::tick();

    // 亡灵在阳光下燃烧（对应 MC 1.21.11 ZombieNautilus 继承自 BURN_IN_DAYLIGHT 标签）
    // 装备鹦鹉螺铠甲（Body 槽）可替代燃烧，由 sunProtectionSlot() 返回 EquipmentSlot::Body
    burnUndead();
}

void ZombieNautilusEntity::finalizeSpawn(IWorld& /*world*/,
    const entity::combat::DifficultyInstance& /*difficulty*/,
    world::spawn::SpawnReason /*spawnReason*/)
{
    // 对应 MC 1.21.11 ZombieNautilus.finalizeSpawn()：
    // 根据生成位置的生物群系选择气候变体
    setVariant(selectVariantForBiome());
}

// ============================================================================
// 变体选择
// ============================================================================

ZombieNautilusVariant ZombieNautilusEntity::selectVariantForBiome() const
{
    // 对应 MC 1.21.11 VariantUtils.selectVariantToSpawn() + BiomeTags.SPAWNS_CORAL_VARIANT_ZOMBIE_NAUTILUS
    // 简化实现：根据生物群系 ID 选择变体
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return ZombieNautilusVariant::Temperate;
    }

    // 构造当前位置的 BlockPos
    BlockPos pos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    // 查询当前位置的生物群系
    const ChunkData* chunk = worldPtr->getChunk(pos.chunkX(), pos.chunkZ());
    if (chunk == nullptr) {
        return ZombieNautilusVariant::Temperate;
    }

    BiomeId biomeId = chunk->getBiomeAtBlock(pos.localX(), pos.y, pos.localZ());

    // 暖水海洋 → 温暖变体（参考 BiomeTags.SPAWNS_CORAL_VARIANT_ZOMBIE_NAUTILUS）
    if (biomeId == Biomes::WarmOcean || biomeId == Biomes::DeepWarmOcean) {
        return ZombieNautilusVariant::Warm;
    }

    // 冻洋/冷水海洋 → 寒冷变体
    if (biomeId == Biomes::FrozenOcean || biomeId == Biomes::DeepFrozenOcean || biomeId == Biomes::ColdOcean ||
        biomeId == Biomes::DeepColdOcean) {
        return ZombieNautilusVariant::Cold;
    }

    // 其他海洋 → 温带变体（默认）
    return ZombieNautilusVariant::Temperate;
}

// ============================================================================
// 同步数据与属性
// ============================================================================

void ZombieNautilusEntity::registerData()
{
    // 调用父类方法注册通用数据参数
    AbstractNautilusEntity::registerData();

    // 僵尸鹦鹉螺的变体存储为成员变量，不通过 DataParameter 同步
    // （MC 原版通过 EntityDataSerializers.ZOMBIE_NAUTILUS_VARIANT 同步 Holder<ZombieNautilusVariant>）
    // 简化实现：变体仅服务端持有，客户端通过 NBT 或初始构造获取
}

void ZombieNautilusEntity::registerAttributes()
{
    // 调用父类方法注册基础属性
    AbstractNautilusEntity::registerAttributes();

    // 僵尸鹦鹉螺比活体鹦鹉螺移动更快
    // 对应 MC 1.21.11 ZombieNautilus.createAttributes():
    // AbstractNautilus.createAttributes().add(Attributes.MOVEMENT_SPEED, 1.1F)
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 1.1f);
}

// ============================================================================
// NBT 序列化
// ============================================================================

void ZombieNautilusEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 调用父类方法
    AbstractNautilusEntity::addAdditionalSaveData(tag);

    // 保存变体
    tag.put("Variant", static_cast<i32>(static_cast<i32>(m_variant)));
}

Result<void> ZombieNautilusEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(AbstractNautilusEntity::readAdditionalSaveData(tag));

    // 加载变体
    if (auto variantVal = entity::serialization::nbt_helper::tryGetInt(tag, "Variant")) {
        i32 variantInt = *variantVal;
        switch (variantInt) {
            case static_cast<i32>(ZombieNautilusVariant::Temperate):
                m_variant = ZombieNautilusVariant::Temperate;
                break;
            case static_cast<i32>(ZombieNautilusVariant::Cold):
                m_variant = ZombieNautilusVariant::Cold;
                break;
            case static_cast<i32>(ZombieNautilusVariant::Warm):
                m_variant = ZombieNautilusVariant::Warm;
                break;
            default:
                // 未知变体，回退到默认（温带）
                m_variant = ZombieNautilusVariant::Temperate;
                break;
        }
    }

    return Result<void>::ok();
}

} // namespace mc
