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

#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/world/block/Block.hpp"
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

namespace mc {
// 前向声明 - ItemStack 在 mc 命名空间中
class ItemStack;
namespace entity {

/**
 * @brief 箭矢拾取状态
 */
enum class PickupStatus : u8 {
    Disallowed,  // 不允许拾取
    Allowed,     // 允许拾取
    CreativeOnly // 仅创造模式拾取
};

/**
 * @brief 抽象箭矢实体基类
 *
 * 所有箭矢类型（普通箭、光灵箭、三叉戟等）的基类。
 * 提供穿透、暴击、伤害计算等通用功能。
 */
class AbstractArrowEntity : public ProjectileEntity {
public:
    ~AbstractArrowEntity() override = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.5f; }

    void tick() override;

    // ========== 箭矢属性 ==========

    /**
     * @brief 获取伤害值
     */
    [[nodiscard]] f32 damage() const { return m_damage; }

    /**
     * @brief 设置伤害值
     */
    void setDamage(f32 damage) { m_damage = damage; }

    /**
     * @brief 获取击退强度
     */
    [[nodiscard]] i32 knockbackStrength() const { return m_knockbackStrength; }

    /**
     * @brief 设置击退强度
     */
    void setKnockbackStrength(i32 strength) { m_knockbackStrength = strength; }

    /**
     * @brief 是否暴击
     */
    [[nodiscard]] bool isCritical() const { return m_critical; }

    /**
     * @brief 设置暴击状态
     */
    void setCritical(bool critical) { m_critical = critical; }

    /**
     * @brief 获取穿透等级
     */
    [[nodiscard]] u8 pierceLevel() const { return m_pierceLevel; }

    /**
     * @brief 设置穿透等级
     */
    void setPierceLevel(u8 level) { m_pierceLevel = level; }

    /**
     * @brief 是否插在方块中
     */
    [[nodiscard]] bool isInGround() const { return m_inGround; }

    /**
     * @brief 设置是否插在方块中（测试用）
     */
    void setInGround(bool inGround) { m_inGround = inGround; }

    /**
     * @brief 获取拾取状态
     */
    [[nodiscard]] PickupStatus pickupStatus() const { return m_pickupStatus; }

    /**
     * @brief 设置拾取状态
     */
    void setPickupStatus(PickupStatus status) { m_pickupStatus = status; }

    /**
     * @brief 是否从弩射出
     */
    [[nodiscard]] bool shotFromCrossbow() const { return m_shotFromCrossbow; }

    /**
     * @brief 设置是否从弩射出
     */
    void setShotFromCrossbow(bool fromCrossbow) { m_shotFromCrossbow = fromCrossbow; }

    /**
     * @brief 是否已造成伤害（用于三叉戟返回逻辑）
     */
    [[nodiscard]] bool hasDealtDamage() const { return m_dealtDamage; }

    /**
     * @brief 获取在方块中的时间
     */
    [[nodiscard]] i32 timeInGround() const { return m_timeInGround; }

    // ========== 物理 ==========

    [[nodiscard]] f32 getGravity() const override { return 0.05f; }
    [[nodiscard]] f32 getAirDrag() const override { return 0.99f; }
    [[nodiscard]] virtual f32 getWaterDrag() const override { return 0.6f; }

    // ========== 箭矢特有方法 ==========

    /**
     * @brief 设置生物射出箭矢的基础伤害（对应 MC AbstractArrow.setBaseDamageFromMob）
     *
     * 根据蓄力值和难度计算基础伤害：damage = power * 2.0 + random(difficulty * 0.11, 0.57425)
     * 生物射出的箭矢不应用弓类附魔（力量/冲击/火焰）。
     * 三叉戟重写此方法以跳过弓类附魔计算。
     *
     * @param power 蓄力值（0.0 ~ 1.0），来自 RangedAttackGoal 的攻击蓄力
     */
    virtual void setBaseDamageFromMob(f32 power);

    /**
     * @brief 从射手武器应用弓类附魔效果
     *
     * 读取射手手持物品的附魔等级，应用到箭矢：
     * - 力量附魔（Power）：每级 +0.5 伤害 + 基础 0.5
     * - 冲击附魔（Punch）：每级增加 1 点击退强度
     * - 火焰附魔（Flame）：设置箭矢着火 100 ticks
     *
     * @param shooter 射手实体
     */
    void applyBowEnchantments(LivingEntity& shooter);

    /**
     * @brief 当玩家与此箭矢碰撞时调用
     *
     * 检查拾取条件并调用 onPlayerPickup。
     *
     * @param player 与此箭矢碰撞的玩家
     */
    void onCollideWithPlayer(Player& player) override;

    /**
     * @brief 玩家拾取箭矢
     * @param player 玩家
     * @return 是否成功拾取
     */
    virtual bool onPlayerPickup(Player& player);

    /**
     * @brief 获取箭矢对应的物品堆（用于拾取）
     * @return 物品堆副本
     *
     * 子类必须实现此方法返回对应的物品：
     * - ArrowEntity: 返回普通箭矢或药水箭
     * - SpectralArrowEntity: 返回光灵箭
     * - TridentEntity: 返回三叉戟
     */
    [[nodiscard]] virtual ItemStack getArrowStack() const = 0;

protected:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit AbstractArrowEntity(EntityInstanceId id);

    /**
     * @brief 箭矢命中实体时的处理
     */
    void onEntityHit(const RayTraceResult& result) override;

    /**
     * @brief 箭矢命中方块时的处理
     */
    void onBlockHit(const RayTraceResult& result) override;

    /**
     * @brief 箭矢插在方块中的tick处理
     */
    void tickInGround();

    /**
     * @brief 检查是否应该从方块中脱落
     * @return 如果应该脱落返回true
     */
    bool shouldDespawn();

    /**
     * @brief 清除穿透和命中记录
     */
    void clearPiercedEntities();

    /**
     * @brief 检查方块变更导致箭矢脱落
     * @return 如果箭矢应该脱落返回true
     */
    bool checkInBlockEmpty();

    /**
     * @brief 箭矢脱落时的弹射处理
     */
    void detachFromBlock();

    /**
     * @brief 射线追踪实体（考虑穿透）
     */
    RayTraceResult rayTraceEntities(const Vector3& start, const Vector3& end) override;

    /**
     * @brief 检查是否可以命中指定实体（考虑穿透）
     */
    [[nodiscard]] bool canHitEntityWithPierce(const mc::Entity& target) const;

    // 属性
    f32 m_damage = 2.0f;         // 基础伤害
    i32 m_knockbackStrength = 0; // 击退强度
    bool m_critical = false;     // 是否暴击
    u8 m_pierceLevel = 0;        // 穿透等级
    bool m_inGround = false;     // 是否插在方块中
    i32 m_ticksInGround = 0;     // 插在方块中的总时间（用于超时移除）
    i32 m_timeInGround = 0;      // 当前连续插在方块中的时间（用于三叉戟返回）
    i32 m_arrowShake = 0;        // 箭矢抖动时间
    PickupStatus m_pickupStatus = PickupStatus::Disallowed;
    bool m_shotFromCrossbow = false;
    bool m_dealtDamage = false; // 是否已造成伤害（三叉戟用）

    // 穿透追踪（使用 unordered_set 实现 O(1) 查找）
    std::unordered_set<EntityInstanceId> m_piercedEntities;

    // 命中的方块状态
    std::optional<BlockState> m_inBlockState;
};

/**
 * @brief 普通箭矢实体
 *
 * 弓和弩射出的标准箭矢。
 * 也用于药水箭（Tipped Arrow），存储药水效果。
 */
class ArrowEntity : public AbstractArrowEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit ArrowEntity(EntityInstanceId id);

    /**
     * @brief 从发射者创建
     * @param shooter 发射者
     * @param world 世界
     */
    static std::unique_ptr<ArrowEntity> createFromShooter(LivingEntity& shooter, IWorld* world);

    // ========== Entity 接口重写 ==========

    void tick() override;

protected:
    /**
     * @brief 箭矢命中实体时的处理
     *
     * 药水箭命中时会给目标施加药水效果。
     */
    void onEntityHit(const RayTraceResult& result) override;

public:
    // ========== 箭矢特有方法 ==========

    /**
     * @brief 设置箭矢颜色
     * @param color RGB颜色值
     */
    void setColor(u32 color) { m_color = color; }

    /**
     * @brief 获取箭矢颜色
     */
    [[nodiscard]] u32 color() const { return m_color; }

    /**
     * @brief 设置是否为光灵箭
     */
    void setGlowing(bool glowing) { m_glowing = glowing; }

    /**
     * @brief 是否为光灵箭
     */
    [[nodiscard]] bool isGlowing() const { return m_glowing; }

    // ========== 药水效果 ==========

    /**
     * @brief 添加药水效果
     * @param effect 效果实例
     */
    void addEffect(const entity::effect::EffectInstance& effect) { m_effects.push_back(effect); }

    /**
     * @brief 设置药水效果列表
     * @param effects 效果列表
     */
    void setEffects(const std::vector<entity::effect::EffectInstance>& effects) { m_effects = effects; }

    /**
     * @brief 获取药水效果列表
     */
    [[nodiscard]] const std::vector<entity::effect::EffectInstance>& effects() const { return m_effects; }

    /**
     * @brief 是否有药水效果
     */
    [[nodiscard]] bool hasEffects() const { return !m_effects.empty(); }

    // ========== AbstractArrowEntity 接口实现 ==========

    /**
     * @brief 获取箭矢对应的物品堆
     * @return 普通箭矢或药水箭物品堆
     */
    [[nodiscard]] ItemStack getArrowStack() const override;

private:
    u32 m_color = 0xFFFFFFFF;                              // 箭矢颜色（药水箭）
    bool m_glowing = false;                                // 是否发光（光灵箭）
    std::vector<entity::effect::EffectInstance> m_effects; // 药水效果列表
};

/**
 * @brief 光灵箭实体
 *
 * 光灵箭会让被命中的实体发光。
 */
class SpectralArrowEntity : public AbstractArrowEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    explicit SpectralArrowEntity(EntityInstanceId id);

    // ========== Entity 接口重写 ==========

    void tick() override;

protected:
    /**
     * @brief 箭矢命中实体时的处理
     *
     * 光灵箭命中时会给目标施加发光效果。
     */
    void onEntityHit(const RayTraceResult& result) override;

public:
    // ========== AbstractArrowEntity 接口实现 ==========

    /**
     * @brief 获取箭矢对应的物品堆
     * @return 光灵箭物品堆
     */
    [[nodiscard]] ItemStack getArrowStack() const override;

private:
    i32 m_glowDuration = 200; // 发光持续时间（ticks）
};

} // namespace entity
} // namespace mc
