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

#pragma once

#include "Packet.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include <array>
#include <memory>
#include <vector>

namespace mc::network {

/**
 * @brief 实体生成包
 *
 * 用于生成非生物实体（物品、经验球等）。
 */
class SpawnEntityPacket : public Packet {
public:
    SpawnEntityPacket()
        : Packet(PacketType::SpawnEntity)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // 实体ID
    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // UUID
    const std::array<u8, 16>& uuid() const { return m_uuid; }
    void setUuid(const std::array<u8, 16>& uuid) { m_uuid = uuid; }

    // 实体类型（字符串ID）
    const std::string& entityTypeId() const { return m_entityTypeId; }
    void setEntityTypeId(const std::string& typeId) { m_entityTypeId = typeId; }

    // 位置
    f32 x() const { return m_x; }
    f32 y() const { return m_y; }
    f32 z() const { return m_z; }
    void setPosition(f32 x, f32 y, f32 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    // 旋转（角度）
    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch)
    {
        m_yaw = yaw;
        m_pitch = pitch;
    }

    // 速度
    i16 velocityX() const { return m_velocityX; }
    i16 velocityY() const { return m_velocityY; }
    i16 velocityZ() const { return m_velocityZ; }
    void setVelocity(i16 vx, i16 vy, i16 vz)
    {
        m_velocityX = vx;
        m_velocityY = vy;
        m_velocityZ = vz;
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
    [[nodiscard]] const ItemStack* itemStack() const { return m_hasItemStack ? &m_itemStack : nullptr; }

    /**
     * @brief 设置 ItemStack
     * @param stack 要设置的物品堆
     */
    void setItemStack(const ItemStack& stack)
    {
        m_itemStack = stack;
        m_hasItemStack = true;
    }

private:
    u32 m_entityId = 0;
    std::array<u8, 16> m_uuid = {};
    std::string m_entityTypeId;
    f32 m_x = 0.0f;
    f32 m_y = 0.0f;
    f32 m_z = 0.0f;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    i16 m_velocityX = 0;
    i16 m_velocityY = 0;
    i16 m_velocityZ = 0;
    bool m_hasItemStack = false; // 是否包含 ItemStack 数据
    ItemStack m_itemStack;       // ItemEntity 的物品数据
};

/**
 * @brief Mob生成包
 *
 * 用于生成Mob实体（动物、怪物等）。
 */
class SpawnMobPacket : public Packet {
public:
    SpawnMobPacket()
        : Packet(PacketType::SpawnMob)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // 实体ID
    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // UUID
    const std::array<u8, 16>& uuid() const { return m_uuid; }
    void setUuid(const std::array<u8, 16>& uuid) { m_uuid = uuid; }

    // 实体类型
    const std::string& entityTypeId() const { return m_entityTypeId; }
    void setEntityTypeId(const std::string& typeId) { m_entityTypeId = typeId; }

    // 位置
    f32 x() const { return m_x; }
    f32 y() const { return m_y; }
    f32 z() const { return m_z; }
    void setPosition(f32 x, f32 y, f32 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    // 旋转（角度）
    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    f32 headYaw() const { return m_headYaw; }
    void setRotation(f32 yaw, f32 pitch, f32 headYaw)
    {
        m_yaw = yaw;
        m_pitch = pitch;
        m_headYaw = headYaw;
    }

    // 速度
    i16 velocityX() const { return m_velocityX; }
    i16 velocityY() const { return m_velocityY; }
    i16 velocityZ() const { return m_velocityZ; }
    void setVelocity(i16 vx, i16 vy, i16 vz)
    {
        m_velocityX = vx;
        m_velocityY = vy;
        m_velocityZ = vz;
    }

    // 数据参数
    const std::vector<u8>& metadata() const { return m_metadata; }
    void setMetadata(const std::vector<u8>& data) { m_metadata = data; }

private:
    u32 m_entityId = 0;
    std::array<u8, 16> m_uuid = {};
    std::string m_entityTypeId;
    f32 m_x = 0.0f;
    f32 m_y = 0.0f;
    f32 m_z = 0.0f;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    f32 m_headYaw = 0.0f;
    i16 m_velocityX = 0;
    i16 m_velocityY = 0;
    i16 m_velocityZ = 0;
    std::vector<u8> m_metadata; // 实体数据参数
};

/**
 * @brief 实体数据同步包
 *
 * 同步实体的数据参数（生命值、姿态、状态等）。
 */
class EntityMetadataPacket : public Packet {
public:
    EntityMetadataPacket()
        : Packet(PacketType::EntityMetadata)
    {}

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
 */
class EntityVelocityPacket : public Packet {
public:
    EntityVelocityPacket()
        : Packet(PacketType::EntityVelocity)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // 速度（单位：1/8000 block/tick）
    i16 velocityX() const { return m_velocityX; }
    i16 velocityY() const { return m_velocityY; }
    i16 velocityZ() const { return m_velocityZ; }
    void setVelocity(i16 vx, i16 vy, i16 vz)
    {
        m_velocityX = vx;
        m_velocityY = vy;
        m_velocityZ = vz;
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
 */
class EntityTeleportPacket : public Packet {
public:
    EntityTeleportPacket()
        : Packet(PacketType::EntityTeleport)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    f32 x() const { return m_x; }
    f32 y() const { return m_y; }
    f32 z() const { return m_z; }
    void setPosition(f32 x, f32 y, f32 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch)
    {
        m_yaw = yaw;
        m_pitch = pitch;
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
 */
class EntityDestroyPacket : public Packet {
public:
    EntityDestroyPacket()
        : Packet(PacketType::EntityDestroy)
    {}

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

    EntityAnimationPacket()
        : Packet(PacketType::EntityAnimation)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    Animation animation() const { return m_animation; }
    void setAnimation(Animation anim) { m_animation = anim; }

    /// 受伤方向角（度，相对实体朝向）。仅 TakeDamage 动画在序列化时携带，
    /// 客户端据此设置 damageTilt 的 hurtDir（ClientboundHurtAnimationPacket.yaw）。
    f32 hurtDir() const { return m_hurtDir; }
    void setHurtDir(f32 dir) { m_hurtDir = dir; }

private:
    u32 m_entityId = 0;
    Animation m_animation = Animation::SwingMainHand;
    f32 m_hurtDir = 0.0f; // 仅 TakeDamage 动画写入/读取
};

/**
 * @brief 实体相对移动包
 *
 * 同步实体的相对移动。
 */
class EntityMovePacket : public Packet {
public:
    EntityMovePacket()
        : Packet(PacketType::EntityMove)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    // 相对移动（单位：1/32 block）
    i16 deltaX() const { return m_deltaX; }
    i16 deltaY() const { return m_deltaY; }
    i16 deltaZ() const { return m_deltaZ; }
    void setDelta(i16 dx, i16 dy, i16 dz)
    {
        m_deltaX = dx;
        m_deltaY = dy;
        m_deltaZ = dz;
    }

    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch)
    {
        m_yaw = yaw;
        m_pitch = pitch;
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
 */
class EntityHeadLookPacket : public Packet {
public:
    EntityHeadLookPacket()
        : Packet(PacketType::EntityHeadLook)
    {}

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
 */
class EntityStatusPacket : public Packet {
public:
    // 状态类型
    // 数值与 MC 原版 EntityStatus byte 对应
    enum class Status : u8 {
        // 兔子跳跃（MC 1.21.11 Rabbit.jumpFromGround() 中 broadcastEntityState(this, (byte)1)）
        // 客户端收到后启动 jumpDuration 计时器，用于 RabbitModel 计算 jumpRotation
        RabbitJump = 1,

        // 通用状态
        Hurt = 2,            // 受击反馈（红色闪烁）
        Death = 3,           // 死亡效果
        TamingFailed = 6,    // 驯服失败（烟雾粒子）
        TamingSucceeded = 7, // 驯服成功（爱心粒子）
        ShakeOffWater = 8,   // 抖落水分（狼）- 开始甩水动画

        // 狼甩水取消（MC 1.21.11 Wolf.tick() 中再次入水时广播 byte 56）
        // 客户端收到后立即取消甩水动画，重置 shakeAnim
        WolfStopShaking = 56,

        // 实体特定状态
        EatBlock = 10, // 吃草/方块动画（羊、 TNT 矿车引燃）

        // 村民状态
        VillagerHeart = 12,  // 村民爱心粒子（繁殖中/幼年村民出生）
        VillagerAngry = 13,  // 村民愤怒粒子（无床位/被玩家攻击）
        VillagerHappy = 14,  // 村民开心粒子（交易成功/获取职业/找到床位/找到集会点）
        VillagerSplash = 42, // 村民水花粒子（突袭中恐慌）

        // 动物状态
        LoveHeart = 18, // 繁殖爱心效果

        // 玩家状态
        // 玩家权限等级变更（status byte = 24 + permissionLevel，permissionLevel 范围 0-4）
        PermissionLevel0 = 24, // 普通玩家（非 OP）
        PermissionLevel1 = 25, // 版主（可绕过重生点保护）
        PermissionLevel2 = 26, // 游戏管理员（可使用命令方块等）
        PermissionLevel3 = 27, // 服务器管理员（可使用 /op、/deop 等）
        PermissionLevel4 = 28, // 服务器所有者（控制台级别权限）

        // 特殊状态
        FireworkExplosion = 17,
        GuardianAttack = 21,       // 守卫者攻击音效
        ArrowHit = 30,             // 箭矢命中音效
        TotemActivate = 35,        // 不死图腾激活
        Dolphin = 38,              // 海豚寻宝粒子
        OcelotTrustFailed = 40,    // 豹猫信任失败（烟雾粒子）
        OcelotTrustSucceeded = 41, // 豹猫信任成功（心形粒子）
        TeleportParticles = 46,    // 传送粒子效果

        // 铁傀儡状态
        IronGolemAttack = 4,    // 铁傀儡攻击动画（举臂）+ 播放攻击音效
        IronGolemHoldRose = 11, // 铁儡开始手持罂粟花
        IronGolemStopRose = 34, // 铁傀儡停止手持罂粟花

        // 疣猪兽/僵尸疣兽状态（MC 原版中铁傀儡和疣猪兽共用状态码 4，客户端按实体类型区分）
        HoglinAttack = 4, // 疣猪兽/僵尸疣兽攻击动画（甩头）+ 播放攻击音效

        // 装备破损状态
        EquipmentBreakMainHand = 47, // 主手装备破损动画 + 音效
        EquipmentBreakOffHand = 48,  // 副手装备破损动画 + 音效
        EquipmentBreakHead = 49,     // 头盔破损动画 + 音效
        EquipmentBreakChest = 50,    // 胸甲破损动画 + 音效
        EquipmentBreakLegs = 51,     // 护腿破损动画 + 音效
        EquipmentBreakFeet = 52,     // 靴子破损动画 + 音效

        // Mob 特定状态
        MobPoof = 60, // 生物变形/消失烟雾粒子
    };

    /**
     * @brief 根据权限等级生成对应的状态字节
     * @param level 权限等级 (0-4)
     * @return 状态枚举值 (PermissionLevel0 ~ PermissionLevel4)
     */
    [[nodiscard]] static Status permissionLevel(i32 level)
    {
        MC_ASSERT_RELEASE(level >= 0 && level <= 4);
        return static_cast<Status>(24 + level);
    }

    /**
     * @brief 从状态字节解析权限等级
     * @param status 状态字节
     * @return 权限等级 (0-4)，如果不是权限等级状态则返回 -1
     */
    [[nodiscard]] static i32 toPermissionLevel(u8 status)
    {
        if (status >= 24 && status <= 28) {
            return static_cast<i32>(status - 24);
        }
        return -1;
    }

    /**
     * @brief 根据装备槽位索引获取对应的破损状态码
     *
     * 对应 MC 原版 LivingEntity.entityEventForEquipmentBreak()
     * 槽位索引与 EquipmentSlot 枚举值对应：
     * 0=MainHand→47, 1=OffHand→48, 2=Feet→52, 3=Legs→51, 4=Chest→50, 5=Head→49
     *
     * @param slotIndex 装备槽位索引（EquipmentSlot 枚举值）
     * @return 破损状态码
     */
    [[nodiscard]] static Status equipmentBreakStatus(u8 slotIndex)
    {
        // 槽位索引与 MC 原版 entityEventForEquipmentBreak 映射
        // EquipmentSlot: MainHand=0, OffHand=1, Feet=2, Legs=3, Chest=4, Head=5
        // EntityStatus:  47=MainHand, 48=OffHand, 49=Head, 50=Chest, 51=Legs, 52=Feet
        switch (slotIndex) {
            case 0:
                return Status::EquipmentBreakMainHand;
            case 1:
                return Status::EquipmentBreakOffHand;
            case 5:
                return Status::EquipmentBreakHead;
            case 4:
                return Status::EquipmentBreakChest;
            case 3:
                return Status::EquipmentBreakLegs;
            case 2:
                return Status::EquipmentBreakFeet;
            default:
                return Status::EquipmentBreakMainHand;
        }
    }

    EntityStatusPacket()
        : Packet(PacketType::EntityStatus)
    {}

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
 */
class CollectItemPacket : public Packet {
public:
    CollectItemPacket()
        : Packet(PacketType::CollectItem)
    {}

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
    u32 m_collectedEntityId = 0; // 被拾取的物品实体ID
    u32 m_collectorEntityId = 0; // 拾取者（玩家）实体ID
    i32 m_pickupItemCount = 1;   // 拾取物品数量
};

// ============================================================================
// 玩家输入包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 玩家输入包
 *
 * 客户端发送玩家的移动输入给服务端，用于骑乘控制。
 * 包含前后左右移动、跳跃和潜行状态。
 */
class PlayerInputPacket : public Packet {
public:
    PlayerInputPacket()
        : Packet(PacketType::PlayerInput)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    /**
     * @brief 获取左右移动速度
     * @return 正值表示向左，负值表示向右
     */
    f32 strafeSpeed() const { return m_strafeSpeed; }
    void setStrafeSpeed(f32 speed) { m_strafeSpeed = speed; }

    /**
     * @brief 获取前后移动速度
     * @return 正值表示前进，负值表示后退
     */
    f32 forwardSpeed() const { return m_forwardSpeed; }
    void setForwardSpeed(f32 speed) { m_forwardSpeed = speed; }

    /**
     * @brief 是否正在跳跃
     */
    bool isJumping() const { return m_jumping; }
    void setJumping(bool jumping) { m_jumping = jumping; }

    /**
     * @brief 是否正在潜行
     */
    bool isSneaking() const { return m_sneaking; }
    void setSneaking(bool sneaking) { m_sneaking = sneaking; }

private:
    f32 m_strafeSpeed = 0.0f;  // 左右移动 (正值=左, 负值=右)
    f32 m_forwardSpeed = 0.0f; // 前后移动 (正值=前, 负值=后)
    bool m_jumping = false;    // 跳跃状态
    bool m_sneaking = false;   // 潜行状态
};

// ============================================================================
// 船划桨状态包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 船划桨状态包
 *
 * 客户端发送船的划桨状态给服务端。
 * 仅包含两个布尔值：左桨是否划动、右桨是否划动。
 */
class SteerBoatPacket : public Packet {
public:
    SteerBoatPacket()
        : Packet(PacketType::SteerBoat)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    /**
     * @brief 左桨是否正在划动
     */
    bool leftPaddle() const { return m_leftPaddle; }
    void setLeftPaddle(bool left) { m_leftPaddle = left; }

    /**
     * @brief 右桨是否正在划动
     */
    bool rightPaddle() const { return m_rightPaddle; }
    void setRightPaddle(bool right) { m_rightPaddle = right; }

    /**
     * @brief 设置两个桨的状态
     */
    void setPaddleState(bool left, bool right)
    {
        m_leftPaddle = left;
        m_rightPaddle = right;
    }

private:
    bool m_leftPaddle = false;  // 左桨状态
    bool m_rightPaddle = false; // 右桨状态
};

// ============================================================================
// 载具移动包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 载具移动包
 *
 * 客户端发送载具（马、船、矿车等）的位置和旋转给服务端。
 * 当玩家骑乘载具时，客户端每tick发送此包同步载具位置。
 */
class MoveVehiclePacket : public Packet {
public:
    MoveVehiclePacket()
        : Packet(PacketType::MoveVehicle)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // 位置
    f64 x() const { return m_x; }
    f64 y() const { return m_y; }
    f64 z() const { return m_z; }
    void setPosition(f64 x, f64 y, f64 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    // 旋转（角度）
    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch)
    {
        m_yaw = yaw;
        m_pitch = pitch;
    }

private:
    f64 m_x = 0.0;
    f64 m_y = 0.0;
    f64 m_z = 0.0;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
};

// ============================================================================
// 载具移动同步包 (服务端 -> 客户端)
// ============================================================================

/**
 * @brief 载具移动同步包
 *
 * 服务端向客户端同步载具的位置和旋转。
 * 当服务端校正载具位置时发送此包。
 */
class VehicleMovePacket : public Packet {
public:
    VehicleMovePacket()
        : Packet(PacketType::VehicleMove)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // 位置
    f64 x() const { return m_x; }
    f64 y() const { return m_y; }
    f64 z() const { return m_z; }
    void setPosition(f64 x, f64 y, f64 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    // 旋转（角度）
    f32 yaw() const { return m_yaw; }
    f32 pitch() const { return m_pitch; }
    void setRotation(f32 yaw, f32 pitch)
    {
        m_yaw = yaw;
        m_pitch = pitch;
    }

private:
    f64 m_x = 0.0;
    f64 m_y = 0.0;
    f64 m_z = 0.0;
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
};

// ============================================================================
// 实体动作包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 实体动作类型枚举
 */
enum class EntityActionType : i32 {
    PressShiftKey = 0,   // 按下潜行键
    ReleaseShiftKey = 1, // 释放潜行键
    StopSleeping = 2,    // 停止睡觉
    StartSprinting = 3,  // 开始疾跑
    StopSprinting = 4,   // 停止疾跑
    StartRidingJump = 5, // 开始骑乘跳跃（马跳跃蓄力）
    StopRidingJump = 6,  // 停止骑乘跳跃（马跳跃释放）
    OpenInventory = 7,   // 打开背包
    StartFallFlying = 8  // 开始滑翔（鞘翅）
};

/**
 * @brief 实体动作包
 *
 * 客户端发送实体动作给服务端。
 * 用于潜行、疾跑、马跳跃蓄力等动作。
 */
class EntityActionPacket : public Packet {
public:
    EntityActionPacket()
        : Packet(PacketType::EntityAction)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    /**
     * @brief 获取实体ID
     */
    u32 entityId() const { return m_entityId; }
    void setEntityId(u32 id) { m_entityId = id; }

    /**
     * @brief 获取动作类型
     */
    EntityActionType action() const { return m_action; }
    void setAction(EntityActionType action) { m_action = action; }

    /**
     * @brief 获取辅助数据
     *
     * 对于 StartRidingJump，表示跳跃力度 (0-100)
     */
    i32 auxData() const { return m_auxData; }
    void setAuxData(i32 data) { m_auxData = data; }

private:
    u32 m_entityId = 0;
    EntityActionType m_action = EntityActionType::PressShiftKey;
    i32 m_auxData = 0;
};

// ============================================================================
// 实体交互包 (客户端 -> 服务端)
// ============================================================================

/**
 * @brief 实体交互类型枚举
 */
enum class UseEntityAction : u8 {
    Interact = 0,  // 右键交互（不指定位置）
    Attack = 1,    // 左键攻击
    InteractAt = 2 // 右键交互（指定具体位置）
};

/**
 * @brief 实体交互包
 *
 * 客户端发送玩家对实体的交互请求（攻击、右键交互）。
 * 服务端收到后调用 Player::interactOn() 或 Player::attack()。
 */
class UseEntityPacket : public Packet {
public:
    UseEntityPacket()
        : Packet(PacketType::UseEntity)
    {}

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;

    // Getters
    u32 entityId() const { return m_entityId; }
    UseEntityAction action() const { return m_action; }
    Hand hand() const { return m_hand; }
    f32 hitX() const { return m_hitX; }
    f32 hitY() const { return m_hitY; }
    f32 hitZ() const { return m_hitZ; }
    bool isSneaking() const { return m_isSneaking; }

    // Setters
    void setEntityId(u32 id) { m_entityId = id; }
    void setAction(UseEntityAction action) { m_action = action; }
    void setHand(Hand hand) { m_hand = hand; }
    void setHitPosition(f32 x, f32 y, f32 z)
    {
        m_hitX = x;
        m_hitY = y;
        m_hitZ = z;
    }
    void setSneaking(bool sneaking) { m_isSneaking = sneaking; }

private:
    u32 m_entityId = 0;
    UseEntityAction m_action = UseEntityAction::Interact;
    Hand m_hand = Hand::MainHand;
    f32 m_hitX = 0.0f;
    f32 m_hitY = 0.0f;
    f32 m_hitZ = 0.0f;
    bool m_isSneaking = false;
};

} // namespace mc::network
