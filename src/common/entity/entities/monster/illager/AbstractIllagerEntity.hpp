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

#include "AbstractRaiderEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include <memory>

namespace mc {

/**
 * @brief 灾厄村民抽象基类
 *
 * 灾厄村民（Illager）的共同基类，包括掠夺者、唤魔者、幻术师、卫道士等。
 *
 * 继承链: MonsterEntity -> PatrollerEntity -> AbstractRaiderEntity -> AbstractIllagerEntity
 *
 * 特性：
 * - 敌对玩家和村民
 * - 参与掠夺事件
 * - 有团队协作能力
 * - 手臂姿势状态
 */
class AbstractIllagerEntity : public AbstractRaiderEntity {
public:
    /**
     * @brief 灾厄村民手臂姿势
     *
     * 用于客户端渲染
     */
    enum class ArmPose : u8 {
        Crossed = 0,        // 交叉
        Attacking = 1,      // 攻击
        Spellcasting = 2,   // 施法
        BowAndArrow = 3,    // 弓箭
        CrossbowHold = 4,   // 弩持有
        CrossbowCharge = 5, // 弩装填
        Celebrating = 6,    // 庆祝
        Neutral = 7         // 中立
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AbstractIllagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~AbstractIllagerEntity() override = default;

    // 禁止拷贝
    AbstractIllagerEntity(const AbstractIllagerEntity&) = delete;
    AbstractIllagerEntity& operator=(const AbstractIllagerEntity&) = delete;

    // 允许移动
    AbstractIllagerEntity(AbstractIllagerEntity&&) = delete;
    AbstractIllagerEntity& operator=(AbstractIllagerEntity&&) = delete;

    /// 本类继承链标识（parent = AbstractRaiderEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段（m_armPose 用普通成员承载、不同步），classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    // ========== 手臂姿势 ==========

    /**
     * @brief 获取当前手臂姿势
     */
    [[nodiscard]] ArmPose getArmPose() const { return m_armPose; }

    /**
     * @brief 设置手臂姿势
     */
    void setArmPose(ArmPose pose) { m_armPose = pose; }

protected:
    ArmPose m_armPose = ArmPose::Crossed;
};

} // namespace mc
