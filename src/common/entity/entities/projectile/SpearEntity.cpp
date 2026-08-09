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

#include "SpearEntity.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../registry/VanillaEntityTypeKeys.hpp"
#include "../../serialization/EntityNbtKeys.hpp"
#include "../../serialization/NbtHelper.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "ProjectileHelper.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/ProjectileArrowStateComponent.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace entity {

namespace {

// ========== NBT 键名常量 ==========
// 参考 MC 1.21.11 AbstractArrow.addAdditionalSaveData() / ThrownTrident.addAdditionalSaveData()
// 注意：键名沿用 MC 1.16.5/1.21.11 的大小写规范，以保持与原版存档兼容。

constexpr const char* NBT_KEY_PICKUP = "pickup";            // 拾取状态（byte）
constexpr const char* NBT_KEY_DAMAGE = "damage";            // 基础伤害（float）
constexpr const char* NBT_KEY_IN_GROUND = "inGround";       // 是否插在方块中（byte/bool）
constexpr const char* NBT_KEY_CRIT = "crit";                // 是否暴击（byte/bool）
constexpr const char* NBT_KEY_PIERCE_LEVEL = "PierceLevel"; // 穿透等级（byte）
constexpr const char* NBT_KEY_DEALT_DAMAGE = "DealtDamage"; // 是否已造成伤害（byte/bool）
constexpr const char* NBT_KEY_KNOCKBACK = "knockback";      // 击退强度（int）

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity)
{
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

SpearEntity::SpearEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractArrowEntity(id, registry)
{
    // 长矛投掷命中伤害与三叉戟一致（8.0）
    setDamage(8.0f);
    setPickupStatus(PickupStatus::Allowed);
}

std::unique_ptr<Entity> SpearEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<SpearEntity>(0, registry);
}

void SpearEntity::onEntityHit(const RayTraceResult& result)
{
    if (!result.hitEntity) {
        return;
    }

    Entity* target = result.hitEntity;

    // 长矛投掷伤害固定 8.0（不随层级变化）
    f32 damage = 8.0f;

    // 获取射击者
    Entity* shooter = getShooter();

    // 创建伤害来源：投掷长矛伤害（使用 DamageType::Spear 类型）
    std::unique_ptr<DamageSource> damageSource;
    if (shooter != nullptr) {
        bool isPlayer = shooter->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Spear, shooter, this, isPlayer);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Spear, this, this, false);
    }

    // 标记已造成伤害
    setDealtDamage(true);

    // 应用伤害
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(target);
    if (livingTarget != nullptr) {
        livingTarget->hurt(*damageSource, damage);
    }

    // 击退效果
    if (knockbackStrength() > 0) {
        f32 ratio = 0.6f * static_cast<f32>(knockbackStrength());
        Vector3 horizontalVel(m_builtIn.velocity->m_velocity.x, 0.0f, m_builtIn.velocity->m_velocity.z);
        if (horizontalVel.lengthSquared() > 0.0f) {
            horizontalVel = horizontalVel.normalized();
            Vector3 knockback(horizontalVel.x * ratio, 0.1f, horizontalVel.z * ratio);
            target->addVelocity(knockback);
        }
    }

    // 速度反转为轻微反弹（与三叉戟一致）
    m_builtIn.velocity->m_velocity = Vector3(m_builtIn.velocity->m_velocity.x * -0.01f,
        m_builtIn.velocity->m_velocity.y * -0.1f,
        m_builtIn.velocity->m_velocity.z * -0.01f);

    // 播放命中音效
    playSound(SoundEvents::ITEM_SPEAR_HIT, 1.0f, 1.0f);
}

void SpearEntity::onBlockHit(const RayTraceResult& result)
{
    setInGround(true);

    auto* arrowState = tryGetComponent<ecs::ProjectileArrowStateComponent>();
    // 保存方块状态
    if (arrowState != nullptr && m_world && result.type == RayTraceResultType::Block) {
        const BlockState* state = m_world->getBlockState(result.blockPos.x, result.blockPos.y, result.blockPos.z);
        if (state != nullptr) {
            *arrowState->m_inBlockState = *state;
        }
    }

    // 清除暴击和穿透状态
    setCritical(false);
    setPierceLevel(0);
    clearPiercedEntities();

    // 播放命中地面音效
    playSound(SoundEvents::ITEM_SPEAR_HIT_GROUND, 1.0f, 1.0f);
}

f32 SpearEntity::getWaterDrag() const
{
    // 长矛水中阻力与三叉戟一致（极小）
    return 0.99f;
}

void SpearEntity::setBaseDamageFromMob(f32 power)
{
    // 长矛不使用弓类附魔（力量/冲击/火焰），公式与三叉戟相同：
    // damage = power * 2.0 + triangle(difficulty * 0.11, 0.57425)
    math::Random rng = createRandomFromEntity(*this);
    f32 difficultyBonus = m_world ? static_cast<f32>(static_cast<u8>(m_world->difficulty())) * 0.11f : 0.0f;
    f32 triangle = difficultyBonus + (rng.nextFloat() - rng.nextFloat()) * 0.57425f;
    setDamage(power * 2.0f + triangle);
}

bool SpearEntity::onPlayerPickup(Player& player)
{
    // 必须在服务端执行
    if (m_world->isClientSide()) {
        return false;
    }

    // 只有当长矛在地上时才能被拾取
    if (!isInGround()) {
        return false;
    }

    // 长矛不能处于抖动状态
    if (arrowShake() > 0) {
        return false;
    }

    // 检查拾取权限
    bool canPickup = (pickupStatus() == PickupStatus::Allowed) ||
        (pickupStatus() == PickupStatus::CreativeOnly && player.isCreative());

    if (!canPickup) {
        return false;
    }

    // 添加到玩家背包
    if (!m_spearStack.isEmpty()) {
        i32 added = player.inventory().add(m_spearStack);
        (void)added;
        if (m_spearStack.getCount() > 0) {
            return false; // 背包满了
        }
    }

    // 播放拾取音效
    if (m_world) {
        playSound(SoundEvents::ENTITY_ITEM_PICKUP, 0.2f, 1.0f);
    }

    remove();
    return true;
}

// ============================================================================
// NBT 序列化
// ============================================================================

void SpearEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现（Entity 基类，AbstractArrowEntity 未重写）
    Entity::addAdditionalSaveData(tag);

    const auto* arrowState = tryGetComponent<ecs::ProjectileArrowStateComponent>();

    using namespace serialization::nbt_helper;

    // 长矛物品堆（参考 ItemEntity::addAdditionalSaveData 的 ItemStack 写入模式）
    // 键名沿用 MC 1.21.11 AbstractArrow 的 "item" 键
    nbt::tags::compound_tag itemTag;
    m_spearStack.toNbt(itemTag);
    tag.value.emplace(serialization::nbt_keys::ITEM, std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));

    // 拾取状态（byte：0=Disallowed, 1=Allowed, 2=CreativeOnly）
    if (arrowState != nullptr) {
        tag.put(NBT_KEY_PICKUP, static_cast<i8>(arrowState->m_pickupStatus));

        // 基础伤害（float）
        tag.put(NBT_KEY_DAMAGE, arrowState->m_damage);

        // 是否插在方块中（bool，底层 byte）
        tag.put(NBT_KEY_IN_GROUND, static_cast<i8>(arrowState->m_inGround ? 1 : 0));

        // 是否暴击（bool，底层 byte）
        tag.put(NBT_KEY_CRIT, static_cast<i8>(arrowState->m_critical ? 1 : 0));

        // 穿透等级（byte）
        tag.put(NBT_KEY_PIERCE_LEVEL, static_cast<i8>(arrowState->m_pierceLevel));

        // 是否已造成伤害（bool，底层 byte）—— 参考 ThrownTrident 的 "DealtDamage" 键
        tag.put(NBT_KEY_DEALT_DAMAGE, static_cast<i8>(arrowState->m_dealtDamage ? 1 : 0));

        // 击退强度（int）
        tag.put(NBT_KEY_KNOCKBACK, arrowState->m_knockbackStrength);
    }
}

Result<void> SpearEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用基类实现，用 MC_TRY 传播错误
    MC_TRY(Entity::readAdditionalSaveData(tag));

    auto* arrowState = tryGetComponent<ecs::ProjectileArrowStateComponent>();

    using namespace serialization::nbt_helper;

    // 长矛物品堆
    const nbt::tags::compound_tag* itemTag = tryGetCompound(tag, serialization::nbt_keys::ITEM);
    if (itemTag != nullptr) {
        auto stackResult = ItemStack::fromNbt(*itemTag);
        if (stackResult.success()) {
            m_spearStack = stackResult.value();
        }
        // 反序列化失败时保留默认空堆，避免存档损坏导致崩溃
    }

    if (arrowState == nullptr) {
        return Result<void>::ok();
    }

    // 拾取状态
    if (auto val = tryGetByte(tag, NBT_KEY_PICKUP)) {
        // 防御性 clamp，避免存档数据越界
        i8 v = *val;
        if (v >= static_cast<i8>(PickupStatus::Disallowed) && v <= static_cast<i8>(PickupStatus::CreativeOnly)) {
            arrowState->m_pickupStatus = static_cast<PickupStatus>(v);
        }
    }

    // 基础伤害
    if (auto val = tryGetFloat(tag, NBT_KEY_DAMAGE)) {
        arrowState->m_damage = *val;
    }

    // 是否插在方块中
    if (auto val = tryGetBool(tag, NBT_KEY_IN_GROUND)) {
        arrowState->m_inGround = *val;
    }

    // 是否暴击
    if (auto val = tryGetBool(tag, NBT_KEY_CRIT)) {
        arrowState->m_critical = *val;
    }

    // 穿透等级
    if (auto val = tryGetByte(tag, NBT_KEY_PIERCE_LEVEL)) {
        arrowState->m_pierceLevel = static_cast<u8>(*val);
    }

    // 是否已造成伤害
    if (auto val = tryGetBool(tag, NBT_KEY_DEALT_DAMAGE)) {
        arrowState->m_dealtDamage = *val;
    }

    // 击退强度
    if (auto val = tryGetInt(tag, NBT_KEY_KNOCKBACK)) {
        arrowState->m_knockbackStrength = *val;
    }

    return Result<void>::ok();
}

} // namespace entity
} // namespace mc
