#pragma once

#include "AbstractIllagerEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 幻术师实体
 *
 * 灾厄村民的法术使用者，会施放失明和分身。
 * 只能通过命令生成。
 *
 * 特性：
 * - 失明法术：使目标失明
 * - 分身法术：创建4个分身
 * - 弓箭攻击：使用弓箭远程攻击
 * - 掠夺：参与掠夺事件
 *
 * 参考 MC 1.16.5 IllusionerEntity
 */
class IllusionerEntity : public AbstractIllagerEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    IllusionerEntity(LegacyEntityType type, EntityId id);
    ~IllusionerEntity() override = default;

    // 禁止拷贝
    IllusionerEntity(const IllusionerEntity&) = delete;
    IllusionerEntity& operator=(const IllusionerEntity&) = delete;

    // 允许移动
    IllusionerEntity(IllusionerEntity&&) = default;
    IllusionerEntity& operator=(IllusionerEntity&&) = default;

    /**
     * @brief 创建幻术师实体
     * @param world 世界实例
     * @return 新的幻术师实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    i32 getAttackInterval() const override { return 20; }
    bool canRangedAttack() const override { return true; }

    // ========== 法术系统 ==========

    /**
     * @brief 是否正在施法
     */
    [[nodiscard]] bool isCasting() const { return m_casting; }

    /**
     * @brief 设置施法状态
     */
    void setCasting(bool casting) { m_casting = casting; }

    /**
     * @brief 施放失明法术
     */
    void castBlindnessSpell();

    /**
     * @brief 施放分身法术
     */
    void castMirrorSpell();

    /**
     * @brief 是否有分身
     */
    [[nodiscard]] bool hasMirrors() const { return !m_mirrorEntities.empty(); }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    // 法术状态
    bool m_casting = false;
    i32 m_blindnessCooldown = 0;
    i32 m_mirrorCooldown = 0;

    // 分身实体ID
    std::vector<EntityId> m_mirrorEntities;

    // 常量
    static constexpr i32 BLINDNESS_COOLDOWN = 100;  // 失明冷却
    static constexpr i32 MIRROR_COOLDOWN = 600;     // 分身冷却
    static constexpr i32 MIRROR_DURATION = 100;     // 分身持续时间
};

} // namespace mc
