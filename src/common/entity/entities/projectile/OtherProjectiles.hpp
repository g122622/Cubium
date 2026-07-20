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

#include "ProjectileEntity.hpp"
#include "ProjectileHelper.hpp"
#include "ThrowableEntity.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 羊驼唾液实体
 *
 * 羊驼发射的唾液，对狼造成伤害。
 */
class LlamaSpitEntity : public ThrowableEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit LlamaSpitEntity(EntityInstanceId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    [[nodiscard]] f32 getGravity() const override { return 0.06f; } // 更高的重力

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;
};

/**
 * @brief 钓鱼浮标实体
 *
 * 钓鱼竿的浮标，用于钓鱼机制。
 */
class FishingBobberEntity : public Entity {
public:
    /**
     * @brief 钓鱼状态
     */
    enum class State : u8 {
        Flying,  // 飞行中
        Hooked,  // 钩住实体
        Bobbing, // 浮在水面
        Fishing  // 钓鱼中（咬钩状态）
    };

    /**
     * @brief 水类型（用于开放水域检测）
     */
    enum class WaterType : u8 {
        AboveWater,  // 水上方块（空气或睡莲）
        InsideWater, // 水内部（完整水源方块）
        Invalid      // 无效
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit FishingBobberEntity(EntityInstanceId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    void tick() override;

    /**
     * @brief 注册实体同步数据参数
     *
     * 重写 Entity::registerData()，注册 FishingBobberEntity 的网络同步参数：
     * - DATA_HOOKED_ENTITY_PARAM：被钩住实体的 ID（+1 偏移，0 表示无）
     * - DATA_BITING_PARAM：是否正在咬钩
     *
     * 对应 MC 1.21.11 FishingHook.defineSynchedData()。
     *
     * 注意：由于 C++ 虚函数在构造函数中不会派生到子类，
     * FishingBobberEntity 构造函数必须显式调用此方法。
     */
    void registerData() override;

    // ========== 钓鱼浮标方法 ==========

    /**
     * @brief 获取钓鱼者
     */
    [[nodiscard]] Player* getAngler() const { return m_angler; }

    /**
     * @brief 设置发射者（钓鱼者）
     * @param shooter 发射者实体
     */
    void setShooter(Entity* shooter);

    /**
     * @brief 获取当前状态
     */
    [[nodiscard]] State state() const { return m_state; }

    /**
     * @brief 发射浮标
     * @param shooter 发射者
     * @param pitch 俯仰角（度）
     * @param yaw 偏航角（度）
     * @param pitchOffset 俯仰角偏移
     * @param velocity 速度
     * @param inaccuracy 不准确度
     */
    void shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy);

    /**
     * @brief 收杆
     * @return 钓到的物品数量（用于耐久消耗）
     */
    i32 reelIn();

    /**
     * @brief 获取被钩住的实体
     * @return 被钩住的实体指针，如果没有则返回 nullptr
     */
    [[nodiscard]] Entity* getCaughtEntity() const { return m_caughtEntity; }

    /**
     * @brief 获取被钩住的实体ID（用于网络同步）
     * @return 实体ID，如果没有则返回 0
     */
    [[nodiscard]] EntityInstanceId getCaughtEntityId() const { return m_caughtEntityId; }

    /**
     * @brief 获取 DATA_HOOKED_ENTITY_PARAM 的参数 ID（客户端元数据同步用）
     *
     * 客户端 ClientEntity::syncMetadataFromDataManager() 通过此 ID 读取
     * 服务端同步过来的"被钩住实体 ID"（存储时 +1，0 表示无），用于：
     * - 客户端钓鱼浮标渲染时确定钓线另一端连接的实体
     *
     * 对应 MC 1.21.11 FishingHook.DATA_HOOKED_ENTITY。
     *
     * @return 数据参数 ID
     */
    [[nodiscard]] static u16 getHookedEntityParamId() { return DATA_HOOKED_ENTITY_PARAM.id(); }

    /**
     * @brief 获取 DATA_BITING_PARAM 的参数 ID（客户端元数据同步用）
     *
     * 客户端 ClientEntity::syncMetadataFromDataManager() 通过此 ID 读取
     * 服务端同步过来的"是否咬钩"状态，用于驱动咬钩动画（浮标下沉）。
     *
     * 对应 MC 1.21.11 FishingHook.DATA_BITING。
     *
     * @return 数据参数 ID
     */
    [[nodiscard]] static u16 getBitingParamId() { return DATA_BITING_PARAM.id(); }

    /**
     * @brief 设置钓鱼附魔加成
     * @param luckBonus 海之眷顾附魔等级
     * @param speedBonus 饵钓附魔等级
     */
    void setFishingBonus(i32 luckBonus, i32 speedBonus)
    {
        m_luckBonus = luckBonus;
        m_speedBonus = speedBonus;
    }

    /**
     * @brief 是否在开放水域
     */
    [[nodiscard]] bool isInOpenWater() const { return m_inOpenWater; }

private:
    /**
     * @brief 检测水面并更新状态
     */
    void _updateWaterState();

    /**
     * @brief 检测是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 检测开放水域
     * @return 是否满足开放水域条件
     */
    [[nodiscard]] bool _checkOpenWater();

    /**
     * @brief 判断单个方块位置的水类型
     *
     * 对应 MC Java FishingHook.getOpenWaterTypeFor。
     * - 空气或睡莲 → AboveWater
     * - 水源方块且碰撞箱为空 → InsideWater
     * - 其他 → Invalid
     *
     * @param pos 方块位置
     * @return 水类型
     */
    [[nodiscard]] WaterType _getOpenWaterTypeForBlock(const BlockPos& pos) const;

    /**
     * @brief 判断一个矩形区域（5×1×5）的水类型
     *
     * 对应 MC Java FishingHook.getOpenWaterTypeForArea。
     * 区域内所有方块必须为同一类型，否则为 Invalid。
     *
     * @param from 起始位置（含）
     * @param to 结束位置（含）
     * @return 区域水类型
     */
    [[nodiscard]] WaterType _getOpenWaterTypeForArea(const BlockPos& from, const BlockPos& to) const;

    /**
     * @brief 钓鱼逻辑tick
     */
    void _catchingFish();

    /**
     * @brief 生成钓鱼粒子
     */
    void _spawnFishingParticles();

    /**
     * @brief 生成收杆物品
     * @return 消耗的耐久度
     */
    i32 _spawnCatchItems();

    /**
     * @brief 生成经验球
     * @param totalXp 总经验值
     */
    void _spawnExperienceOrbs(i32 totalXp);

    /**
     * @brief 设置咬钩等待时间
     */
    void _setWaitTime();

    /**
     * @brief 执行射线检测
     * @return 射线检测结果
     */
    [[nodiscard]] RayTraceResult _performRayTrace();

    /**
     * @brief 检查是否可以命中指定实体
     * @param target 目标实体
     * @return 是否可以命中
     */
    [[nodiscard]] bool _canHitEntity(const Entity& target) const;

    /**
     * @brief 命中实体时的回调
     * @param result 射线检测结果
     */
    void _onEntityHit(const RayTraceResult& result);

    /**
     * @brief 命中方块时的回调
     * @param result 射线检测结果
     */
    void _onBlockHit(const RayTraceResult& result);

    /**
     * @brief 拉动被钩住的实体
     */
    void _bringInHookedEntity();

    /**
     * @brief 同步被钩住实体ID（用于客户端）
     */
    void _syncCaughtEntityId();

    Player* m_angler = nullptr;            // 钓鱼者
    Entity* m_caughtEntity = nullptr;      // 被钩住的实体
    EntityInstanceId m_caughtEntityId = 0; // 被钩住实体ID（用于网络同步，存储时+1，0表示无）
    State m_state = State::Flying;         // 当前状态
    i32 m_ticksCaughtDelay = 0;            // 咬钩等待计时器
    i32 m_ticksCatchableDelay = 0;         // 鱼接近计时器
    i32 m_ticksCatchable = 0;              // 可捕获窗口期
    f32 m_fishAngle = 0.0f;                // 鱼的角度（用于动画）
    bool m_inOpenWater = false;            // 是否在开放水域
    i32 m_luckBonus = 0;                   // 海之眷顾附魔等级
    i32 m_speedBonus = 0;                  // 饵钓附魔等级
    i32 m_outOfWaterTime = 0;              // 离开水的时间计数器
    i32 m_lifetime = 0;                    // 存在时间

    // ========== 网络同步数据参数 ==========
    // 对应 MC 1.21.11 FishingHook 的 DATA_HOOKED_ENTITY / DATA_BITING。
    // 静态成员在 OtherProjectiles.cpp 中通过 EntityDataManager::createKey<T>() 定义，
    // 在静态初始化阶段分配全局唯一 ID。
    static entity::DataParameter<i32> DATA_HOOKED_ENTITY_PARAM; ///< 被钩住实体 ID（+1 偏移，0=无）
    static entity::DataParameter<bool> DATA_BITING_PARAM;       ///< 是否咬钩

    // 允许测试类访问私有方法
    friend class FishingBobberTestAccess;
};

/**
 * @brief 潜影贝子弹实体
 *
 * 潜影贝发射的跟踪子弹，造成漂浮效果。
 *
 * 特性：
 * - 沿轴向移动，追踪目标
 * - 命中后造成4点伤害和10秒漂浮效果
 * - 被击中时会消失并产生爆炸粒子
 */
class ShulkerBulletEntity : public ProjectileEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 默认构造函数
     */
    explicit ShulkerBulletEntity(EntityInstanceId id);

    /**
     * @brief 带目标的构造函数
     * @param world 世界
     * @param shooter 发射者（潜影贝）
     * @param target 目标实体
     * @param axis 初始移动轴
     */
    ShulkerBulletEntity(IWorld* world, LivingEntity* shooter, Entity* target, Axis axis);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.3125f; }
    [[nodiscard]] f32 height() const override { return 0.3125f; }

    void tick() override;

    // ========== 投掷物属性 ==========

    [[nodiscard]] bool isBurning() const { return false; }
    [[nodiscard]] f32 getBrightness() const { return 1.0f; }
    [[nodiscard]] bool canBeCollidedWith() const override { return true; }

    // ========== 潜影贝子弹方法 ==========

    /**
     * @brief 设置目标
     */
    void setTarget(Entity* target);

    /**
     * @brief 获取目标
     */
    [[nodiscard]] Entity* target() const { return m_target; }

    /**
     * @brief 获取当前移动方向
     */
    [[nodiscard]] Direction direction() const { return m_direction; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;
    void onImpact(const RayTraceResult& result) override;

    /**
     * @brief 检查是否可以命中指定实体
     */
    [[nodiscard]] bool canHitEntity(const Entity& target) const override;

private:
    /**
     * @brief 选择下一个移动方向
     * @param excludedAxis 排除的轴（避免反向移动）
     */
    void _selectNextMoveDirection(Axis excludedAxis);

    /**
     * @brief 设置移动方向
     */
    void _setDirection(Direction dir);

    /**
     * @brief 更新飞行逻辑
     */
    void _updateFlight();

    Entity* m_target = nullptr;            ///< 目标实体
    std::string m_targetUuid;              ///< 目标UUID（用于重新查找）
    Direction m_direction = Direction::Up; ///< 当前移动方向
    i32 m_flightSteps = 0;                 ///< 剩余飞行步数
    Vector3d m_targetDelta;                ///< 目标速度增量

    // 常量
    static constexpr f32 BULLET_SPEED = 0.15;          ///< 子弹速度
    static constexpr f32 ACCELERATION = 1.025;         ///< 加速度因子
    static constexpr i32 MIN_STEPS = 10;               ///< 最小飞行步数
    static constexpr i32 MAX_STEPS_EXTRA = 5;          ///< 额外飞行步数范围
    static constexpr f32 LEVITATION_DURATION = 200.0f; ///< 漂浮效果持续时间（ticks）
    static constexpr f32 DAMAGE = 4.0f;                ///< 伤害值
};

/**
 * @brief 唤魔者尖牙实体
 *
 * 唤魔者召唤的尖牙攻击，从地下冒出造成伤害。
 *
 * 特性：
 * - 预热延迟：尖牙出现前有预热时间
 * - 范围伤害：对碰撞箱内的生物造成魔法伤害
 * - 队伍判断：不伤害唤魔者及其队友
 * - 有限生命：攻击后自动消失
 */
class EvokerFangsEntity : public Entity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit EvokerFangsEntity(EntityInstanceId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.8f; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    void tick() override;

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 尖牙方法 ==========

    /**
     * @brief 设置所有者
     *
     * 同时设置缓存指针和 UUID，确保两者同步。
     * 参考 MC 1.21.11 EvokerFangs.setOwner(LivingEntity)，
     * 使用双重追踪模式（缓存指针 + UUID），owner 实体失效后可通过 UUID 重新查找。
     *
     * @param owner 所有者实体（唤魔者），可以为 nullptr
     */
    void setOwner(LivingEntity* owner);

    /**
     * @brief 获取所有者（非const版本，可能触发 UUID 懒加载查找）
     *
     * 如果缓存指针有效且实体存活，直接返回缓存指针。
     * 如果缓存指针失效但 UUID 非空，尝试通过 UUID 在世界中重新查找 owner。
     * 参考 AreaEffectCloudEntity::getOwner() 的双重追踪模式。
     *
     * @return 所有者实体指针，可能为 nullptr
     */
    [[nodiscard]] LivingEntity* getOwner();

    /**
     * @brief 获取所有者（const版本，不触发懒加载查找）
     * @return 所有者实体缓存指针，可能为 nullptr
     */
    [[nodiscard]] LivingEntity* owner() const { return m_owner; }

    /**
     * @brief 获取所有者 UUID
     * @return 所有者 UUID 字符串（32字符十六进制），可能为空
     */
    [[nodiscard]] const std::string& ownerUuid() const { return m_ownerUuid; }

    /**
     * @brief 仅设置所有者 UUID（用于 NBT 反序列化）
     *
     * 仅设置 UUID 字段，清空缓存指针，等到 getOwner() 被调用时再通过 UUID 懒加载查找。
     *
     * @param uuid 所有者 UUID 字符串
     */
    void setOwnerUuid(const std::string& uuid);

    /**
     * @brief 设置预热延迟
     * @param delay 预热延迟（ticks）
     */
    void setWarmupDelay(i32 delay) { m_warmupDelay = delay; }

    /**
     * @brief 获取预热延迟
     */
    [[nodiscard]] i32 warmupDelay() const { return m_warmupDelay; }

    /**
     * @brief 获取动画进度
     * @param partialTicks 部分tick时间
     * @return 动画进度（0.0-1.0）
     */
    [[nodiscard]] f32 getAnimationProgress(f32 partialTicks) const;

private:
    /**
     * @brief 对范围内实体造成伤害
     */
    void _damageEntities();

    LivingEntity* m_owner = nullptr;        ///< 所有者缓存指针（唤魔者）
    std::string m_ownerUuid;                ///< 所有者 UUID（持久化，用于跨 tick 重新查找）
    i32 m_warmupDelay = 0;                  ///< 预热延迟（ticks）
    bool m_sentAttackEvent = false;         ///< 是否已发送攻击事件
    i32 m_lifeTicks = 22;                   ///< 生命时长（ticks），默认22
    bool m_clientSideAttackStarted = false; ///< 客户端攻击开始标志
};

/**
 * @brief 末影之眼实体
 *
 * 末影之眼会飞向要塞。
 */
class EyeOfEnderEntity : public Entity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit EyeOfEnderEntity(EntityInstanceId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    void tick() override;

    // ========== 末影之眼方法 ==========

    /**
     * @brief 设置目标位置（要塞方向）
     */
    void moveTo(BlockCoord targetX, BlockCoord targetZ);

    /**
     * @brief 获取目标X坐标
     */
    [[nodiscard]] BlockCoord targetX() const { return m_targetX; }

    /**
     * @brief 获取目标Z坐标
     */
    [[nodiscard]] BlockCoord targetZ() const { return m_targetZ; }

    /**
     * @brief 是否应该碎裂
     */
    [[nodiscard]] bool shouldBreak() const { return m_break; }

private:
    BlockCoord m_targetX = 0; // 目标X
    BlockCoord m_targetZ = 0; // 目标Z
    i32 m_lifetime = 0;       // 存在时间
    bool m_break = false;     // 是否碎裂
};

/**
 * @brief 烟花火箭实体
 *
 * 烟花火箭可以发射、爆炸并产生各种效果。
 * 从弩发射的烟花火箭会对周围实体造成伤害。
 *
 * 伤害机制：
 * - 爆炸半径：5 格
 * - 基础伤害：5 点
 * - 每个爆炸效果增加：+2 点伤害
 * - 距离衰减：damage * sqrt((5 - distance) / 5)
 * - 视线检测：两条射线（脚部和腰部），任一未被方块阻挡即可造成伤害
 */
class FireworkRocketEntity : public ProjectileEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit FireworkRocketEntity(EntityInstanceId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.25f; }
    [[nodiscard]] f32 height() const override { return 0.25f; }

    // 无战利品表，覆写基类方法返回空字符串（ProjectileEntity基类已覆写，此处显式标记）
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    void tick() override;

    // ========== NBT 持久化 ==========

    /**
     * @brief 写出实体额外数据到 NBT
     *
     * 持久化字段：FireworksItem、Life、LifeTime、ShotAtAngle
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 从 NBT 读入实体额外数据
     *
     * 读取顺序：先调用基类 readAdditionalSaveData，再读子类字段。
     * 读取后会重新调用 setFireworkItem 以恢复 m_flightTime 等运行时状态。
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 烟花火箭方法 ==========

    /**
     * @brief 设置烟花物品
     * @param item 烟花火箭物品堆
     *
     * 从物品中读取飞行时间和爆炸效果数据。
     */
    void setFireworkItem(const ItemStack& item);

    /**
     * @brief 获取烟花物品
     * @return 烟花火箭物品堆（可能为空）
     */
    [[nodiscard]] const ItemStack& fireworkItem() const { return m_fireworkItem; }

    /**
     * @brief 是否从弩射出
     */
    [[nodiscard]] bool shotFromCrossbow() const { return m_shotFromCrossbow; }

    /**
     * @brief 设置是否从弩射出
     */
    void setShotFromCrossbow(bool value) { m_shotFromCrossbow = value; }

    /**
     * @brief 获取飞行时间
     */
    [[nodiscard]] i32 flightTime() const { return m_flightTime; }

    /**
     * @brief 设置飞行时间
     */
    void setFlightTime(i32 time) { m_flightTime = time; }

    /**
     * @brief 获取总生命时间（爆炸阈值）
     *
     * 返回创建时一次性确定的总生命 tick 数。爆炸条件为 m_lifetime >= m_lifeTime。
     * 若尚未计算（实体刚创建且未触发懒初始化），返回 -1。
     *
     * 公式：lifeTime = flightTime * 10 + rand.nextInt(6) + rand.nextInt(7)
     */
    [[nodiscard]] i32 lifeTime() const { return m_lifeTime; }

    /**
     * @brief 设置总生命时间（仅供测试和 NBT 反序列化使用）
     *
     * 注意：常规代码不应调用此方法，lifeTime 应由 _ensureLifeTimeComputed() 懒初始化或
     * 由 readAdditionalSaveData 从 NBT 恢复。
     */
    void setLifeTime(i32 time) { m_lifeTime = time; }

    /**
     * @brief 获取已存在时间
     */
    [[nodiscard]] i32 lifetime() const { return m_lifetime; }

    /**
     * @brief 获取爆炸效果数量
     * @return 爆炸效果数量（0 表示无爆炸效果）
     */
    [[nodiscard]] i32 getExplosionCount() const;

    /**
     * @brief 检查视线是否被方块阻挡
     * @param target 目标实体
     * @return 如果视线未被阻挡返回 true
     */
    [[nodiscard]] bool canSeeEntity(const Entity& target) const;

    /**
     * @brief 处理弩发射的伤害
     *
     * 对爆炸半径 5 格内的 LivingEntity 造成伤害。
     * 伤害计算：5 + 爆炸效果数量 * 2，根据距离衰减。
     */
    void dealExplosionDamage();

private:
    /**
     * @brief 爆炸
     */
    void _explode();

    /**
     * @brief 懒计算总生命时间 m_lifeTime
     *
     * 使用世界随机数生成器一次性确定 lifeTime = flightTime * 10 + nextInt(6) + nextInt(7)。
     * 仅在服务端执行（客户端不跑 FireworkRocketEntity::tick）；若 m_lifeTime 已为非负值
     * （NBT 反序列化后已恢复），则跳过计算。
     */
    void _ensureLifeTimeComputed();

    ItemStack m_fireworkItem;        // 烟花火箭物品
    i32 m_flightTime = 1;            // 飞行等级（从物品 NBT Fireworks.Flight 读取）
    i32 m_lifetime = 0;              // 已存在时间（每 tick 递增）
    i32 m_lifeTime = -1;             // 总生命时间（爆炸阈值，-1 表示尚未计算）
    bool m_shotFromCrossbow = false; // 是否从弩射出
};

} // namespace entity
} // namespace mc
