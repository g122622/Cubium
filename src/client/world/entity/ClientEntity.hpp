#pragma once

#include "../../../common/core/Types.hpp"
#include "../../../common/util/math/Vector3.hpp"
#include "../../../common/item/core/ItemStack.hpp"
#include <string>
#include <memory>

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
     * 将目标位置变为当前位置，当前位置变为上一帧位置
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

    /**
     * @brief 设置旋转（立即设置）
     */
    void setRotation(f32 yaw, f32 pitch);

    /**
     * @brief 设置头部旋转
     */
    void setHeadRotation(f32 headYaw);

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
     * @brief 获取腿部摆动进度
     * 用于行走动画，范围 0 到 2π
     */
    [[nodiscard]] f32 limbSwing() const { return m_limbSwing; }

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

    // ========== 存活时间 ==========

    [[nodiscard]] u32 ticksExisted() const { return m_ticksExisted; }

    /**
     * @brief 更新实体（每tick调用）
     */
    void tick();

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

private:
    // 基本信息
    EntityId m_id;
    String m_typeId;
    String m_uuid;

    // 位置
    Vector3 m_position;
    Vector3 m_prevPosition;
    Vector3 m_targetPosition;  // 从网络包接收的目标位置

    // 旋转
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    f32 m_prevYaw = 0.0f;
    f32 m_prevPitch = 0.0f;
    f32 m_headYaw = 0.0f;      // 头部偏航角（动物特有）
    f32 m_prevHeadYaw = 0.0f;

    // 速度
    Vector3 m_velocity;

    // 动画状态
    f32 m_limbSwing = 0.0f;        // 腿部摆动进度
    f32 m_limbSwingAmount = 0.0f;  // 腿部摆动强度

    // 状态
    bool m_onGround = false;
    bool m_removed = false;
    bool m_child = false;

    // 尺寸
    f32 m_width = 0.6f;
    f32 m_height = 1.8f;

    // 存活时间
    u32 m_ticksExisted = 0;

    // ItemEntity 物品数据
    std::unique_ptr<ItemStack> m_itemStack;

    // ExperienceOrb 经验值数据
    i32 m_xpValue = 1;  // 默认值为1
};

} // namespace mc::client
