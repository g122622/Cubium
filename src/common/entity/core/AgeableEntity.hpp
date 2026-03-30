#pragma once

#include "CreatureEntity.hpp"
#include "../../core/Types.hpp"

namespace mc {

/**
 * @brief 可成长实体基类
 *
 * 支持幼体/成体状态的实体，可以随时间成长。
 * 用于动物（猪、牛、羊、鸡）等。
 *
 * 参考 MC 1.16.5 AgeableEntity
 */
class AgeableEntity : public CreatureEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AgeableEntity(LegacyEntityType type, EntityId id);
    ~AgeableEntity() override = default;

    // 禁止拷贝
    AgeableEntity(const AgeableEntity&) = delete;
    AgeableEntity& operator=(const AgeableEntity&) = delete;

    // 允许移动
    AgeableEntity(AgeableEntity&&) = default;
    AgeableEntity& operator=(AgeableEntity&&) = default;

    // ========== 年龄系统 ==========

    /**
     * @brief 获取年龄
     * @return 年龄值（负数=幼体，0或正数=成体）
     */
    [[nodiscard]] i32 getGrowingAge() const { return m_growingAge; }

    /**
     * @brief 设置年龄
     * @param age 年龄值
     */
    void setGrowingAge(i32 age);

    /**
     * @brief 是否为幼体
     */
    [[nodiscard]] bool isChild() const { return m_growingAge < 0; }

    /**
     * @brief 设置为幼体
     * @param child 是否为幼体
     */
    void setChild(bool child);

    /**
     * @brief 成长（增加年龄）
     * @param seconds 成长的秒数
     */
    void ageUp(i32 seconds);

    /**
     * @brief 添加年龄（可用于加速成长）
     * @param amount 增加量
     */
    void addGrowingAge(i32 amount);

    // ========== 成长速度 ==========

    /**
     * @brief 获取成长速度倍率
     */
    [[nodiscard]] f32 getGrowthSpeed() const { return m_growthSpeed; }

    /**
     * @brief 设置成长速度倍率
     * @param speed 速度倍率（1.0=正常）
     */
    void setGrowthSpeed(f32 speed) { m_growthSpeed = speed; }

    // ========== 繁殖相关 ==========

    /**
     * @brief 获取繁殖冷却时间
     */
    [[nodiscard]] i32 getLoveTimer() const { return m_loveTimer; }

    /**
     * @brief 设置繁殖冷却时间
     */
    void setLoveTimer(i32 timer) { m_loveTimer = timer; }

    /**
     * @brief 是否可以繁殖
     */
    [[nodiscard]] bool canBreed() const;

    /**
     * @brief 是否处于爱心状态（可以繁殖）
     */
    [[nodiscard]] bool isInLove() const { return m_loveTimer > 0; }

    /**
     * @brief 设置爱心状态
     * @param playerInLove 使其进入爱心状态的玩家ID（暂未使用）
     */
    void setInLove(u64 playerInLove = 0);

    /**
     * @brief 重置爱心状态
     */
    void resetLove() { m_loveTimer = 0; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    /**
     * @brief 年龄更新（每tick调用）
     */
    void updateAge();

    /**
     * @brief 繁殖冷却更新（每tick调用）
     */
    void updateLove();

    /**
     * @brief 幼体尺寸缩放
     * @return 幼体的尺寸缩放比例
     */
    [[nodiscard]] f32 getChildScale() const;

    /**
     * @brief 幼体变成成体时调用
     */
    virtual void onGrowUp() {}

private:
    i32 m_growingAge = 0;      // 年龄（负数=幼体）
    i32 m_loveTimer = 0;       // 繁殖冷却/爱心计时器
    f32 m_growthSpeed = 1.0f;  // 成长速度倍率
    i32 m_forcedAge = 0;       // 强制成长值（用于加速）
    i32 m_forcedAgeTimer = 0;  // 强制成长计时器

    // 常量
    static constexpr i32 BABY_AGE = -24000;     // 幼体起始年龄
    static constexpr i32 MAX_AGE = 0;           // 成体年龄
    static constexpr i32 LOVE_TIMER_MAX = 600;  // 爱心状态持续时间（30秒）
    static constexpr f32 BABY_SCALE = 0.5f;     // 幼体缩放比例
};

} // namespace mc
