#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/item/core/ItemStack.hpp"
#include <string>
#include <memory>
#include <vector>

namespace mc::client {

/**
 * @brief 客户端实体代理类
 *
 * 存储客户端实体的渲染相关信息，包括位置插值、动画状态等。
 * 与服务端Entity类不同，这个类专注于渲染需求。
 *
 * 关键特性：
 * - 位置和旋转的平滑插值
 * - 动画状态跟踪（limbSwing等）
 * - 元数据缓存
 *
 * 参考 MC 1.16.5 客户端实体渲染
 */
class ClientEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param typeId 实体类型标识符（如 "pig", "cow"）
     */
    ClientEntity(EntityId id, const String& typeId);
    ~ClientEntity() = default;

    // 禁止拷贝
    ClientEntity(const ClientEntity&) = delete;
    ClientEntity& operator=(const ClientEntity&) = delete;

    // 允许移动
    ClientEntity(ClientEntity&&) = default;
    ClientEntity& operator=(ClientEntity&&) = default;

    // ========== 基本信息 ==========

    [[nodiscard]] EntityId id() const { return m_id; }
    [[nodiscard]] const String& typeId() const { return m_typeId; }
    [[nodiscard]] const String& uuid() const { return m_uuid; }
    void setUuid(const String& uuid) { m_uuid = uuid; }

    // ========== 位置插值配置 ==========

    /**
     * @brief 设置位置插值速度
     * @param speed 插值速度 (0.0-1.0)，越大越快
     */
    void setInterpolationSpeed(f32 speed);

    /**
     * @brief 获取位置插值速度
     */
    [[nodiscard]] f32 interpolationSpeed() const { return m_interpolationSpeed; }

    /**
     * @brief 启用/禁用平滑插值
     */
    void setSmoothInterpolation(bool enabled) { m_smoothInterpolation = enabled; }
    [[nodiscard]] bool smoothInterpolationEnabled() const { return m_smoothInterpolation; }

    // ========== 位置 ==========

    [[nodiscard]] Vector3 position() const { return m_position; }
    [[nodiscard]] f32 x() const { return m_position.x; }
    [[nodiscard]] f32 y() const { return m_position.y; }
    [[nodiscard]] f32 z() const { return m_position.z; }

    // 上一帧位置（用于插值）
    [[nodiscard]] Vector3 prevPosition() const { return m_prevPosition; }
    [[nodiscard]] f32 prevX() const { return m_prevPosition.x; }
    [[nodiscard]] f32 prevY() const { return m_prevPosition.y; }
    [[nodiscard]] f32 prevZ() const { return m_prevPosition.z; }

    // 目标位置（从网络包接收）
    [[nodiscard]] Vector3 targetPosition() const { return m_targetPosition; }

    /**
     * @brief 设置实体位置（立即传送）
     */
    void setPosition(f32 x, f32 y, f32 z);

    /**
     * @brief 设置目标位置（用于插值）
     */
    void setTargetPosition(f32 x, f32 y, f32 z);

    /**
     * @brief 更新位置（每tick调用）
     * 对目标位置进行平滑插值
     */
    void tickPosition();

    /**
     * @brief 计算插值位置
     * @param partialTick 部分 tick (0.0-1.0)
     * @return 插值后的位置
     */
    [[nodiscard]] Vector3 getInterpolatedPosition(f32 partialTick) const;

    // ========== 旋转 ==========

    [[nodiscard]] f32 yaw() const { return m_yaw; }
    [[nodiscard]] f32 pitch() const { return m_pitch; }
    [[nodiscard]] f32 prevYaw() const { return m_prevYaw; }
    [[nodiscard]] f32 prevPitch() const { return m_prevPitch; }

    // 头部朝向（用于动物渲染）
    [[nodiscard]] f32 headYaw() const { return m_headYaw; }
    [[nodiscard]] f32 prevHeadYaw() const { return m_prevHeadYaw; }

    // 目标旋转（用于平滑插值）
    [[nodiscard]] f32 targetYaw() const { return m_targetYaw; }
    [[nodiscard]] f32 targetPitch() const { return m_targetPitch; }
    [[nodiscard]] f32 targetHeadYaw() const { return m_targetHeadYaw; }

    /**
     * @brief 设置旋转（立即设置）
     */
    void setRotation(f32 yaw, f32 pitch);

    /**
     * @brief 设置目标旋转（用于插值）
     */
    void setTargetRotation(f32 yaw, f32 pitch);

    /**
     * @brief 设置头部旋转
     */
    void setHeadRotation(f32 headYaw);

    /**
     * @brief 设置目标头部旋转（用于插值）
     */
    void setTargetHeadRotation(f32 headYaw);

    /**
     * @brief 更新旋转（每tick调用）
     */
    void tickRotation();

    /**
     * @brief 计算插值后的yaw
     */
    [[nodiscard]] f32 getInterpolatedYaw(f32 partialTick) const;

    /**
     * @brief 计算插值后的pitch
     */
    [[nodiscard]] f32 getInterpolatedPitch(f32 partialTick) const;

    /**
     * @brief 计算插值后的头部yaw
     */
    [[nodiscard]] f32 getInterpolatedHeadYaw(f32 partialTick) const;

    // ========== 速度 ==========

    [[nodiscard]] Vector3 velocity() const { return m_velocity; }
    void setVelocity(f32 x, f32 y, f32 z);

    // ========== 动画状态 ==========

    /**
     * @brief 获取上一帧腿部摆动进度
     */
    [[nodiscard]] f32 prevLimbSwing() const { return m_prevLimbSwing; }

    /**
     * @brief 获取腿部摆动进度
     * 用于行走动画，范围 0 到 2π
     */
    [[nodiscard]] f32 limbSwing() const { return m_limbSwing; }

    /**
     * @brief 获取上一帧腿部摆动强度
     */
    [[nodiscard]] f32 prevLimbSwingAmount() const { return m_prevLimbSwingAmount; }

    /**
     * @brief 获取腿部摆动强度
     * 表示移动速度，0表示静止，越大表示移动越快
     */
    [[nodiscard]] f32 limbSwingAmount() const { return m_limbSwingAmount; }

    /**
     * @brief 更新动画状态
     * @param distanceMoved 移动距离
     */
    void updateAnimation(f32 distanceMoved);

    // ========== 身体朝向（用于渲染） ==========

    /**
     * @brief 获取渲染用的身体偏航角
     */
    [[nodiscard]] f32 renderYawOffset() const { return m_yaw; }

    /**
     * @brief 获取上一帧渲染用的身体偏航角
     */
    [[nodiscard]] f32 prevRenderYawOffset() const { return m_prevYaw; }

    /**
     * @brief 获取头部偏航角
     */
    [[nodiscard]] f32 rotationYawHead() const { return m_headYaw; }

    /**
     * @brief 获取上一帧头部偏航角
     */
    [[nodiscard]] f32 prevRotationYawHead() const { return m_prevHeadYaw; }

    // ========== 追踪位置系统（用于披风摆动，参考 MC 1.16.5 PlayerEntity） ==========

    /**
     * @brief 获取追踪位置 X
     * 用于计算披风摆动角度
     */
    [[nodiscard]] f64 chasingPosX() const { return m_chasingPosX; }

    /**
     * @brief 获取追踪位置 Y
     */
    [[nodiscard]] f64 chasingPosY() const { return m_chasingPosY; }

    /**
     * @brief 获取追踪位置 Z
     */
    [[nodiscard]] f64 chasingPosZ() const { return m_chasingPosZ; }

    /**
     * @brief 获取上一帧追踪位置 X
     */
    [[nodiscard]] f64 prevChasingPosX() const { return m_prevChasingPosX; }

    /**
     * @brief 获取上一帧追踪位置 Y
     */
    [[nodiscard]] f64 prevChasingPosY() const { return m_prevChasingPosY; }

    /**
     * @brief 获取上一帧追踪位置 Z
     */
    [[nodiscard]] f64 prevChasingPosZ() const { return m_prevChasingPosZ; }

    // ========== 相机偏航角系统（用于披风摆动，参考 MC 1.16.5 PlayerEntity） ==========

    /**
     * @brief 获取相机偏航角
     * 用于计算披风摆动强度
     */
    [[nodiscard]] f32 cameraYaw() const { return m_cameraYaw; }

    /**
     * @brief 获取上一帧相机偏航角
     */
    [[nodiscard]] f32 prevCameraYaw() const { return m_prevCameraYaw; }

    // ========== 鞘翅角度系统（用于鞘翅展开动画，参考 MC 1.16.5 AbstractClientPlayerEntity） ==========

    /**
     * @brief 获取鞘翅 X 轴旋转角度
     */
    [[nodiscard]] f32 rotateElytraX() const { return m_rotateElytraX; }

    /**
     * @brief 获取鞘翅 Y 轴旋转角度
     */
    [[nodiscard]] f32 rotateElytraY() const { return m_rotateElytraY; }

    /**
     * @brief 获取鞘翅 Z 轴旋转角度
     */
    [[nodiscard]] f32 rotateElytraZ() const { return m_rotateElytraZ; }

    /**
     * @brief 更新鞘翅角度（每tick调用）
     * @param targetX 目标 X 轴角度
     * @param targetY 目标 Y 轴角度
     * @param targetZ 目标 Z 轴角度
     */
    void updateElytraAngles(f32 targetX, f32 targetY, f32 targetZ);

    // ========== 悬浮起始偏移（用于 ItemEntity，参考 MC 1.16.5 ItemEntity） ==========

    /**
     * @brief 获取悬浮起始偏移
     * 用于物品实体的浮动动画随机化
     */
    [[nodiscard]] f32 hoverStart() const { return m_hoverStart; }

    /**
     * @brief 设置悬浮起始偏移
     */
    void setHoverStart(f32 hoverStart) { m_hoverStart = hoverStart; }

    // ========== 状态标志 ==========

    [[nodiscard]] bool onGround() const { return m_onGround; }
    void setOnGround(bool onGround) { m_onGround = onGround; }

    [[nodiscard]] bool isRemoved() const { return m_removed; }
    void remove() { m_removed = true; }

    /**
     * @brief 检查是否存活（未移除）
     */
    [[nodiscard]] bool isAlive() const { return !m_removed; }

    // ========== 实体尺寸 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const { return m_width; }
    void setWidth(f32 width) { m_width = width; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const { return m_height; }
    void setHeight(f32 height) { m_height = height; }

    // ========== 年龄（用于幼年动物渲染） ==========

    /**
     * @brief 是否是幼年个体
     */
    [[nodiscard]] bool isChild() const { return m_child; }
    void setChild(bool child) { m_child = child; }

    // ========== 受伤和死亡状态 ==========

    /**
     * @brief 获取受伤时间
     * 用于渲染受伤闪烁效果，范围 0-10
     */
    [[nodiscard]] i32 hurtTime() const { return m_hurtTime; }
    void setHurtTime(i32 time) { m_hurtTime = time; }

    /**
     * @brief 获取死亡时间
     * 用于渲染死亡淡出效果
     */
    [[nodiscard]] i32 deathTime() const { return m_deathTime; }
    void setDeathTime(i32 time) { m_deathTime = time; }

    // ========== 行为状态 ==========

    /**
     * @brief 是否正在蹲伏
     */
    [[nodiscard]] bool isSneaking() const { return m_sneaking; }
    void setSneaking(bool sneaking) { m_sneaking = sneaking; }

    /**
     * @brief 是否正在游泳
     */
    [[nodiscard]] bool isSwimming() const { return m_swimming; }
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 是否正在乘坐载具
     */
    [[nodiscard]] bool isRiding() const { return m_riding; }
    void setRiding(bool riding) { m_riding = riding; }

    /**
     * @brief 是否正在坐下（用于动物）
     */
    [[nodiscard]] bool isSitting() const { return m_sitting; }
    void setSitting(bool sitting) { m_sitting = sitting; }

    /**
     * @brief 是否正在燃烧
     */
    [[nodiscard]] bool isOnFire() const { return m_onFire; }
    void setOnFire(bool onFire) { m_onFire = onFire; }

    /**
     * @brief 是否不可见（隐身效果）
     */
    [[nodiscard]] bool isInvisible() const { return m_invisible; }
    void setInvisible(bool invisible) { m_invisible = invisible; }

    // ========== 攻击动画 ==========

    /**
     * @brief 获取攻击进度
     * @return 攻击进度 (0.0-1.0)
     */
    [[nodiscard]] f32 swingProgress() const { return m_swingProgress; }
    void setSwingProgress(f32 progress) { m_swingProgress = progress; }

    /**
     * @brief 获取上一帧攻击进度
     */
    [[nodiscard]] f32 prevSwingProgress() const { return m_prevSwingProgress; }

    /**
     * @brief 更新攻击进度
     * @param partialTick 部分 tick
     * @return 插值后的攻击进度
     */
    [[nodiscard]] f32 getInterpolatedSwingProgress(f32 partialTick) const {
        return m_prevSwingProgress + (m_swingProgress - m_prevSwingProgress) * partialTick;
    }

    // ========== 装备（用于层渲染） ==========

    /**
     * @brief 获取主手物品
     * @return 物品堆指针，如果没有返回 nullptr
     */
    [[nodiscard]] const ItemStack* getMainHandItem() const {
        return m_mainHandItem ? &*m_mainHandItem : nullptr;
    }
    void setMainHandItem(const ItemStack& item) { m_mainHandItem = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取副手物品
     * @return 物品堆指针，如果没有返回 nullptr
     */
    [[nodiscard]] const ItemStack* getOffHandItem() const {
        return m_offHandItem ? &*m_offHandItem : nullptr;
    }
    void setOffHandItem(const ItemStack& item) { m_offHandItem = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取头部装备
     */
    [[nodiscard]] const ItemStack* getHeadArmor() const {
        return m_headArmor ? &*m_headArmor : nullptr;
    }
    void setHeadArmor(const ItemStack& item) { m_headArmor = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取胸甲
     */
    [[nodiscard]] const ItemStack* getChestArmor() const {
        return m_chestArmor ? &*m_chestArmor : nullptr;
    }
    void setChestArmor(const ItemStack& item) { m_chestArmor = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取护腿
     */
    [[nodiscard]] const ItemStack* getLegsArmor() const {
        return m_legsArmor ? &*m_legsArmor : nullptr;
    }
    void setLegsArmor(const ItemStack& item) { m_legsArmor = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取靴子
     */
    [[nodiscard]] const ItemStack* getFeetArmor() const {
        return m_feetArmor ? &*m_feetArmor : nullptr;
    }
    void setFeetArmor(const ItemStack& item) { m_feetArmor = std::make_unique<ItemStack>(item); }

    // ========== 存活时间 ==========

    [[nodiscard]] u32 ticksExisted() const { return m_ticksExisted; }

    // ========== 元数据缓存 ==========

    /**
     * @brief 获取原始实体元数据
     */
    [[nodiscard]] const std::vector<u8>& metadata() const { return m_metadata; }

    /**
     * @brief 获取元数据管理器
     */
    [[nodiscard]] entity::EntityDataManager& dataManager() { return m_dataManager; }
    [[nodiscard]] const entity::EntityDataManager& dataManager() const { return m_dataManager; }

    /**
     * @brief 设置原始实体元数据
     */
    void setMetadata(const std::vector<u8>& metadata);

    /**
     * @brief 触发元数据同步后的本地状态刷新
     */
    void syncMetadataFromDataManager() {}

    /**
     * @brief 更新实体（每tick调用）
     */
    void tick();

    /**
     * @brief 更新平滑插值（每帧调用）
     * @param deltaTime 帧时间（秒）
     */
    void updateInterpolation(f32 deltaTime);

    // ========== ItemStack 支持（用于 ItemEntity 渲染） ==========

    /**
     * @brief 是否持有物品
     * 用于 ItemEntity 渲染
     */
    [[nodiscard]] bool hasItem() const { return m_itemStack != nullptr; }

    /**
     * @brief 获取物品堆
     * @return 物品堆指针，如果没有物品返回 nullptr
     */
    [[nodiscard]] const ItemStack* itemStack() const { return m_itemStack.get(); }

    /**
     * @brief 设置物品堆
     * 用于客户端接收 SpawnEntity 包时设置 ItemEntity 的物品
     * @param stack 物品堆
     */
    void setItemStack(const ItemStack& stack) {
        m_itemStack = std::make_unique<ItemStack>(stack);
    }

    // ========== XP 支持（用于 ExperienceOrb 渲染） ==========

    /**
     * @brief 获取经验值
     * 用于 ExperienceOrb 渲染
     */
    [[nodiscard]] i32 xpValue() const { return m_xpValue; }

    /**
     * @brief 设置经验值
     * 用于客户端接收 SpawnExperienceOrb 包时设置经验球的值
     * @param value 经验值
     */
    void setXpValue(i32 value) { m_xpValue = value; }

    // ========== 闪电支持（用于 LightningBolt 渲染） ==========

    /**
     * @brief 获取闪电形状随机种子
     * 用于 LightningBoltRenderer 生成一致的闪电形状
     */
    [[nodiscard]] u64 boltVertex() const { return m_boltVertex; }

    /**
     * @brief 设置闪电形状随机种子
     * @param boltVertex 随机种子
     */
    void setBoltVertex(u64 boltVertex) { m_boltVertex = boltVertex; }

private:
    // 基本信息
    EntityId m_id;
    String m_typeId;
    String m_uuid;

    // 位置
    Vector3 m_position;
    Vector3 m_prevPosition;
    Vector3 m_targetPosition;  // 从网络包接收的目标位置

    // 平滑插值配置
    f32 m_interpolationSpeed = 0.3f;   // 插值速度 (0.0-1.0)
    bool m_smoothInterpolation = true;  // 是否启用平滑插值

    // 旋转
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    f32 m_prevYaw = 0.0f;
    f32 m_prevPitch = 0.0f;
    f32 m_headYaw = 0.0f;      // 头部偏航角（动物特有）
    f32 m_prevHeadYaw = 0.0f;
    f32 m_targetYaw = 0.0f;    // 目标旋转（用于平滑插值）
    f32 m_targetPitch = 0.0f;
    f32 m_targetHeadYaw = 0.0f;

    // 速度
    Vector3 m_velocity;

    // 动画状态
    f32 m_prevLimbSwing = 0.0f;      // 上一帧腿部摆动进度
    f32 m_limbSwing = 0.0f;          // 腿部摆动进度
    f32 m_prevLimbSwingAmount = 0.0f;  // 上一帧腿部摆动强度
    f32 m_limbSwingAmount = 0.0f;    // 腿部摆动强度

    // 状态
    bool m_onGround = false;
    bool m_removed = false;
    bool m_child = false;

    // 尺寸
    f32 m_width = 0.6f;
    f32 m_height = 1.8f;

    // 受伤和死亡状态
    i32 m_hurtTime = 0;    // 受伤时间 (0-10)
    i32 m_deathTime = 0;   // 死亡时间

    // 行为状态
    bool m_sneaking = false;
    bool m_swimming = false;
    bool m_riding = false;
    bool m_sitting = false;
    bool m_onFire = false;
    bool m_invisible = false;

    // 攻击动画
    f32 m_swingProgress = 0.0f;
    f32 m_prevSwingProgress = 0.0f;

    // 装备（用于层渲染）
    std::unique_ptr<ItemStack> m_mainHandItem;
    std::unique_ptr<ItemStack> m_offHandItem;
    std::unique_ptr<ItemStack> m_headArmor;
    std::unique_ptr<ItemStack> m_chestArmor;
    std::unique_ptr<ItemStack> m_legsArmor;
    std::unique_ptr<ItemStack> m_feetArmor;

    // 存活时间
    u32 m_ticksExisted = 0;

    // ItemEntity 物品数据
    std::unique_ptr<ItemStack> m_itemStack;

    // ExperienceOrb 经验值数据
    i32 m_xpValue = 1;  // 默认值为1

    // LightningBolt 闪电形状随机种子
    u64 m_boltVertex = 0;

    // 追踪位置系统（用于披风摆动）
    // 参考 MC 1.16.5 PlayerEntity: chasingPosX/Y/Z 和 prevChasingPosX/Y/Z
    f64 m_chasingPosX = 0.0;
    f64 m_chasingPosY = 0.0;
    f64 m_chasingPosZ = 0.0;
    f64 m_prevChasingPosX = 0.0;
    f64 m_prevChasingPosY = 0.0;
    f64 m_prevChasingPosZ = 0.0;

    // 相机偏航角系统（用于披风摆动）
    // 参考 MC 1.16.5 PlayerEntity: cameraYaw 和 prevCameraYaw
    f32 m_cameraYaw = 0.0f;
    f32 m_prevCameraYaw = 0.0f;

    // 鞘翅角度系统（用于鞘翅展开动画）
    // 参考 MC 1.16.5 AbstractClientPlayerEntity: rotateElytraX/Y/Z
    f32 m_rotateElytraX = 0.0f;
    f32 m_rotateElytraY = 0.0f;
    f32 m_rotateElytraZ = 0.0f;

    // 悬浮起始偏移（用于 ItemEntity 浮动动画随机化）
    f32 m_hoverStart = 0.0f;

    // 原始元数据缓存
    std::vector<u8> m_metadata;

    // 解析后的元数据
    entity::EntityDataManager m_dataManager;
};

} // namespace mc::client
