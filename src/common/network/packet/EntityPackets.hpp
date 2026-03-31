#pragma once

#include "Packet.hpp"
#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../item/core/ItemStack.hpp"
#include <vector>
#include <array>
#include <memory>

namespace mc::network {

/**
 * @brief 实体生成包
 *
 * 用于生成非生物实体（物品、经验球等）。
 *
 * 参考 MC 1.16.5 SpawnObjectPacket
 */
class SpawnEntityPacket : public Packet {
public:
    SpawnEntityPacket() : Packet(PacketType::SpawnEntity) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // 实体ID
    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // UUID
    const std::array<u8, 16>& uuid() const { return m_uuid; }
    void setUuid(const std::array<u8, 16>& uuid) { m_uuid = uuid; }

    // 实体类型（字符串ID）
    const String& entityTypeId() const { return m_entityTypeId; }
    void setEntityTypeId(const String& typeId) { m_entityTypeId = typeId; }

    // 位置
    f32 x() const { return m_x; }
    f32 y() const { return m_y; }
    f32 z() const { return m_z; }
    void setPosition(f32 x, f32 y, f32 z) {
        m_x = x; m_y = y; m_z = z;
    }

    // 旋转（角度）
    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch) {
        m_yaw = yaw; m_pitch = pitch;
    }

    // 速度
    i16 velocityX() const { return m_velocityX; }
    i16 velocityY() const { return m_velocityY; }
    i16 velocityZ() const { return m_velocityZ; }
    void setVelocity(i16 vx, i16 vy, i16 vz) {
        m_velocityX = vx; m_velocityY = vy; m_velocityZ = vz;
    }

    // ========== ItemStack 支持（用于 ItemEntity） ==========

    /**
     * @brief 是否包含 ItemStack 数据
     * 仅当实体类型为 minecraft:item 时有效
     */
    [[nodiscard]] bool hasItemStack() const { return m_hasItemStack; }

    /**
     * @brief 获取 ItemStack
     * @return ItemStack 指针，如果没有则返回 nullptr
     */
    [[nodiscard]] const ItemStack* itemStack() const {
        return m_hasItemStack ? &m_itemStack : nullptr;
    }

    /**
     * @brief 设置 ItemStack
     * @param stack 要设置的物品堆
     */
    void setItemStack(const ItemStack& stack) {
        m_itemStack = stack;
        m_hasItemStack = true;
    }

private:
    u32 m_entityId = 0;
    std::array<u8, 16> m_uuid = {};
    String m_entityTypeId;
    f32 m_x = 0.0f;
    f32 m_y = 0.0f;
    f32 m_z = 0.0f;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    i16 m_velocityX = 0;
    i16 m_velocityY = 0;
    i16 m_velocityZ = 0;
    bool m_hasItemStack = false;     // 是否包含 ItemStack 数据
    ItemStack m_itemStack;           // ItemEntity 的物品数据
};

/**
 * @brief Mob生成包
 *
 * 用于生成Mob实体（动物、怪物等）。
 *
 * 参考 MC 1.16.5 SpawnMobPacket
 */
class SpawnMobPacket : public Packet {
public:
    SpawnMobPacket() : Packet(PacketType::SpawnMob) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // 实体ID
    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // UUID
    const std::array<u8, 16>& uuid() const { return m_uuid; }
    void setUuid(const std::array<u8, 16>& uuid) { m_uuid = uuid; }

    // 实体类型
    const String& entityTypeId() const { return m_entityTypeId; }
    void setEntityTypeId(const String& typeId) { m_entityTypeId = typeId; }

    // 位置
    f32 x() const { return m_x; }
    f32 y() const { return m_y; }
    f32 z() const { return m_z; }
    void setPosition(f32 x, f32 y, f32 z) {
        m_x = x; m_y = y; m_z = z;
    }

    // 旋转（角度）
    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    f32 headYaw() const { return m_headYaw; }
    void setRotation(f32 yaw, f32 pitch, f32 headYaw) {
        m_yaw = yaw; m_pitch = pitch; m_headYaw = headYaw;
    }

    // 速度
    i16 velocityX() const { return m_velocityX; }
    i16 velocityY() const { return m_velocityY; }
    i16 velocityZ() const { return m_velocityZ; }
    void setVelocity(i16 vx, i16 vy, i16 vz) {
        m_velocityX = vx; m_velocityY = vy; m_velocityZ = vz;
    }

    // 数据参数
    const std::vector<u8>& metadata() const { return m_metadata; }
    void setMetadata(const std::vector<u8>& data) { m_metadata = data; }

private:
    u32 m_entityId = 0;
    std::array<u8, 16> m_uuid = {};
    String m_entityTypeId;
    f32 m_x = 0.0f;
    f32 m_y = 0.0f;
    f32 m_z = 0.0f;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    f32 m_headYaw = 0.0f;
    i16 m_velocityX = 0;
    i16 m_velocityY = 0;
    i16 m_velocityZ = 0;
    std::vector<u8> m_metadata;   // 实体数据参数
};

/**
 * @brief 实体数据同步包
 *
 * 同步实体的数据参数（生命值、姿态、状态等）。
 *
 * 参考 MC 1.16.5 EntityMetadataPacket
 */
class EntityMetadataPacket : public Packet {
public:
    EntityMetadataPacket() : Packet(PacketType::EntityMetadata) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    const std::vector<u8>& metadata() const { return m_metadata; }
    void setMetadata(const std::vector<u8>& data) { m_metadata = data; }

private:
    u32 m_entityId = 0;
    std::vector<u8> m_metadata;
};

/**
 * @brief 实体速度包
 *
 * 同步实体的运动速度。
 *
 * 参考 MC 1.16.5 EntityVelocityPacket
 */
class EntityVelocityPacket : public Packet {
public:
    EntityVelocityPacket() : Packet(PacketType::EntityVelocity) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // 速度（单位：1/8000 block/tick）
    i16 velocityX() const { return m_velocityX; }
    i16 velocityY() const { return m_velocityY; }
    i16 velocityZ() const { return m_velocityZ; }
    void setVelocity(i16 vx, i16 vy, i16 vz) {
        m_velocityX = vx; m_velocityY = vy; m_velocityZ = vz;
    }

private:
    u32 m_entityId = 0;
    i16 m_velocityX = 0;
    i16 m_velocityY = 0;
    i16 m_velocityZ = 0;
};

/**
 * @brief 实体传送包
 *
 * 传送实体到指定位置。
 *
 * 参考 MC 1.16.5 EntityTeleportPacket
 */
class EntityTeleportPacket : public Packet {
public:
    EntityTeleportPacket() : Packet(PacketType::EntityTeleport) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    f32 x() const { return m_x; }
    f32 y() const { return m_y; }
    f32 z() const { return m_z; }
    void setPosition(f32 x, f32 y, f32 z) {
        m_x = x; m_y = y; m_z = z;
    }

    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch) {
        m_yaw = yaw; m_pitch = pitch;
    }

    bool onGround() const { return m_onGround; }
    void setOnGround(bool ground) { m_onGround = ground; }

private:
    u32 m_entityId = 0;
    f32 m_x = 0.0f;
    f32 m_y = 0.0f;
    f32 m_z = 0.0f;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    bool m_onGround = false;
};

/**
 * @brief 实体销毁包
 *
 * 通知客户端销毁指定实体。
 *
 * 参考 MC 1.16.5 DestroyEntitiesPacket
 */
class EntityDestroyPacket : public Packet {
public:
    EntityDestroyPacket() : Packet(PacketType::EntityDestroy) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    const std::vector<u32>& entityIds() const { return m_entityIds; }
    void setEntityIds(const std::vector<u32>& ids) { m_entityIds = ids; }
    void addEntityId(u32 id) { m_entityIds.push_back(id); }

private:
    std::vector<u32> m_entityIds;
};

/**
 * @brief 实体动画包
 *
 * 播放实体动画（挥手、受伤、起床等）。
 *
 * 参考 MC 1.16.5 EntityAnimationPacket
 */
class EntityAnimationPacket : public Packet {
public:
    // 动画类型
    enum class Animation : u8 {
        SwingMainHand = 0,
        TakeDamage = 1,
        LeaveBed = 2,
        SwingOffHand = 3,
        CriticalEffect = 4,
        MagicCriticalEffect = 5
    };

    EntityAnimationPacket() : Packet(PacketType::EntityAnimation) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    Animation animation() const { return m_animation; }
    void setAnimation(Animation anim) { m_animation = anim; }

private:
    u32 m_entityId = 0;
    Animation m_animation = Animation::SwingMainHand;
};

/**
 * @brief 实体相对移动包
 *
 * 同步实体的相对移动。
 *
 * 参考 MC 1.16.5 EntityMovePacket
 */
class EntityMovePacket : public Packet {
public:
    EntityMovePacket() : Packet(PacketType::EntityMove) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // 相对移动（单位：1/32 block）
    i16 deltaX() const { return m_deltaX; }
    i16 deltaY() const { return m_deltaY; }
    i16 deltaZ() const { return m_deltaZ; }
    void setDelta(i16 dx, i16 dy, i16 dz) {
        m_deltaX = dx; m_deltaY = dy; m_deltaZ = dz;
    }

    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch) {
        m_yaw = yaw; m_pitch = pitch;
    }

    bool onGround() const { return m_onGround; }
    void setOnGround(bool ground) { m_onGround = ground; }

private:
    u32 m_entityId = 0;
    i16 m_deltaX = 0;
    i16 m_deltaY = 0;
    i16 m_deltaZ = 0;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    bool m_onGround = false;
};

/**
 * @brief 实体头部朝向包
 *
 * 同步实体的头部朝向。
 *
 * 参考 MC 1.16.5 EntityHeadLookPacket
 */
class EntityHeadLookPacket : public Packet {
public:
    EntityHeadLookPacket() : Packet(PacketType::EntityHeadLook) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    f32 headYaw() const { return m_headYaw; }
    void setHeadYaw(f32 yaw) { m_headYaw = yaw; }

private:
    u32 m_entityId = 0;
    f32 m_headYaw = 0.0f;
};

/**
 * @brief 实体状态包
 *
 * 通知客户端实体的状态变化（受伤、死亡等）。
 *
 * 参考 MC 1.16.5 EntityStatusPacket
 */
class EntityStatusPacket : public Packet {
public:
    // 状态类型
    enum class Status : u8 {
        // 通用状态
        Hurt = 2,
        Death = 3,
        TamingFailed = 6,
        TamingSucceeded = 7,
        ShakeOffWater = 8,

        // 动物状态
        LoveHeart = 18,         // 繁殖爱心效果

        // 玩家状态
        PermissionLevelChange = 26,
        ReduceDebugInfo = 27,
        HideParticles = 28,

        // 特殊状态
        FireworkExplosion = 17,
        ArrowHit = 30,
        TotemActivate = 35,

        // 动物特定状态
        SheepEatGrass = 45,     // 羊吃草
        ChickenLayEgg = 47,     // 鸡下蛋

        // 守卫者/潜影贝攻击动画
        GuardianAttack = 21,
        ShulkerOpen = 46
    };

    EntityStatusPacket() : Packet(PacketType::EntityStatus) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    Status status() const { return m_status; }
    void setStatus(Status status) { m_status = status; }

private:
    u32 m_entityId = 0;
    Status m_status = Status::Hurt;
};

/**
 * @brief 物品拾取动画包
 *
 * 通知客户端播放物品拾取动画（物品飞向玩家）。
 * 客户端收到此包后：
 * 1. 播放物品飞向玩家的动画
 * 2. 播放拾取音效
 * 3. 从世界中移除物品实体
 *
 * 参考 MC 1.16.5 CollectItemPacket
 */
class CollectItemPacket : public Packet {
public:
    CollectItemPacket() : Packet(PacketType::CollectItem) {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    /**
     * @brief 获取被拾取的实体ID
     */
    u32 collectedEntityId() const { return m_collectedEntityId; }
    void setCollectedEntityId(u32 id) { m_collectedEntityId = id; }

    /**
     * @brief 获取拾取者实体ID
     */
    u32 collectorEntityId() const { return m_collectorEntityId; }
    void setCollectorEntityId(u32 id) { m_collectorEntityId = id; }

    /**
     * @brief 获取拾取物品数量
     * MC 1.16.5+: 此字段用于显示拾取的物品数量
     */
    i32 pickupItemCount() const { return m_pickupItemCount; }
    void setPickupItemCount(i32 count) { m_pickupItemCount = count; }

private:
    u32 m_collectedEntityId = 0;  // 被拾取的物品实体ID
    u32 m_collectorEntityId = 0;  // 拾取者（玩家）实体ID
    i32 m_pickupItemCount = 1;     // 拾取物品数量
};

} // namespace mc::network
