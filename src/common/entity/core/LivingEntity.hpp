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

#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/CombatTracker.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectManager.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"

#include <array>
#include <map>
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
    MainHand = 0, // 主手
    OffHand = 1,  // 副手
    Feet = 2,     // 靴子
    Legs = 3,     // 护腿
    Chest = 4,    // 胸甲
    Head = 5,     // 头盔
    Count = 6     // 槽位数量
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
 */
class LivingEntity : public Entity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param world 世界指针（可选）
     */
    LivingEntity(EntityId id, IWorld* world = nullptr);

    ~LivingEntity() override = default;

    // 禁止拷贝
    LivingEntity(const LivingEntity&) = delete;
    LivingEntity& operator=(const LivingEntity&) = delete;

    // 禁止移动（基类 Entity 不可移动）
    LivingEntity(LivingEntity&&) = delete;
    LivingEntity& operator=(LivingEntity&&) = delete;

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
     * @brief 获取吸收伤害值（金苹果效果）
     */
    [[nodiscard]] f32 absorptionAmount() const { return m_absorption; }

    /**
     * @brief 设置吸收伤害值
     * @param amount 新的吸收值，会被限制在 [0, maxAbsorption] 范围内
     */
    void setAbsorptionAmount(f32 amount);

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
     *
     * 伤害处理流程：
     * 1. 检查无敌状态
     * 2. 检查无敌帧（允许累积伤害）
     * 3. 调用 actuallyHurt() 进行实际伤害计算
     * 4. 击退处理
     * 5. 战斗追踪器记录
     * 6. 死亡检查
     *
     * @param source 伤害来源
     * @param amount 伤害量
     * @return 是否成功造成伤害
     */
    virtual bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 实际受伤处理
     *
     * 在无敌检查通过后调用，进行实际的伤害计算：
     * 1. 护甲减伤
     * 2. 药水效果减伤
     * 3. 附魔保护减伤
     * 4. 吸收值处理
     * 5. 扣血
     *
     * @param source 伤害来源
     * @param amount 伤害量
     */
    virtual void actuallyHurt(DamageSource& source, f32 amount);

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
     * @brief 由 /kill 命令调用
     *
     * 重写 Entity::onKillCommand()，使用虚空伤害杀死实体。
     * 这确保实体会经历完整的死亡流程（触发死亡事件、掉落物品等）。
     */
    void onKillCommand() override;

    /**
     * @brief 检查是否可以格挡伤害来源
     *
     * 检查实体是否正在使用盾牌格挡，以及伤害来源是否可以被格挡。
     *
     * @param source 伤害来源
     * @return 是否可以格挡
     */
    [[nodiscard]] virtual bool canBlockDamageSource(DamageSource& source) const;

    /**
     * @brief 受伤时损坏护甲
     *
     * 默认空实现，由 Player 子类重写。
     *
     * @param source 伤害来源
     * @param amount 伤害量
     */
    virtual void damageArmor(DamageSource& source, f32 amount);

    /**
     * @brief 受伤时损坏盾牌
     *
     * @param amount 伤害量
     */
    virtual void damageShield(f32 amount);

    /**
     * @brief 掉落经验
     *
     * 在死亡时调用，生成经验球。子类可以重写此方法。
     */
    virtual void dropExperience() {}

    /**
     * @brief 检查药水效果是否可以应用
     *
     * 子类可以重写此方法来免疫某些药水效果。
     * 例如：凋灵免疫凋零效果。
     *
     * @param effect 药水效果实例
     * @return 是否可以应用此效果
     */
    [[nodiscard]] virtual bool isPotionApplicable(const entity::effect::EffectInstance& effect) const;

    /**
     * @brief 检查是否可以被药水影响
     *
     * 盔甲架重写此方法返回 false，其他生物返回 true。
     *
     * @return 是否可以被药水影响
     */
    [[nodiscard]] virtual bool canBeHitWithPotion() const { return true; }

    /**
     * @brief 摔落伤害处理
     *
     * 子类可以重写此方法来免疫摔落伤害。
     * 例如：凋灵、末影龙免疫摔落伤害。
     *
     * @param distance 摔落距离（格数）
     * @param damageMultiplier 伤害倍率
     * @return 是否受到摔落伤害
     */
    [[nodiscard]] virtual bool onLivingFall(f32 distance, f32 damageMultiplier);

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
    [[nodiscard]] f64 getAttributeValue(const std::string& name, f64 defaultValue = 0.0) const;

    /**
     * @brief 设置属性基础值
     * @param name 属性名称
     * @param value 新值
     */
    void setAttributeBaseValue(const std::string& name, f64 value);

    // ========== 装备 ==========

    /**
     * @brief 获取装备
     * @param slot 装备槽位
     */
    [[nodiscard]] virtual const ItemStack& getEquipment(EquipmentSlot slot) const;

    /**
     * @brief 设置装备
     * @param slot 装备槽位
     * @param stack 物品堆
     */
    virtual void setEquipment(EquipmentSlot slot, const ItemStack& stack);

    /**
     * @brief 装备损坏回调
     *
     * 当装备物品耐久度耗尽时调用，广播装备破损动画并播放音效。
     * 对应 MC 原版 LivingEntity.onEquippedItemBroken()。
     * ServerPlayer 重写此方法以额外更新物品损坏统计。
     *
     * @param item 损坏的物品类型
     * @param slot 损坏物品所在的装备槽位
     */
    virtual void onEquippedItemBroken(const Item& item, EquipmentSlot slot);

    /**
     * @brief 广播装备破损事件
     *
     * 向追踪玩家广播装备破损状态码，客户端据此播放破损动画和音效。
     * 对应 MC 原版 LivingEntity.broadcastBreakEvent()。
     *
     * @param slot 破损的装备槽位
     */
    void broadcastBreakEvent(EquipmentSlot slot);

    /**
     * @brief 检测装备更新
     *
     * 每tick调用，检测装备槽位变化并同步属性修饰符。
     * 当装备发生变化时：
     * 1. 移除旧物品的属性修饰符
     * 2. 添加新物品的属性修饰符
     * 3. 同步装备数据到客户端
     *
     * 对应 MC 原版 LivingEntity.detectEquipmentUpdates()。
     */
    void detectEquipmentUpdates();

    /**
     * @brief 检查两个物品堆是否不同（用于装备变化检测）
     *
     * 对应 MC 原版 LivingEntity.equipmentHasChanged()。
     *
     * @param a 第一个物品堆
     * @param b 第二个物品堆
     * @return 如果两个物品堆不同返回 true
     */
    [[nodiscard]] static bool equipmentHasChanged(const ItemStack& a, const ItemStack& b);

    /**
     * @brief 停止基于位置的物品效果
     *
     * 当装备从槽位移除时调用，移除物品提供的属性修饰符。
     * 对应 MC 原版 LivingEntity.stopLocationBasedEffects() 中移除属性修饰符的部分。
     * TODO: 当附魔基于位置的效果系统（Frost Walker、Soul Speed 等）实现后，
     * 此方法应同时调用 EnchantmentHelper.stopLocationBasedEffects() 来停用
     * 位置相关的附魔效果。
     *
     * @param stack 物品堆
     * @param slot 装备槽位
     */
    void stopLocationBasedEffects(const ItemStack& stack, EquipmentSlot slot);

    /**
     * @brief 将 Hand 转换为 EquipmentSlot
     *
     * @param hand 手部槽位
     * @return 对应的装备槽位
     */
    [[nodiscard]] static EquipmentSlot handToEquipmentSlot(Hand hand)
    {
        return hand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand;
    }

    /**
     * @brief 对物品施加耐久损耗，若物品损坏则触发 onEquippedItemBroken 回调
     *
     * 对应 MC 原版 ItemStack.hurtAndBreak(int, LivingEntity, EquipmentSlot)。
     * 在调用 attemptDamageItem 之前保存物品指针（因为损坏后 ItemStack 会被清空），
     * 若物品损坏则调用 onEquippedItemBroken 广播破损动画、播放音效、更新统计。
     *
     * @param stack 要损坏的物品堆引用
     * @param amount 耐久损耗量
     * @param entity 执行损坏的实体（用于耐久保护附魔和回调），可以为 nullptr
     * @param slot 物品所在的装备槽位
     * @return true 若物品损坏（耐久耗尽），false 否则
     */
    static bool hurtAndBreak(ItemStack& stack, i32 amount, LivingEntity* entity, EquipmentSlot slot);

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

    /**
     * @brief 获取护甲槽位数组
     *
     * 返回头盔、胸甲、护腿、靴子的指针数组，用于附魔保护计算。
     *
     * @return 护甲槽位数组 [头盔, 胸甲, 护腿, 靴子]
     */
    [[nodiscard]] std::array<const ItemStack*, 4> getArmorSlots() const
    {
        return {
            &m_equipment[static_cast<size_t>(EquipmentSlot::Head)],  // 头盔
            &m_equipment[static_cast<size_t>(EquipmentSlot::Chest)], // 胸甲
            &m_equipment[static_cast<size_t>(EquipmentSlot::Legs)],  // 护腿
            &m_equipment[static_cast<size_t>(EquipmentSlot::Feet)]   // 靴子
        };
    }

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

    // ========== 生物属性 ==========

    /**
     * @brief 获取生物属性类型
     *
     * 用于附魔（如亡灵杀手、节肢杀手）对特定生物类型造成额外伤害。
     * 默认返回 Undefined。
     */
    [[nodiscard]] virtual CreatureAttribute getCreatureAttribute() const { return CreatureAttribute::Undefined; }

    // ========== 空气供应和溺水 ==========

    /**
     * @brief 是否可以在水下呼吸
     *
     * 亡灵生物（僵尸、骷髅等）可以在水下呼吸。
     *
     * @return 如果可以在水下呼吸返回 true
     */
    [[nodiscard]] virtual bool canBreatheUnderwater() const
    {
        return getCreatureAttribute() == CreatureAttribute::Undead;
    }

    /**
     * @brief 减少空气供应
     *
     * 考虑水下呼吸附魔的概率性空气节约。
     *
     * @param currentAir 当前空气量
     * @return 减少后的空气量
     */
    [[nodiscard]] i32 decreaseAirSupply(i32 currentAir);

    /**
     * @brief 计算下一个空气值（恢复空气）
     *
     * @param currentAir 当前空气量
     * @return 恢复后的空气量
     */
    [[nodiscard]] i32 determineNextAir(i32 currentAir) const;

    /**
     * @brief 更新空气供应和溺水伤害
     *
     * 每tick调用，处理空气消耗和溺水伤害。
     */
    virtual void updateAirSupply();

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
     * @brief 获取无敌帧计时器
     */
    [[nodiscard]] i32 hurtResistantTime() const { return m_hurtResistantTime; }

    /**
     * @brief 设置无敌帧计时器
     *
     * 用于陷阱触发时设置初始无敌帧。
     * @param ticks 无敌帧 tick 数
     */
    void setHurtResistantTime(i32 ticks) { m_hurtResistantTime = ticks; }

    /**
     * @brief 是否处于受伤无敌状态
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const override;

    /**
     * @brief 获取最近受伤来源
     */
    [[nodiscard]] DamageSource* lastDamageSource() const { return m_lastDamageSource.get(); }

    // ========== 受伤追踪（Target Goals 使用）==========

    /**
     * @brief 获取最近攻击该实体的实体
     *
     * @return 最近攻击者，无则返回nullptr
     */
    [[nodiscard]] LivingEntity* getLastHurtBy() { return m_lastHurtBy; }
    [[nodiscard]] const LivingEntity* getLastHurtBy() const { return m_lastHurtBy; }

    /**
     * @brief 获取最近被攻击的时间戳（tick）
     *
     * @return tick 时间戳
     */
    [[nodiscard]] i32 lastHurtByTimestamp() const { return m_lastHurtByTimestamp; }

    /**
     * @brief 设置最近攻击者
     *
     * @param attacker 攻击者
     */
    virtual void setLastHurtBy(LivingEntity* attacker);

    /**
     * @brief 获取该实体最近攻击的目标
     *
     * @return 最近攻击的目标，无则返回nullptr
     */
    [[nodiscard]] LivingEntity* getLastHurtTarget() { return m_lastHurtTarget; }
    [[nodiscard]] const LivingEntity* getLastHurtTarget() const { return m_lastHurtTarget; }

    /**
     * @brief 获取最近攻击目标的时间戳（tick）
     *
     * @return tick 时间戳
     */
    [[nodiscard]] i32 lastHurtTargetTimestamp() const { return m_lastHurtTargetTimestamp; }

    /**
     * @brief 设置最近攻击的目标
     *
     * @param target 攻击目标
     */
    void setLastHurtTarget(LivingEntity* target);

    // ========== 击退 ==========

    /**
     * @brief 应用击退效果
     *
     * 击退强度会被击退抗性属性降低。
     *
     * @param strength 击退强度（默认为1.0）
     * @param ratioX X方向比例（归一化后会乘以强度）
     * @param ratioZ Z方向比例（归一化后会乘以强度）
     */
    void applyKnockback(f32 strength, f64 ratioX, f64 ratioZ);

    /**
     * @brief 应用击退效果（带来源实体）
     *
     * 从攻击者位置计算击退方向。
     *
     * @param attacker 攻击者实体
     * @param strength 击退强度
     */
    void applyKnockbackFrom(LivingEntity* attacker, f32 strength);

    /**
     * @brief 应用额外击退（冲刺击退/攻击击退）
     *
     * 在 hurt() 之后调用，应用来自攻击者的额外击退力。
     * 基类版本仅对 LivingEntity 目标调用 knockback() 并减缓攻击者水平速度。
     * Player 子类重写此方法以处理 ServerPlayer 目标的速度重复应用问题。
     *
     * @param target 击退目标实体
     * @param strength 额外击退强度（包含冲刺加成和附魔击退）
     * @param preHurtVelocity 目标在 hurt() 调用之前的速度（用于 ServerPlayer 速度修正）
     */
    virtual void causeExtraKnockback(Entity& target, f32 strength, const Vector3& preHurtVelocity);

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
     * @brief 设置身体旋转偏移
     */
    void setRenderYawOffset(f32 yaw) { m_renderYawOffset = yaw; }

    /**
     * @brief 获取头部旋转
     * 头部的实际朝向
     */
    [[nodiscard]] f32 rotationYawHead() const { return m_rotationYawHead; }
    [[nodiscard]] f32 prevRotationYawHead() const { return m_prevRotationYawHead; }

    /**
     * @brief 设置头部偏航角
     */
    void setRotationYawHead(f32 yaw) { m_rotationYawHead = yaw; }

    /**
     * @brief 设置头部俯仰角
     */
    void setRotationPitch(f32 pitch) { m_pitch = pitch; }

    /**
     * @brief 是否正在挥动手臂
     */
    [[nodiscard]] bool isSwingInProgress() const { return m_swingInProgress; }

    /**
     * @brief 获取挥动手臂进度
     */
    [[nodiscard]] i32 swingProgressInt() const { return m_swingProgressInt; }

    /**
     * @brief 获取正在挥动的手
     *
     * 返回当前正在挥动的手（主手或副手）。
     *
     * @return 正在挥动的手，默认为主手
     */
    [[nodiscard]] Hand swingingHand() const { return m_swingingHand; }

    /**
     * @brief 挥动手臂（攻击动画）- 主手
     *
     * 触发攻击动画，持续6 tick。
     */
    void swingArm() { swing(Hand::MainHand); }

    /**
     * @brief 挥动手臂（攻击动画）- 指定手
     *
     * 触发攻击动画，持续6 tick。
     *
     * @param hand 挥动的手（主手或副手）
     */
    void swing(Hand hand)
    {
        // 条件判断允许在动画进行到一半时重新触发
        if (!m_swingInProgress || m_swingProgressInt >= getArmSwingAnimationEnd() / 2 || m_swingProgressInt < 0) {
            m_swingProgressInt = -1;
            m_swingInProgress = true;
            m_swingingHand = hand;
        }
    }

    /**
     * @brief 获取手臂挥动动画持续 tick 数
     *
     * 默认返回 6 tick，急迫效果减少，挖掘疲劳增加。
     *
     * @return 动画持续 tick 数
     */
    [[nodiscard]] i32 getArmSwingAnimationEnd() const;

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
     *
     * @param strafing 横向移动量（左右）
     * @param vertical 垂直移动量（上下，用于飞行/游泳）
     * @param forward 前进移动量（前后）
     */
    virtual void travel(f32 strafing, f32 vertical, f32 forward);

    /**
     * @brief 执行移动（Vector3版本）
     *
     * 便捷方法，将Vector3分解为三个分量调用travel(f32, f32, f32)。
     *
     * @param travelVec 移动向量 (x=左右, y=上下, z=前后)
     */
    virtual void travel(const Vector3& travelVec) { travel(travelVec.x, travelVec.y, travelVec.z); }

    /**
     * @brief AI步进更新
     *
     * 处理AI移动逻辑，应用阻力，调用travel方法。
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

    // ========== 物品使用 ==========

    /**
     * @brief 开始使用物品
     *
     * 设置正在使用的手和物品，开始物品使用倒计时。
     *
     * @param hand 使用的手
     */
    void setActiveHand(Hand hand);

    /**
     * @brief 停止使用物品
     *
     * 重置使用状态，调用物品的 onPlayerStoppedUse。
     */
    void stopActiveHand();

    /**
     * @brief 获取正在使用的手
     * @return 正在使用的手，如果没有则返回 Hand::MainHand
     */
    [[nodiscard]] Hand getActiveHand() const { return m_activeHand; }

    /**
     * @brief 获取正在使用的物品
     * @return 正在使用的物品堆
     */
    [[nodiscard]] const ItemStack& getActiveItem() const { return m_activeItem; }

    /**
     * @brief 获取剩余使用时间
     * @return 剩余使用时间（ticks）
     */
    [[nodiscard]] i32 getItemInUseCount() const { return m_activeItemUseCount; }

    /**
     * @brief 检查是否正在使用物品
     * @return 是否正在使用物品
     */
    [[nodiscard]] bool isUsingItem() const { return m_activeItemUseCount > 0 && !m_activeItem.isEmpty(); }

    // ========== 三叉戟激流攻击 ==========

    /**
     * @brief 检查是否正在进行激流攻击（旋转攻击）
     *
     * 通过检查 LIVING_FLAGS 的第2位（0x04）来判断。
     *
     * @return 如果正在进行激流攻击返回 true
     */
    [[nodiscard]] bool isSpinAttacking() const;

    /**
     * @brief 开始激流攻击
     *
     * 设置 SpinAttack 标志并初始化持续时间。
     * 持续时间内实体会以 SpinAttack 姿态旋转前进。
     *
     * @param duration 攻击持续时间（ticks）
     */
    void startSpinAttack(i32 duration);

    /**
     * @brief 停止激流攻击
     *
     * 清除 SpinAttack 标志。
     */
    void stopSpinAttack();

    /**
     * @brief 更新激流攻击状态
     *
     * 在 tick() 中调用，递减持续时间计时器。
     */
    void updateSpinAttack();

    /**
     * @brief 获取激流攻击剩余时间
     * @return 剩余 ticks，0 表示不在攻击中
     */
    [[nodiscard]] i32 spinAttackDuration() const { return m_spinAttackDuration; }

    /**
     * @brief 更新物品使用
     *
     * 每tick调用，递减使用计时器。
     */
    void updateActiveItem();

    /**
     * @brief 获取效果等级
     * @param type 效果类型
     * @return 效果等级（0 = 无效果）
     */
    [[nodiscard]] i32 getEffectLevel(entity::effect::EffectType type) const;

    // ========== 攻击附魔回调 ==========

    /**
     * @brief 攻击目标时调用附魔回调
     *
     * 在攻击成功后调用，触发武器附魔的效果（如节肢杀家的缓慢效果）。
     *
     * @param target 被攻击的目标实体
     */
    void onAttackEntity(Entity& target);

    // ========== 死亡 ==========

    /**
     * @brief 是否正在死亡
     */
    [[nodiscard]] bool isDying() const { return m_deathTime > 0; }

    /**
     * @brief 获取死亡时间
     */
    [[nodiscard]] i32 deathTime() const { return m_deathTime; }

    // ========== 箭矢计数 ==========

    /**
     * @brief 获取插在身上的箭矢数量
     *
     * 用于渲染层（ArrowLayer）渲染插在实体身上的箭矢。
     *
     * @return 箭矢数量
     */
    [[nodiscard]] i32 getArrowCount() const { return m_arrowCount; }

    /**
     * @brief 设置插在身上的箭矢数量
     *
     * 当箭矢命中实体时调用以增加计数。
     *
     * @param count 箭矢数量
     */
    void setArrowCountInEntity(i32 count);

    /**
     * @brief 更新箭矢自动脱落逻辑
     *
     * 箭矢数量越多，脱落间隔越短：
     * - 1 支箭约 29 秒脱落
     * - 15 支箭约 15 秒脱落
     */
    void tickArrows();

    // ========== 方块交互 ==========

    /**
     * @brief 检查实体是否小心行走（潜行状态）
     *
     * 重写基类方法。LivingEntity 在潜行时返回 true。
     * 小心行走的实体不会触发 onEntityWalk 回调。
     *
     * @return 如果实体正在潜行返回true
     */
    [[nodiscard]] bool isSteppingCarefully() const override { return isSneaking(); }

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

    /**
     * @brief 使用自定义伤害来源处理摔落伤害
     * @param distance 摔落距离
     * @param damageMultiplier 伤害倍率
     * @param source 伤害来源
     *
     * 重写 Entity::causeFallDamage 以支持自定义伤害来源。
     * 计算逻辑与 handleFallDamage 相同，但使用传入的伤害来源而非默认的 DamageSources::fall()。
     */
    void causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source) override;

    // ========== NBT 序列化 ==========

    /**
     * @brief 序列化 LivingEntity 特有数据
     *
     * 写入 Health, AbsorptionAmount, HurtTime, DeathTime, HurtByTimestamp,
     * FallFlying, ActiveEffects, Attributes, equipment 等。
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 反序列化 LivingEntity 特有数据
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    /**
     * @brief 更新动画参数
     */
    virtual void updateAnimation();

    /**
     * @brief 更新移动动画参数
     *
     * 在 travel() 结束时调用，更新 limbSwingAmount 和 limbSwing。
     *
     * @param includeVertical 是否包含垂直位移
     *                        true: 飞行实体（蜜蜂、鹦鹉等）包含 Y 轴位移
     *                        false: 普通实体只计算水平位移
     */
    void updateTravelAnimation(bool includeVertical);

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

    /**
     * @brief 获取摔落声音
     *
     * 子类可重写以提供特定摔落音效。
     *
     * @param fallHeight 摔落高度（格数）
     * @return 摔落音效，默认返回空
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getFallSound(i32 fallHeight) const;

    /**
     * @brief 播放摔落音效
     *
     * 在 handleFallDamage 中调用，播放实体摔落音效和方块摔落音效。
     *
     * @param distance 摔落距离
     */
    void playFallSound(f32 distance);

protected:
    /**
     * @brief 计算护甲减伤后的伤害
     *
     * @param source 伤害来源
     * @param damage 原始伤害
     * @return 减伤后的伤害
     */
    [[nodiscard]] virtual f32 applyArmorCalculations(DamageSource& source, f32 damage);

    /**
     * @brief 计算药水效果减伤后的伤害
     *
     * 包括抗性药水和附魔保护。
     *
     * @param source 伤害来源
     * @param damage 原始伤害
     * @return 减伤后的伤害
     */
    [[nodiscard]] virtual f32 applyPotionDamageCalculations(DamageSource& source, f32 damage);

    /**
     * @brief 计算最终伤害
     *
     * 结合护甲、药水、附魔、吸收值等所有减伤效果。
     *
     * @param source 伤害来源
     * @param damage 原始伤害
     * @return 最终伤害
     */
    [[nodiscard]] f32 computeFinalDamage(DamageSource& source, f32 damage);

    /**
     * @brief 进入战斗状态
     *
     * 子类可重写以发送数据包通知客户端。
     */
    virtual void sendEnterCombat() {}

    /**
     * @brief 结束战斗状态
     *
     * 子类可重写以发送数据包通知客户端。
     */
    virtual void sendEndCombat() {}

    // 生命值
    f32 m_health = 20.0f;
    f32 m_lastHealth = 20.0f; // 上一tick的生命值
    f32 m_absorption = 0.0f;  // 吸收值（金苹果）

    // 属性
    entity::attribute::AttributeMap m_attributes;

    // 装备
    std::array<ItemStack, static_cast<size_t>(EquipmentSlot::Count)> m_equipment;

    // 上一tick的装备快照（用于检测装备变化并同步属性修饰符）
    // 对应 MC 原版 LivingEntity.lastEquipmentItems
    std::array<ItemStack, static_cast<size_t>(EquipmentSlot::Count)> m_lastEquipment;

    // 是否已初始化上一tick装备快照
    bool m_lastEquipmentInitialized = false;

    // 主手偏好
    HandSide m_primaryHand = HandSide::Right; // 默认右手为主手

    // 受伤无敌帧
    i32 m_hurtTime = 0;                                // 受伤无敌时间
    i32 m_maxHurtTime = 10;                            // 最大受伤无敌时间
    static constexpr i32 MAX_HURT_RESISTANT_TIME = 20; // 最大无敌帧（20 tick = 1秒）
    f32 m_lastDamage = 0.0f;                           // 最近伤害量（用于累积伤害）
    std::unique_ptr<DamageSource> m_lastDamageSource;  // 最近伤害来源
    i32 m_hurtResistantTime = 0;                       // 无敌帧计时器

    // 战斗状态
    bool m_inCombat = false;       // 是否在战斗中
    i32 m_lastDamageTimestamp = 0; // 最后受伤时间戳

    // 死亡
    i32 m_deathTime = 0; // 死亡时间

    // 回血
    i32 m_healTime = 0;         // 回血计时器
    i32 m_regenTickCounter = 0; // 生命恢复 tick 计数器

    // 渲染插值属性
    f32 m_limbSwing = 0.0f;               // 步态动画周期
    f32 m_prevLimbSwing = 0.0f;           // 上一帧步态周期
    f32 m_limbSwingAmount = 0.0f;         // 步态动画速度
    f32 m_prevLimbSwingAmount = 0.0f;     // 上一帧步态速度
    f32 m_swingProgress = 0.0f;           // 攻击动画进度
    f32 m_prevSwingProgress = 0.0f;       // 上一帧攻击进度
    i32 m_swingProgressInt = 0;           // 攻击动画计数
    bool m_swingInProgress = false;       // 是否正在攻击动画
    Hand m_swingingHand = Hand::MainHand; // 正在挥动的手

    // 身体旋转
    f32 m_renderYawOffset = 0.0f;     // 身体旋转偏移
    f32 m_prevRenderYawOffset = 0.0f; // 上一帧身体旋转
    f32 m_rotationYawHead = 0.0f;     // 头部旋转
    f32 m_prevRotationYawHead = 0.0f; // 上一帧头部旋转

    // 跳跃
    bool m_isJumping = false;
    i32 m_jumpTicks = 0;                              // 跳跃冷却
    f32 m_jumpUpwardsMotion = physics::JUMP_VELOCITY; // 跳跃初速度（MC默认值）

    // 移动
    f32 m_moveStrafing = 0.0f;        // 横向移动（左右）
    f32 m_moveForward = 0.0f;         // 前进移动（前后）
    f32 m_jumpMovementFactor = 0.02f; // 跳跃时的移动因子
    f32 m_landMovementFactor = 0.1f;  // 陆地移动因子（AI移动速度）

    // 移动距离（用于动画）
    f32 m_movedDistance = 0.0f;     // 移动距离
    f32 m_prevMovedDistance = 0.0f; // 上一帧移动距离

    // 受伤动画
    f32 m_attackedAtYaw = 0.0f; // 受伤时的偏航角

    // 最近攻击追踪（Target Goals 使用）
    LivingEntity* m_lastHurtBy = nullptr;     // 最近攻击该实体的实体
    i32 m_lastHurtByTimestamp = 0;            // 被攻击时间戳
    LivingEntity* m_lastHurtTarget = nullptr; // 该实体最近攻击的目标
    i32 m_lastHurtTargetTimestamp = 0;        // 攻击目标时间戳

    // 最近攻击
    i32 m_ticksSinceLastSwing = 0; // 上次攻击后的 tick

    // 战斗追踪
    CombatTracker m_combatTracker; // 战斗追踪器

    // 效果管理
    entity::effect::EffectManager m_effectManager; // 效果管理器

    // 物品使用状态
    Hand m_activeHand = Hand::MainHand; // 正在使用的手
    ItemStack m_activeItem;             // 正在使用的物品堆
    i32 m_activeItemUseCount = 0;       // 剩余使用时间（ticks）

    // 溺水伤害计时器
    i32 m_drownDamageTimer = 0; // 溺水伤害间隔计时器

    // 三叉戟激流攻击状态
    i32 m_spinAttackDuration = 0; // 激流攻击剩余持续时间（ticks）

    // 箭矢计数
    i32 m_arrowCount = 0;    // 插在身上的箭矢数量
    i32 m_arrowHitTimer = 0; // 箭矢脱落计时器

    // 静态数据参数（通过 EntityDataManager::createKey 自动分配唯一 ID）
    static entity::DataParameter<i8> DATA_LIVING_FLAGS_PARAM;
    static entity::DataParameter<f32> DATA_HEALTH_PARAM;
    static entity::DataParameter<i32> DATA_POTION_EFFECTS_PARAM;
    static entity::DataParameter<i32> DATA_ARROW_COUNT_PARAM;
};

} // namespace mc
