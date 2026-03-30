#pragma once

#include "../../core/MobEntity.hpp"
#include "../../../interfaces/IAngerable.hpp"
#include "../../../core/Types.hpp"
#include <memory>

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
 * 参考 MC 1.16.5 GolemEntity
 */
class GolemEntity : public MobEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    GolemEntity(LegacyEntityType type, EntityId id);
    ~GolemEntity() override = default;

    // 禁止拷贝
    GolemEntity(const GolemEntity&) = delete;
    GolemEntity& operator=(const GolemEntity&) = delete;

    // 允许移动
    GolemEntity(GolemEntity&&) = default;
    GolemEntity& operator=(GolemEntity&&) = default;

    // ========== IAngerable 接口实现 ==========

    void setAttackTarget(LivingEntity* target) override { m_attackTarget = target; }
    [[nodiscard]] LivingEntity* getAttackTarget() const override { return m_attackTarget; }
    void setRevengeTarget(LivingEntity* target) override;
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
    void updateAnger();

private:
    // 愤怒系统
    LivingEntity* m_attackTarget = nullptr;
    i32 m_angerTime = 0;
    std::optional<u64> m_revengeTargetId;

    // 常量
    static constexpr i32 MAX_ANGER_TIME = 600; // 30秒
};

} // namespace mc
