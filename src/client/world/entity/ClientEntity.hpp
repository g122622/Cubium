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

#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {

class BlockState;

namespace entity {
// 前向声明：EntityType 完整定义在 common/entity/core/EntityType.hpp，
// ClientEntity 仅持有其 const 指针（m_entityType 缓存）并按名查询。
class EntityType;
} // namespace entity

} // namespace mc

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
 */
class ClientEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param typeId 实体类型标识符（如 "pig", "cow"）
     */
    ClientEntity(EntityInstanceId id, const std::string& typeId);
    ~ClientEntity() = default;

    // 禁止拷贝
    ClientEntity(const ClientEntity&) = delete;
    ClientEntity& operator=(const ClientEntity&) = delete;

    // 禁止移动（包含 EntityDataManager）
    ClientEntity(ClientEntity&&) = delete;
    ClientEntity& operator=(ClientEntity&&) = delete;

    // ========== 基本信息 ==========

    [[nodiscard]] EntityInstanceId id() const { return m_id; }

    /**
     * @brief 获取实体类型标识符字符串
     * @return 资源位置（如 "minecraft:pig"），用于渲染键、资源解析、显示
     *
     * 与服务端 Entity::getTypeId() 命名统一。字符串用途（渲染查找、ResourceLocation
     * 解析、调试显示）请用本方法；类型判等请用 entityType() 指针比较。
     */
    [[nodiscard]] const std::string& getTypeId() const { return m_typeId; }

    /**
     * @brief 获取实体类型的运行时指针（懒查询）
     *
     * @return 指向 EntityRegistry 内部 EntityType 对象的 const 指针；若 m_typeId
     *         为空或注册表未注册该类型（如部分模组实体），返回 nullptr。
     *
     * 指针稳定性：返回值指向 EntityRegistry::m_types（std::deque）内对象，
     * 与 VanillaEntityTypeKeys::* 同源，地址稳定，可安全用于指针比较。
     * 与服务端 Entity::entityType() 行为一致。
     */
    [[nodiscard]] const entity::EntityType* entityType() const;

    [[nodiscard]] const std::string& uuid() const { return m_uuid; }
    void setUuid(const std::string& uuid) { m_uuid = uuid; }

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

    // ========== 追踪位置系统（用于披风摆动） ==========

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

    // ========== 相机偏航角系统（用于披风摆动） ==========

    /**
     * @brief 获取相机偏航角
     * 用于计算披风摆动强度
     */
    [[nodiscard]] f32 cameraYaw() const { return m_cameraYaw; }

    /**
     * @brief 获取上一帧相机偏航角
     */
    [[nodiscard]] f32 prevCameraYaw() const { return m_prevCameraYaw; }

    // ========== 鞘翅角度系统（用于鞘翅展开动画） ==========

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

    // ========== 悬浮起始偏移（用于 ItemEntity） ==========

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

    /**
     * @brief 获取实体眼睛高度（从脚底到眼睛的距离）
     *
     * 根据实体类型和当前姿态（蹲伏、游泳、睡眠等）计算眼睛高度。
     * 对于玩家实体，蹲伏时为 1.27，游泳/鞘翅飞行/旋转攻击时为 0.4，
     * 睡眠时为 0.2，站立时为 1.62。
     * 对于其他实体，使用实体注册表中注册的 EntitySize 的眼高，
     * 幼年个体眼高为站立时的一半。
     */
    [[nodiscard]] f32 eyeHeight() const { return m_eyeHeight; }

    /**
     * @brief 设置实体眼睛高度
     * @param eyeHeight 眼睛高度
     */
    void setEyeHeight(f32 eyeHeight) { m_eyeHeight = eyeHeight; }

    /**
     * @brief 根据实体类型和当前状态刷新眼睛高度
     *
     * 从实体注册表中查找基础眼高，然后根据姿态（蹲伏、游泳、睡眠）
     * 和年龄（幼年）进行调整。当实体类型、姿态或年龄发生变化时应调用此方法。
     */
    void refreshEyeHeight();

    // ========== 年龄（用于幼年动物渲染） ==========

    /**
     * @brief 是否是幼年个体
     */
    [[nodiscard]] bool isChild() const { return m_child; }
    void setChild(bool child)
    {
        if (m_child != child) {
            m_child = child;
            refreshEyeHeight();
        }
    }

    // ========== 受伤和死亡状态 ==========

    /**
     * @brief 获取受伤时间
     * 用于渲染受伤闪烁效果，范围 0-10
     */
    [[nodiscard]] i32 hurtTime() const { return m_hurtTime; }
    void setHurtTime(i32 time) { m_hurtTime = time; }

    /**
     * @brief 获取受伤方向角（度，相对实体朝向）
     *
     * 由服务端 hurt 动画包同步，第三人称实体渲染的 damageTilt 据此倾斜。
     */
    [[nodiscard]] f32 hurtDir() const { return m_hurtDir; }
    void setHurtDir(f32 dir) { m_hurtDir = dir; }

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
    void setSneaking(bool sneaking)
    {
        if (m_sneaking != sneaking) {
            m_sneaking = sneaking;
            refreshEyeHeight();
        }
    }

    /**
     * @brief 是否正在游泳
     */
    [[nodiscard]] bool isSwimming() const { return m_swimming; }
    void setSwimming(bool swimming)
    {
        if (m_swimming != swimming) {
            m_swimming = swimming;
            refreshEyeHeight();
        }
    }

    /**
     * @brief 是否在视觉上表现为游泳姿态
     *
     * 客户端镜像 DrownedEntity::isVisuallySwimming()（溺尸专属判定）与
     * LivingEntity::isVisuallySwimming()（基类判定）的并集语义：
     * - 溺尸等通过 Swimming 标志位驱动视觉游泳的实体：isSwimming() && !isRiding()
     * - 玩家等通过 Pose 驱动的实体：isSwimming() 为 true 时也算视觉游泳
     *
     * 该返回值驱动 ClientEntity::tick() 中 m_swimAmount 的渐入渐出，
     * 进而通过 getInterpolatedSwimAmount 供渲染器读取。
     */
    [[nodiscard]] bool isVisuallySwimming() const { return isSwimming() && !isRiding(); }

    /**
     * @brief 获取上一帧游泳动画渐变量
     */
    [[nodiscard]] f32 swimAmountO() const noexcept { return m_swimAmountO; }

    /**
     * @brief 获取当前帧游泳动画渐变量
     */
    [[nodiscard]] f32 swimAmount() const noexcept { return m_swimAmount; }

    /**
     * @brief 计算指定 partialTicks 下的插值游泳动画量
     *
     * 对应 MC 1.21.11 LivingEntity.getSwimAmount(float partialTick)。
     * 渲染器（EntityRendererManager）在构建 AnimationContext 时调用此方法，
     * 将结果写入 context.swimAmount，驱动 DrownedModel::setAngles 中的
     * 手臂/腿部游泳覆盖动画。
     *
     * @param partialTicks 帧内插值因子 [0, 1)
     * @return 插值后的游泳动画量 [0, 1]
     */
    [[nodiscard]] f32 getInterpolatedSwimAmount(f32 partialTicks) const noexcept
    {
        return m_swimAmountO + (m_swimAmount - m_swimAmountO) * partialTicks;
    }

    /**
     * @brief 是否正在乘坐载具
     */
    [[nodiscard]] bool isRiding() const { return m_riding; }
    void setRiding(bool riding) { m_riding = riding; }

    /**
     * @brief 获取正在骑乘的载具ID
     */
    [[nodiscard]] EntityInstanceId vehicleId() const { return m_vehicleId; }
    void setVehicleId(EntityInstanceId vehicleId) { m_vehicleId = vehicleId; }

    /**
     * @brief 获取乘客列表（如果此实体是载具）
     */
    [[nodiscard]] const std::vector<u32>& passengers() const { return m_passengers; }

    /**
     * @brief 设置乘客列表（由 SetPassengersPacket 更新）
     */
    void setPassengers(std::vector<u32> passengers) { m_passengers = std::move(passengers); }

    /**
     * @brief 拴绳持有者实体 ID（由 SetEntityLink 包同步，0=未被拴）
     *
     * sourceId 为被拴实体（本实体），destId 为持有者（玩家或 LeashKnot）。
     * 当前客户端无 leash 渲染，字段先行落地；未来 LeashRopeRenderer 读取此值在 mob 与
     * holder 之间画牵绳。
     *
     * TODO: 实现客户端牵绳渲染（LeashRopeRenderer），当前仅存字段。
     */
    [[nodiscard]] EntityInstanceId leashHolderId() const { return m_leashHolderId; }
    void setLeashHolderId(EntityInstanceId id) { m_leashHolderId = id; }

    /**
     * @brief 是否正在坐下（用于动物）
     */
    [[nodiscard]] bool isSitting() const { return m_sitting; }
    void setSitting(bool sitting) { m_sitting = sitting; }

    /**
     * @brief 是否正在睡眠
     */
    [[nodiscard]] bool isSleeping() const { return m_sleeping; }
    void setSleeping(bool sleeping)
    {
        if (m_sleeping != sleeping) {
            m_sleeping = sleeping;
            refreshEyeHeight();
        }
    }

    /**
     * @brief 获取睡眠位置
     *
     * 当实体睡眠时，床的方块位置。
     */
    [[nodiscard]] const BlockPos& sleepingPosition() const { return m_sleepingPosition; }
    void setSleepingPosition(const BlockPos& pos) { m_sleepingPosition = pos; }
    void clearSleepingPosition() { m_sleepingPosition = BlockPos(0, 0, 0); }

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

    /**
     * @brief 是否正在鞘翅飞行
     *
     * 从实体元数据中读取 FallFlying 标志位。
     */
    [[nodiscard]] bool isFallFlying() const;

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
     * @brief 获取正在挥动的手
     * @return 0=主手, 1=副手
     */
    [[nodiscard]] i32 swingHand() const { return m_swingHand; }

    /**
     * @brief 检查是否正在进行挥动动画
     */
    [[nodiscard]] bool isSwingInProgress() const { return m_swingInProgress; }

    /**
     * @brief 触发挥动动画
     * @param hand 0=主手, 1=副手
     */
    void triggerSwingAnimation(i32 hand);

    /**
     * @brief 触发受伤动画
     * 设置受伤时间为10 tick
     */
    void triggerHurtAnimation();

    /**
     * @brief 触发起床动画
     * 清除睡眠状态
     */
    void triggerLeaveBedAnimation();

    /**
     * @brief 更新攻击进度
     * @param partialTick 部分 tick
     * @return 插值后的攻击进度
     */
    [[nodiscard]] f32 getInterpolatedSwingProgress(f32 partialTick) const
    {
        return m_prevSwingProgress + (m_swingProgress - m_prevSwingProgress) * partialTick;
    }

    // ========== 北极熊站立动画 ==========

    /**
     * @brief 是否正在站立（北极熊特有）
     */
    [[nodiscard]] bool isStanding() const { return m_isStanding; }
    void setStanding(bool standing) { m_isStanding = standing; }

    /**
     * @brief 获取站立动画进度
     * 范围 [0, 6]，0 表示四足站立，6 表示完全站立
     */
    [[nodiscard]] f32 clientSideStandAnimation() const { return m_clientSideStandAnimation; }
    void setClientSideStandAnimation(f32 value) { m_clientSideStandAnimation = value; }

    /**
     * @brief 获取上一帧站立动画进度
     */
    [[nodiscard]] f32 clientSideStandAnimation0() const { return m_clientSideStandAnimation0; }
    void setClientSideStandAnimation0(f32 value) { m_clientSideStandAnimation0 = value; }

    /**
     * @brief 获取站立动画缩放值
     * @param partialTick 部分 tick 值
     * @return 动画缩放值 [0, 1]
     */
    [[nodiscard]] f32 getStandingAnimationScale(f32 partialTick) const
    {
        return (m_clientSideStandAnimation0 +
                   (m_clientSideStandAnimation - m_clientSideStandAnimation0) * partialTick) /
            6.0f;
    }

    /**
     * @brief 更新站立动画状态（每 tick 调用）
     */
    void updateStandingAnimation();

    // ========== 河豚膨胀状态 ==========

    /**
     * @brief 获取河豚膨胀状态
     * 0 = Deflated, 1 = SemiPuffed, 2 = FullyPuffed
     */
    [[nodiscard]] i32 puffState() const { return m_puffState; }
    void setPuffState(i32 state) { m_puffState = std::clamp(state, 0, 2); }

    // ========== 吃草动画状态 ==========

    /**
     * @brief 获取吃草动画计时器
     * 羊吃草时的头部低头动画计时器，收到 EatBlock 状态包时设为 40，
     * 每tick递减，为0时动画结束。
     *
     * 数据流：ClientApplicationNetwork.onEntityStatus(EatBlock) → setEatAnimationTimer(40)
     * → ClientEntity.tick() 递减 → AnimationContext.eatAnimationTimer
     * → EntityRendererManager 传递给 SheepModel::setEatAnimationTimer()
     * → SheepModel 计算 getHeadEatPositionScale/getHeadEatAngleScale 驱动头部动画
     */
    [[nodiscard]] i32 eatAnimationTimer() const { return m_eatAnimationTimer; }

    /**
     * @brief 设置吃草动画计时器
     * @param timer 计时器值（原版为 40 ticks）
     */
    void setEatAnimationTimer(i32 timer) { m_eatAnimationTimer = timer; }

    // ========== TNT 矿车引信状态 ==========

    /**
     * @brief 获取 TNT 矿车引信计时器
     * TNT 矿车被引燃后的倒计时值，-1 表示未引燃。
     * 收到 EatBlock 状态包时设为 80（4秒），每tick递减。
     *
     * 数据流：ClientApplicationNetwork.onEntityStatus(EatBlock, typeId == "minecraft:tnt_minecart")
     * → setFuseTimer(80) → ClientEntity.tick() 递减 → 渲染器用于闪烁效果
     */
    [[nodiscard]] i32 fuseTimer() const { return m_fuseTimer; }

    /**
     * @brief 设置 TNT 矿车引信计时器
     * @param timer 引信计时器值（原版为 80 ticks，-1 表示未引燃）
     */
    void setFuseTimer(i32 timer) { m_fuseTimer = timer; }

    // ========== 美西螈状态 ==========

    /**
     * @brief 获取美西螈变体
     * 0=Lucy(白化), 1=Wild(野生), 2=Gold(金色), 3=Cyan(青色), 4=Blue(蓝色)
     */
    [[nodiscard]] i32 axolotlVariant() const { return m_axolotlVariant; }
    void setAxolotlVariant(i32 variant) { m_axolotlVariant = std::clamp(variant, 0, 4); }

    /**
     * @brief 获取美西螈装死状态
     */
    [[nodiscard]] bool axolotlPlayingDead() const { return m_axolotlPlayingDead; }
    void setAxolotlPlayingDead(bool playingDead) { m_axolotlPlayingDead = playingDead; }

    // ========== 豹猫信任状态 ==========

    /**
     * @brief 是否已被信任（豹猫特有）
     */
    [[nodiscard]] bool isTrusting() const { return m_trusting; }
    void setTrusting(bool trusting) { m_trusting = trusting; }

    // ========== 猫动画状态 ==========

    /**
     * @brief 猫是否躺下
     */
    [[nodiscard]] bool isCatLieDown() const { return m_catLieDown; }
    void setCatLieDown(bool lying) { m_catLieDown = lying; }

    /**
     * @brief 猫是否处于放松状态（看向睡眠主人）
     */
    [[nodiscard]] bool isCatRelaxStateOne() const { return m_catRelaxStateOne; }
    void setCatRelaxStateOne(bool relax) { m_catRelaxStateOne = relax; }

    // ========== 装备（用于层渲染） ==========

    /**
     * @brief 获取主手物品
     * @return 物品堆指针，如果没有返回 nullptr
     */
    [[nodiscard]] const ItemStack* getMainHandItem() const { return m_mainHandItem ? &*m_mainHandItem : nullptr; }
    void setMainHandItem(const ItemStack& item) { m_mainHandItem = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取副手物品
     * @return 物品堆指针，如果没有返回 nullptr
     */
    [[nodiscard]] const ItemStack* getOffHandItem() const { return m_offHandItem ? &*m_offHandItem : nullptr; }
    void setOffHandItem(const ItemStack& item) { m_offHandItem = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取头部装备
     */
    [[nodiscard]] const ItemStack* getHeadArmor() const { return m_headArmor ? &*m_headArmor : nullptr; }
    void setHeadArmor(const ItemStack& item) { m_headArmor = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取胸甲
     */
    [[nodiscard]] const ItemStack* getChestArmor() const { return m_chestArmor ? &*m_chestArmor : nullptr; }
    void setChestArmor(const ItemStack& item) { m_chestArmor = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取护腿
     */
    [[nodiscard]] const ItemStack* getLegsArmor() const { return m_legsArmor ? &*m_legsArmor : nullptr; }
    void setLegsArmor(const ItemStack& item) { m_legsArmor = std::make_unique<ItemStack>(item); }

    /**
     * @brief 获取靴子
     */
    [[nodiscard]] const ItemStack* getFeetArmor() const { return m_feetArmor ? &*m_feetArmor : nullptr; }
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
    void syncMetadataFromDataManager();

    /**
     * @brief 检查实体是否处于愤怒状态
     *
     * 用于蜜蜂等实体的愤怒状态检测。
     * 愤怒时间存储在元数据参数 ID 1（i32 类型）。
     *
     * @return 如果愤怒时间 > 0 返回 true
     */
    [[nodiscard]] bool isAngry() const;

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
    void setItemStack(const ItemStack& stack);
    void clearItemStack();
    [[nodiscard]] u32 itemRenderStateVersion() const { return m_itemRenderStateVersion; }
    [[nodiscard]] std::optional<i32> metadataPickupDelay() const { return m_metadataPickupDelay; }
    [[nodiscard]] std::optional<i32> metadataAge() const { return m_metadataAge; }

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

    // ========== 铁傀儡状态 ==========

    /**
     * @brief 获取攻击动画计时器
     * 铁傀儡举臂攻击动画，收到 IronGolemAttack 状态包时设为 10，
     * 每tick递减，为0时动画结束。
     */
    [[nodiscard]] i32 ironGolemAttackTimer() const { return m_ironGolemAttackTimer; }

    /**
     * @brief 设置攻击动画计时器
     * @param timer 计时器值（原版为 10 ticks）
     */
    void setIronGolemAttackTimer(i32 timer) { m_ironGolemAttackTimer = timer; }

    /**
     * @brief 是否举起手臂（攻击动画）
     */
    [[nodiscard]] bool ironGolemArmsRaised() const { return m_ironGolemArmsRaised; }

    /**
     * @brief 设置手臂举起状态
     */
    void setIronGolemArmsRaised(bool raised) { m_ironGolemArmsRaised = raised; }

    /**
     * @brief 是否手持罂粟花（给村民送花动画）
     */
    [[nodiscard]] bool ironGolemHoldingRose() const { return m_ironGolemHoldingRose; }

    /**
     * @brief 设置手持花朵状态
     */
    void setIronGolemHoldingRose(bool holding) { m_ironGolemHoldingRose = holding; }

    // ========== 疣猪兽/僵尸疣兽攻击动画 ==========

    /**
     * @brief 获取撞飞攻击动画剩余 tick
     * 疣猪兽/僵尸疣兽甩头攻击动画，收到 HoglinAttack 状态包时设为 10，
     * 每tick递减，为0时动画结束。
     * 对应 MC 原版 HoglinBase.getAttackAnimationRemainingTicks()。
     *
     * 数据流：ClientApplicationNetwork.onEntityStatus(HoglinAttack)
     * → setFlingAnimationTicks(10) → ClientEntity.tick() 递减 → AnimationContext → BoarModel
     */
    [[nodiscard]] i32 flingAnimationTicks() const { return m_flingAnimationTicks; }

    /**
     * @brief 设置撞飞攻击动画剩余 tick
     * @param ticks 剩余 tick 数（原版为 10 ticks）
     */
    void setFlingAnimationTicks(i32 ticks) { m_flingAnimationTicks = ticks; }

    // ========== 狼甩水动画状态 ==========

    /**
     * @brief 是否正在甩水（狼特有）
     *
     * 收到 ShakeOffWater(8) 状态包时设为 true，
     * 收到 WolfStopShaking(56) 状态包或甩水完成时设为 false。
     *
     * 数据流：ClientApplicationNetwork.onEntityStatus(ShakeOffWater)
     * → setWolfShaking(true) → ClientEntity.tick() 推进 shakeAnim
     * → AnimationContext.wolfShakeAnim → WolfModel::setAnimState
     */
    [[nodiscard]] bool wolfShaking() const { return m_wolfIsShaking; }

    /**
     * @brief 设置狼甩水状态
     * @param shaking 是否正在甩水
     */
    void setWolfShaking(bool shaking) { m_wolfIsShaking = shaking; }

    /**
     * @brief 获取狼甩水动画进度（当前 tick）
     *
     * 范围 [0, 2]，每 tick +0.05，达到 2.0 时甩水完成。
     * 对应 MC Wolf.shakeAnim。
     */
    [[nodiscard]] f32 wolfShakeAnim() const { return m_wolfShakeAnim; }

    /**
     * @brief 设置狼甩水动画进度
     * @param anim 甩水进度（0.0-2.0）
     */
    void setWolfShakeAnim(f32 anim) { m_wolfShakeAnim = anim; }

    /**
     * @brief 获取上一 tick 的狼甩水进度（用于插值）
     */
    [[nodiscard]] f32 wolfShakeAnimO() const { return m_wolfShakeAnimO; }

    /**
     * @brief 设置上一 tick 的狼甩水进度
     * @param anim 上一 tick 的甩水进度
     */
    void setWolfShakeAnimO(f32 anim) { m_wolfShakeAnimO = anim; }

    /**
     * @brief 获取狼湿润状态
     *
     * 收到 ShakeOffWater 时设为 true，甩水完成时设为 false。
     * 用于渲染湿润着色（变暗）。
     */
    [[nodiscard]] bool wolfIsWet() const { return m_wolfIsWet; }

    /**
     * @brief 设置狼湿润状态
     * @param wet 是否湿润
     */
    void setWolfIsWet(bool wet) { m_wolfIsWet = wet; }

    /**
     * @brief 获取狼乞求食物头部角度（当前 tick）
     *
     * 对应 MC Wolf.interestedAngle。范围 [0, 1]。
     */
    [[nodiscard]] f32 wolfInterestedAngle() const { return m_wolfInterestedAngle; }

    /**
     * @brief 设置狼乞求食物头部角度
     * @param angle 乞求角度（0.0-1.0）
     */
    void setWolfInterestedAngle(f32 angle) { m_wolfInterestedAngle = angle; }

    /**
     * @brief 获取上一 tick 的狼乞求角度（用于插值）
     */
    [[nodiscard]] f32 wolfInterestedAngleO() const { return m_wolfInterestedAngleO; }

    /**
     * @brief 设置上一 tick 的狼乞求角度
     * @param angle 上一 tick 的乞求角度
     */
    void setWolfInterestedAngleO(f32 angle) { m_wolfInterestedAngleO = angle; }

    /**
     * @brief 获取狼是否感兴趣（乞求食物）
     *
     * 通过元数据同步自服务端 WolfEntity::DATA_INTERESTED_PARAM。
     * 由 BegGoal.startExecuting/resetTask 在服务端调用 setInterested 修改。
     * ClientEntity::tick 根据 m_wolfIsInterested 推进 m_wolfInterestedAngle 插值。
     */
    [[nodiscard]] bool wolfIsInterested() const { return m_wolfIsInterested; }

    /**
     * @brief 设置狼是否感兴趣
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     * 修改后，ClientEntity::tick 会推进 m_wolfInterestedAngle 向 1.0 或 0.0 插值。
     *
     * @param interested 是否感兴趣
     */
    void setWolfIsInterested(bool interested) { m_wolfIsInterested = interested; }

    /**
     * @brief 获取狼是否已被驯服
     *
     * 通过元数据同步自服务端 TameableEntity::DATA_TAMED_PARAM。
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用 setWolfTamed 更新。
     * WolfCollarLayer::shouldRender 读取此状态判断是否渲染项圈。
     */
    [[nodiscard]] bool wolfTamed() const { return m_wolfTamed; }

    /**
     * @brief 设置狼是否已被驯服
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     *
     * @param tamed 是否已被驯服
     */
    void setWolfTamed(bool tamed) { m_wolfTamed = tamed; }

    /**
     * @brief 获取狼颈圈颜色
     *
     * 通过元数据同步自服务端 WolfEntity::DATA_COLLAR_COLOR_PARAM。
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用 setWolfCollarColor 更新。
     * WolfCollarLayer::renderPipeline 读取此颜色渲染项圈色调。
     *
     * @return 染料颜色（DyeColor 枚举值）
     */
    [[nodiscard]] DyeColor wolfCollarColor() const { return m_wolfCollarColor; }

    /**
     * @brief 设置狼颈圈颜色
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     *
     * @param color 染料颜色
     */
    void setWolfCollarColor(DyeColor color) { m_wolfCollarColor = color; }

    /**
     * @brief 获取狼是否处于愤怒状态
     *
     * 通过元数据同步自服务端 WolfEntity::DATA_ANGER_TIME_PARAM。
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用 setWolfIsAngry 更新。
     * EntityRendererManager 在 wolf 模型分支读取此状态（通过 AnimationContext::isAngry）
     * 驱动 WolfModel::setAnimState 的 isAngry 参数（愤怒时尾巴停止摆动），
     * 以及尾巴抬起角度（1.539f ≈ 88°）。
     *
     * @return 如果狼处于愤怒状态返回 true
     */
    [[nodiscard]] bool wolfIsAngry() const { return m_wolfIsAngry; }

    /**
     * @brief 设置狼是否处于愤怒状态
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     *
     * @param angry 是否处于愤怒状态
     */
    void setWolfIsAngry(bool angry) { m_wolfIsAngry = angry; }

    // ========== 马类状态 ==========

    /**
     * @brief 马类是否已驯服
     *
     * 通过元数据同步自服务端 AbstractHorseEntity::STATUS_PARAM（bit1, mask 0x02）。
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用 setHorseTamed 更新。
     * 7 个马类子类（Horse/Donkey/Mule/Llama/TraderLlama/SkeletonHorse/ZombieHorse）
     * 共用同一 STATUS_PARAM，经 typeId 判断 + hasParam 自动覆盖。
     */
    [[nodiscard]] bool horseTamed() const { return m_horseTamed; }

    void setHorseTamed(bool tamed) { m_horseTamed = tamed; }

    /**
     * @brief 马类是否装备鞍
     *
     * 通过元数据同步自服务端 AbstractHorseEntity::STATUS_PARAM（bit2, mask 0x04）。
     * HorseRenderer 鞍渲染层读取此状态判断是否渲染鞍。
     */
    [[nodiscard]] bool horseSaddled() const { return m_horseSaddled; }

    void setHorseSaddled(bool saddled) { m_horseSaddled = saddled; }

    /**
     * @brief 马类是否已繁殖
     *
     * 通过元数据同步自服务端 AbstractHorseEntity::STATUS_PARAM（bit3, mask 0x08）。
     */
    [[nodiscard]] bool horseBred() const { return m_horseBred; }

    void setHorseBred(bool bred) { m_horseBred = bred; }

    /**
     * @brief 马类是否正在吃草
     *
     * 通过元数据同步自服务端 AbstractHorseEntity::STATUS_PARAM（bit4, mask 0x10）。
     * 驱动低头吃草动画（headLean 插值）。
     */
    [[nodiscard]] bool horseEating() const { return m_horseEating; }

    void setHorseEating(bool eating) { m_horseEating = eating; }

    /**
     * @brief 马类是否正在扬蹄
     *
     * 通过元数据同步自服务端 AbstractHorseEntity::STATUS_PARAM（bit5, mask 0x20）。
     * 驱动扬蹄动画（rearingAmount 插值）与乘客位置偏移。
     */
    [[nodiscard]] bool horseRearing() const { return m_horseRearing; }

    void setHorseRearing(bool rearing) { m_horseRearing = rearing; }

    /**
     * @brief 马类嘴巴是否张开
     *
     * 通过元数据同步自服务端 AbstractHorseEntity::STATUS_PARAM（bit6, mask 0x40）。
     * 驱动张嘴动画（mouthOpenness 插值）。
     */
    [[nodiscard]] bool horseMouthOpen() const { return m_horseMouthOpen; }

    void setHorseMouthOpen(bool open) { m_horseMouthOpen = open; }

    // ========== 末影人状态 ==========

    /**
     * @brief 获取末影人持有的方块状态
     *
     * 通过元数据同步自服务端 EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM。
     * 服务端存储 BlockState 的 stateId（i32），客户端收到后通过
     * BlockRegistry::instance().getBlockState(stateId) 解析为 BlockState 指针并缓存。
     * 返回 nullptr 表示未持有方块。
     *
     * 由 HeldBlockLayer 读取以渲染末影人手持方块。
     */
    [[nodiscard]] const ::mc::BlockState* endermanHeldBlockState() const { return m_endermanHeldBlockState; }

    /**
     * @brief 设置末影人持有的方块状态
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     *
     * @param state 方块状态指针（nullptr 表示未持有）
     */
    void setEndermanHeldBlockState(const ::mc::BlockState* state) { m_endermanHeldBlockState = state; }

    /**
     * @brief 末影人是否正在被注视（尖叫状态）
     *
     * 通过元数据同步自服务端 EndermanEntity::DATA_SCREAMING_PARAM。
     * 由 EndermanModel 读取以设置 setAttacking 姿态。
     */
    [[nodiscard]] bool endermanScreaming() const { return m_endermanScreaming; }

    /**
     * @brief 设置末影人注视状态
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     *
     * @param screaming 是否正在被注视
     */
    void setEndermanScreaming(bool screaming) { m_endermanScreaming = screaming; }

    // ========== 下落方块状态 ==========

    /**
     * @brief 获取下落方块的方块状态
     *
     * 对齐 MC 1.21.11：BlockState 经 AddEntity 包 data 字段下发 stateId
     * （见 ClientPlayVisitor AddEntity 分支调用 setFallingBlockState）。
     * 返回 nullptr 表示未设置（空气）。
     *
     * 由 FallingBlockRenderer 读取以渲染下落方块模型。
     */
    [[nodiscard]] const ::mc::BlockState* fallingBlockState() const { return m_fallingBlockState; }

    /**
     * @brief 设置下落方块的方块状态
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     *
     * @param state 方块状态指针（nullptr 表示未设置）
     */
    void setFallingBlockState(const ::mc::BlockState* state) { m_fallingBlockState = state; }

    // ========== TNT 实体状态 ==========

    /**
     * @brief 获取 TNT 引信剩余 tick
     *
     * 通过元数据同步自服务端 TNTEntity::DATA_FUSE_PARAM。
     * 对应 MC 1.21.11 PrimedTnt.getFuse()。
     * 由 TNTRenderer 读取以计算闪烁缩放和白色闪烁帧。
     */
    [[nodiscard]] i32 tntFuse() const { return m_tntFuse; }

    /**
     * @brief 设置 TNT 引信剩余 tick
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     */
    void setTntFuse(i32 fuse) { m_tntFuse = fuse; }

    /**
     * @brief 获取 TNT 方块状态
     *
     * 通过元数据同步自服务端 TNTEntity::DATA_BLOCK_STATE_PARAM（BlockStateValue→BLOCK_STATE id14）。
     * 对应 MC 1.21.11 PrimedTnt.getBlockState()。
     * 由 TNTRenderer 读取以渲染 TNT 方块模型。
     */
    [[nodiscard]] const ::mc::BlockState* tntBlockState() const { return m_tntBlockState; }

    /**
     * @brief 设置 TNT 方块状态
     *
     * 由 syncMetadataFromDataManager 在收到元数据更新时调用。
     */
    void setTntBlockState(const ::mc::BlockState* state) { m_tntBlockState = state; }

    // ========== 骷髅拉弓状态 ==========
    // 注：骷髅拉弓渲染状态不再用独立镜像字段——对齐 vanilla 1.21.11
    // AbstractSkeletonRenderer.getArmPose：据 Mob.isAggressive()（见下方
    // m_isAggressive，由 DATA_MOB_FLAGS_PARAM 位 2 同步）+ 主手持弓判定渲染
    // BowAndArrow。原 m_chargingBow 镜像（同步自 AbstractSkeletonEntity::
    // DATA_CHARGING_BOW_PARAM id16）已移除——该 id16 字段致 vanilla Stray/
    // WitherSkeleton 客户端（访问器数组长度=16）set_entity_data 越界崩溃。

    // ========== Mob 激怒/攻击中状态 ==========

    /**
     * @brief 获取 Mob 是否处于激怒/攻击中状态（客户端镜像）
     *
     * 通过元数据同步自服务端 MobEntity::DATA_MOB_FLAGS_PARAM 的位 2
     * （对应 MC 1.21.11 Mob.MOB_FLAG_AGGRESSIVE / DATA_MOB_FLAGS_ID 位 2）。
     *
     * 服务端写入路径：
     *   - MeleeAttackGoal::startExecuting() → CreatureEntity::setAggroed(true)
     *     → MobEntity::setAggressive(true) → 数据参数置位 MOB_FLAG_AGGRESSIVE。
     *   - MeleeAttackGoal::resetTask() → setAggroed(false) → 清除置位。
     *
     * 客户端读取路径：
     *   - ClientEntity::syncMetadataFromDataManager 解析 DATA_MOB_FLAGS_PARAM，
     *     提取位 2 后调用 setIsAggressive。
     *   - EntityRendererManager::_applyZombieState 在僵尸模型分支读取此状态，
     *     推送给 ZombieModel::setAggressive，驱动 animateZombieArms 的空手攻击抬臂动画
     *     （aggressive 时基础抬臂角度为 -PI/1.5，否则 -PI/2.25）。
     *
     * 覆盖类型：僵尸、尸壳、溺尸、僵尸村民等所有继承自 ZombieEntity 的实体，
     * 以及未来任何需要在客户端表现攻击姿态的 Mob。
     *
     * @return 是否处于激怒状态
     */
    [[nodiscard]] bool isAggressive() const { return m_isAggressive; }

    /**
     * @brief 设置 Mob 是否处于激怒/攻击中状态
     *
     * @param aggressive 是否处于激怒状态
     */
    void setIsAggressive(bool aggressive) { m_isAggressive = aggressive; }

    // ========== 玩家渲染状态（供 PlayerRenderer 层 GPU 管线路径读取） ==========
    //
    // 这两个字段当前未通过网络元数据同步（MC 的 DATA_PLAYER_MODE_CUSTOMISATION /
    // DATA_PLAYER_MAIN_HAND 在本项目尚未实现协议同步）。本地玩家由
    // ClientApplicationSession 在每帧渲染前从真实 Player 对象回填（保证第三人称本地
    // 玩家层渲染正确）；远程玩家保持默认值（右撇子 + 全部件显示），与离线/单人默认
    // 皮肤场景一致。待玩家元数据协议补齐后在 syncMetadataFromDataManager 中刷新。

    /**
     * @brief 是否右撇子（主手为右手）
     *
     * 由 HeldItemLayer 读取以决定主/副手物品映射到左右手的渲染槽。
     * @return true 右撇子（默认）；本地玩家由真实 Player 回填
     */
    [[nodiscard]] bool isRightHanded() const { return m_rightHanded; }
    void setRightHanded(bool rightHanded) { m_rightHanded = rightHanded; }

    /**
     * @brief 玩家皮肤部件可见性位掩码（PlayerModelPart）
     *
     * 由 CapeLayer/PlayerModel 读取以决定披风/外套等外层部件是否渲染。
     * 默认 PLAYER_MODEL_PARTS_ALL_MASK（全部件显示）。
     * @return 部件位掩码
     */
    [[nodiscard]] u8 playerModelParts() const { return m_playerModelParts; }
    void setPlayerModelParts(u8 mask) { m_playerModelParts = mask; }
    [[nodiscard]] bool isWearing(PlayerModelPart part) const
    {
        return (m_playerModelParts & getPlayerModelPartMask(part)) != 0;
    }

    // ========== 钓鱼浮标状态 ==========

    /**
     * @brief 获取被钩住实体 ID（客户端镜像）
     *
     * 通过元数据同步自服务端 FishingBobberEntity::DATA_HOOKED_ENTITY_PARAM。
     * 由 syncMetadataFromDataManager 在收到元数据更新时写入。
     *
     * 值约定（对应 MC 1.21.11 FishingHook.DATA_HOOKED_ENTITY）：
     *   0  = 无被钩住实体
     *   >0 = 实体 ID + 1，使用时需减 1 得到真实实体 ID
     *
     * 渲染器（如 FishingBobberRenderer）可读取此值并通过世界查找实体，
     * 将钓线另一端连接到被钩住的实体（而非默认的钓鱼者位置上方）。
     *
     * @return 被钩住实体 ID（+1 偏移），0 表示无
     */
    [[nodiscard]] i32 fishingHookedEntityId() const { return m_fishingHookedEntityId; }

    /**
     * @brief 获取是否咬钩（客户端镜像）
     *
     * 通过元数据同步自服务端 FishingBobberEntity::DATA_BITING_PARAM。
     * 由 syncMetadataFromDataManager 在收到元数据更新时写入。
     *
     * 对应 MC 1.21.11 FishingHook.onSyncedDataUpdated(DATA_BITING)：
     * 咬钩时浮标获得向下速度（-0.4 * random[0.6,1.0]），渲染器可据此播放下沉动画。
     *
     * @return 如果浮标正处于咬钩状态返回 true
     */
    [[nodiscard]] bool fishingBiting() const { return m_fishingBiting; }

    // ========== 兔子跳跃动画状态 ==========

    /**
     * @brief 启动兔子跳跃动画
     *
     * 收到 RabbitJump(1) 状态包时调用，对应 MC 1.21.11 Rabbit.handleEntityEvent(byte 1)：
     *   jumpDuration = 10; jumpTicks = 0;
     *
     * 数据流：ClientApplicationNetwork.onEntityStatus(RabbitJump)
     * → setRabbitJumpStart() → ClientEntity::tick() 推进 jumpTicks
     * → rabbitJumpCompletion(partialTick) → EntityRendererManager 计算 jumpRotation
     * → RabbitModel::setJumpRotation → setAngles 中影响腿部旋转
     */
    void setRabbitJumpStart()
    {
        m_rabbitJumpDuration = 10;
        m_rabbitJumpTicks = 0;
    }

    /**
     * @brief 获取兔子跳跃动画完成度（0.0 ~ 1.0+）
     *
     * 对应 MC 1.21.11 Rabbit.getJumpCompletion(float partialTick)：
     *   jumpDuration == 0 ? 0.0F : (jumpTicks + partialTick) / jumpDuration
     *
     * 用于渲染线程计算 jumpRotation = sin(completion * PI)。
     *
     * @param partialTick 渲染部分 tick（0.0 ~ 1.0）
     * @return 跳跃动画完成度；若未在跳跃中（jumpDuration==0）返回 0
     */
    [[nodiscard]] f32 rabbitJumpCompletion(f32 partialTick) const
    {
        if (m_rabbitJumpDuration == 0) {
            return 0.0f;
        }
        return (static_cast<f32>(m_rabbitJumpTicks) + partialTick) / static_cast<f32>(m_rabbitJumpDuration);
    }

    /**
     * @brief 兔子是否正在跳跃动画中
     */
    [[nodiscard]] bool rabbitIsJumping() const { return m_rabbitJumpDuration != 0; }

    /**
     * @brief 推进兔子跳跃动画计时器（每 tick 调用一次）
     *
     * 对应 MC 1.21.11 Rabbit.aiStep() 中的跳跃推进逻辑：
     *   if (jumpTicks != jumpDuration) jumpTicks++;
     *   else if (jumpDuration != 0) { jumpTicks = 0; jumpDuration = 0; }
     *
     * 由 ClientEntity::tick() 调用。
     */
    void tickRabbitJump()
    {
        if (m_rabbitJumpTicks != m_rabbitJumpDuration) {
            ++m_rabbitJumpTicks;
        } else if (m_rabbitJumpDuration != 0) {
            m_rabbitJumpTicks = 0;
            m_rabbitJumpDuration = 0;
        }
    }

    // ========== 凋灵侧头朝向 ==========

    /**
     * @brief 获取凋灵侧头偏航角（度，已减去身体偏航角，插值后）
     *
     * 对应 MC 1.21.11 WitherRenderState.yHeadRots[index] - bodyRot。
     * 由 EntityRendererManager 在 wither 分支读取并填充 AnimationContext.witherSideHeadYaw。
     *
     * @param index 侧头索引 (0=左头, 1=右头)
     * @param partialTick 部分 tick (0.0-1.0)，用于插值
     * @return 插值后的偏航角（度）
     */
    [[nodiscard]] f32 getInterpolatedWitherSideHeadYaw(i32 index, f32 partialTick) const
    {
        return m_prevWitherSideHeadYaw[index] +
            (m_witherSideHeadYaw[index] - m_prevWitherSideHeadYaw[index]) * partialTick;
    }

    /**
     * @brief 获取凋灵侧头俯仰角（度，插值后）
     *
     * 对应 MC 1.21.11 WitherRenderState.xHeadRots[index]。
     *
     * @param index 侧头索引 (0=左头, 1=右头)
     * @param partialTick 部分 tick (0.0-1.0)，用于插值
     * @return 插值后的俯仰角（度）
     */
    [[nodiscard]] f32 getInterpolatedWitherSideHeadPitch(i32 index, f32 partialTick) const
    {
        return m_prevWitherSideHeadPitch[index] +
            (m_witherSideHeadPitch[index] - m_prevWitherSideHeadPitch[index]) * partialTick;
    }

    /**
     * @brief 推进凋灵侧头朝向（每 tick 调用一次）
     *
     * 由于客户端不运行 WitherEntity::aiStep()（ClientEntity 是独立代理类），
     * 此方法在 ClientEntityManager::tick() 中对凋灵实体调用，
     * 镜像 MC 1.21.11 WitherBoss.aiStep() 中 j=0..1 的侧头朝向计算逻辑：
     * - 备份 prev 值
     * - 若侧头有追踪目标（通过 HEAD_TARGET_2/3 元数据同步的目标 ID），
     *   从目标位置反算 yaw/pitch，用 rotlerp 逐步逼近
     * - 若无目标，yaw 逐步回正到身体偏航角（renderYawOffset = yaw）
     *
     * @param entityLookup 实体查找回调，返回指定 ID 的 ClientEntity 指针（可能为 nullptr）
     */
    void tickWitherSideHeads(const std::function<const ClientEntity*(EntityInstanceId)>& entityLookup);

private:
    // 基本信息
    EntityInstanceId m_id;
    std::string m_typeId;
    // 缓存的 EntityType 指针，由 entityType() 懒查询填充。mutable 以支持
    // const 方法内的懒查询。指向 EntityRegistry::m_types 内对象，地址稳定。
    mutable const entity::EntityType* m_entityType = nullptr;
    std::string m_uuid;

    // 位置
    Vector3 m_position;
    Vector3 m_prevPosition;
    Vector3 m_targetPosition; // 从网络包接收的目标位置

    // 平滑插值配置
    f32 m_interpolationSpeed = 0.3f;   // 插值速度 (0.0-1.0)
    bool m_smoothInterpolation = true; // 是否启用平滑插值

    // 旋转
    f32 m_yaw = 0.0f;
    f32 m_pitch = 0.0f;
    f32 m_prevYaw = 0.0f;
    f32 m_prevPitch = 0.0f;
    f32 m_headYaw = 0.0f; // 头部偏航角（动物特有）
    f32 m_prevHeadYaw = 0.0f;
    f32 m_targetYaw = 0.0f; // 目标旋转（用于平滑插值）
    f32 m_targetPitch = 0.0f;
    f32 m_targetHeadYaw = 0.0f;

    // 速度
    Vector3 m_velocity;

    // 动画状态
    f32 m_prevLimbSwing = 0.0f;       // 上一帧腿部摆动进度
    f32 m_limbSwing = 0.0f;           // 腿部摆动进度
    f32 m_prevLimbSwingAmount = 0.0f; // 上一帧腿部摆动强度
    f32 m_limbSwingAmount = 0.0f;     // 腿部摆动强度

    // 状态
    bool m_onGround = false;
    bool m_removed = false;
    bool m_child = false;

    // 尺寸
    f32 m_width = 0.6f;
    f32 m_height = 1.8f;
    f32 m_eyeHeight = 1.62f; // 眼睛高度，默认为玩家站立眼高

    // 受伤和死亡状态
    i32 m_hurtTime = 0;   // 受伤时间 (0-10)
    f32 m_hurtDir = 0.0f; // 受伤方向角（度，相对实体朝向）— damageTilt 用
    i32 m_deathTime = 0;  // 死亡时间

    // 行为状态
    bool m_sneaking = false;
    bool m_swimming = false;
    bool m_riding = false;
    bool m_sitting = false;
    bool m_sleeping = false; // 睡眠状态
    bool m_onFire = false;
    bool m_invisible = false;
    EntityInstanceId m_vehicleId = 0;     // 正在骑乘的载具ID
    std::vector<u32> m_passengers;        // 乘客列表（如果此实体是载具）
    EntityInstanceId m_leashHolderId = 0; // 拴绳持有者实体ID（0=未被拴，由 SetEntityLink 同步）
    BlockPos m_sleepingPosition{0, 0, 0}; // 睡眠位置（床的方块位置）

    // 攻击动画
    f32 m_swingProgress = 0.0f;
    f32 m_prevSwingProgress = 0.0f;
    i32 m_swingHand = 0;                             // 正在挥动的手 (0=主手, 1=副手)
    i32 m_swingTickCounter = 0;                      // 挥动动画计数器
    bool m_swingInProgress = false;                  // 是否正在挥动
    static constexpr i32 DEFAULT_SWING_DURATION = 6; // 默认挥动持续时间 (tick)

    // 游泳动画渐变量（客户端镜像 MC LivingEntity.swimAmount / swimAmountO）
    // 在 ClientEntity::tick() 中由 isVisuallySwimming() 驱动渐入渐出（±0.09/tick），
    // 渲染器通过 getInterpolatedSwimAmount(partialTicks) 插值读取，
    // 驱动 DrownedModel::setAngles 中的手臂/腿部游泳覆盖动画。
    f32 m_swimAmount = 0.0f;
    f32 m_swimAmountO = 0.0f;

    // 北极熊站立动画
    f32 m_clientSideStandAnimation0 = 0.0f;
    f32 m_clientSideStandAnimation = 0.0f;
    bool m_isStanding = false;

    // 河豚膨胀状态 (0=Deflated, 1=SemiPuffed, 2=FullyPuffed)
    i32 m_puffState = 0;

    // 吃草动画计时器（羊等实体的头部低头动画，收到 EatBlock 状态包时设为 40）
    i32 m_eatAnimationTimer = 0;

    // TNT 矿车引信计时器（-1 表示未引燃，收到 EatBlock 状态包时设为 80）
    i32 m_fuseTimer = -1;

    // 美西螈变体 (0=Lucy, 1=Wild, 2=Gold, 3=Cyan, 4=Blue)
    i32 m_axolotlVariant = 0;

    // 美西螈装死状态
    bool m_axolotlPlayingDead = false;

    // 豹猫信任状态
    bool m_trusting = false;

    // 猫动画状态
    bool m_catLieDown = false;       ///< 猫躺下状态
    bool m_catRelaxStateOne = false; ///< 猫放松状态

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
    std::optional<i32> m_metadataPickupDelay;
    std::optional<i32> m_metadataAge;
    u32 m_itemRenderStateVersion = 0;

    // ExperienceOrb 经验值数据
    i32 m_xpValue = 1; // 默认值为1

    // LightningBolt 闪电形状随机种子
    u64 m_boltVertex = 0;

    // 铁傀儡状态
    i32 m_ironGolemAttackTimer = 0;      // 攻击动画计时器（收到 IronGolemAttack 时设为 10）
    bool m_ironGolemArmsRaised = false;  // 是否举起手臂
    bool m_ironGolemHoldingRose = false; // 是否手持罂粟花

    // 疣猪兽/僵尸疣兽攻击动画
    i32 m_flingAnimationTicks = 0; // 撞飞攻击动画计时器（收到 HoglinAttack 时设为 10）

    // 狼甩水动画状态（对应 MC Wolf.isWet/isShaking/shakeAnim/shakeAnimO/interestedAngle/interestedAngleO）
    bool m_wolfIsShaking = false;    ///< 是否正在甩水（收到 ShakeOffWater(8) 时设 true）
    bool m_wolfIsWet = false;        ///< 是否湿润（收到 ShakeOffWater 时设 true，甩水完成时设 false）
    bool m_wolfIsInterested = false; ///< 是否感兴趣（乞求食物，通过元数据同步自服务端）
    bool m_wolfTamed = false;        ///< 是否已被驯服（通过元数据同步自服务端 TameableEntity::DATA_TAMED_PARAM）
    bool m_wolfIsAngry = false;      ///< 是否处于愤怒状态（通过元数据同步自服务端 WolfEntity::DATA_ANGER_TIME_PARAM）
    DyeColor m_wolfCollarColor =
        DyeColor::Red;                 ///< 颈圈颜色（通过元数据同步自服务端 WolfEntity::DATA_COLLAR_COLOR_PARAM）
    f32 m_wolfShakeAnim = 0.0f;        ///< 甩水动画进度（每 tick +0.05，达到 2.0 时完成）
    f32 m_wolfShakeAnimO = 0.0f;       ///< 上一 tick 的甩水进度（用于插值）
    f32 m_wolfInterestedAngle = 0.0f;  ///< 乞求食物头部角度（向 1.0 或 0.0 插值）
    f32 m_wolfInterestedAngleO = 0.0f; ///< 上一 tick 的乞求角度（用于插值）

    // 马类状态（通过元数据同步自服务端 AbstractHorseEntity::STATUS_PARAM，i8 位标志 6 bit）
    // 7 个马类子类共用同一 STATUS_PARAM，syncMetadataFromDataManager 经 typeId 判断 + hasParam
    // 读取后按 6 bit 掩码拆解写入下列 6 bool。HorseRenderer 鞍层等读取这些状态驱动渲染。
    bool m_horseTamed = false;     ///< 是否已驯服（bit1, mask 0x02）
    bool m_horseSaddled = false;   ///< 是否装备鞍（bit2, mask 0x04）
    bool m_horseBred = false;      ///< 是否已繁殖（bit3, mask 0x08）
    bool m_horseEating = false;    ///< 是否正在吃草（bit4, mask 0x10）
    bool m_horseRearing = false;   ///< 是否正在扬蹄（bit5, mask 0x20）
    bool m_horseMouthOpen = false; ///< 嘴巴是否张开（bit6, mask 0x40）

    // 末影人状态（通过元数据同步自服务端 EndermanEntity）
    const ::mc::BlockState* m_endermanHeldBlockState =
        nullptr; ///< 末影人持有的方块状态（通过元数据同步自服务端 EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM）
    bool m_endermanScreaming =
        false; ///< 末影人是否被注视（通过元数据同步自服务端 EndermanEntity::DATA_SCREAMING_PARAM）

    // 下落方块状态（通过元数据同步自服务端 FallingBlockEntity）
    // 服务端存储 BlockState 的 stateId（i32），0 表示未设置。
    // 客户端通过 BlockRegistry::getBlockState(stateId) 解析为 BlockState*。
    // 由 FallingBlockRenderer 读取以渲染下落方块模型。
    const ::mc::BlockState* m_fallingBlockState = nullptr;

    // TNT 实体状态（通过元数据同步自服务端 TNTEntity）
    // m_tntFuse：引信剩余 tick（对应 MC 1.21.11 PrimedTnt.DATA_FUSE_ID）
    // m_tntBlockState：TNT 方块状态（对应 MC 1.21.11 PrimedTnt.DATA_BLOCK_STATE_ID）
    // 由 TNTRenderer 读取以渲染 TNT 方块模型和闪烁动画。
    // 默认 0（未点燃），与服务端 DataParameter 默认值一致。
    i32 m_tntFuse = 0;
    const ::mc::BlockState* m_tntBlockState = nullptr;

    // Mob 激怒/攻击中状态（通过元数据同步自服务端 MobEntity::DATA_MOB_FLAGS_PARAM 位 2）
    // 对应 MC 1.21.11 Mob.isAggressive()。由 MeleeAttackGoal 等攻击目标在 start/reset 时设置，
    // 驱动 ZombieModel 的空手攻击抬臂动画（animateZombieArms）。
    bool m_isAggressive = false; ///< 是否处于激怒状态（驱动 ZombieModel 攻击手臂动画）

    // 玩家渲染状态（供 PlayerRenderer 层 GPU 管线路径读取，见 isRightHanded/playerModelParts 注释）
    bool m_rightHanded = true;                           ///< 右撇子（默认）；本地玩家由真实 Player 回填
    u8 m_playerModelParts = PLAYER_MODEL_PARTS_ALL_MASK; ///< 皮肤部件位掩码（默认全部件显示）

    // 兔子跳跃动画状态（对应 MC 1.21.11 Rabbit.jumpTicks / jumpDuration）
    // 收到 RabbitJump(1) 状态包时启动；由 tickRabbitJump() 在 ClientEntity::tick() 中推进
    i32 m_rabbitJumpTicks = 0;    ///< 当前跳跃已持续的 tick
    i32 m_rabbitJumpDuration = 0; ///< 当前跳跃总持续 tick；为 0 表示未在跳跃中

    // 凋灵侧头朝向（对应 MC 1.21.11 WitherBoss.yRotHeads/xRotHeads[2]）
    // 客户端不运行 WitherEntity::aiStep()，由 tickWitherSideHeads() 本地镜像计算。
    // index 0 = 左头（对应 m_heads[1]），index 1 = 右头（对应 m_heads[2]）。
    std::array<f32, 2> m_witherSideHeadYaw = {0.0f, 0.0f};
    std::array<f32, 2> m_witherSideHeadPitch = {0.0f, 0.0f};
    std::array<f32, 2> m_prevWitherSideHeadYaw = {0.0f, 0.0f};
    std::array<f32, 2> m_prevWitherSideHeadPitch = {0.0f, 0.0f};

    // 凋灵三头追踪目标实体 ID（通过 HEAD_TARGET_1/2/3 元数据同步）
    // index 0 = 主头，index 1 = 左头，index 2 = 右头。
    // 由 syncMetadataFromDataManager 读取，供 tickWitherSideHeads 查找目标位置。
    std::array<i32, 3> m_witherHeadTargetId = {0, 0, 0};

    // 钓鱼浮标状态（通过 DATA_HOOKED_ENTITY / DATA_BITING 元数据同步自服务端 FishingBobberEntity）
    // 对应 MC 1.21.11 FishingHook.onSyncedDataUpdated():
    //   m_fishingHookedEntityId: 被钩住实体 ID（+1 偏移，0=无），供渲染器查找另一端实体位置。
    //   m_fishingBiting: 是否咬钩，供渲染器播放咬钩下沉动画。
    i32 m_fishingHookedEntityId = 0; ///< 被钩住实体 ID（0 表示无）
    bool m_fishingBiting = false;    ///< 是否咬钩

    // 追踪位置系统（用于披风摆动）
    f64 m_chasingPosX = 0.0;
    f64 m_chasingPosY = 0.0;
    f64 m_chasingPosZ = 0.0;
    f64 m_prevChasingPosX = 0.0;
    f64 m_prevChasingPosY = 0.0;
    f64 m_prevChasingPosZ = 0.0;

    // 相机偏航角系统（用于披风摆动）
    f32 m_cameraYaw = 0.0f;
    f32 m_prevCameraYaw = 0.0f;

    // 鞘翅角度系统（用于鞘翅展开动画）
    f32 m_rotateElytraX = 0.0f;
    f32 m_rotateElytraY = 0.0f;
    f32 m_rotateElytraZ = 0.0f;

    // 悬浮起始偏移（用于 ItemEntity 浮动动画随机化）
    f32 m_hoverStart = 0.0f;

    // 原始元数据缓存
    std::vector<u8> m_metadata;

    // 解析后的元数据
    entity::EntityDataManager m_dataManager;

    void _updateItemRenderStateVersion();

    // 类型安全地按参数 ID 读取元数据值。
    // 客户端 m_dataManager 的槽位索引来自服务端数据包字节索引，与服务端 C++ DataParameter::id()
    // （全局自增 s_nextId）并不一致，因此 getRaw(id) 取到的可能是任意类型。仅当槽位存在且
    // 存储类型与请求类型一致时返回值，否则返回 nullopt，避免 get<T>() 抛 bad_variant_access。
    template <typename T>
    [[nodiscard]] std::optional<T> _readMetadata(u16 id) const
    {
        const auto* value = m_dataManager.getRaw(id);
        if (value == nullptr) {
            return std::nullopt;
        }
        if (!std::holds_alternative<T>(value->value())) {
            return std::nullopt;
        }
        return value->get<T>();
    }
};

} // namespace mc::client
