#include "ItemEntity.hpp"
#include "../player/Player.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../physics/PhysicsConstants.hpp"
#include "../../../world/entity/EntityManager.hpp"
#include <cmath>
#include <chrono>
#include <atomic>

namespace mc {

// ============================================================================
// 静态工厂方法
// ============================================================================

std::unique_ptr<Entity> ItemEntity::create(IWorld* /*world*/) {
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
    : Entity(LegacyEntityType::Item, id)
    , m_itemStack(stack)
{
    setPosition(x, y, z);
    setRotation(0.0f, 0.0f);

    // 初始化速度（轻微随机）
    math::Random rng(static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    m_velocity.x = rng.nextFloat(-0.1f, 0.1f);
    m_velocity.y = 0.2f;  // 轻微向上
    m_velocity.z = rng.nextFloat(-0.1f, 0.1f);
}

ItemEntity::ItemEntity(EntityId id, const ItemStack& stack,
                       f32 x, f32 y, f32 z,
                       f32 vx, f32 vy, f32 vz)
    : Entity(LegacyEntityType::Item, id)
    , m_itemStack(stack)
{
    setPosition(x, y, z);
    setRotation(0.0f, 0.0f);
    setVelocity(vx, vy, vz);
}

// ============================================================================
// 物品操作
// ============================================================================

void ItemEntity::setItemStack(const ItemStack& stack) {
    m_itemStack = stack;
}

// ============================================================================
// Entity 接口
// ============================================================================

void ItemEntity::tick() {
    // 更新前保存位置
    m_prevPosition = m_position;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;

    // 增加年龄
    m_age++;

    // 检查是否过期
    if (isExpired()) {
        remove();
        return;
    }

    // 减少拾取延迟
    if (m_pickupDelay > 0) {
        m_pickupDelay--;
    }

    // 更新物理
    updatePhysics();

    // 更新存活时间
    m_ticksExisted++;

    // TODO: 更新合并检测
    // updateMerge();
}

// ============================================================================
// 玩家拾取
// ============================================================================

bool ItemEntity::onPlayerPickup(Player& player) {
    // 检查是否可以拾取
    if (!canBePickedUp()) {
        return false;
    }

    // 检查所有者限制（防止立即拾取自己丢弃的物品）
    // 如果玩家 UUID 与所有者匹配，检查拾取延迟
    // 参考 MC 1.16.5: 丢弃者需要等待 10 ticks 才能拾取自己的物品
    if (!m_ownerUuid.empty() && player.uuid() == m_ownerUuid) {
        if (m_age < DEFAULT_PICKUP_DELAY) {
            return false;
        }
    }

    // 将物品添加到玩家背包
    PlayerInventory& inventory = player.inventory();
    i32 remaining = inventory.add(m_itemStack);

    if (remaining > 0) {
        // 只拾取了部分
        m_itemStack.setCount(remaining);
        return false;
    }

    // 完全拾取，标记为移除
    remove();
    return true;
}

void ItemEntity::setOwner(const String& ownerUuid, const String& throwerUuid) {
    m_ownerUuid = ownerUuid;
    m_throwerUuid = throwerUuid;
}

// ============================================================================
// 物品合并
// ============================================================================

bool ItemEntity::tryMergeWith(ItemEntity& other) {
    if (!canMergeWith(other)) {
        return false;
    }

    // 计算合并后的数量
    i32 total = m_itemStack.getCount() + other.m_itemStack.getCount();
    i32 maxStack = m_itemStack.getMaxStackSize();

    if (total <= maxStack) {
        // 全部合并到当前实体
        m_itemStack.setCount(total);
        other.remove();
        return true;
    }

    // 部分合并
    i32 toTake = maxStack - m_itemStack.getCount();
    m_itemStack.grow(toTake);
    other.m_itemStack.shrink(toTake);
    return true;
}

bool ItemEntity::canMergeWith(const ItemEntity& other) const {
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

    return true;
}

// ============================================================================
// 物理更新
// ============================================================================

void ItemEntity::updatePhysics() {
    // TODO: 检测是否在水中或熔岩中
    // 目前使用普通物理
    applyNormalPhysics();

    // 应用速度
    if (!m_onGround || std::abs(m_velocity.x) > 0.01f ||
        std::abs(m_velocity.z) > 0.01f || m_velocity.y > 0.01f) {
        // 使用简单物理
        f32 dx = m_velocity.x;
        f32 dy = m_velocity.y;
        f32 dz = m_velocity.z;

        // 带碰撞移动
        if (m_physicsEngine) {
            Vector3 actual = moveWithCollision(dx, dy, dz);
            dx = actual.x;
            dy = actual.y;
            dz = actual.z;
        } else {
            move(dx, dy, dz);
        }

        // 碰撞后减速
        if (m_collidedHorizontally) {
            m_velocity.x *= -0.5f;
            m_velocity.z *= -0.5f;
        }

        if (m_collidedVertically) {
            if (m_velocity.y < 0.0f) {
                // 落地
                m_velocity.y = 0.0f;
                m_onGround = true;
                m_fallDistance = 0.0f;
            } else {
                // 撞到天花板
                m_velocity.y = -m_velocity.y * 0.5f;
            }
        }
    }

    // 应用阻力和重力
    applyPhysics(1.0f / 20.0f);
}

void ItemEntity::applyNormalPhysics() {
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

void ItemEntity::applyWaterPhysics() {
    // 在水中：缓慢下沉
    m_velocity.y -= SINK_SPEED;
    m_velocity.x *= 0.95f;
    m_velocity.z *= 0.95f;
}

void ItemEntity::applyLavaPhysics() {
    // 在熔岩中：漂浮并着火
    m_velocity.y += BUOYANCY;
    m_velocity.x *= 0.95f;
    m_velocity.z *= 0.95f;

    // TODO: 设置着火
    // addFlag(EntityFlags::OnFire);
}

// ============================================================================
// 序列化
// ============================================================================

void ItemEntity::serialize(network::PacketSerializer& ser) const {
    // 实体类型和ID
    ser.writeU32(static_cast<u32>(m_legacyType));
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

Result<std::unique_ptr<ItemEntity>> ItemEntity::deserialize(
    network::PacketDeserializer& deser, EntityId id) {

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

} // namespace mc
