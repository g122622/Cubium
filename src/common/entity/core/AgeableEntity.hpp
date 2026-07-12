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

#include "CreatureEntity.hpp"
#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 可成长实体基类
 *
 * 支持幼体/成体状态的实体，可以随时间成长。
 * 用于动物（猪、牛、羊、鸡）等。
 */
class AgeableEntity : public CreatureEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    AgeableEntity(EntityId id) noexcept;
    ~AgeableEntity() override = default;

    // 禁止拷贝
    AgeableEntity(const AgeableEntity&) = delete;
    AgeableEntity& operator=(const AgeableEntity&) = delete;

    // 允许移动
    AgeableEntity(AgeableEntity&&) = delete;
    AgeableEntity& operator=(AgeableEntity&&) = delete;

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
    [[nodiscard]] bool isChild() const override { return m_growingAge < 0; }

    /**
     * @brief 设置为幼体
     * @param child 是否为幼体
     *
     * @note 声明为 virtual 以允许子类（如 SnifferEntity）覆盖幼年期长度。
     *       MC 原版通过 Mob.setBaby(int age) 在子类中覆盖，本项目将 setChild 作为
     *       等价入口。普通动物调用此方法将年龄设置为 BABY_AGE（-24000，20 分钟），
     *       SnifferEntity 覆盖为 SNIFFER_BABY_AGE_TICKS（-48000，40 分钟）。
     */
    virtual void setChild(bool child);

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

    // ========== 常量 ==========

    static constexpr i32 BABY_AGE = -24000;        // 幼体起始年龄（20分钟）
    static constexpr i32 MAX_AGE = 0;              // 成体年龄
    static constexpr i32 BREEDING_COOLDOWN = 6000; // 繁殖冷却（5分钟）
    static constexpr i32 LOVE_TIMER_MAX = 600;     // 爱心状态持续时间（30秒）
    static constexpr f32 BABY_SCALE = 0.5f;        // 幼体缩放比例

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

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

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
     * @brief 获取基础宽度（子类应重写）
     * @return 成体的宽度
     */
    [[nodiscard]] virtual f32 getBaseWidth() const { return CreatureEntity::width(); }

    /**
     * @brief 获取基础高度（子类应重写）
     * @return 成体的高度
     */
    [[nodiscard]] virtual f32 getBaseHeight() const { return CreatureEntity::height(); }

    /**
     * @brief 获取实体宽度（考虑幼体缩放）
     */
    [[nodiscard]] f32 width() const override { return getBaseWidth() * (isChild() ? BABY_SCALE : 1.0f); }

    /**
     * @brief 获取实体高度（考虑幼体缩放）
     */
    [[nodiscard]] f32 height() const override { return getBaseHeight() * (isChild() ? BABY_SCALE : 1.0f); }

    /**
     * @brief 幼体变成成体时调用
     */
    virtual void onGrowUp() {}

private:
    i32 m_growingAge = 0;     // 年龄（负数=幼体）
    i32 m_loveTimer = 0;      // 繁殖冷却/爱心计时器
    f32 m_growthSpeed = 1.0f; // 成长速度倍率
    i32 m_forcedAge = 0;      // 强制成长值（用于加速）
    i32 m_forcedAgeTimer = 0; // 强制成长计时器
};

} // namespace mc
