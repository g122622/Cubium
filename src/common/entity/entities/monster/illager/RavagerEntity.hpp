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
#include "AbstractRaiderEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 劫掠兽实体
 *
 * 大型敌对生物，与掠夺者一起参与掠夺事件。
 *
 * 特性：
 * - 强力攻击：高伤害近战攻击
 * - 冲撞：冲撞目标
 * - 破坏方块：可以破坏某些方块
 * - 掠夺：参与掠夺事件
 * - 骑乘：可被掠夺者骑乘
 *
 * 参考 MC 1.16.5 RavagerEntity
 */
class RavagerEntity : public AbstractRaiderEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    RavagerEntity(LegacyEntityType type, EntityId id);
    ~RavagerEntity() override = default;

    // 禁止拷贝
    RavagerEntity(const RavagerEntity&) = delete;
    RavagerEntity& operator=(const RavagerEntity&) = delete;

    // 允许移动
    RavagerEntity(RavagerEntity&&) = default;
    RavagerEntity& operator=(RavagerEntity&&) = default;

    /**
     * @brief 创建劫掠兽实体
     * @param world 世界实例
     * @return 新的劫掠兽实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 攻击系统 ==========

    /**
     * @brief 是否正在攻击
     */
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

    /**
     * @brief 设置攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

    /**
     * @brief 是否正在咆哮
     */
    [[nodiscard]] bool isRoaring() const { return m_roaring; }

    /**
     * @brief 开始咆哮
     */
    void startRoaring();

    /**
     * @brief 是否正在冲撞
     */
    [[nodiscard]] bool isCharging() const { return m_charging; }

    /**
     * @brief 开始冲撞
     */
    void startCharging();

    /**
     * @brief 获取攻击冷却
     */
    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }

    // ========== 骑乘系统 ==========

    /**
     * @brief 是否正在被骑乘
     */
    [[nodiscard]] bool hasRider() const { return m_rider != nullptr; }

    /**
     * @brief 获取骑乘者
     */
    [[nodiscard]] Entity* getRider() const { return m_rider; }

    /**
     * @brief 设置骑乘者
     */
    void setRider(Entity* rider) { m_rider = rider; }

    // ========== 破坏系统 ==========

    /**
     * @brief 是否可以破坏方块
     */
    [[nodiscard]] bool canBreakBlocks() const { return m_canBreakBlocks; }

    /**
     * @brief 设置是否可以破坏方块
     */
    void setCanBreakBlocks(bool canBreak) { m_canBreakBlocks = canBreak; }

    // ========== 属性 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 1.95f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 2.2f; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.05f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 攻击状态
    bool m_attacking = false;
    bool m_roaring = false;
    bool m_charging = false;
    i32 m_roarTime = 0;
    i32 m_chargeTime = 0;
    i32 m_attackCooldown = 0;

    // 骑乘
    Entity* m_rider = nullptr;

    // 破坏
    bool m_canBreakBlocks = true;

    // 常量
    static constexpr i32 ROAR_DURATION = 20;    // 咆哮持续时间
    static constexpr i32 CHARGE_DURATION = 40;  // 冲撞持续时间
    static constexpr i32 ATTACK_COOLDOWN = 30;  // 攻击冷却
    static constexpr f32 ATTACK_DAMAGE = 12.0f; // 攻击伤害
};

} // namespace mc
