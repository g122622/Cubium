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

#include "../MonsterEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 恼鬼实体
 *
 * 由唤魔者召唤的小型飞行敌对生物。
 *
 * 特性：
 * - 飞行：可以穿墙飞行
 * - 小碰撞箱：极小的体型
 * - 有限生命：存活约2分钟后死亡
 * - 穿墙：可以穿过任何方块
 * - 召唤：由唤魔者召唤
 */
class VexEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    VexEntity(EntityInstanceId id);
    ~VexEntity() override = default;

    // 禁止拷贝
    VexEntity(const VexEntity&) = delete;
    VexEntity& operator=(const VexEntity&) = delete;

    // 允许移动
    VexEntity(VexEntity&&) = delete;
    VexEntity& operator=(VexEntity&&) = delete;

    /// 本类继承链标识（parent = MonsterEntity::classInfo()）。见 Entity::classInfo()。
    // 独立链（不经 Raider），透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建恼鬼实体
     * @param world 世界实例
     * @return 新的恼鬼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 生命周期 ==========

    /**
     * @brief 是否有限生命
     */
    [[nodiscard]] bool hasLimitedLife() const { return m_limitedLife; }

    /**
     * @brief 设置有限生命
     */
    void setLimitedLife(bool limited) { m_limitedLife = limited; }

    /**
     * @brief 获取剩余生命时间
     */
    [[nodiscard]] i32 getLifeTime() const { return m_lifeTime; }

    /**
     * @brief 设置生命时间
     */
    void setLifeTime(i32 time) { m_lifeTime = time; }

    // ========== 主人系统 ==========

    /**
     * @brief 获取主人
     */
    [[nodiscard]] LivingEntity* getOwner() const { return m_owner; }

    /**
     * @brief 设置主人
     */
    void setOwner(LivingEntity* owner) { m_owner = owner; }

    // ========== 攻击系统 ==========

    /**
     * @brief 是否正在充电攻击
     */
    [[nodiscard]] bool isCharging() const { return m_charging; }

    /**
     * @brief 设置充电攻击状态
     */
    void setCharging(bool charging) { m_charging = charging; }

    // ========== 飞行系统 ==========

    /**
     * @brief 恼鬼可以飞行
     */
    [[nodiscard]] bool canFly() const { return true; }

    // ========== 属性 ==========

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.4f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 0.8f; }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.4f; }

    /**
     * @brief 恼鬼不会燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 生命周期
    bool m_limitedLife = true;
    i32 m_lifeTime = 2400; // 约2分钟

    // 主人
    LivingEntity* m_owner = nullptr;

    // 攻击状态
    bool m_charging = false;
};

} // namespace mc
