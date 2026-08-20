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

#include "common/entity/entities/item/ItemEntity.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/explosion/ExplosionImmunityContext.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace mc::trace;

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

// ============================================================================
// 静态数据参数
// ============================================================================

entity::DataParameter<network::ir::play::ItemStackView> ItemEntity::DATA_ITEM_PARAM =
    entity::EntityDataManager::createKey<network::ir::play::ItemStackView>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = Entity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& ItemEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"ItemEntity", &Entity::classInfo()};
    return s_classInfo;
}

// ============================================================================
// 静态工厂方法
// ============================================================================

std::unique_ptr<Entity> ItemEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    // 创建一个空的物品实体，使用临时ID 0
    // 实际ID会在 EntityManager::addEntity() 时分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    ItemStack emptyStack;
    return std::make_unique<ItemEntity>(0, emptyStack, 0.0f, 0.0f, 0.0f, registry);
}

// ============================================================================
// 构造函数
// ============================================================================

ItemEntity::ItemEntity(EntityInstanceId id, const ItemStack& stack, f32 x, f32 y, f32 z, ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
    , m_itemStack(stack)
{
    // ItemEntity 直接继承 Entity，DATA_ITEM_PARAM 应分配到继承链 id 8（Entity 8 字段之后）。
    // 用 ClassRegisterGuard 提供 ItemEntity 类上下文，使 registerParam 沿继承链分配 id。
    {
        entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());
        m_dataManager.registerParam(DATA_ITEM_PARAM, network::ir::toItemStackView(stack));
    }
    setPosition(x, y, z);
    setRotation(0.0f, 0.0f);

    // 初始化速度（轻微随机）
    math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    m_builtIn.velocity->m_velocity.x = rng.nextFloat(-0.1f, 0.1f);
    m_builtIn.velocity->m_velocity.y = 0.2f; // 轻微向上
    m_builtIn.velocity->m_velocity.z = rng.nextFloat(-0.1f, 0.1f);
}

ItemEntity::ItemEntity(EntityInstanceId id,
    const ItemStack& stack,
    f32 x,
    f32 y,
    f32 z,
    f32 vx,
    f32 vy,
    f32 vz,
    ecs::EntityRegistry& registry)
    : Entity(id, nullptr, registry)
    , m_itemStack(stack)
{
    // 同上：用 ClassRegisterGuard 提供 ItemEntity 类上下文，分配继承链 id。
    {
        entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());
        m_dataManager.registerParam(DATA_ITEM_PARAM, network::ir::toItemStackView(stack));
    }
    setPosition(x, y, z);
    setRotation(0.0f, 0.0f);
    setVelocity(vx, vy, vz);
}

// ============================================================================
// 物品操作
// ============================================================================

void ItemEntity::setItemStack(const ItemStack& stack)
{
    m_itemStack = stack;
    m_dataManager.set(DATA_ITEM_PARAM, network::ir::toItemStackView(stack));
}

// ============================================================================
// Entity 接口
// ============================================================================

bool ItemEntity::dampensVibrations() const
{
    // 羊毛物品掉落时阻尼振动
    const auto* item = m_itemStack.getItem();
    return item != nullptr && item->isIn(item::tag::ItemTags::DAMPENS_VIBRATIONS());
}

bool ItemEntity::isImmuneToFire() const
{
    // 防火物品（如下界合金物品、下界星）免疫火焰，否则回退到实体类型的默认行为
    if (!m_itemStack.isEmpty() && !m_itemStack.canBeHurtBy(DamageSources::inFire())) {
        return true;
    }
    return Entity::isImmuneToFire();
}

bool ItemEntity::ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const
{
    // 仅当爆炸影响方块类实体时才受影响；否则掉落物忽略爆炸（不被击飞/销毁）。
    return ctx.shouldAffectBlocklikeEntities ? Entity::ignoreExplosion(ctx) : true;
}

bool ItemEntity::hurt(DamageSource& source, f32 amount)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Game.Entity, "ItemEntity::hurt", "entityId", id(), "amount", amount);

    // 1. 检查无敌状态
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 2. 检查 mobGriefing 游戏规则：如果伤害来源是 Mob 且 mobGriefing 关闭，则不受伤害
    if (m_world != nullptr) {
        Entity* sourceEntity = source.getEntity();
        if (sourceEntity != nullptr && dynamic_cast<MobEntity*>(sourceEntity) != nullptr) {
            if (!m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
                return false;
            }
        }
    }

    // 3. 检查物品是否能被此伤害源伤害（防火物品免疫火焰伤害）
    if (!m_itemStack.canBeHurtBy(source)) {
        return false;
    }

    // 4. 标记受伤（触发速度同步到客户端）
    markHurt();

    // 5. 减少生命值
    m_health = static_cast<i32>(m_health - amount);

    // 6. 发送 ENTITY_DAMAGE 游戏事件（用于幽匿感测体检测）
    if (m_world != nullptr) {
        BlockPos blockPos(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y)),
            static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
        m_world->gameEvent(
            gameevent::GameEvents::ENTITY_DAMAGE, blockPos, gameevent::GameEvent::Context::of(source.getEntity()));
    }

    // 7. 如果生命值降至 0 或以下，销毁物品
    if (m_health <= 0) {
        // 调用物品的 onDestroyed 回调（子类可重写以执行特殊逻辑，如播放破坏音效）
        // 注意：getItem() 返回 const Item*，但 onDestroyed 是非 const 方法
        // 这里使用 const_cast 是安全的，因为 Item 对象在注册表中以非 const 方式持有
        if (m_world != nullptr) {
            auto* item = const_cast<Item*>(m_itemStack.getItem());
            if (item != nullptr) {
                item->onDestroyed(m_itemStack, *m_world, *this);
            }
        }
        discard();
    }

    return true;
}

void ItemEntity::tick()
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Game.Entity, "ItemEntity::tick", "entityId", id(), "age", m_age, "count", getCount());

    // 递增存活时间（Entity::tick 会在 baseTick 前递增，ItemEntity 直接调用 baseTick 需手动递增）
    m_ticksExisted++;

    // 空物品立即移除
    if (m_itemStack.isEmpty()) {
        remove();
        return;
    }

    // 基础 tick：更新前一帧位置、处理火焰计时器、环境状态等
    Entity::baseTick();

    // 增加年龄
    m_age++;

    // 检查是否过期
    if (isExpired()) {
        remove();
        return;
    }

    // 减少拾取延迟：仅当 >0 且非创造假物品时递减
    if (m_pickupDelay > 0 && m_pickupDelay != FAKE_PICKUP_DELAY) {
        m_pickupDelay--;
    }

    // 更新物理
    _updatePhysics();

    // 触发方块碰撞：对齐 MC Java Entity.tick() 在移动后无条件调用 checkInsideBlocks()
    // （即 doBlockCollisions()）的语义，使物品实体能触发所处方块的 onEntityCollision 回调
    // （压力板探测、漏斗吸取、仙人掌/火/岩浆销毁物品、甜浆果丛等）。
    //
    // 为何不能依赖 Entity::move() 内部的 doBlockCollisions()（Entity.cpp:1172/1249）：
    // ItemEntity::_updatePhysics() 仅在「速度>0.001 或不在地面」时才调 move（ItemEntity.cpp:502），
    // 物品落地静止后跳过 move，于是 doBlockCollisions 不再被调用，导致静止物品永不触发
    // onEntityCollision（压力板/漏斗失效）。vanilla Java 的 checkInsideBlocks 是每 tick 无条件执行，
    // 与实体是否移动无关，故此处手动无条件调用，参照 BoatEntity.cpp:238、ThrowableEntity.cpp:114 的
    // 手动调用模式。放 _updatePhysics() 之后，确保用最新位置/碰撞箱遍历方块。
    //
    // 注意：岩浆点燃/伤害由 LiquidBlock::onEntityCollision 统一处理（LiquidBlock.cpp:370-371），
    // 不再在 _applyLavaPhysics 重复调用（见下 _applyLavaPhysics 注释）。
    doBlockCollisions();

    // 更新合并检测
    _updateMerge();
}

// ============================================================================
// 玩家拾取
// ============================================================================

bool ItemEntity::onPlayerPickup(Player& player)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Game.Entity,
        "ItemEntity::onPlayerPickup",
        "entityId",
        id(),
        "playerId",
        player.playerId(),
        "count",
        getCount());

    if (!canBePickedUp()) {
        return false;
    }

    if (!m_ownerUuid.empty() && player.uuid() != m_ownerUuid) {
        const bool nearDespawn = m_lifetime != INFINITE_LIFETIME && (m_lifetime - m_age) <= 200;
        if (!nearDespawn) {
            return false;
        }
    }

    PlayerInventory& inventory = player.inventory();
    inventory.add(m_itemStack);

    if (!m_itemStack.isEmpty()) {
        return false;
    }

    remove();
    return true;
}

void ItemEntity::setOwner(const std::string& ownerUuid, const std::string& throwerUuid)
{
    m_ownerUuid = ownerUuid;
    m_throwerUuid = throwerUuid;
}

// ============================================================================
// 物品合并
// ============================================================================

void ItemEntity::_updateMerge()
{
    // 检查是否允许合并
    if (m_itemStack.isEmpty() || isRemoved()) {
        return;
    }

    // 检查是否可以合并（堆叠未满、拾取延迟不是无限、年龄在允许范围内）
    if (m_itemStack.getCount() >= m_itemStack.getMaxStackSize()) {
        return; // 已满堆，不需要合并
    }

    // 无限拾取延迟的物品不合并
    if (m_pickupDelay == FAKE_PICKUP_DELAY) {
        return;
    }

    // 无限寿命的物品不合并（age == -32768 或 -6000）
    if (m_age == -32768 || m_age < -6000) {
        return;
    }

    // 如果没有世界，无法查询其他实体
    if (!m_world) {
        return;
    }

    // 合并检测间隔：移动时每 2 tick 检测，静止时每 40 tick 检测
    bool hasMoved = m_builtIn.stateVector->m_pos.distanceSquared(m_builtIn.stateVector->m_posPrev) > 0.0001f;
    i32 checkInterval = hasMoved ? 2 : 40;
    if (m_ticksExisted % checkInterval != 0) {
        return;
    }

    // 搜索附近可合并的物品实体
    AxisAlignedBB searchBox = m_builtIn.aabbShape->m_aabb.expand(MERGE_RANGE, 0.0f, MERGE_RANGE);
    std::vector<Entity*> nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : nearbyEntities) {
        // 只处理物品实体
        if (entity->entityType() != entity::VanillaEntityTypeKeys::ITEM) {
            continue;
        }

        ItemEntity* other = static_cast<ItemEntity*>(entity);

        // 检查是否可以合并
        if (!other->canMergeWith(*this)) {
            continue;
        }

        // 检查所有者匹配
        if (m_ownerUuid != other->m_ownerUuid) {
            continue;
        }

        // 执行合并：数量较少的合并到数量较多的
        if (other->m_itemStack.getCount() < m_itemStack.getCount()) {
            // other 数量少，合并到当前实体（this）
            tryMergeWith(*other);
        } else {
            // 当前实体数量少或不大于 other，合并到 other
            other->tryMergeWith(*this);
        }

        // 如果当前实体已满堆或被移除，停止搜索
        if (m_itemStack.getCount() >= m_itemStack.getMaxStackSize() || isRemoved()) {
            break;
        }
    }
}

bool ItemEntity::tryMergeWith(ItemEntity& other)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Game.Entity,
        "ItemEntity::tryMergeWith",
        "thisId",
        id(),
        "otherId",
        other.id(),
        "thisCount",
        getCount(),
        "otherCount",
        other.getCount());

    if (!canMergeWith(other)) {
        return false;
    }

    // 计算合并后的数量
    i32 total = m_itemStack.getCount() + other.m_itemStack.getCount();
    i32 maxStack = m_itemStack.getMaxStackSize();

    if (total <= maxStack) {
        // 全部合并到当前实体
        m_itemStack.setCount(total);

        // 合并后更新属性：取较大的拾取延迟，取较小的年龄
        m_pickupDelay = std::max(m_pickupDelay, other.m_pickupDelay);
        m_age = std::min(m_age, other.m_age);

        other.remove();
        return true;
    }

    // 部分合并
    i32 toTake = maxStack - m_itemStack.getCount();
    m_itemStack.grow(toTake);
    other.m_itemStack.shrink(toTake);

    // 更新属性
    m_pickupDelay = std::max(m_pickupDelay, other.m_pickupDelay);
    m_age = std::min(m_age, other.m_age);

    return true;
}

bool ItemEntity::canMergeWith(const ItemEntity& other) const
{
    // 检查是否可以合并
    if (m_itemStack.isEmpty() || other.m_itemStack.isEmpty()) {
        return false;
    }

    // 检查物品类型是否相同
    if (!m_itemStack.isSameItem(other.m_itemStack)) {
        return false;
    }

    // 检查耐久度是否相同
    if (m_itemStack.getDamage() != other.m_itemStack.getDamage()) {
        return false;
    }

    // 检查是否已达到堆叠上限
    if (m_itemStack.getCount() >= m_itemStack.getMaxStackSize()) {
        return false;
    }

    // 检查其他实体是否已达到堆叠上限
    if (other.m_itemStack.getCount() >= other.m_itemStack.getMaxStackSize()) {
        return false;
    }

    // 检查其他实体是否可合并
    if (other.isRemoved()) {
        return false;
    }

    if (other.m_pickupDelay == FAKE_PICKUP_DELAY) {
        return false;
    }

    if (other.m_age == -32768 || other.m_age < -6000) {
        return false;
    }

    return true;
}

// ============================================================================
// 物理更新
// ============================================================================

void ItemEntity::_updatePhysics()
{
    // 1. 检测环境
    bool inWater = false;
    bool inLava = false;
    if (m_world) {
        const i32 blockX = static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x));
        const i32 blockY = static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y));
        const i32 blockZ = static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z));
        inWater = m_world->isWaterAt(blockX, blockY, blockZ);
        inLava = m_world->isLavaAt(blockX, blockY, blockZ);
    }

    // 2. 应用重力和阻力（仅一次）
    if (inWater) {
        _applyWaterPhysics();
    } else if (inLava) {
        _applyLavaPhysics();
    } else {
        _applyNormalPhysics();
    }

    // 3. 执行移动
    if (std::abs(m_builtIn.velocity->m_velocity.x) > 0.001f || std::abs(m_builtIn.velocity->m_velocity.y) > 0.001f ||
        std::abs(m_builtIn.velocity->m_velocity.z) > 0.001f || !m_builtIn.physicsState->m_onGround) {

        // 带碰撞移动
        if (m_physicsEngine || m_world) {
            Vector3 actual = moveWithCollision(
                m_builtIn.velocity->m_velocity.x, m_builtIn.velocity->m_velocity.y, m_builtIn.velocity->m_velocity.z);

            // 落地反弹逻辑
            if (m_builtIn.physicsState->m_collidedVertically) {
                if (m_builtIn.velocity->m_velocity.y < 0.0f) {
                    // 落地：反弹并减速
                    m_builtIn.velocity->m_velocity.y = -m_builtIn.velocity->m_velocity.y * 0.5f;
                    m_builtIn.physicsState->m_onGround = true;
                    m_builtIn.physicsState->m_fallDistance = 0.0f;
                } else {
                    // 撞到天花板
                    m_builtIn.velocity->m_velocity.y = 0.0f;
                }
            }

            // 水平碰撞反弹
            if (m_builtIn.physicsState->m_collidedHorizontally) {
                m_builtIn.velocity->m_velocity.x *= -0.5f;
                m_builtIn.velocity->m_velocity.z *= -0.5f;
            }
        } else {
            move(m_builtIn.velocity->m_velocity.x, m_builtIn.velocity->m_velocity.y, m_builtIn.velocity->m_velocity.z);
        }
    }

    // 注意：不再调用 Entity::applyPhysics()，因为重力/阻力已在上面应用
}

void ItemEntity::_applyNormalPhysics()
{
    // 使用统一物理常量
    m_builtIn.velocity->m_velocity.y -= physics::ITEM_GRAVITY;
    m_builtIn.velocity->m_velocity.y *= physics::ITEM_DRAG;

    m_builtIn.velocity->m_velocity.x *= physics::ITEM_HORIZONTAL_DRAG;
    m_builtIn.velocity->m_velocity.z *= physics::ITEM_HORIZONTAL_DRAG;

    // 速度阈值
    constexpr f32 VELOCITY_THRESHOLD = 0.001f;
    if (std::abs(m_builtIn.velocity->m_velocity.x) < VELOCITY_THRESHOLD) m_builtIn.velocity->m_velocity.x = 0.0f;
    if (std::abs(m_builtIn.velocity->m_velocity.y) < VELOCITY_THRESHOLD) m_builtIn.velocity->m_velocity.y = 0.0f;
    if (std::abs(m_builtIn.velocity->m_velocity.z) < VELOCITY_THRESHOLD) m_builtIn.velocity->m_velocity.z = 0.0f;
}

void ItemEntity::_applyWaterPhysics()
{
    // 水中物理：轻微浮力 + 阻力
    // 浮力：当速度小于 0.06 时，向上推 0.0005
    if (m_builtIn.velocity->m_velocity.y < 0.06f) {
        m_builtIn.velocity->m_velocity.y += 0.0005f;
    }
    // 水中阻力：0.99
    m_builtIn.velocity->m_velocity.x *= 0.99f;
    m_builtIn.velocity->m_velocity.y *= 0.99f;
    m_builtIn.velocity->m_velocity.z *= 0.99f;
}

void ItemEntity::_applyLavaPhysics()
{
    // 岩浆中物理：浮力 + 阻力
    // 浮力：向上推 0.0005
    m_builtIn.velocity->m_velocity.y += 0.0005f;
    // 岩浆阻力：0.95
    m_builtIn.velocity->m_velocity.x *= 0.95f;
    m_builtIn.velocity->m_velocity.y *= 0.95f;
    m_builtIn.velocity->m_velocity.z *= 0.95f;

    // 岩浆点燃和伤害：由 LiquidBlock::onEntityCollision 统一处理（LiquidBlock.cpp:370-371 调
    // entity.lavaIgnite() + entity.lavaHurt()），对应 MC Java LavaFluid.entityInside()。
    // 此前因 ItemEntity::tick 未调 doBlockCollisions()，岩浆方块碰撞回调无法触发，故在此手动补偿
    // 调 lavaIgnite()/lavaHurt()。现 tick 已无条件调 doBlockCollisions()（见 ItemEntity::tick），
    // 若保留此补偿会导致每 tick 双倍岩浆伤害（4+4=8，物品仅 5 生命，瞬间销毁，违反 vanilla），
    // 故移除补偿，岩浆伤害完全交由 LiquidBlock::onEntityCollision 处理。
}

// ============================================================================
// NBT 序列化
// ============================================================================

void ItemEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现
    Entity::addAdditionalSaveData(tag);

    // Item (compound) - 物品堆 NBT
    nbt::tags::compound_tag itemTag;
    m_itemStack.toNbt(itemTag);
    tag.value.emplace(nbt_keys::ITEM, std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));

    // Health (i16) - 生命值（默认 5）
    tag.put(nbt_keys::HEALTH, static_cast<i16>(m_health));

    // Age (i32) - 实体年龄
    tag.put(nbt_keys::AGE, m_age);

    // PickupDelay (i32) - 拾取延迟
    tag.put(nbt_keys::PICKUP_DELAY, m_pickupDelay);

    // Owner (string, optional) - 所有者 UUID
    if (!m_ownerUuid.empty()) {
        tag.put(nbt_keys::OWNER, m_ownerUuid);
    }

    // Thrower (string, optional) - 投掷者 UUID
    if (!m_throwerUuid.empty()) {
        tag.put(nbt_keys::THROWER, m_throwerUuid);
    }
}

Result<void> ItemEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用基类实现
    MC_TRY(Entity::readAdditionalSaveData(tag));

    // Item (compound) - 物品堆 NBT
    const nbt::tags::compound_tag* itemTag = nbt_helper::tryGetCompound(tag, nbt_keys::ITEM);
    if (itemTag != nullptr) {
        auto stackResult = ItemStack::fromNbt(*itemTag);
        if (stackResult.success()) {
            m_itemStack = stackResult.value();
            // 同步物品本体（含 itemId/count/componentsPatch）到数据管理器
            m_dataManager.set(DATA_ITEM_PARAM, network::ir::toItemStackView(m_itemStack));
        }
    }

    // Health (i16) - 生命值（默认 5）
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::HEALTH)) {
        m_health = static_cast<i32>(*val);
    }

    // Age (i32) - 实体年龄
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::AGE)) {
        m_age = *val;
    }

    // PickupDelay (i32) - 拾取延迟
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::PICKUP_DELAY)) {
        m_pickupDelay = *val;
    }

    // Owner (string, optional) - 所有者 UUID
    if (auto val = nbt_helper::tryGetString(tag, nbt_keys::OWNER)) {
        m_ownerUuid = *val;
    }

    // Thrower (string, optional) - 投掷者 UUID
    if (auto val = nbt_helper::tryGetString(tag, nbt_keys::THROWER)) {
        m_throwerUuid = *val;
    }

    return Result<void>::ok();
}

} // namespace mc
