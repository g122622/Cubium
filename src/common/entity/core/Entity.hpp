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

#include "../../core/Result.hpp"
#include "../../core/Types.hpp"
#include "../../item/core/ActionResult.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../sound/SoundCategory.hpp"
#include "../../util/AxisAlignedBB.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/nbt/Nbt.hpp"
#include "../../util/text/ITextComponent.hpp"
#include "../../world/block/BlockPos.hpp"
#include "EntityDataManager.hpp"
#include "EntityPose.hpp"
#include "EntitySize.hpp"
#include "EntityTypeIdNumber.hpp"
#include "MoverType.hpp"
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace mc {

// 前向声明
class PhysicsEngine;
class IWorld;
class BlockState;
class DamageSource;

namespace entity {

// 前向声明 EntityTypeId
using EntityTypeId = u16;

} // namespace entity

namespace scoreboard {
class Team;
} // namespace scoreboard

/**
 * @brief 实体推动反应类型
 *
 * 定义实体被活塞等推动时的反应行为。
 */
enum class PushReaction : u8 {
    Normal,  // 正常推动
    Destroy, // 被推动时销毁
    Ignore   // 忽略推动
};

// 引入 mc::entity::EntityPose 到 mc 命名空间以保持兼容
using EntityPose = entity::EntityPose;

// ============================================================================
// 实体标志位
// ============================================================================

enum class EntityFlags : u8 {
    None = 0,
    OnFire = 1 << 0,
    Crouching = 1 << 1,
    Sprinting = 1 << 3,
    Swimming = 1 << 4,
    Invisible = 1 << 5,
    Glowing = 1 << 6,
    FallFlying = 1 << 7
};

inline EntityFlags operator|(EntityFlags a, EntityFlags b)
{
    return static_cast<EntityFlags>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline EntityFlags operator&(EntityFlags a, EntityFlags b)
{
    return static_cast<EntityFlags>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool hasFlag(EntityFlags flags, EntityFlags flag)
{
    return (static_cast<u8>(flags) & static_cast<u8>(flag)) != 0;
}

// ============================================================================
// 实体基类
// ============================================================================

/**
 * @brief 实体基类
 *
 * 所有游戏实体（玩家、生物、物品等）的基类。
 * 提供位置、速度、旋转等基本属性，以及碰撞检测和物理模拟支持。
 *
 * 注意：
 * - 位置使用 Vector3 存储，但组件是 f64 精度
 * - 碰撞箱使用 AxisAlignedBB（f32 精度）
 * - 子类应重写 width()、height()、eyeHeight() 方法
 *
 * 数据参数（通过 EntityDataManager 同步）：
 * - FLAGS: 实体标志（燃烧、潜行等）
 * - AIR: 空气值
 * - CUSTOM_NAME: 自定义名称
 * - CUSTOM_NAME_VISIBLE: 名称可见性
 * - SILENT: 静音标志
 * - NO_GRAVITY: 无重力标志
 * - POSE: 姿态
 */
class Entity {
public:
    // ========== 静态数据参数 ==========
    // 子类应定义自己的数据参数

    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param world 世界指针（可选）
     */
    Entity(EntityId id, IWorld* world = nullptr);
    virtual ~Entity() = default;

    // 禁止拷贝
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // 禁止移动（因为 EntityDataManager 包含 std::mutex）
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 注册数据参数
     *
     * 子类应重写此方法来注册自己的数据参数。
     * 在构造函数中调用。
     */
    virtual void registerData();

    // ========== 基本属性 ==========

    [[nodiscard]] EntityId id() const { return m_id; }
    /**
     * @brief 设置实体ID
     *
     * 仅由EntityManager在分配ID时调用。
     * 不应该在其他地方使用。
     */
    void setId(EntityId id) { m_id = id; }
    [[nodiscard]] const std::string& uuid() const { return m_uuid; }
    void setUuid(const std::string& uuid) { m_uuid = uuid; }

    /**
     * @brief 获取实体类型标识符
     * @return 实体类型字符串（用于网络同步和渲染）
     */
    [[nodiscard]] virtual std::string getTypeId() const;

    /**
     * @brief 设置实体类型标识符
     *
     * 当实体由注册表工厂创建时，调用方应传入注册名（如 minecraft:pig）。
     */
    void setTypeId(const std::string& typeId) { m_typeId = typeId; }

    /**
     * @brief 获取实体类型数字ID
     * @return 实体类型数字ID，用于快速类型比较
     *
     * 使用示例：
     * @code
     * if (entity->typeId() == entity::EntityTypeIdNumber::PIG) {
     *     // 处理猪
     * }
     * @endcode
     */
    [[nodiscard]] entity::EntityTypeId typeId() const;

    // ========== 世界访问 ==========

    [[nodiscard]] IWorld* world() { return m_world; }
    [[nodiscard]] const IWorld* world() const { return m_world; }
    void setWorld(IWorld* world) { m_world = world; }

    // ========== 数据管理 ==========

    [[nodiscard]] entity::EntityDataManager& dataManager() { return m_dataManager; }
    [[nodiscard]] const entity::EntityDataManager& dataManager() const { return m_dataManager; }

    // ========== 位置 ==========

    [[nodiscard]] Vector3 position() const { return m_position; }
    [[nodiscard]] f32 x() const { return m_position.x; }
    [[nodiscard]] f32 y() const { return m_position.y; }
    [[nodiscard]] f32 z() const { return m_position.z; }

    // 前一帧位置（用于插值）
    [[nodiscard]] Vector3 prevPosition() const { return m_prevPosition; }
    [[nodiscard]] f32 prevX() const { return m_prevPosition.x; }
    [[nodiscard]] f32 prevY() const { return m_prevPosition.y; }
    [[nodiscard]] f32 prevZ() const { return m_prevPosition.z; }

    /**
     * @brief 计算到另一个实体的距离
     * @param other 另一个实体
     * @return 距离（非平方）
     */
    [[nodiscard]] f32 distanceTo(const Entity& other) const { return m_position.distance(other.m_position); }

    /**
     * @brief 计算到另一个实体的距离平方
     * @param other 另一个实体
     * @return 距离的平方（避免开方运算，适合比较）
     */
    [[nodiscard]] f32 distanceSqTo(const Entity& other) const { return m_position.distanceSquared(other.m_position); }

    /**
     * @brief 计算到指定位置的距离平方
     * @param px 目标X坐标
     * @param py 目标Y坐标
     * @param pz 目标Z坐标
     * @return 距离的平方
     */
    [[nodiscard]] f32 distanceSqTo(f32 px, f32 py, f32 pz) const
    {
        f32 dx = px - m_position.x;
        f32 dy = py - m_position.y;
        f32 dz = pz - m_position.z;
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 计算到指定位置的水平距离平方（忽略Y轴）
     */
    [[nodiscard]] f32 distanceHorizontalSqTo(f32 px, f32 pz) const
    {
        f32 dx = px - m_position.x;
        f32 dz = pz - m_position.z;
        return dx * dx + dz * dz;
    }

    // ========== 旋转 ==========

    [[nodiscard]] f32 yaw() const { return m_yaw; }
    [[nodiscard]] f32 pitch() const { return m_pitch; }
    [[nodiscard]] f32 prevYaw() const { return m_prevYaw; }
    [[nodiscard]] f32 prevPitch() const { return m_prevPitch; }

    // ========== 速度 ==========

    [[nodiscard]] Vector3 velocity() const { return m_velocity; }
    [[nodiscard]] f32 velocityX() const { return m_velocity.x; }
    [[nodiscard]] f32 velocityY() const { return m_velocity.y; }
    [[nodiscard]] f32 velocityZ() const { return m_velocity.z; }

    // ========== 状态 ==========

    [[nodiscard]] bool onGround() const { return m_onGround; }
    [[nodiscard]] bool isRemoved() const { return m_removed; }
    [[nodiscard]] EntityPose pose() const { return m_pose; }
    [[nodiscard]] EntityFlags flags() const { return m_flags; }
    [[nodiscard]] virtual bool isChild() const { return false; }

    // ========== 声音 ==========

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] virtual sound::SoundCategory getSoundCategory() const { return sound::SoundCategory::Neutral; }

    /**
     * @brief 播放实体声音
     * @param soundEventId 声音事件ID
     * @param volume 音量倍率
     * @param pitch 音调倍率
     */
    void playSound(const ResourceLocation& soundEventId, f32 volume, f32 pitch) const;

    /**
     * @brief 获取溅水声音
     *
     * 子类可覆盖以提供特定溅水音效。
     *
     * @return 溅水声音事件
     */
    [[nodiscard]] virtual ResourceLocation getSplashSound() const;

    /**
     * @brief 获取高速溅水声音
     *
     * 当实体高速入水时播放此音效。
     * 子类可覆盖以提供特定高速溅水音效。
     *
     * @return 高速溅水声音事件
     */
    [[nodiscard]] virtual ResourceLocation getHighspeedSplashSound() const;

    /**
     * @brief 执行水花溅射效果
     *
     * 当实体入水时播放水花音效并生成气泡和水溅粒子。
     *
     * 实现细节：
     * - 根据实体速度计算溅水强度 f1
     * - f1 < 0.25 时播放普通溅水声，否则播放高速溅水声
     * - 音量使用 f1，音调使用 1.0 + (rand - rand) * 0.4
     * - 生成 (1 + width * 20) 个气泡粒子和水溅粒子
     * - 粒子位置在实体包围盒内随机，Y 坐标固定在水面上方
     *
     * Player 类覆盖此方法以检查观察者模式。
     */
    virtual void doWaterSplashEffect();

    // ========== 设置属性 ==========

    void setPosition(f32 x, f32 y, f32 z);
    void setPosition(const Vector3& pos) { setPosition(pos.x, pos.y, pos.z); }

    /**
     * @brief 将当前位置与旋转记录为下一次渲染插值的起点
     */
    void snapshotInterpolationState();

    void setRotation(f32 yaw, f32 pitch);
    void setVelocity(f32 x, f32 y, f32 z);
    void setVelocity(const Vector3& vel) { setVelocity(vel.x, vel.y, vel.z); }

    /**
     * @brief 添加速度增量
     * @param dx X方向增量
     * @param dy Y方向增量
     * @param dz Z方向增量
     */
    void addVelocity(f32 dx, f32 dy, f32 dz)
    {
        m_velocity.x += dx;
        m_velocity.y += dy;
        m_velocity.z += dz;
    }

    /**
     * @brief 添加速度增量
     * @param delta 速度增量向量
     */
    void addVelocity(const Vector3& delta) { addVelocity(delta.x, delta.y, delta.z); }

    /**
     * @brief 缩放速度向量
     * @param factor 缩放因子
     *
     * 用于阻力计算。
     */
    void scaleVelocity(f32 factor)
    {
        m_velocity.x *= factor;
        m_velocity.y *= factor;
        m_velocity.z *= factor;
    }

    /**
     * @brief 根据偏航角计算相对移动并添加到速度
     *
     * 根据实体的偏航角，将相对移动向量转换为绝对移动向量，
     * 并乘以移动因子后添加到当前速度。
     *
     * @param factor 移动因子（速度倍率）
     * @param strafe 左右移动（负值向左，正值向右）
     * @param vertical 垂直移动（负值向下，正值向上）
     * @param forward 前后移动（负值向后，正值向前）
     */
    void moveRelative(f32 factor, f32 strafe, f32 vertical, f32 forward);

    /**
     * @brief 设置是否在地面上
     *
     * 当实体落地时（onGround从false变为true），
     * 会清空攀爬位置追踪。
     *
     * @param onGround 是否在地面上
     */
    void setOnGround(bool onGround)
    {
        if (onGround && !m_onGround) {
            // 落地时清空攀爬位置
            m_lastClimbPos = std::nullopt;
        }
        m_onGround = onGround;
    }
    void setPose(EntityPose pose);
    void setFlags(EntityFlags flags);

    // 标志操作
    void addFlag(EntityFlags flag);
    void removeFlag(EntityFlags flag);
    [[nodiscard]] bool hasFlag(EntityFlags flag) const { return mc::hasFlag(m_flags, flag); }

    /**
     * @brief 检查是否正在鞘翅飞行
     *
     * 通过检查 FallFlying 标志位（第7位）来判断。
     *
     * @return 如果正在鞘翅飞行返回 true
     */
    [[nodiscard]] bool isElytraFlying() const { return hasFlag(EntityFlags::FallFlying); }

    /**
     * @brief 检查实体是否发光
     *
     * 发光效果来源：
     * 1. 发光药水效果 (StatusEffectType::GLOWING)
     * 2. 发光鱿鱼实体类型
     * 3. 团队发光规则
     *
     * 客户端检查数据参数中的 Glowing 标志位，
     * 服务端检查 m_glowing 字段。
     *
     * @return 如果实体发光返回 true
     */
    [[nodiscard]] bool isGlowing() const;

    /**
     * @brief 设置实体发光状态
     *
     * 在服务端设置 m_glowing 字段并同步 Glowing 标志位。
     *
     * @param glowing 是否发光
     */
    void setGlowing(bool glowing);

    /**
     * @brief 获取实体所属队伍
     *
     * 基类默认返回 nullptr。
     * ServerPlayer 子类重写此方法，通过服务器记分板获取玩家所在队伍。
     * TameableEntity 子类重写此方法，继承主人的队伍。
     *
     * @return 队伍指针，如果实体不在任何队伍返回 nullptr
     */
    [[nodiscard]] virtual scoreboard::Team* getTeam() { return nullptr; }
    [[nodiscard]] virtual const scoreboard::Team* getTeam() const { return nullptr; }

    /**
     * @brief 检查实体是否在指定队伍中
     *
     * @param team 要检查的队伍
     * @return true 如果实体属于该队伍
     */
    [[nodiscard]] bool isOnScoreboardTeam(const scoreboard::Team* team) const;

    /**
     * @brief 检查两个实体是否为盟友关系
     *
     * 双向检查：this 认为 other 是盟友，或 other 认为 this 是盟友。
     * 子类可重写 considersEntityAsAlly 来自定义盟友判定逻辑。
     *
     * @param other 另一个实体
     * @return true 如果两个实体互为盟友
     */
    [[nodiscard]] bool isAlliedTo(const Entity& other) const;

    /**
     * @brief 检查此实体是否将指定队伍视为盟友
     *
     * 默认实现通过 getTeam() 判断是否同一队伍。
     * 子类可重写此方法以支持更复杂的盟友关系（如驯服动物继承主人的队伍）。
     *
     * @param team 要检查的队伍
     * @return true 如果此实体属于该队伍或与该队伍为盟友关系
     */
    [[nodiscard]] virtual bool isAlliedTo(const scoreboard::Team* team) const;

    /**
     * @brief 检查此实体是否将指定实体视为盟友
     *
     * 默认实现调用 isAlliedTo(other.getTeam())。
     * 子类可重写此方法以支持更复杂的盟友关系。
     *
     * @param other 另一个实体
     * @return true 如果此实体将 other 视为盟友
     */
    [[nodiscard]] virtual bool considersEntityAsAlly(const Entity& other) const;

    /**
     * @brief 检查两个实体是否在同一队伍
     *
     * 旧版 API，仅检查 this 是否属于 other 的队伍。
     * 新代码应优先使用 isAlliedTo() 进行双向检查。
     *
     * @param other 另一个实体
     * @return true 如果两个实体在同一队伍
     */
    [[nodiscard]] bool isOnSameTeam(const Entity& other) const;

    // ========== 尺寸 ==========

    /**
     * @brief 获取实体宽度
     * @return 实体宽度（方块单位）
     */
    [[nodiscard]] virtual f32 width() const { return 0.6f; }

    /**
     * @brief 获取实体高度
     * @return 实体高度（方块单位）
     */
    [[nodiscard]] virtual f32 height() const { return 1.8f; }

    /**
     * @brief 获取眼睛高度
     * @return 眼睛距离脚底的高度（方块单位）
     */
    [[nodiscard]] virtual f32 eyeHeight() const { return 1.62f; }

    /**
     * @brief 获取指定姿态下的实体尺寸
     * @param pose 目标姿态
     * @return 对应姿态的尺寸信息
     */
    [[nodiscard]] virtual entity::EntitySize getDimensions(EntityPose pose) const;

    /**
     * @brief 获取当前缓存的实体尺寸
     * @return 当前尺寸
     */
    [[nodiscard]] entity::EntitySize currentDimensions() const { return m_dimensions; }

    /**
     * @brief 刷新当前尺寸和碰撞箱
     *
     * 当姿态或其他会影响尺寸的状态变化时调用。
     */
    void refreshDimensions();

    /**
     * @brief 获取步进高度
     * @return 实体可以自动步进的最大高度（默认为0，LivingEntity默认为0.6）
     */
    [[nodiscard]] virtual f32 stepHeight() const { return m_stepHeight; }

    /**
     * @brief 设置步进高度
     *
     * 用于设置实体可以自动步进的最大高度：
     * - 玩家/大多数生物：0.6（可走上台阶）
     * - 马、铁傀儡、末影人等：1.0（可走上完整方块）
     * - 盔甲架：0.0（无法步进）
     *
     * @param height 步进高度
     */
    void setStepHeight(f32 height) { m_stepHeight = height; }

    // ========== 碰撞箱 ==========

    /**
     * @brief 获取实体碰撞箱
     * @return 基于当前位置的AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB boundingBox() const
    {
        if (!m_dimensionsInitialized) {
            const_cast<Entity*>(this)->refreshDimensions();
        }
        return m_boundingBox;
    }

    /**
     * @brief 实体是否可被碰撞或射线命中
     */
    [[nodiscard]] virtual bool canBeCollidedWith() const { return true; }

    /**
     * @brief 命中检测时额外扩张的碰撞边界
     */
    [[nodiscard]] virtual f32 getCollisionBorderSize() const { return 0.0f; }

    // ========== 更新 ==========

    virtual void tick();
    virtual void update();

    // ========== 移动 ==========

    /**
     * @brief 直接移动实体（无碰撞检测）
     * @param dx, dy, dz 移动增量
     */
    void move(f32 dx, f32 dy, f32 dz);

    /**
     * @brief 使用指定移动类型移动实体
     *
     * 用于区分不同来源的移动（活塞推动、玩家推动、自身移动等）。
     *
     * @param type 移动类型
     * @param delta 移动增量
     */
    void move(entity::MoverType type, const Vector3& delta);

    /**
     * @brief 旋转实体
     * @param deltaYaw 偏航角增量（度）
     * @param deltaPitch 俯仰角增量（度）
     */
    void rotate(f32 deltaYaw, f32 deltaPitch);

    // ========== 推动反应 ==========

    /**
     * @brief 获取实体的推动反应类型
     *
     * 子类可重写以返回不同的推动反应。
     *
     * @return 推动反应类型
     */
    [[nodiscard]] virtual PushReaction getPushReaction() const { return PushReaction::Normal; }

    // ========== 物理 ==========

    /**
     * @brief 设置物理引擎（已废弃，优先使用 World 的物理引擎）
     * @param engine 物理引擎指针（不拥有所有权）
     * @deprecated 使用 World::physicsEngine() 替代
     */
    void setPhysicsEngine(PhysicsEngine* engine) { m_physicsEngine = engine; }

    /**
     * @brief 获取物理引擎
     *
     * 优先返回 World 的物理引擎，如果没有则返回显式设置的物理引擎。
     */
    [[nodiscard]] PhysicsEngine* physicsEngine();
    [[nodiscard]] const PhysicsEngine* physicsEngine() const;

    /**
     * @brief 带碰撞检测的移动
     *
     * 使用物理引擎进行带碰撞检测的移动。
     * 会更新 m_onGround、m_collidedHorizontally、m_collidedVertically 状态。
     *
     * @param dx, dy, dz 期望移动增量
     * @return 实际移动增量
     */
    Vector3 moveWithCollision(f32 dx, f32 dy, f32 dz);

    /**
     * @brief 应用物理效果（重力、阻力）
     *
     * 更新速度：应用重力和空气阻力。
     * 如果在地面，重置Y速度为0。
     *
     * @param deltaTime 时间增量（秒）
     */
    void applyPhysics(f32 deltaTime);

    /**
     * @brief 检测是否在地面上
     *
     * 通过检测实体下方是否有方块碰撞来判断。
     * 如果 World 存在，使用 World 的碰撞检测；
     * 否则使用物理引擎（如果有）。
     */
    void checkOnGround();

    // ========== 碰撞状态 ==========

    [[nodiscard]] bool collidedHorizontally() const { return m_collidedHorizontally; }
    [[nodiscard]] bool collidedVertically() const { return m_collidedVertically; }
    [[nodiscard]] f32 fallDistance() const { return m_fallDistance; }

    /**
     * @brief 设置摔落距离
     * @param distance 摔落距离
     */
    void setFallDistance(f32 distance) { m_fallDistance = distance; }

    // ========== 移除 ==========

    /**
     * @brief 移除实体
     *
     * 标记实体为已移除状态。子类可以重写此方法在移除前执行额外逻辑
     * （例如史莱姆分裂）。
     */
    virtual void remove() { m_removed = true; }

    /**
     * @brief 由 /kill 命令调用
     *
     * 默认实现直接调用 remove()。
     * LivingEntity 重写此方法使用虚空伤害杀死实体。
     */
    virtual void onKillCommand() { remove(); }

    // ========== 维度 ==========

    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dimension) { m_dimension = dimension; }

    // ========== 传送门 ==========

    /**
     * @brief 获取传送冷却时间
     * @return 剩余冷却时间（tick），0 表示可以传送
     */
    [[nodiscard]] i32 portalCooldown() const { return m_portalCooldown; }

    /**
     * @brief 设置传送冷却时间
     * @param cooldown 冷却时间（tick）
     */
    void setPortalCooldown(i32 cooldown) { m_portalCooldown = cooldown; }

    /**
     * @brief 检查是否可以传送
     * @return 如果冷却时间为 0 则返回 true
     */
    [[nodiscard]] bool canTeleport() const { return m_portalCooldown <= 0; }

    /**
     * @brief 获取在传送门中的累计时间
     * @return 累计时间（tick）
     */
    [[nodiscard]] i32 portalTime() const { return m_portalTime; }

    /**
     * @brief 设置在传送门中的累计时间
     * @param time 累计时间（tick）
     */
    void setPortalTime(i32 time) { m_portalTime = time; }

    /**
     * @brief 重置传送门计时
     */
    void resetPortalTime() { m_portalTime = 0; }

    /**
     * @brief 检查是否在传送门中
     */
    [[nodiscard]] bool isInPortal() const { return m_inPortal; }

    /**
     * @brief 设置是否在传送门中
     */
    void setInPortal(bool inPortal) { m_inPortal = inPortal; }

    /**
     * @brief 获取在传送门中停留所需的最大时间
     *
     * 玩家需要 80 tick (4秒) 在传送门中才能传送。
     * 其他实体只需要 1 tick（因为基类返回 0，递增后立即满足条件）。
     *
     * @return 传送所需的最大时间（tick）
     */
    [[nodiscard]] virtual i32 getMaxInPortalTime() const { return 0; }

    /**
     * @brief 处理传送门 tick
     *
     * 每帧调用，更新传送冷却和传送门计时。
     * 玩家需要 80 tick (4秒) 在传送门中才能传送。
     * 其他实体需要约 1 tick。
     *
     * @return true 如果应该触发传送
     */
    virtual bool tickPortal();

    /**
     * @brief 当传送门触发时调用
     *
     * 当实体在传送门中停留足够时间后触发。
     * 子类（如 ServerPlayer）可重写此方法以实现实际的维度切换逻辑。
     *
     * @return true 如果传送成功
     */
    virtual bool onPortalTriggered();

    /**
     * @brief 设置实体所在的传送门方块位置
     *
     * 当实体进入传送门方块时调用。
     *
     * @param pos 传送门方块位置
     */
    void setPortalPos(const BlockPos& pos) { m_portalPos = pos; }

    /**
     * @brief 获取实体所在的传送门方块位置
     */
    [[nodiscard]] const BlockPos& portalPos() const { return m_portalPos; }

    /**
     * @brief 触发传送冷却
     *
     * 传送后设置冷却时间，防止立即再次传送。
     * 默认冷却时间为 300 tick (15秒)。
     */
    void triggerPortalCooldown() { m_portalCooldown = getPortalCooldown(); }

    /**
     * @brief 获取默认传送冷却时间
     */
    [[nodiscard]] virtual i32 getPortalCooldown() const { return 300; }

    /**
     * @brief 检查实体是否不是Boss
     *
     * Boss实体（末影龙、凋灵）不能使用传送门。
     * 默认返回 true，Boss实体覆盖返回 false。
     *
     * @return 如果实体不是Boss返回 true
     */
    [[nodiscard]] virtual bool isNonBoss() const { return true; }

    // ========== 随机传送 ==========

    /**
     * @brief 尝试传送到指定位置
     *
     * 安全传送流程：
     * 1. 检查目标区块是否已加载
     * 2. 从目标位置向下查找固体地面
     * 3. 检查目标位置是否有碰撞
     * 4. 检查目标位置是否在液体中
     * 5. 如果检查失败，恢复原位置
     *
     * @param x 目标 X 坐标
     * @param y 目标 Y 坐标
     * @param z 目标 Z 坐标
     * @param playEffects 是否播放传送效果（粒子、音效）
     * @return 如果传送成功返回 true
     */
    bool attemptTeleport(f64 x, f64 y, f64 z, bool playEffects = true);

    /**
     * @brief 随机传送到附近位置
     *
     * 在指定范围内随机选择位置尝试传送：
     * - 水平方向：以实体为中心，±range 格范围
     * - 垂直方向：以实体为中心，±range 格范围
     *
     * 最多尝试 16 次，找到第一个有效位置后立即传送。
     * 如果所有尝试都失败，保持原位置不变。
     *
     * @param range 随机范围（水平和垂直）
     * @param playEffects 是否播放传送效果
     * @param avoidFluid 是否避免液体（水、岩浆）
     * @return 如果传送成功返回 true
     */
    bool randomTeleport(f64 range, bool playEffects = true, bool avoidFluid = true);

    /**
     * @brief 查找安全的传送位置
     *
     * 从目标位置向下查找第一个安全的站立位置。
     * 安全位置定义：
     * - 有固体方块作为脚底
     * - 脚底上方两格无碰撞
     * - 不在液体中
     *
     * @param x 目标 X 坐标
     * @param y 目标 Y 坐标（起始搜索高度）
     * @param z 目标 Z 坐标
     * @param avoidFluid 是否避免液体
     * @return 安全位置，如果找不到返回 nullopt
     */
    [[nodiscard]] std::optional<Vector3d> findSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid = true) const;

    /**
     * @brief 检查位置是否可以安全站立
     *
     * 检查实体在该位置是否会有碰撞和液体问题。
     *
     * @param x 目标 X 坐标
     * @param y 目标 Y 坐标（脚底高度）
     * @param z 目标 Z 坐标
     * @param avoidFluid 是否检查液体
     * @return 如果位置安全返回 true
     */
    [[nodiscard]] bool isSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid = true) const;

    // ========== 存活时间 ==========

    [[nodiscard]] u32 ticksExisted() const { return m_ticksExisted; }

    // ========== 存活状态 ==========

    /**
     * @brief 检查实体是否存活
     * @return 如果实体未被移除且未死亡则返回 true
     */
    [[nodiscard]] virtual bool isAlive() const { return !m_removed; }

    // ========== 玩家碰撞 ==========

    /**
     * @brief 当玩家与此实体碰撞时调用
     *
     * 用于处理玩家拾取物品、经验球、箭矢等。
     * 子类可重写以实现特定的碰撞行为。
     *
     * @param player 与此实体碰撞的玩家
     */
    virtual void onCollideWithPlayer(class Player& player)
    {
        // 默认实现：无操作
        (void)player;
    }

    // ========== 玩家交互 ==========

    /**
     * @brief 处理玩家初始交互
     *
     * 当玩家右键点击实体时首先调用此方法。
     * 子类可重写此方法处理特定的交互行为（如骑乘、打开容器等）。
     *
     * 基类默认实现返回 Pass，表示不处理交互。
     *
     * @param player 与此实体交互的玩家
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    virtual ActionResultType processInitialInteract(class Player& player, Hand hand);

    /**
     * @brief 处理玩家指定位置的交互
     *
     * 当玩家右键点击实体的特定位置时调用。
     * 基类默认调用 processInitialInteract。
     * 子类可重写此方法处理基于点击位置的交互（如盔甲架装备槽）。
     *
     * hitPosition 是相对于实体坐标的局部坐标（0 到 实体尺寸 的范围），
     * 可用于确定玩家点击的是实体的哪个部位。
     *
     * @param player 与此实体交互的玩家
     * @param hitPosition 点击位置（相对于实体坐标系）
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    virtual ActionResultType applyPlayerInteraction(class Player& player, const Vector3& hitPosition, Hand hand);

    // ========== 环境检测 ==========

    /**
     * @brief 检查实体是否在水中
     *
     * 需要世界引用才能正常工作
     */
    [[nodiscard]] virtual bool isInWater() const { return m_inWater; }

    /**
     * @brief 设置水中状态（测试用）
     *
     * 正常情况下应该通过 updateEnvironmentState() 自动更新。
     * 此方法主要用于测试目的。
     */
    void setInWater(bool inWater) { m_inWater = inWater; }

    /**
     * @brief 检查实体是否在岩浆中
     */
    [[nodiscard]] virtual bool isInLava() const { return m_inLava; }

    /**
     * @brief 检查实体是否在雨中
     *
     * 需要满足：世界正在下雨 + 实体位置可以看到天空 + 生物群系允许降水
     *
     * @return 如果实体在雨中返回 true
     */
    [[nodiscard]] bool isInRain() const;

    /**
     * @brief 检查实体是否湿润
     *
     * 用于三叉戟激流附魔、末影人躲避等逻辑
     *
     * @return 如果实体在水中或雨中返回 true
     */
    [[nodiscard]] bool isWet() const { return m_inWater || isInRain(); }

    /**
     * @brief 检查眼睛是否在水下
     *
     * 用于判断是否可以游泳、是否消耗氧气等
     *
     * @return 如果眼睛位置在水方块中返回 true
     */
    [[nodiscard]] bool areEyesInWater() const { return m_eyesInWater; }

    /**
     * @brief 检查眼睛是否在岩浆中
     */
    [[nodiscard]] bool areEyesInLava() const { return m_eyesInLava; }

    /**
     * @brief 检查是否可以游泳
     *
     * 用于判断游泳姿态切换
     *
     * @return 如果可以游泳返回 true
     */
    [[nodiscard]] bool canSwim() const { return m_eyesInWater && m_inWater; }

    /**
     * @brief 获取实体眼睛位置的亮度
     *
     * 用于判断怪物是否在阳光下燃烧等。
     *
     * @return 亮度值 (0.0 - 1.0)，如果世界不存在返回 0.0
     */
    [[nodiscard]] f32 getBrightness() const;

    /**
     * @brief 检查实体是否在梯子或藤蔓上
     *
     * 检查实体所在位置的方块是否为可攀爬方块。
     * 可攀爬方块包括：梯子、藤蔓、脚手架。
     *
     * @return 如果实体在可攀爬方块上返回 true
     */
    [[nodiscard]] virtual bool isOnLadder() const;

    /**
     * @brief 获取最后攀爬位置
     *
     * 当实体在攀爬方块（梯子、藤蔓、脚手架等）上时，
     * 记录攀爬位置。用于摔落死亡消息的生成。
     *
     * @return 攀爬位置，如果没有攀爬则返回空
     */
    [[nodiscard]] const std::optional<BlockPos>& getLastClimbPos() const { return m_lastClimbPos; }

    /**
     * @brief 设置最后攀爬位置
     * @param pos 攀爬位置
     */
    void setLastClimbPos(const BlockPos& pos) { m_lastClimbPos = pos; }

    /**
     * @brief 清空最后攀爬位置
     *
     * 在实体落地时调用。
     */
    void clearLastClimbPos() { m_lastClimbPos = std::nullopt; }

    /**
     * @brief 获取实体浸入水的高度
     * @return 流体高度（0.0-1.0），如果不在水中返回0
     */
    [[nodiscard]] virtual f32 getFluidHeight() const { return m_fluidHeight; }

    /**
     * @brief 获取水浸入高度
     * @return 水浸入高度（0.0-1.0）
     */
    [[nodiscard]] f32 waterHeight() const { return m_waterHeight; }

    /**
     * @brief 获取岩浆浸入高度
     * @return 岩浆浸入高度（0.0-1.0）
     */
    [[nodiscard]] f32 lavaHeight() const { return m_lavaHeight; }

    /**
     * @brief 设置流体高度
     */
    void setFluidHeight(f32 height) { m_fluidHeight = height; }

    /**
     * @brief 检查是否能看见另一个实体
     *
     * 通过射线检测判断视线是否被方块阻挡。
     *
     * @param other 目标实体
     * @return 如果视线未被阻挡返回true
     */
    [[nodiscard]] virtual bool canSee(const Entity& other) const;

    /**
     * @brief 检查实体是否着火
     */
    [[nodiscard]] bool isOnFire() const { return m_fire > 0; }

    /**
     * @brief 获取着火时间（tick）
     */
    [[nodiscard]] i32 fire() const { return m_fire; }

    /**
     * @brief 获取火焰计时器
     *
     * 与 fire() 功能相同。
     */
    [[nodiscard]] i32 getFireTimer() const { return m_fire; }

    /**
     * @brief 设置着火时间
     *
     * 只增加燃烧时间，不会减少。
     * 如果当前燃烧时间已经大于等于传入值，则不改变。
     *
     * @param ticks 燃烧时间（tick）
     */
    void setFire(i32 ticks)
    {
        if (m_fire < ticks) {
            m_fire = ticks;
        }
    }

    /**
     * @brief 强制设置火焰计时器
     *
     * 直接设置火焰计时器值，不检查当前值。
     * 用于增加/减少火焰时间，包括设置为负值（表示短暂火焰免疫期）。
     *
     * @param ticks 火焰计时器值
     */
    void forceFireTicks(i32 ticks) { m_fire = ticks; }

    /**
     * @brief 检查是否免疫火焰
     *
     * 默认实现查询实体类型的火焰免疫标志。
     * 子类可重写以提供运行时可变的免疫状态。
     *
     * @return 如果免疫火焰返回 true
     */
    [[nodiscard]] virtual bool isImmuneToFire() const;

    // ========== 空气管理 ==========

    /**
     * @brief 获取空气值
     */
    [[nodiscard]] i32 air() const { return m_air; }

    /**
     * @brief 设置空气值
     */
    void setAir(i32 air);

    /**
     * @brief 获取最大空气值
     */
    [[nodiscard]] virtual i32 maxAir() const { return 300; }

    // ========== 无敌 ==========

    /**
     * @brief 检查是否无敌
     */
    [[nodiscard]] bool isInvulnerable() const { return m_invulnerable; }

    /**
     * @brief 设置无敌状态
     */
    void setInvulnerable(bool invulnerable) { m_invulnerable = invulnerable; }

    /**
     * @brief 检查是否免疫爆炸伤害
     *
     * 默认返回 false，凋灵、末影龙等实体需要重写此方法。
     *
     * @return 如果免疫爆炸返回 true
     */
    [[nodiscard]] virtual bool isImmuneToExplosions() const { return false; }

    /**
     * @brief 受伤入口方法
     *
     * 处理实体受伤的通用逻辑。基类实现检查无敌状态。
     * 子类（如 LivingEntity）应重写此方法以实现具体的受伤逻辑。
     *
     * @param source 伤害来源
     * @param amount 伤害量
     * @return 是否成功造成伤害
     */
    virtual bool hurt(DamageSource& source, f32 amount);

    /**
     * @brief 检查是否对特定伤害类型免疫
     *
     * @param source 伤害来源
     * @return 如果免疫该伤害类型返回 true
     */
    [[nodiscard]] virtual bool isInvulnerableTo(DamageSource& source) const;

    // ========== 自定义名称 ==========

    /**
     * @brief 获取自定义名称组件
     * @return 自定义名称组件指针，如果没有返回 nullptr
     */
    [[nodiscard]] const text::ITextComponent* getCustomNameComponent() const { return m_customName.get(); }

    /**
     * @brief 获取自定义名称的纯文本
     * @return 自定义名称纯文本，如果没有返回空字符串
     */
    [[nodiscard]] std::string customNameText() const
    {
        return m_customName ? m_customName->getUnformattedText() : std::string();
    }

    /**
     * @brief 检查是否有自定义名称
     * @return 如果有自定义名称返回true
     */
    [[nodiscard]] bool hasCustomName() const { return m_customName != nullptr; }

    /**
     * @brief 获取显示名称
     *
     * 返回自定义名称组件，如果没有则返回默认名称。
     *
     * @return 显示名称组件
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getDisplayName() const;

    /**
     * @brief 设置自定义名称组件
     * @param name 名称组件（所有权转移）
     */
    void setCustomNameComponent(std::unique_ptr<text::ITextComponent> name);

    /**
     * @brief 设置自定义名称（纯文本，向后兼容）
     * @param name 名称字符串
     */
    void setCustomName(const std::string& name);

    /**
     * @brief 检查自定义名称是否可见
     */
    [[nodiscard]] bool isCustomNameVisible() const { return m_customNameVisible; }

    /**
     * @brief 设置自定义名称可见性
     */
    void setCustomNameVisible(bool visible);

    // ========== 静音 ==========

    /**
     * @brief 检查是否静音
     */
    [[nodiscard]] bool isSilent() const { return m_silent; }

    /**
     * @brief 设置静音状态
     */
    void setSilent(bool silent);

    // ========== 重力 ==========

    /**
     * @brief 检查是否受重力影响
     */
    [[nodiscard]] bool hasNoGravity() const { return m_noGravity; }

    /**
     * @brief 设置是否受重力影响
     */
    void setNoGravity(bool noGravity);

    // ========== 实体标签 ==========

    /**
     * @brief 获取实体的所有标签
     *
     * 标签用于命令系统和数据包谓词。
     *
     * @return 标签集合的常量引用
     */
    [[nodiscard]] const std::set<std::string>& getTags() const { return m_tags; }

    /**
     * @brief 添加标签
     *
     * 每个实体最多可以有 1024 个标签。
     *
     * @param tag 标签名称
     * @return 如果成功添加返回 true（标签不存在且未达到上限）
     */
    bool addTag(const std::string& tag);

    /**
     * @brief 移除标签
     *
     * @param tag 标签名称
     * @return 如果成功移除返回 true（标签存在）
     */
    bool removeTag(const std::string& tag);

    /**
     * @brief 检查是否拥有指定标签
     *
     * @param tag 标签名称
     * @return 如果拥有该标签返回 true
     */
    [[nodiscard]] bool hasTag(const std::string& tag) const;

    /**
     * @brief 获取标签数量
     *
     * @return 当前标签数量
     */
    [[nodiscard]] size_t getTagCount() const { return m_tags.size(); }

    /**
     * @brief 清空所有标签
     */
    void clearTags() { m_tags.clear(); }

    // ========== 运动速度乘数（甜浆果丛等减速效果） ==========

    /**
     * @brief 设置运动速度乘数
     *
     * 用于甜浆果丛、蜘蛛网等减速效果。
     * 每帧在实体移动前，速度会乘以这个乘数。
     * 退出减速区域时自动清除。
     *
     * @param multiplier 速度乘数 (x, y, z 分量)
     */
    void setMotionMultiplier(const Vector3& multiplier)
    {
        m_motionMultiplier = multiplier;
        m_hasMotionMultiplier = true;
    }

    /**
     * @brief 清除运动速度乘数
     *
     * 当实体退出减速方块时调用。
     */
    void clearMotionMultiplier()
    {
        m_motionMultiplier = Vector3(1.0f, 1.0f, 1.0f);
        m_hasMotionMultiplier = false;
    }

    /**
     * @brief 检查是否有运动速度乘数
     */
    [[nodiscard]] bool hasMotionMultiplier() const { return m_hasMotionMultiplier; }

    /**
     * @brief 获取运动速度乘数
     */
    [[nodiscard]] const Vector3& motionMultiplier() const { return m_motionMultiplier; }

    // ========== 摔落伤害 ==========

    /**
     * @brief 处理摔落伤害
     * @param distance 摔落距离
     * @param damageMultiplier 伤害倍率
     */
    virtual void handleFallDamage(f32 distance, f32 damageMultiplier);

    /**
     * @brief 更新摔落距离
     * 在移动时调用，跟踪摔落距离以便着地时计算伤害
     */
    void updateFallDistance();

    // ========== 闪电击中 ==========

    /**
     * @brief 当实体被闪电击中时调用
     *
     * 子类可以重写此方法来处理被闪电击中的特殊效果：
     * - 哞菇：红色 -> 棕色
     * - 苦力怕：变成高压苦力怕
     * - 村民：变成女巫
     * - 猪：变成僵尸猪灵
     *
     * 注意：闪电伤害由 LightningBoltEntity 单独处理，
     * 此方法用于处理特殊变形效果。
     */
    virtual void onStruckByLightning() {}

    // ========== 乘客/骑乘系统 ==========

    /**
     * @brief 获取乘客列表
     * @return 乘客实体ID列表
     */
    [[nodiscard]] const std::vector<EntityId>& getPassengers() const { return m_passengers; }

    /**
     * @brief 检查是否有乘客
     */
    [[nodiscard]] bool hasPassengers() const { return !m_passengers.empty(); }

    /**
     * @brief 检查是否被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const { return hasPassengers(); }

    /**
     * @brief 获取所骑乘的车辆
     * @return 车辆实体ID，如果没有则返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityId getVehicle() const { return m_vehicle; }

    /**
     * @brief 检查是否正在骑乘
     */
    [[nodiscard]] bool isRiding() const { return m_vehicle != INVALID_ENTITY_ID; }

    /**
     * @brief 检查指定实体是否是乘客
     * @param entityId 实体ID
     */
    [[nodiscard]] bool isPassenger(EntityId entityId) const;

    /**
     * @brief 添加乘客
     * @param passenger 乘客实体
     * @return 是否成功添加
     */
    bool addPassenger(Entity& passenger);

    /**
     * @brief 移除乘客
     * @param passenger 乘客实体
     */
    void removePassenger(Entity& passenger);

    /**
     * @brief 开始骑乘
     * @param vehicle 车辆实体
     * @return 是否成功骑乘
     */
    bool startRiding(Entity& vehicle);

    /**
     * @brief 停止骑乘
     */
    void stopRiding();

    /**
     * @brief 下马（核心下车逻辑）
     *
     * 内部方法，被 stopRiding() 调用
     */
    void dismount();

    /**
     * @brief 移除所有乘客
     *
     * 从后向前遍历乘客列表并调用每个乘客的 stopRiding()
     */
    void removePassengers();

    /**
     * @brief 检查是否可以被指定实体骑乘
     * @param vehicle 载具实体
     * @return 如果可以骑乘返回true
     *
     * 默认检查：不在潜行状态 + 骑乘冷却为0
     */
    [[nodiscard]] virtual bool canBeRidden(const Entity& vehicle) const;

    /**
     * @brief 检查是否可以在水中骑乘
     * @return 如果可以在水中骑乘返回true
     *
     * 默认返回true，LivingEntity重写返回false
     */
    [[nodiscard]] virtual bool canBeRiddenInWater() const { return true; }

    /**
     * @brief 检查是否与指定实体骑乘同一载具
     * @param other 其他实体
     * @return 如果骑乘同一载具返回true
     */
    [[nodiscard]] bool isRidingSameEntity(const Entity& other) const;

    /**
     * @brief 获取最底层的骑乘实体
     * @return 最底层载具的指针，如果没有骑乘返回nullptr
     *
     * 沿着骑乘链向下遍历直到找到最底层的载具
     */
    [[nodiscard]] Entity* getLowestRidingEntity();
    [[nodiscard]] const Entity* getLowestRidingEntity() const;

    /**
     * @brief 递归检查是否骑乘或被骑乘指定实体
     * @param other 目标实体
     * @return 如果存在骑乘关系返回true
     */
    [[nodiscard]] bool isRidingOrBeingRiddenBy(const Entity& other) const;

    /**
     * @brief 分离所有乘客和载具
     *
     * 同时移除所有乘客并下车
     */
    void detach();

    /**
     * @brief 获取第一个乘客
     * @return 第一个乘客的实体ID，如果没有则返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityId getFirstPassenger() const
    {
        return m_passengers.empty() ? INVALID_ENTITY_ID : m_passengers.front();
    }

    /**
     * @brief 获取控制乘客（通常是第一个乘客）
     * @return 控制乘客的实体ID，如果没有则返回 INVALID_ENTITY_ID
     *
     * 子类可重写此方法以返回不同的控制乘客
     */
    [[nodiscard]] virtual EntityId getControllingPassenger() const { return getFirstPassenger(); }

    /**
     * @brief 检查是否可以由乘客控制方向
     * @return 如果可以被乘客控制返回true
     */
    [[nodiscard]] virtual bool canBeSteered() const { return false; }

    /**
     * @brief 获取骑乘时的乘客数量限制
     * @return 最大乘客数量
     */
    [[nodiscard]] virtual i32 getMaxPassengers() const { return 1; }

    /**
     * @brief 检查是否可以容纳更多乘客
     *
     * 子类可重写此方法添加额外检查（如 BoatEntity 检查是否在水下）。
     * 默认实现只检查乘客数量限制。
     */
    [[nodiscard]] virtual bool canFitPassenger() const
    {
        return static_cast<i32>(m_passengers.size()) < getMaxPassengers();
    }

    /**
     * @brief 获取载具骑乘高度偏移
     * @return 载具顶部到乘客底部的距离
     *
     * 默认返回实体高度的75%
     */
    [[nodiscard]] virtual f64 getMountedYOffset() const;

    /**
     * @brief 获取乘客Y偏移
     * @return 乘客相对于载具骑乘点的Y偏移
     *
     * Entity默认返回0，Player重写返回-0.35
     */
    [[nodiscard]] virtual f64 getYOffset() const { return 0.0; }

    /**
     * @brief 获取骑乘位置（世界坐标）
     * @return 骑乘位置的世界坐标
     */
    [[nodiscard]] virtual Vector3 getRidingPosition() const;

    /**
     * @brief 更新乘客位置
     *
     * 每帧调用以更新所有乘客的位置
     */
    void updatePassengers();

    /**
     * @brief 更新骑乘实体的状态
     *
     * 当作为乘客时调用，更新位置和旋转
     */
    virtual void updateRidden();

    /**
     * @brief 将载具的朝向应用到乘客
     * @param passenger 乘客实体
     *
     * 用于船等需要同步旋转的载具
     */
    virtual void applyOrientationToEntity(Entity& passenger);

    /**
     * @brief 检查乘客是否可以控制方向
     * @return 如果乘客可以控制载具方向返回true
     */
    [[nodiscard]] bool canPassengerSteer() const;

    /**
     * @brief 获取骑乘冷却时间
     * @return 剩余冷却时间（tick），0表示可以骑乘
     */
    [[nodiscard]] i32 rideCooldown() const { return m_rideCooldown; }

    /**
     * @brief 检查是否可以骑乘（冷却为0）
     */
    [[nodiscard]] bool canRide() const { return m_rideCooldown <= 0; }

    /**
     * @brief 检查实体是否可以更新
     * @return 如果实体可以更新返回true
     *
     * 用于控制乘客是否执行tick更新
     */
    [[nodiscard]] virtual bool canUpdate() const { return true; }

    /**
     * @brief 更新单个乘客的位置
     * @param passenger 乘客实体
     *
     * 子类可重写此方法以自定义乘客位置计算
     */
    virtual void updatePassengerPosition(Entity& passenger);

protected:
    /**
     * @brief 定位乘客（内部方法）
     * @param passenger 乘客实体
     */
    void positionRider(Entity& passenger);

public:
    // ========== 更新 ==========

    /**
     * @brief 基础 tick 更新
     */
    virtual void baseTick();

    /**
     * @brief 更新环境状态（水中、岩浆中）
     */
    virtual void updateEnvironmentState();

    /**
     * @brief 将数据参数同步回实体字段
     *
     * 客户端接收元数据包后调用，用于把 DataManager 中的值写回实体成员。
     */
    virtual void syncMetadataFromDataManager();

    // ========== NBT 序列化 ==========

    /**
     * @brief 将实体完整序列化到 NBT
     *
     * 写入位置、运动、旋转、UUID 等基类数据，然后调用 addAdditionalSaveData()。
     *
     * @param tag NBT 复合标签（输出参数）
     */
    void writeToNBT(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 反序列化实体
     *
     * 读取位置、运动、旋转、UUID 等基类数据，然后调用 readAdditionalSaveData()。
     *
     * @param tag NBT 复合标签
     * @return 成功或错误
     */
    Result<void> readFromNBT(const nbt::tags::compound_tag& tag);

    // ========== 方块交互 ==========

    /**
     * @brief 检查实体是否正在潜行
     *
     * 默认返回 false。Player 类重写此方法返回实际的潜行状态。
     *
     * @return 如果实体正在潜行返回true
     */
    [[nodiscard]] virtual bool isSneaking() const { return false; }

    /**
     * @brief 检查实体是否小心行走（潜行状态）
     *
     * 小心行走的实体不会触发 onEntityWalk 回调。
     *
     * @return 如果实体正在潜行返回true
     */
    [[nodiscard]] virtual bool isSteppingCarefully() const { return isSneaking(); }

    /**
     * @brief 检查实体是否阻尼振动
     *
     * 阻尼振动的实体不会触发振动信号。
     * 监守者始终阻尼振动，掉落的羊毛物品也阻尼振动。
     * 参考: net.minecraft.world.entity.Entity.dampensVibrations()
     *
     * @return 如果实体阻尼振动返回true
     */
    [[nodiscard]] virtual bool dampensVibrations() const { return false; }

    /**
     * @brief 检查实体是否可以触发行走事件
     *
     * 某些实体（如盔甲架、船等）不会触发行走相关事件。
     *
     * @return 默认返回true
     */
    [[nodiscard]] virtual bool canTriggerWalking() const { return true; }

    /**
     * @brief 检查实体是否不触发压力板/绊线
     *
     * 某些实体（如盔甲架、蝙蝠、投射物等）不会触发压力板和绊线。
     * 默认返回 false（会触发）。
     * 蝙蝠重写返回 true（蝙蝠不触发）。
     * 盔甲架、物品实体、投射物等也应该重写返回 true。
     *
     * @return 如果实体不触发压力板返回true
     */
    [[nodiscard]] virtual bool doesEntityNotTriggerPressurePlate() const { return false; }

    /**
     * @brief 获取实体的比较器输出信号强度
     *
     * 某些实体可以输出模拟红石信号（0-15），供比较器读取。
     * 例如：箱子矿车、漏斗矿车（基于容器填充率）、命令方块矿车（基于成功次数）、物品展示框（基于旋转）。
     * 默认返回 0（无信号）。
     *
     * @return 比较器信号强度 0-15
     */
    [[nodiscard]] virtual i32 getComparatorOutput() const { return 0; }

    /**
     * @brief 播放脚步声
     *
     * 当实体在方块上行走时调用。子类可重写以自定义脚步声。
     *
     * 默认实现使用脚下方块的声音类型播放脚步声。
     *
     * @param pos 方块位置
     * @param blockState 方块状态
     */
    virtual void playStepSound(const BlockPos& pos, const BlockState* blockState);

    /**
     * @brief 检查实体是否无视碰撞
     *
     * 无视碰撞的实体可以穿过方块，不会触发碰撞检测。
     *
     * @return 如果实体无视碰撞返回true
     */
    [[nodiscard]] bool noClip() const { return m_noClip; }

    /**
     * @brief 设置实体是否无视碰撞
     * @param noClip 是否无视碰撞
     */
    void setNoClip(bool noClip) { m_noClip = noClip; }

    /**
     * @brief 执行方块碰撞回调（无参数版本）
     *
     * 遍历实体碰撞箱覆盖的所有方块，调用方块的 onEntityCollision 方法。
     * 用于在 tick() 中手动触发方块碰撞检测，例如船、矿车等不触发行走的实体。
     */
    void doBlockCollisions();

    /**
     * @brief 执行方块碰撞回调（移动后版本）
     *
     * 在实体移动后调用，处理与方块的交互：
     * - onLanded: 垂直碰撞后着地
     * - onEntityWalk: 在地面上行走
     * - onInsideBlock: 实体进入方块碰撞箱
     *
     * @param actualMovement 实际移动向量
     * @param desiredMovement 期望移动向量
     */
    void doBlockCollisionsAfterMove(const Vector3& actualMovement, const Vector3& desiredMovement);

    /**
     * @brief 当实体进入方块碰撞箱时调用
     *
     * 每帧遍历实体碰撞箱覆盖的所有方块时调用。
     * ServerPlayer 重写此方法触发 EnterBlockTrigger 成就。
     *
     * @param blockState 方块状态
     */
    virtual void onInsideBlock(const BlockState& blockState)
    {
        // 基类空实现，子类可重写
        (void)blockState;
    }

    // toString，用于调试，所有实体统一
    [[nodiscard]] std::string toString() const;

protected:
    /**
     * @brief 根据实体类型和后缀构造声音事件ID
     * @param suffix 声音后缀（例如 ambient、hurt、death）
     * @return 声音事件ID，无效类型返回空
     */
    [[nodiscard]] std::optional<ResourceLocation> makeSoundEventId(std::string_view suffix) const;

    EntityId m_id;
    std::string m_uuid;     // UUID 字符串
    std::string m_typeId;   // 资源标识符（如 minecraft:pig）
    Vector3 m_position;     // 当前位置
    Vector3 m_prevPosition; // 上一帧位置
    Vector3 m_velocity;     // 速度

    f32 m_yaw = 0.0f;   // 偏航角 (Y轴旋转)
    f32 m_pitch = 0.0f; // 俯仰角 (X轴旋转)
    f32 m_prevYaw = 0.0f;
    f32 m_prevPitch = 0.0f;

    bool m_onGround = false;
    bool m_removed = false;
    bool m_noClip = false;  // 是否无视碰撞（用于三叉戟返回等）
    bool m_glowing = false; // 发光状态（服务端使用）
    EntityPose m_pose = EntityPose::Standing;
    EntityFlags m_flags = EntityFlags::None;
    entity::EntitySize m_dimensions = entity::EntitySize::flexible(0.6f, 1.8f);
    AxisAlignedBB m_boundingBox = AxisAlignedBB::fromPosition(Vector3(0.0f, 0.0f, 0.0f), 0.6f, 1.8f);
    bool m_dimensionsInitialized = false;

    // 物理相关
    PhysicsEngine* m_physicsEngine = nullptr;
    bool m_collidedHorizontally = false;
    bool m_collidedVertically = false;
    f32 m_fallDistance = 0.0f;
    f32 m_stepHeight = 0.0f; // 步进高度，默认0.0f，LivingEntity设置为0.6f

    DimensionId m_dimension = 0;
    u32 m_ticksExisted = 0;

    // 传送门相关
    i32 m_portalCooldown = 0; // 传送冷却（防止频繁传送，单位：tick）
    i32 m_portalTime = 0;     // 在传送门中的累计时间（单位：tick）
    bool m_inPortal = false;  // 是否在传送门中
    BlockPos m_portalPos;     // 所在传送门方块的位置

    // 世界引用
    IWorld* m_world = nullptr;

    // 数据管理器
    entity::EntityDataManager m_dataManager;

    // 环境状态
    bool m_inWater = false;
    bool m_inLava = false;
    bool m_eyesInWater = false; // 眼睛是否在水下
    bool m_eyesInLava = false;  // 眼睛是否在岩浆中
    f32 m_fluidHeight = 0.0f;   // 流体高度（方块单位，已废弃）
    f32 m_waterHeight = 0.0f;   // 水浸入高度（0.0-1.0）
    f32 m_lavaHeight = 0.0f;    // 岩浆浸入高度（0.0-1.0）
    i32 m_fire = 0;             // 着火时间（tick）

    // 攀爬追踪（用于摔落死亡消息）
    std::optional<BlockPos> m_lastClimbPos; // 最后攀爬位置

    // 空气值
    i32 m_air = 300; // 默认最大空气值

    // 无敌
    bool m_invulnerable = false;

    // 自定义名称
    std::unique_ptr<text::ITextComponent> m_customName; ///< 自定义名称
    bool m_customNameVisible = false;

    // 静音
    bool m_silent = false;

    // 重力
    bool m_noGravity = false;

    // 实体标签（最多1024个标签）
    std::set<std::string> m_tags;

    // 运动速度乘数（用于甜浆果丛等减速效果）
    Vector3 m_motionMultiplier = Vector3(1.0f, 1.0f, 1.0f);
    bool m_hasMotionMultiplier = false;

    // 乘客/骑乘系统
    std::vector<EntityId> m_passengers;     // 乘客列表
    EntityId m_vehicle = INVALID_ENTITY_ID; // 正在骑乘的车辆
    i32 m_rideCooldown = 0;                 // 骑乘冷却（tick），用于防止快速上下骑乘

    /**
     * @brief 设置车辆（内部方法）
     */
    void setVehicle(EntityId vehicle) { m_vehicle = vehicle; }

    /**
     * @brief 序列化子类特有数据
     *
     * 子类重写此方法添加自己的持久化数据，必须调用基类实现。
     *
     * @param tag NBT 复合标签（输出参数）
     */
    virtual void addAdditionalSaveData(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 反序列化子类特有数据
     *
     * 子类重写此方法读取自己的持久化数据，必须调用基类实现。
     *
     * @param tag NBT 复合标签
     * @return 成功或错误
     */
    virtual Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag);

    /**
     * @brief 重新应用当前位置到碰撞箱
     */
    void reapplyPosition();
};

} // namespace mc
