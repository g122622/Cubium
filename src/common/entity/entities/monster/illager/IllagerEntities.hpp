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

#include "../../../../core/Types.hpp"
#include "../../../interfaces/ICrossbowUser.hpp"
#include "AbstractIllagerEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include <memory>

// Forward declarations
namespace mc {
class ItemStack;
class LivingEntity;
} // namespace mc

namespace mc {

/**
 * @brief 掠夺者实体
 *
 * 使用弩进行远程攻击的灾厄村民。
 *
 * 特性：
 * - 装备弩
 * - 可以加入掠夺事件
 * - 可以成为掠夺队长
 *
 * 继承链: MonsterEntity -> PatrollerEntity -> AbstractRaiderEntity -> AbstractIllagerEntity -> PillagerEntity
 */
class PillagerEntity : public AbstractIllagerEntity, public entity::ICrossbowUser {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    PillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~PillagerEntity() override = default;

    // 禁止拷贝
    PillagerEntity(const PillagerEntity&) = delete;
    PillagerEntity& operator=(const PillagerEntity&) = delete;

    // 允许移动
    PillagerEntity(PillagerEntity&&) = delete;
    PillagerEntity& operator=(PillagerEntity&&) = delete;

    /// 本类继承链标识（parent = AbstractIllagerEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    // ========== IRangedAttackMob 接口 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override { return 20; }
    [[nodiscard]] bool canRangedAttack() const override { return true; }

    // ========== 尺寸 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 1.95f; }

    // ========== ICrossbowUser 接口 ==========

    void setChargingCrossbow(bool charging) override { m_isCharging = charging; }
    [[nodiscard]] bool isChargingCrossbow() const override { return m_isCharging; }
    void onCrossbowLoadComplete(::mc::ItemStack& crossbow) override;
    void shootCrossbow(::mc::LivingEntity* target, ::mc::ItemStack& crossbow, f32 charge) override;
    [[nodiscard]] i32 getCrossbowChargeTime() const override { return 25; }

    // ========== 弩相关（旧接口，保持兼容） ==========

    /**
     * @brief 是否正在装填弩
     */
    [[nodiscard]] bool isCharging() const { return m_isCharging; }

    /**
     * @brief 设置装填状态
     */
    void setCharging(bool charging) { m_isCharging = charging; }

    // ========== 生命周期 ==========

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_isCharging = false;
};

/**
 * @brief 卫道士实体
 *
 * 手持铁斧进行近战攻击的灾厄村民。
 *
 * 特性：
 * - 高攻击伤害
 * - 可以加入掠夺事件
 * - 攻击村民和玩家
 * - "Johnny" 彩蛋：攻击所有生物
 *
 * 继承链: MonsterEntity -> PatrollerEntity -> AbstractRaiderEntity -> AbstractIllagerEntity -> VindicatorEntity
 */
class VindicatorEntity : public AbstractIllagerEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    VindicatorEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~VindicatorEntity() override = default;

    // 禁止拷贝
    VindicatorEntity(const VindicatorEntity&) = delete;
    VindicatorEntity& operator=(const VindicatorEntity&) = delete;

    // 允许移动
    VindicatorEntity(VindicatorEntity&&) = delete;
    VindicatorEntity& operator=(VindicatorEntity&&) = delete;

    /// 本类继承链标识（parent = AbstractIllagerEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    // ========== 行为状态 ==========

    /**
     * @brief 是否处于攻击状态
     */
    [[nodiscard]] bool isAggressive() const { return m_aggressive; }

    /**
     * @brief 设置攻击状态
     */
    void setAggressive(bool aggressive) { m_aggressive = aggressive; }

    // ========== Johnny 彩蛋 ==========

    /**
     * @brief 是否处于 Johnny 模式
     *
     * 对齐 vanilla 1.21.11 Vindicator.isJohnny（Vindicator.java:54）。卫道士被命名牌
     * 命名为 "Johnny" 时激活 VindicatorJohnnyAttackGoal，攻击所有可攻击生物（非仅
     * 玩家/村民/铁傀儡）。一旦激活即锁存（即便后续改名也不再取消），对齐 vanilla
     * setCustomName 中 `if (!isJohnny && name.equals("Johnny")) isJohnny = true`。
     */
    [[nodiscard]] bool isJohnny() const { return m_isJohnny; }

    /**
     * @brief 设置 Johnny 模式（测试/存档恢复用）
     */
    void setJohnny(bool johnny) { m_isJohnny = johnny; }

    /**
     * @brief 重写命名设置以触发 Johnny 锁存
     *
     * 对齐 vanilla Vindicator.setCustomName（Vindicator.java:145-150）：调基类后，
     * 若当前未激活 Johnny 且新名称纯文本为 "Johnny"，则激活。经 setCustomName(string)
     * 委托，命名牌等文本路径同样触发。
     */
    void setCustomNameComponent(std::unique_ptr<text::ITextComponent> name) override;

    // ========== 尺寸 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 1.95f; }

    // ========== 生命周期 ==========

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_aggressive = false;
    bool m_isJohnny = false; // 对齐 vanilla Vindicator.isJohnny（Vindicator.java:54）
};

} // namespace mc
