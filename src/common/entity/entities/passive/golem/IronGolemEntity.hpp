#pragma once

#include "../../../../core/Types.hpp"
#include "GolemEntity.hpp"
#include <memory>

namespace mc {

// Forward declarations
class VillagerEntity;

/**
 * @brief 铁傀儡实体
 *
 * 保护村民的大型傀儡。
 *
 * 特性：
 * - 保护村民：攻击威胁村民的生物
 * - 中立：对玩家中立，除非被激怒
 * - 举起手臂：攻击时会举起手臂
 * - 击飞：攻击会将敌人击飞
 * - 生成：村民足够多时自然生成
 * - 掉落：铁锭、罂粟
 *
 * 参考 MC 1.16.5 IronGolemEntity
 */
class IronGolemEntity : public GolemEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    IronGolemEntity(LegacyEntityType type, EntityId id);
    ~IronGolemEntity() override = default;

    // 禁止拷贝
    IronGolemEntity(const IronGolemEntity&) = delete;
    IronGolemEntity& operator=(const IronGolemEntity&) = delete;

    // 允许移动
    IronGolemEntity(IronGolemEntity&&) = default;
    IronGolemEntity& operator=(IronGolemEntity&&) = default;

    /**
     * @brief 创建铁傀儡实体
     * @param world 世界实例
     * @return 新的铁傀儡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 攻击状态 ==========

    /**
     * @brief 是否举起手臂
     */
    [[nodiscard]] bool isArmsRaised() const { return m_armsRaised; }

    /**
     * @brief 设置手臂状态
     */
    void setArmsRaised(bool raised) { m_armsRaised = raised; }

    /**
     * @brief 获取攻击计时器
     */
    [[nodiscard]] i32 getAttackTimer() const { return m_attackTimer; }

    /**
     * @brief 设置攻击计时器
     */
    void setAttackTimer(i32 timer) { m_attackTimer = timer; }

    // ========== 生成 ==========

    /**
     * @brief 是否由玩家创建
     */
    [[nodiscard]] bool isPlayerCreated() const { return m_playerCreated; }

    /**
     * @brief 设置玩家创建标记
     */
    void setPlayerCreated(bool created) { m_playerCreated = created; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.1f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 1.4f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 2.7f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 攻击状态
    bool m_armsRaised = false;
    i32 m_attackTimer = 0;

    // 生成标记
    bool m_playerCreated = false;

    // 攻击冷却
    i32 m_attackCooldown = 0;

    // 常量
    static constexpr i32 ATTACK_DURATION = 10; // 攻击动画持续时间
    static constexpr i32 ATTACK_COOLDOWN = 20; // 攻击冷却
    static constexpr f32 ATTACK_DAMAGE = 7.0f; // 攻击伤害
    static constexpr f32 KNOCKBACK = 1.5f;     // 击退力度
};

} // namespace mc
