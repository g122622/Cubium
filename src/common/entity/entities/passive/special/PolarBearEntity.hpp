#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 北极熊实体
 *
 * 生活在冰原的大型中立动物。
 *
 * 特性：
 * - 保护幼崽：幼熊附近有成年熊时会攻击玩家
 * - 游泳：擅长游泳
 * - 站立：可以后腿站立
 * - 攻击：被攻击时会反击
 * - 幼崽：小北极熊
 *
 * 参考 MC 1.16.5 PolarBearEntity
 */
class PolarBearEntity : public AnimalEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    PolarBearEntity(LegacyEntityType type, EntityId id);
    ~PolarBearEntity() override = default;

    // 禁止拷贝
    PolarBearEntity(const PolarBearEntity&) = delete;
    PolarBearEntity& operator=(const PolarBearEntity&) = delete;

    // 允许移动
    PolarBearEntity(PolarBearEntity&&) = default;
    PolarBearEntity& operator=(PolarBearEntity&&) = default;

    /**
     * @brief 创建北极熊实体
     * @param world 世界实例
     * @return 新的北极熊实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 状态 ==========

    /**
     * @brief 是否正在站立
     */
    [[nodiscard]] bool isStanding() const { return m_standing; }

    /**
     * @brief 设置站立状态
     */
    void setStanding(bool standing);

    /**
     * @brief 是否正在警告
     * 站立并张开前爪
     */
    [[nodiscard]] bool isWarning() const { return m_warning; }

    /**
     * @brief 设置警告状态
     */
    void setWarning(bool warning);

    // ========== 繁殖 ==========

    /**
     * @brief 北极熊不可繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 北极熊不可繁殖
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override {
        (void)partner;
        return nullptr;
    }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.7f : 1.4f; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 刻更新 ==========
    void tick() override;

private:
    // 站立状态
    bool m_standing = false;
    bool m_warning = false;
    i32 m_standTimer = 0;
    i32 m_warningTimer = 0;

    // 常量
    static constexpr i32 STAND_DURATION_MIN = 100;  // 最小站立时间
    static constexpr i32 STAND_DURATION_MAX = 400;  // 最大站立时间
    static constexpr i32 WARNING_DURATION = 40;     // 警告持续时间
};

} // namespace mc
