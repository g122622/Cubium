#pragma once

#include "Entity.hpp"
#include "../attribute/AttributeMap.hpp"
#include "../attribute/Attributes.hpp"
#include "../damage/DamageSource.hpp"
#include "../damage/CombatTracker.hpp"
#include "../effect/EffectManager.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../sound/SoundCategory.hpp"
#include "common/physics/PhysicsConstants.hpp"

#include <array>
#include <memory>

namespace mc {

// 前向声明
class World;

/**
 * @brief 装备槽位
 *
 * 定义实体可穿戴的装备槽位
 */
enum class EquipmentSlot : u8 {
    MainHand = 0,   // 主手
    OffHand = 1,    // 副手
    Feet = 2,       // 靴子
    Legs = 3,       // 护腿
    Chest = 4,      // 胸甲
    Head = 5,       // 头盔
    Count = 6       // 槽位数量
};

/**
 * @brief 生物实体基类
 *
 * 所有有生命值的实体的基类，包括玩家、怪物、动物等。
 * 提供生命值、属性、装备、药水效果等功能。
 *
 * 数据参数（通过 EntityDataManager 同步）：
 * - LIVING_FLAGS: 生物标志（手部动画等）
 * - HEALTH: 当前生命值
 * - POTION_EFFECTS: 药水效果颜色
 * - ARROW_COUNT: 箭矢数量
 *
 * 渲染属性（用于客户端插值）：
 * - limbSwing, limbSwingAmount: 步态动画
 * - swingProgress: 攻击动画
 * - renderYawOffset: 身体旋转偏移
 * - rotationYawHead: 头部旋转
 *
 * 参考 MC 1.16.5 LivingEntity
 */
class LivingEntity : public Entity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     * @param world 世界指针（可选）
     */
    LivingEntity(LegacyEntityType type, EntityId id, IWorld* world = nullptr);

    ~LivingEntity() override = default;

    // 禁止拷贝
    LivingEntity(const LivingEntity&) = delete;
    LivingEntity& operator=(const LivingEntity&) = delete;

    // 允许移动
    LivingEntity(LivingEntity&&) = default;
    LivingEntity& operator=(LivingEntity&&) = default;

    // ========== 初始化 ==========

    void registerData() override;

    /**
     * @brief 注册默认属性
     *
     * 子类应重写此方法来注册自己的属性
     */
    virtual void registerAttributes();

    // ========== 生命值 ==========

    /**
     * @brief 获取当前生命值
     */
    [[nodiscard]] f32 health() const { return m_health; }

    /**
     * @brief 获取最大生命值
     */
    [[nodiscard]] f32 maxHealth() const;

    /**
     * @brief 设置生命值
     * @param health 新生命值
     */
    void setHealth(f32 health);

    /**
     * @brief 治疗实体
     * @param amount 治疗量
     */
    void heal(f32 amount);

    /**
     * @brief 获取声音音量
     */
    [[nodiscard]] virtual f32 getSoundVolume() const { return 1.0f; }

    /**
     * @brief 获取声音音调
     */
    [[nodiscard]] virtual f32 getSoundPitch() const;

    /**
     * @brief 受伤
     * @param source 伤害来源
     * @param amount 伤害量
     * @return 是否成功造成伤害
     */
    virtual bool hurt(DamageSource& source, f32 amount);

    /**
     * @brief 是否死亡
     */
    [[nodiscard]] bool isDead() const { return m_health <= 0.0f; }

    /**
     * @brief 死亡
     * @param cause 死亡原因
     */
    virtual void die(DamageSource& cause);

    /**
     * @brief 掉落经验
     *
     * 在死亡时调用，生成经验球。子类可以重写此方法。
     */
    virtual void dropExperience() {}

    // ========== 属性 ==========

    /**
     * @brief 获取属性映射表
     */
    entity::attribute::AttributeMap& attributes() { return m_attributes; }
    [[nodiscard]] const entity::attribute::AttributeMap& attributes() const { return m_attributes; }

    /**
     * @brief 获取属性值
     * @param name 属性名称
     * @param defaultValue 默认值
     */
    [[nodiscard]] f64 getAttributeValue(const String& name, f64 defaultValue = 0.0) const;

    /**
     * @brief 设置属性基础值
     * @param name 属性名称
     * @param value 新值
     */
    void setAttributeBaseValue(const String& name, f64 value);

    // ========== 装备 ==========

    /**
     * @brief 获取装备
     * @param slot 装备槽位
     */
    [[nodiscard]] const ItemStack& getEquipment(EquipmentSlot slot) const;

    /**
     * @brief 设置装备
     * @param slot 装备槽位
     * @param stack 物品堆
     */
    void setEquipment(EquipmentSlot slot, const ItemStack& stack);

    /**
     * @brief 获取主手物品
     */
    [[nodiscard]] const ItemStack& getMainHandItem() const { return getEquipment(EquipmentSlot::MainHand); }

    /**
     * @brief 设置主手物品
     */
    void setMainHandItem(const ItemStack& stack) { setEquipment(EquipmentSlot::MainHand, stack); }

    /**
     * @brief 获取副手物品
     */
    [[nodiscard]] const ItemStack& getOffHandItem() const { return getEquipment(EquipmentSlot::OffHand); }

    /**
     * @brief 设置副手物品
     */
    void setOffHandItem(const ItemStack& stack) { setEquipment(EquipmentSlot::OffHand, stack); }

    // ========== 主手偏好 ==========

    /**
     * @brief 获取主手侧边
     * @return 左手或右手
     */
    [[nodiscard]] HandSide getPrimaryHand() const { return m_primaryHand; }

    /**
     * @brief 是否右撇子（右手为主手）
     */
    [[nodiscard]] bool isRightHanded() const { return m_primaryHand == HandSide::Right; }

    /**
     * @brief 设置主手侧边
     */
    void setPrimaryHand(HandSide hand) { m_primaryHand = hand; }

    // ========== 受伤无敌帧 ==========

    /**
     * @brief 获取受伤无敌时间
     */
    [[nodiscard]] i32 hurtTime() const { return m_hurtTime; }

    /**
     * @brief 获取最大受伤无敌时间
     */
    [[nodiscard]] i32 maxHurtTime() const { return m_maxHurtTime; }

    /**
     * @brief 是否处于受伤无敌状态
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const;

    /**
     * @brief 获取最近受伤来源
     */
    [[nodiscard]] DamageSource* lastDamageSource() const { return m_lastDamageSource.get(); }

    // ========== 渲染属性（用于客户端插值）==========

    /**
     * @brief 获取步态动画周期
     * 用于腿部动画
     */
    [[nodiscard]] f32 limbSwing() const { return m_limbSwing; }
    [[nodiscard]] f32 prevLimbSwing() const { return m_prevLimbSwing; }

    /**
     * @brief 获取步态动画速度
     * 表示移动速度对动画的影响
     */
    [[nodiscard]] f32 limbSwingAmount() const { return m_limbSwingAmount; }
    [[nodiscard]] f32 prevLimbSwingAmount() const { return m_prevLimbSwingAmount; }

    /**
     * @brief 获取攻击动画进度
     * 0.0 - 1.0，表示挥动手臂的进度
     */
    [[nodiscard]] f32 swingProgress() const { return m_swingProgress; }
    [[nodiscard]] f32 prevSwingProgress() const { return m_prevSwingProgress; }

    /**
     * @brief 获取身体旋转偏移
     * 用于身体朝向与头部朝向的分离
     */
    [[nodiscard]] f32 renderYawOffset() const { return m_renderYawOffset; }
    [[nodiscard]] f32 prevRenderYawOffset() const { return m_prevRenderYawOffset; }

    /**
     * @brief 获取头部旋转
     * 头部的实际朝向
     */
    [[nodiscard]] f32 rotationYawHead() const { return m_rotationYawHead; }
    [[nodiscard]] f32 prevRotationYawHead() const { return m_prevRotationYawHead; }

    /**
     * @brief 是否正在挥动手臂
     */
    [[nodiscard]] bool isSwingInProgress() const { return m_swingInProgress; }

    /**
     * @brief 获取挥动手臂进度
     */
    [[nodiscard]] i32 swingProgressInt() const { return m_swingProgressInt; }

    // ========== 跳跃 ==========

    /**
     * @brief 是否正在跳跃
     */
    [[nodiscard]] bool isJumping() const { return m_isJumping; }

    /**
     * @brief 设置跳跃状态
     */
    virtual void setJumping(bool jumping) { m_isJumping = jumping; }

    /**
     * @brief 执行跳跃
     *
     * 设置垂直速度为跳跃初速度。
     * 参考 MC LivingEntity.jump()
     */
    void jump();

    /**
     * @brief 获取跳跃冷却
     */
    [[nodiscard]] i32 jumpTicks() const { return m_jumpTicks; }

    /**
     * @brief 获取跳跃初速度
     */
    [[nodiscard]] f32 jumpUpwardsMotion() const { return m_jumpUpwardsMotion; }

    // ========== 移动 ==========

    /**
     * @brief 获取横向移动速度
     */
    [[nodiscard]] f32 moveStrafing() const { return m_moveStrafing; }

    /**
     * @brief 获取前进移动速度
     */
    [[nodiscard]] f32 moveForward() const { return m_moveForward; }

    /**
     * @brief 设置移动方向
     */
    void setMoveStrafing(f32 strafing) { m_moveStrafing = strafing; }
    void setMoveForward(f32 forward) { m_moveForward = forward; }

    /**
     * @brief 获取AI移动速度
     *
     * 参考 MC 1.16.5 LivingEntity.getAIMoveSpeed()
     */
    [[nodiscard]] f32 aiMoveSpeed() const { return m_landMovementFactor; }

    /**
     * @brief 设置AI移动速度
     */
    void setAIMoveSpeed(f32 speed) { m_landMovementFactor = speed; }

    /**
     * @brief 执行移动（AI物理更新核心方法）
     *
     * 根据 moveStrafing 和 moveForward 计算移动向量并执行物理移动。
     * 这是 MC LivingEntity.travel() 的核心逻辑。
     *
     * 参考 MC 1.16.5 LivingEntity.travel()
     *
     * @param strafing 横向移动量（左右）
     * @param vertical 垂直移动量（上下，用于飞行/游泳）
     * @param forward 前进移动量（前后）
     */
    virtual void travel(f32 strafing, f32 vertical, f32 forward);

    /**
     * @brief AI步进更新
     *
     * 处理AI移动逻辑，应用阻力，调用travel方法。
     * 参考 MC 1.16.5 LivingEntity.aiStep() / livingTick()
     */
    virtual void aiStep();

    // ========== 战斗追踪 ==========

    /**
     * @brief 获取战斗追踪器
     */
    [[nodiscard]] CombatTracker& combatTracker() { return m_combatTracker; }
    [[nodiscard]] const CombatTracker& combatTracker() const { return m_combatTracker; }

    // ========== 效果系统 ==========

    /**
     * @brief 获取效果管理器
     */
    [[nodiscard]] entity::effect::EffectManager& effectManager() { return m_effectManager; }
    [[nodiscard]] const entity::effect::EffectManager& effectManager() const { return m_effectManager; }

    /**
     * @brief 添加效果
     * @param effect 效果实例
     * @return 是否成功添加
     */
    bool addEffect(entity::effect::EffectInstance effect);

    /**
     * @brief 移除效果
     * @param type 效果类型
     */
    void removeEffect(entity::effect::EffectType type);

    /**
     * @brief 移除所有效果
     */
    void removeAllEffects();

    /**
     * @brief 检查是否有效果
     * @param type 效果类型
     */
    [[nodiscard]] bool hasEffect(entity::effect::EffectType type) const;

    /**
     * @brief 获取效果实例
     * @param type 效果类型
     * @return 效果实例指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const entity::effect::EffectInstance* getEffect(entity::effect::EffectType type) const;

    /**
     * @brief 获取效果等级
     * @param type 效果类型
     * @return 效果等级（0 = 无效果）
     */
    [[nodiscard]] i32 getEffectLevel(entity::effect::EffectType type) const;

    // ========== 死亡 ==========

    /**
     * @brief 是否正在死亡
     */
    [[nodiscard]] bool isDying() const { return m_deathTime > 0; }

    /**
     * @brief 获取死亡时间
     */
    [[nodiscard]] i32 deathTime() const { return m_deathTime; }

    // ========== 刻更新 ==========

    void tick() override;

    /**
     * @brief 将数据参数同步回实体字段
     */
    void syncMetadataFromDataManager() override;

    /**
     * @brief 生命值刻更新
     */
    virtual void tickHealth();

    /**
     * @brief 死亡刻更新
     */
    virtual void tickDeath();

    // ========== 摔落伤害 ==========

    /**
     * @brief 处理摔落伤害
     * @param distance 摔落距离
     * @param damageMultiplier 伤害倍率
     */
    void handleFallDamage(f32 distance, f32 damageMultiplier) override;

protected:
    /**
     * @brief 更新动画参数
     */
    virtual void updateAnimation();

    /**
     * @brief 播放受伤声音
     */
    virtual void playHurtSound(DamageSource& source);

    /**
     * @brief 播放死亡声音
     */
    virtual void playDeathSound();

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getHurtSound(DamageSource& source) const;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getDeathSound() const;

    // 生命值
    f32 m_health = 20.0f;
    f32 m_lastHealth = 20.0f;           // 上一tick的生命值
    f32 m_absorption = 0.0f;             // 吸收值（金苹果）

    // 属性
    entity::attribute::AttributeMap m_attributes;

    // 装备
    std::array<ItemStack, static_cast<size_t>(EquipmentSlot::Count)> m_equipment;

    // 主手偏好
    HandSide m_primaryHand = HandSide::Right;  // 默认右手为主手

    // 受伤无敌帧
    i32 m_hurtTime = 0;                  // 受伤无敌时间
    i32 m_maxHurtTime = 10;              // 最大受伤无敌时间
    static constexpr i32 MAX_HURT_RESISTANT_TIME = 20;  // 最大无敌帧
    f32 m_lastDamage = 0.0f;             // 最近伤害量
    std::unique_ptr<DamageSource> m_lastDamageSource;  // 最近伤害来源

    // 死亡
    i32 m_deathTime = 0;                 // 死亡时间

    // 回血
    i32 m_healTime = 0;                  // 回血计时器

    // 渲染插值属性
    f32 m_limbSwing = 0.0f;              // 步态动画周期
    f32 m_prevLimbSwing = 0.0f;          // 上一帧步态周期
    f32 m_limbSwingAmount = 0.0f;        // 步态动画速度
    f32 m_prevLimbSwingAmount = 0.0f;    // 上一帧步态速度
    f32 m_swingProgress = 0.0f;          // 攻击动画进度
    f32 m_prevSwingProgress = 0.0f;      // 上一帧攻击进度
    i32 m_swingProgressInt = 0;          // 攻击动画计数
    bool m_swingInProgress = false;      // 是否正在攻击动画

    // 身体旋转
    f32 m_renderYawOffset = 0.0f;        // 身体旋转偏移
    f32 m_prevRenderYawOffset = 0.0f;    // 上一帧身体旋转
    f32 m_rotationYawHead = 0.0f;        // 头部旋转
    f32 m_prevRotationYawHead = 0.0f;    // 上一帧头部旋转

    // 跳跃
    bool m_isJumping = false;
    i32 m_jumpTicks = 0;                 // 跳跃冷却
    f32 m_jumpUpwardsMotion = physics::JUMP_VELOCITY;     // 跳跃初速度（MC默认值）

    // 移动
    f32 m_moveStrafing = 0.0f;           // 横向移动（左右）
    f32 m_moveForward = 0.0f;            // 前进移动（前后）
    f32 m_jumpMovementFactor = 0.02f;    // 跳跃时的移动因子
    f32 m_landMovementFactor = 0.1f;     // 陆地移动因子（AI移动速度）

    // 移动距离（用于动画）
    f32 m_movedDistance = 0.0f;          // 移动距离
    f32 m_prevMovedDistance = 0.0f;      // 上一帧移动距离

    // 受伤动画
    f32 m_attackedAtYaw = 0.0f;          // 受伤时的偏航角

    // 最近攻击
    i32 m_ticksSinceLastSwing = 0;       // 上次攻击后的 tick

    // 战斗追踪
    CombatTracker m_combatTracker;       // 战斗追踪器

    // 效果管理
    entity::effect::EffectManager m_effectManager;  // 效果管理器
};

} // namespace mc
