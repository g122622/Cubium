#pragma once

#include "../../../../core/Types.hpp"
#include "GuardianEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 远古守卫者实体
 *
 * 海底神殿的Boss级怪物。
 *
 * 特性：
 * - 更强大：比普通守卫者更强
 * - 挖掘疲劳：给予附近的玩家挖掘疲劳
 * - 激光攻击：更强的激光攻击
 * - Boss血条：显示Boss血条
 *
 * 参考 MC 1.16.5 ElderGuardianEntity
 */
class ElderGuardianEntity : public GuardianEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    ElderGuardianEntity(LegacyEntityType type, EntityId id);

    ~ElderGuardianEntity() override = default;

    // 禁止拷贝
    ElderGuardianEntity(const ElderGuardianEntity&) = delete;
    ElderGuardianEntity& operator=(const ElderGuardianEntity&) = delete;

    // 允许移动
    ElderGuardianEntity(ElderGuardianEntity&&) = default;
    ElderGuardianEntity& operator=(ElderGuardianEntity&&) = default;

    /**
     * @brief 创建远古守卫者实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 挖掘疲劳 ==========

    /**
     * @brief 是否应该给予挖掘疲劳
     */
    [[nodiscard]] bool shouldApplyMiningFatigue() const { return true; }

    /**
     * @brief 获取挖掘疲劳范围
     */
    [[nodiscard]] f32 getMiningFatigueRange() const { return MINING_FATIGUE_RANGE; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.0f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerAttributes() override;

private:
    i32 m_fatigueTimer = 0;

    static constexpr f32 MINING_FATIGUE_RANGE = 50.0f; // 50格范围
    static constexpr i32 FATIGUE_INTERVAL = 600;       // 每30秒应用一次
};

} // namespace mc
