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

#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

// ============================================================================
// 静态数据参数
// ============================================================================

entity::DataParameter<i32> ItemEntity::DATA_ITEM_COUNT_PARAM = entity::EntityDataManager::createKey<i32>();

// ============================================================================
// 静态工厂方法
// ============================================================================

std::unique_ptr<Entity> ItemEntity::create(IWorld* /*world*/)
{
    // 创建一个空的物品实体，使用临时ID 0
    // 实际ID会在 EntityManager::addEntity() 时分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    ItemStack emptyStack;
    return std::make_unique<ItemEntity>(0, emptyStack, 0.0f, 0.0f, 0.0f);
}

// ============================================================================
// 构造函数
// ============================================================================

ItemEntity::ItemEntity(EntityId id, const ItemStack& stack, f32 x, f32 y, f32 z)
    : Entity(id)
    , m_itemStack(stack)
{
    m_dataManager.registerParam(DATA_ITEM_COUNT_PARAM, stack.getCount());
    setPosition(x, y, z);
    setRotation(0.0f, 0.0f);

    // 初始化速度（轻微随机）
    math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    m_velocity.x = rng.nextFloat(-0.1f, 0.1f);
    m_velocity.y = 0.2f; // 轻微向上
    m_velocity.z = rng.nextFloat(-0.1f, 0.1f);
}

ItemEntity::ItemEntity(EntityId id, const ItemStack& stack, f32 x, f32 y, f32 z, f32 vx, f32 vy, f32 vz)
    : Entity(id)
    , m_itemStack(stack)
{
    m_dataManager.registerParam(DATA_ITEM_COUNT_PARAM, stack.getCount());
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
    m_dataManager.set(DATA_ITEM_COUNT_PARAM, stack.getCount());
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

bool ItemEntity::hurt(DamageSource& source, f32 amount)
{
    MC_TRACE_EVENT("game.entity", "ItemEntity::hurt", "entityId", id(), "amount", amount);

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
        BlockPos blockPos(static_cast<i32>(std::floor(m_position.x)),
            static_cast<i32>(std::floor(m_position.y)),
            static_cast<i32>(std::floor(m_position.z)));
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
    MC_TRACE_EVENT("game.entity", "ItemEntity::tick", "entityId", id(), "age", m_age, "count", getCount());

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

    // 更新合并检测
    _updateMerge();
}

// ============================================================================
// 玩家拾取
// ============================================================================

bool ItemEntity::onPlayerPickup(Player& player)
{
    MC_TRACE_EVENT("game.entity",
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
    bool hasMoved = m_position.distanceSquared(m_prevPosition) > 0.0001f;
    i32 checkInterval = hasMoved ? 2 : 40;
    if (m_ticksExisted % checkInterval != 0) {
        return;
    }

    // 搜索附近可合并的物品实体
    AxisAlignedBB searchBox = m_boundingBox.expand(MERGE_RANGE, 0.0f, MERGE_RANGE);
    std::vector<Entity*> nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : nearbyEntities) {
        // 只处理物品实体
        if (entity->typeId() != entity::EntityTypeIdNumber::ITEM) {
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
    MC_TRACE_EVENT("game.entity",
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
        const i32 blockX = static_cast<i32>(std::floor(m_position.x));
        const i32 blockY = static_cast<i32>(std::floor(m_position.y));
        const i32 blockZ = static_cast<i32>(std::floor(m_position.z));
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
    if (std::abs(m_velocity.x) > 0.001f || std::abs(m_velocity.y) > 0.001f || std::abs(m_velocity.z) > 0.001f ||
        !m_onGround) {

        // 带碰撞移动
        if (m_physicsEngine || m_world) {
            Vector3 actual = moveWithCollision(m_velocity.x, m_velocity.y, m_velocity.z);

            // 落地反弹逻辑
            if (m_collidedVertically) {
                if (m_velocity.y < 0.0f) {
                    // 落地：反弹并减速
                    m_velocity.y = -m_velocity.y * 0.5f;
                    m_onGround = true;
                    m_fallDistance = 0.0f;
                } else {
                    // 撞到天花板
                    m_velocity.y = 0.0f;
                }
            }

            // 水平碰撞反弹
            if (m_collidedHorizontally) {
                m_velocity.x *= -0.5f;
                m_velocity.z *= -0.5f;
            }
        } else {
            move(m_velocity.x, m_velocity.y, m_velocity.z);
        }
    }

    // 注意：不再调用 Entity::applyPhysics()，因为重力/阻力已在上面应用
}

void ItemEntity::_applyNormalPhysics()
{
    // 使用统一物理常量
    m_velocity.y -= physics::ITEM_GRAVITY;
    m_velocity.y *= physics::ITEM_DRAG;

    m_velocity.x *= physics::ITEM_HORIZONTAL_DRAG;
    m_velocity.z *= physics::ITEM_HORIZONTAL_DRAG;

    // 速度阈值
    constexpr f32 VELOCITY_THRESHOLD = 0.001f;
    if (std::abs(m_velocity.x) < VELOCITY_THRESHOLD) m_velocity.x = 0.0f;
    if (std::abs(m_velocity.y) < VELOCITY_THRESHOLD) m_velocity.y = 0.0f;
    if (std::abs(m_velocity.z) < VELOCITY_THRESHOLD) m_velocity.z = 0.0f;
}

void ItemEntity::_applyWaterPhysics()
{
    // 水中物理：轻微浮力 + 阻力
    // 浮力：当速度小于 0.06 时，向上推 0.0005
    if (m_velocity.y < 0.06f) {
        m_velocity.y += 0.0005f;
    }
    // 水中阻力：0.99
    m_velocity.x *= 0.99f;
    m_velocity.y *= 0.99f;
    m_velocity.z *= 0.99f;
}

void ItemEntity::_applyLavaPhysics()
{
    // 岩浆中物理：浮力 + 阻力
    // 浮力：向上推 0.0005
    m_velocity.y += 0.0005f;
    // 岩浆阻力：0.95
    m_velocity.x *= 0.95f;
    m_velocity.y *= 0.95f;
    m_velocity.z *= 0.95f;

    // 岩浆点燃和伤害
    // 参考 MC Java: LavaFluid.entityInside() -> lavaIgnite() + lavaHurt()
    // ItemEntity 不经过 Entity::tick() -> doBlockCollisions() 路径，
    // 因此需要在此处手动调用岩浆点燃和伤害
    lavaIgnite();
    lavaHurt();
}

// ============================================================================
// 序列化
// ============================================================================

void ItemEntity::serialize(network::PacketSerializer& ser) const
{
    // 实体类型和ID
    ser.writeU32(static_cast<u32>(entity::EntityTypeIdNumber::ITEM));
    ser.writeU32(static_cast<u32>(m_id));

    // 位置（网络协议使用 f64）
    ser.writeF64(static_cast<f64>(m_position.x));
    ser.writeF64(static_cast<f64>(m_position.y));
    ser.writeF64(static_cast<f64>(m_position.z));

    // 速度（网络协议使用 f64）
    ser.writeF64(static_cast<f64>(m_velocity.x));
    ser.writeF64(static_cast<f64>(m_velocity.y));
    ser.writeF64(static_cast<f64>(m_velocity.z));

    // 旋转
    ser.writeF32(m_yaw);
    ser.writeF32(m_pitch);

    // 物品堆
    m_itemStack.serialize(ser);

    // 额外数据
    ser.writeI32(m_age);
    ser.writeI32(m_pickupDelay);
    ser.writeI32(m_lifetime);
    ser.writeBool(m_unpickable);
}

Result<std::unique_ptr<ItemEntity>> ItemEntity::deserialize(network::PacketDeserializer& deser, EntityId id)
{

    // 读取位置（网络协议使用 f64）
    auto xResult = deser.readF64();
    if (xResult.failed()) return xResult.error();
    f32 x = static_cast<f32>(xResult.value());

    auto yResult = deser.readF64();
    if (yResult.failed()) return yResult.error();
    f32 y = static_cast<f32>(yResult.value());

    auto zResult = deser.readF64();
    if (zResult.failed()) return zResult.error();
    f32 z = static_cast<f32>(zResult.value());

    // 读取速度（网络协议使用 f64）
    auto vxResult = deser.readF64();
    if (vxResult.failed()) return vxResult.error();
    f32 vx = static_cast<f32>(vxResult.value());

    auto vyResult = deser.readF64();
    if (vyResult.failed()) return vyResult.error();
    f32 vy = static_cast<f32>(vyResult.value());

    auto vzResult = deser.readF64();
    if (vzResult.failed()) return vzResult.error();
    f32 vz = static_cast<f32>(vzResult.value());

    // 读取旋转
    auto yawResult = deser.readF32();
    if (yawResult.failed()) return yawResult.error();

    auto pitchResult = deser.readF32();
    if (pitchResult.failed()) return pitchResult.error();

    // 读取物品堆
    auto stackResult = ItemStack::deserialize(deser);
    if (stackResult.failed()) return stackResult.error();

    auto entity = std::make_unique<ItemEntity>(id, stackResult.value(), x, y, z);
    entity->setVelocity(vx, vy, vz);
    entity->setRotation(yawResult.value(), pitchResult.value());

    // 读取额外数据
    auto ageResult = deser.readI32();
    if (ageResult.failed()) return ageResult.error();
    entity->m_age = ageResult.value();

    auto delayResult = deser.readI32();
    if (delayResult.failed()) return delayResult.error();
    entity->m_pickupDelay = delayResult.value();

    auto lifetimeResult = deser.readI32();
    if (lifetimeResult.failed()) return lifetimeResult.error();
    entity->m_lifetime = lifetimeResult.value();

    auto unpickableResult = deser.readBool();
    if (unpickableResult.failed()) return unpickableResult.error();
    entity->m_unpickable = unpickableResult.value();

    return entity;
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
            // 同步数量到数据管理器
            m_dataManager.set(DATA_ITEM_COUNT_PARAM, m_itemStack.getCount());
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
