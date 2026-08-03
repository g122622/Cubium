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

#include "common/core/Types.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/interfaces/IAngerable.hpp"
#include <memory>
#include <optional>

namespace mc {

/**
 * @brief 傀儡实体基类
 *
 * 由玩家或村庄自然生成的保护性生物。
 *
 * 特性：
 * - 保护：保护村民或玩家
 * - 中立：通常对玩家中立
 * - 强壮：高生命值和攻击力
 *
 * 继承链：Entity -> LivingEntity -> MobEntity -> CreatureEntity -> GolemEntity
 */
class GolemEntity : public CreatureEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    GolemEntity(EntityInstanceId id);
    ~GolemEntity() override = default;

    /// 本类继承链标识（parent = CreatureEntity::classInfo()）。见 Entity::classInfo()。
    // TODO(实体同步对齐, 见 entity-sync-alignment-decisions-2026-07): 本类是 1.16.5 遗留中间层，
    // vanilla 1.21.11 类树已调整（PathfinderMob/WaterAnimal/AgeableWaterCreature 等），本项目保留此层。
    // 若后期 vanilla 此层有同步字段须补 registerData+ClassRegisterGuard，当前仅占位 classInfo。
    static const entity::EntityClassInfo& classInfo();

    // 禁止拷贝
    GolemEntity(const GolemEntity&) = delete;
    GolemEntity& operator=(const GolemEntity&) = delete;

    // 允许移动
    GolemEntity(GolemEntity&&) = delete;
    GolemEntity& operator=(GolemEntity&&) = delete;

    // ========== IAngerable 接口实现 ==========

    void setAttackTarget(LivingEntity* target) override { MobEntity::setAttackTarget(target); }
    [[nodiscard]] LivingEntity* getAttackTarget() const override
    {
        return const_cast<GolemEntity*>(this)->MobEntity::attackTarget();
    }
    void setRevengeTarget(LivingEntity* target) override;
    [[nodiscard]] LivingEntity* getRevengeTarget() const override;
    [[nodiscard]] i32 getRevengeTimer() const override { return m_revengeTimer; }
    [[nodiscard]] bool isAngry() const override { return m_angerTime > 0; }
    void setAngry(bool angry) override;
    [[nodiscard]] i32 getAngerTime() const override { return m_angerTime; }
    void setAngerTime(i32 time) override { m_angerTime = time; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 更新愤怒状态
     */
    void updateAnger() override;

private:
    // 愤怒系统（m_attackTarget 使用 MobEntity::m_attackTarget，不重复声明）
    i32 m_angerTime = 0;
    i32 m_revengeTimer = 0;
    std::optional<u64> m_revengeTargetId;

    // 常量
    static constexpr i32 MAX_ANGER_TIME = 600; // 30秒
};

} // namespace mc
