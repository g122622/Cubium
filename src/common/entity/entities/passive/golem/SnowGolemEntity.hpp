#pragma once

#include "GolemEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 雪傀儡实体
 *
 * 由玩家创造的傀儡。
 *
 * 特性：
 * - 投掷雪球：攻击敌人
 * - 留下雪迹：行走时会留下雪层
 * - 融化：在高温生物群系或水中会融化
 * - 掉落：雪球
 * - 南瓜头：可以用剪刀取下南瓜
 *
 * 参考 MC 1.16.5 SnowGolemEntity
 */
class SnowGolemEntity : public GolemEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SnowGolemEntity(LegacyEntityType type, EntityId id);
    ~SnowGolemEntity() override = default;

    // 禁止拷贝
    SnowGolemEntity(const SnowGolemEntity&) = delete;
    SnowGolemEntity& operator=(const SnowGolemEntity&) = delete;

    // 允许移动
    SnowGolemEntity(SnowGolemEntity&&) = default;
    SnowGolemEntity& operator=(SnowGolemEntity&&) = default;

    /**
     * @brief 创建雪傀儡实体
     * @param world 世界实例
     * @return 新的雪傀儡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 南瓜头 ==========

    /**
     * @brief 是否戴着南瓜
     */
    [[nodiscard]] bool hasPumpkin() const { return m_hasPumpkin; }

    /**
     * @brief 设置南瓜状态
     */
    void setPumpkin(bool hasPumpkin) { m_hasPumpkin = hasPumpkin; }

    /**
     * @brief 取下南瓜
     * 用剪刀取下南瓜
     */
    void shearPumpkin();

    // ========== 融化 ==========

    /**
     * @brief 是否会融化
     * 检查当前环境是否会导致融化
     */
    [[nodiscard]] bool willMelt() const;

    /**
     * @brief 获取融化计时器
     */
    [[nodiscard]] i32 getMeltTimer() const { return m_meltTimer; }

    // ========== 攻击 ==========

    /**
     * @brief 获取攻击冷却
     */
    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }

    /**
     * @brief 重置攻击冷却
     */
    void resetAttackCooldown() { m_attackCooldown = ATTACK_COOLDOWN; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.7f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 南瓜头
    bool m_hasPumpkin = true;

    // 融化计时器
    i32 m_meltTimer = 0;

    // 攻击冷却
    i32 m_attackCooldown = 0;

    // 雪层放置冷却
    i32 m_snowPlaceCooldown = 0;

    // 常量
    static constexpr i32 ATTACK_COOLDOWN = 10;       // 雪球攻击冷却
    static constexpr i32 SNOW_PLACE_INTERVAL = 20;   // 雪层放置间隔
    static constexpr f32 SNOWBALL_DAMAGE = 0.0f;     // 雪球伤害（对烈焰人3）
    static constexpr i32 MELT_DAMAGE_INTERVAL = 20;  // 融化伤害间隔
    static constexpr f32 MELT_DAMAGE = 1.0f;         // 融化伤害量
};

} // namespace mc
