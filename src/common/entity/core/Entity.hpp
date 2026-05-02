#pragma once

#include "../../core/Types.hpp"
#include "../../core/Result.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/AxisAlignedBB.hpp"
#include "EntityPose.hpp"
#include "EntitySize.hpp"
#include "EntityDataManager.hpp"
#include "MoverType.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../sound/SoundCategory.hpp"
#include "../../util/text/ITextComponent.hpp"
#include <string>
#include <memory>
#include <array>
#include <functional>

namespace mc {

// 前向声明
class PhysicsEngine;
class IWorld;

/**
 * @brief 实体推动反应类型
 *
 * 定义实体被活塞等推动时的反应行为。
 * 参考 MC 1.16.5 net.minecraft.block.material.PushReaction
 */
enum class PushReaction : u8 {
    Normal,   // 正常推动
    Destroy,  // 被推动时销毁
    Ignore    // 忽略推动
};

// ============================================================================
// 旧实体类型枚举（兼容）
// TODO 彻底移除这个旧枚举，改用 mc::entity::EntityType
//
// 注意：新代码应使用 mc::entity::EntityType 类
// 此枚举保留用于向后兼容
// ============================================================================
enum class LegacyEntityType : u32 {
    Unknown = 0,
    Player = 1,
    Item = 2,           // 物品实体
    ExperienceOrb = 3,  // 经验球实体

    // 被动生物
    Pig = 10,
    Cow = 11,
    Sheep = 12,
    Chicken = 13,
    Rabbit = 14,
    Mooshroom = 15,
    Wolf = 16,
    Cat = 17,
    Ocelot = 18,
    Parrot = 19,
    Fox = 20,
    Panda = 21,
    PolarBear = 22,
    Turtle = 23,
    Bee = 24,
    Strider = 25,
    Squid = 26,
    Dolphin = 27,
    Cod = 28,
    Salmon = 29,
    Pufferfish = 30,
    TropicalFish = 31,
    Bat = 32,
    IronGolem = 33,
    SnowGolem = 34,
    Horse = 35,
    Donkey = 36,
    Mule = 37,
    SkeletonHorse = 38,
    ZombieHorse = 39,
    Llama = 40,
    TraderLlama = 41,

    // 敌对生物
    Zombie = 50,
    Skeleton = 51,
    Husk = 52,
    Drowned = 53,
    Stray = 54,
    WitherSkeleton = 55,
    Phantom = 56,
    Spider = 57,
    CaveSpider = 58,
    Endermite = 59,
    Silverfish = 60,
    Creeper = 61,
    Slime = 62,
    Giant = 63,
    Enderman = 64,
    Shulker = 65,
    Ghast = 66,
    MagmaCube = 67,
    Piglin = 68,
    PiglinBrute = 69,
    Hoglin = 70,
    Zoglin = 71,
    Vindicator = 72,
    Evoker = 73,
    Illusioner = 74,
    Pillager = 75,
    Guardian = 76,
    ElderGuardian = 77,
    Witch = 78,
    Ravager = 79,
    Blaze = 80,

    // Boss
    Wither = 90,
    EnderDragon = 91,

    // 载具
    Boat = 110,
    Minecart = 111,

    // 投掷物
    Snowball = 120,
    Egg = 121,
    EnderPearl = 122,
    ExperienceBottle = 123,
    Potion = 124,
    Arrow = 125,
    SpectralArrow = 126,
    Trident = 127,
    Fireball = 128,
    SmallFireball = 129,
    DragonFireball = 130,
    WitherSkull = 131,
    LlamaSpit = 132,
    ShulkerBullet = 133,
    EvokerFangs = 134,
    FishingBobber = 135,
    EyeOfEnder = 136,
    FireworkRocket = 137,

    // 其他
    Villager = 100,
    // 后续添加更多
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

inline EntityFlags operator|(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline EntityFlags operator&(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool hasFlag(EntityFlags flags, EntityFlags flag) {
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
 *
 * 参考 MC 1.16.5 Entity
 */
class Entity {
public:
    // ========== 静态数据参数 ==========
    // 子类应定义自己的数据参数

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     * @param world 世界指针（可选）
     */
    Entity(LegacyEntityType type, EntityId id, IWorld* world = nullptr);
    virtual ~Entity() = default;

    // 禁止拷贝
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;

    // 允许移动
    Entity(Entity&&) = default;
    Entity& operator=(Entity&&) = default;

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
    [[nodiscard]] LegacyEntityType legacyType() const { return m_legacyType; }
    [[nodiscard]] const String& uuid() const { return m_uuid; }
    void setUuid(const String& uuid) { m_uuid = uuid; }

    /**
     * @brief 获取实体类型标识符
     * @return 实体类型字符串（用于网络同步和渲染）
     */
    [[nodiscard]] virtual String getTypeId() const;

    /**
     * @brief 设置实体类型标识符
     *
     * 当实体由注册表工厂创建时，调用方应传入注册名（如 minecraft:pig）。
     */
    void setTypeId(const String& typeId) { m_typeId = typeId; }

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
    [[nodiscard]] f32 distanceTo(const Entity& other) const {
        return m_position.distance(other.m_position);
    }

    /**
     * @brief 计算到另一个实体的距离平方
     * @param other 另一个实体
     * @return 距离的平方（避免开方运算，适合比较）
     */
    [[nodiscard]] f32 distanceSqTo(const Entity& other) const {
        return m_position.distanceSquared(other.m_position);
    }

    /**
     * @brief 计算到指定位置的距离平方
     * @param px 目标X坐标
     * @param py 目标Y坐标
     * @param pz 目标Z坐标
     * @return 距离的平方
     */
    [[nodiscard]] f32 distanceSqTo(f32 px, f32 py, f32 pz) const {
        f32 dx = px - m_position.x;
        f32 dy = py - m_position.y;
        f32 dz = pz - m_position.z;
        return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief 计算到指定位置的水平距离平方（忽略Y轴）
     */
    [[nodiscard]] f32 distanceHorizontalSqTo(f32 px, f32 pz) const {
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
    void addVelocity(f32 dx, f32 dy, f32 dz) {
        m_velocity.x += dx;
        m_velocity.y += dy;
        m_velocity.z += dz;
    }

    /**
     * @brief 添加速度增量
     * @param delta 速度增量向量
     */
    void addVelocity(const Vector3& delta) { addVelocity(delta.x, delta.y, delta.z); }

    void setOnGround(bool onGround) { m_onGround = onGround; }
    void setPose(EntityPose pose);
    void setFlags(EntityFlags flags);

    // 标志操作
    void addFlag(EntityFlags flag);
    void removeFlag(EntityFlags flag);
    [[nodiscard]] bool hasFlag(EntityFlags flag) const {
        return mc::hasFlag(m_flags, flag);
    }

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
     * @return 实体可以自动步进的最大高度（玩家为0.6）
     */
    [[nodiscard]] virtual f32 stepHeight() const { return 0.0f; }

    // ========== 碰撞箱 ==========

    /**
     * @brief 获取实体碰撞箱
     * @return 基于当前位置的AABB碰撞箱
     */
    [[nodiscard]] AxisAlignedBB boundingBox() const {
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
     * 参考 MC 1.16.5 Entity.move(MoverType, Vec3)
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
     * 参考 MC 1.16.5 Entity.getPushReaction()
     * 子类可重写以返回不同的推动反应。
     *
     * @return 推动反应类型
     */
    [[nodiscard]] virtual PushReaction getPushReaction() const {
        return PushReaction::Normal;
    }

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

    void remove() { m_removed = true; }

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
     * @brief 处理传送门 tick
     *
     * 每帧调用，更新传送冷却和传送门计时。
     * 玩家需要 80 tick (4秒) 在传送门中才能传送。
     * 其他实体需要约 1 tick。
     *
     * @return true 如果应该触发传送
     */
    virtual bool tickPortal();

    // ========== 存活时间 ==========

    [[nodiscard]] u32 ticksExisted() const { return m_ticksExisted; }

    // ========== 存活状态 ==========

    /**
     * @brief 检查实体是否存活
     * @return 如果实体未被移除且未死亡则返回 true
     */
    [[nodiscard]] virtual bool isAlive() const { return !m_removed; }

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
     * 参考 MC 1.16.5 Entity.isInRain()
     * 需要满足：世界正在下雨 + 实体位置可以看到天空 + 生物群系允许降水
     *
     * @return 如果实体在雨中返回 true
     */
    [[nodiscard]] bool isInRain() const;

    /**
     * @brief 检查实体是否湿润
     *
     * 参考 MC 1.16.5 Entity.isWet()
     * MC: isWet() = isInWater() || isInRain()
     * 用于三叉戟激流附魔、末影人躲避等逻辑
     *
     * @return 如果实体在水中或雨中返回 true
     */
    [[nodiscard]] bool isWet() const { return m_inWater || isInRain(); }

    /**
     * @brief 检查眼睛是否在水下
     *
     * 参考 MC 1.16.5 Entity.areEyesInFluid(FluidTags.WATER)
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
     * MC 1.16.5: canSwim() = eyesInWater && inWater
     * 用于判断游泳姿态切换
     *
     * @return 如果可以游泳返回 true
     */
    [[nodiscard]] bool canSwim() const { return m_eyesInWater && m_inWater; }

    /**
     * @brief 检查实体是否在梯子或藤蔓上
     *
     * 检查实体所在位置的方块是否为可攀爬方块。
     * 可攀爬方块包括：梯子、藤蔓、脚手架。
     *
     * 参考: MC 1.16.5 LivingEntity.isOnLadder()
     *
     * @return 如果实体在可攀爬方块上返回 true
     */
    [[nodiscard]] virtual bool isOnLadder() const;

    /**
     * @brief 获取实体浸入水的高度
     * MC 1.16.5: func_233571_b_(FluidTags.WATER)
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
     * @brief 设置着火时间
     */
    void setFire(i32 ticks) { m_fire = ticks; }

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

    // ========== 自定义名称 ==========

    /**
     * @brief 获取自定义名称组件
     * @return 自定义名称组件指针，如果没有返回 nullptr
     */
    [[nodiscard]] const text::ITextComponent* getCustomNameComponent() const {
        return m_customName.get();
    }

    /**
     * @brief 获取自定义名称的纯文本
     * @return 自定义名称纯文本，如果没有返回空字符串
     */
    [[nodiscard]] String customNameText() const {
        return m_customName ? m_customName->getUnformattedText() : String();
    }

    /**
     * @brief 检查是否有自定义名称
     * @return 如果有自定义名称返回true
     */
    [[nodiscard]] bool hasCustomName() const {
        return m_customName != nullptr;
    }

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
    void setCustomName(const String& name);

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
     * MC 1.16.5: Entity.func_241841_a(ServerWorld, LightningBoltEntity)
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
     * @param clearVehicle 是否清除车辆引用
     */
    void stopRiding(bool clearVehicle = true);

    /**
     * @brief 获取第一个乘客
     * @return 第一个乘客的实体ID，如果没有则返回 INVALID_ENTITY_ID
     */
    [[nodiscard]] EntityId getFirstPassenger() const {
        return m_passengers.empty() ? INVALID_ENTITY_ID : m_passengers.front();
    }

    /**
     * @brief 获取骑乘者在车辆中的位置偏移
     * @return 骑乘位置偏移
     */
    [[nodiscard]] virtual Vector3 getRidingPosition() const;

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

    // ========== 方块交互 ==========

    /**
     * @brief 检查实体是否正在潜行
     *
     * 默认返回 false。Player 类重写此方法返回实际的潜行状态。
     * 参考: MC 1.16.5 Entity.isSneaking()
     *
     * @return 如果实体正在潜行返回true
     */
    [[nodiscard]] virtual bool isSneaking() const { return false; }

    /**
     * @brief 检查实体是否小心行走（潜行状态）
     *
     * 小心行走的实体不会触发 onEntityWalk 回调。
     * 参考: MC 1.16.5 Entity.isSteppingCarefully()
     *
     * @return 如果实体正在潜行返回true
     */
    [[nodiscard]] virtual bool isSteppingCarefully() const { return isSneaking(); }

    /**
     * @brief 检查实体是否可以触发行走事件
     *
     * 某些实体（如盔甲架、船等）不会触发行走相关事件。
     * 参考: MC 1.16.5 Entity.canTriggerWalking()
     *
     * @return 默认返回true
     */
    [[nodiscard]] virtual bool canTriggerWalking() const { return true; }

    /**
     * @brief 检查实体是否无视碰撞
     *
     * 无视碰撞的实体可以穿过方块，不会触发碰撞检测。
     * 参考: MC 1.16.5 Entity.noClip
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
     * @brief 执行方块碰撞回调
     *
     * 在实体移动后调用，处理与方块的交互：
     * - onLanded: 垂直碰撞后着地
     * - onEntityWalk: 在地面上行走
     *
     * 参考: MC 1.16.5 Entity.move() 中的方块回调处理
     *
     * @param actualMovement 实际移动向量
     * @param desiredMovement 期望移动向量
     */
    void doBlockCollisions(const Vector3& actualMovement, const Vector3& desiredMovement);

    // toString，用于调试，所有实体统一
    [[nodiscard]] String toString() const;

protected:
    /**
     * @brief 根据实体类型和后缀构造声音事件ID
     * @param suffix 声音后缀（例如 ambient、hurt、death）
     * @return 声音事件ID，无效类型返回空
     */
    [[nodiscard]] std::optional<ResourceLocation> makeSoundEventId(StringView suffix) const;

    EntityId m_id;
    LegacyEntityType m_legacyType;
    String m_uuid;              // UUID 字符串
    String m_typeId;            // 资源标识符（如 minecraft:pig）
    Vector3 m_position;         // 当前位置
    Vector3 m_prevPosition;     // 上一帧位置
    Vector3 m_velocity;         // 速度

    f32 m_yaw = 0.0f;           // 偏航角 (Y轴旋转)
    f32 m_pitch = 0.0f;         // 俯仰角 (X轴旋转)
    f32 m_prevYaw = 0.0f;
    f32 m_prevPitch = 0.0f;

    bool m_onGround = false;
    bool m_removed = false;
    bool m_noClip = false;       // 是否无视碰撞（用于三叉戟返回等）
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

    DimensionId m_dimension = 0;
    u32 m_ticksExisted = 0;

    // 传送门相关
    i32 m_portalCooldown = 0;    // 传送冷却（防止频繁传送，单位：tick）
    i32 m_portalTime = 0;        // 在传送门中的累计时间（单位：tick）
    bool m_inPortal = false;     // 是否在传送门中

    // 世界引用
    IWorld* m_world = nullptr;

    // 数据管理器
    entity::EntityDataManager m_dataManager;

    // 环境状态
    bool m_inWater = false;
    bool m_inLava = false;
    bool m_eyesInWater = false;    // 眼睛是否在水下
    bool m_eyesInLava = false;     // 眼睛是否在岩浆中
    f32 m_fluidHeight = 0.0f;      // 流体高度（方块单位，已废弃）
    f32 m_waterHeight = 0.0f;      // 水浸入高度（0.0-1.0）
    f32 m_lavaHeight = 0.0f;       // 岩浆浸入高度（0.0-1.0）
    i32 m_fire = 0;                // 着火时间（tick）

    // 空气值
    i32 m_air = 300;            // 默认最大空气值

    // 无敌
    bool m_invulnerable = false;

    // 自定义名称
    std::unique_ptr<text::ITextComponent> m_customName;  ///< 自定义名称
    bool m_customNameVisible = false;

    // 静音
    bool m_silent = false;

    // 重力
    bool m_noGravity = false;

    // 乘客/骑乘系统
    std::vector<EntityId> m_passengers;  // 乘客列表
    EntityId m_vehicle = INVALID_ENTITY_ID;  // 正在骑乘的车辆

    /**
     * @brief 设置车辆（内部方法）
     */
    void setVehicle(EntityId vehicle) { m_vehicle = vehicle; }

    /**
     * @brief 重新应用当前位置到碰撞箱
     */
    void reapplyPosition();
};

} // namespace mc
