#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 蜘蛛实体
 *
 * 可以爬墙的敌对生物。
 *
 * 特性：
 * - 爬墙：可以垂直爬上墙壁
 * - 夜间攻击：仅在黑暗中攻击玩家
 * - 中立：白天中立
 * - 攀爬：可以攀爬
 *
 * 参考 MC 1.16.5 SpiderEntity
 */
class SpiderEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SpiderEntity(LegacyEntityType type, EntityId id);
    ~SpiderEntity() override = default;

    // 禁止拷贝
    SpiderEntity(const SpiderEntity&) = delete;
    SpiderEntity& operator=(const SpiderEntity&) = delete;

    // 允许移动
    SpiderEntity(SpiderEntity&&) = default;
    SpiderEntity& operator=(SpiderEntity&&) = default;

    /**
     * @brief 创建蜘蛛实体
     * @param world 世界实例
     * @return 新的蜘蛛实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 攀爬系统 ==========

    /**
     * @brief 是否正在攀爬
     */
    [[nodiscard]] bool isClimbing() const { return m_climbing; }

    /**
     * @brief 设置攀爬状态
     */
    void setClimbing(bool climbing) { m_climbing = climbing; }

    /**
     * @brief 是否可以攀爬
     */
    [[nodiscard]] bool canClimb() const { return true; }

    // ========== 攻击状态 ==========

    /**
     * @brief 是否应该攻击
     * 蜘蛛只在黑暗中攻击
     */
    [[nodiscard]] bool shouldAttack(LivingEntity* target) const override;

    // ========== 阳光燃烧 ==========

    /**
     * @brief 蜘蛛不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.65f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 1.4f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 0.9f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    bool m_climbing = false;
    bool m_wasOnGround = false;
};

} // namespace mc
