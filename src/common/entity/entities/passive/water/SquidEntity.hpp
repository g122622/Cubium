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

#include "../../../../core/Types.hpp"
#include "../water/WaterMobEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include <memory>

namespace mc {

// 前向声明
class DamageSource;

/**
 * @brief 鱿鱼实体
 *
 * 生活在海洋中的无脊椎动物。
 *
 * 特性：
 * - 喷墨：受到攻击时会喷出墨汁
 * - 游泳：在水中优雅地游动
 * - 挣扎：离开水会扑腾
 * - 掉落：墨囊
 */
class SquidEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    SquidEntity(EntityInstanceId id);
    ~SquidEntity() override = default;

    // 禁止拷贝
    SquidEntity(const SquidEntity&) = delete;
    SquidEntity& operator=(const SquidEntity&) = delete;

    // 允许移动
    SquidEntity(SquidEntity&&) = delete;
    SquidEntity& operator=(SquidEntity&&) = delete;

    /**
     * @brief 创建鱿鱼实体
     * @param world 世界实例
     * @return 新的鱿鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 游泳行为 ==========

    /**
     * @brief 是否在游泳
     */
    [[nodiscard]] bool isSwimming() const { return m_swimming; }

    /**
     * @brief 设置游泳状态
     */
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 获取游泳方向
     */
    [[nodiscard]] f32 getSwimAngle() const { return m_swimAngle; }

    /**
     * @brief 设置游泳方向
     */
    void setSwimAngle(f32 angle) { m_swimAngle = angle; }

    // ========== 喷墨 ==========

    /**
     * @brief 是否正在喷墨
     */
    [[nodiscard]] bool isSprayingInk() const { return m_sprayingInk; }

    /**
     * @brief 喷墨
     *
     * 受击时触发，生成墨汁粒子并播放喷墨音效。
     * 粒子类型和音效由 getInkParticle() / getSquirtSound() 虚函数决定，
     * 子类（如 GlowSquidEntity）可重写以提供发光变种。
     */
    void sprayInk();

    /**
     * @brief 获取喷墨粒子类型
     *
     * 子类可重写以提供不同的墨汁粒子（如发光鱿鱼返回 GlowSquidInk）。
     */
    [[nodiscard]] virtual particle::ParticleTypeId getInkParticle() const { return particle::ParticleTypeId::SquidInk; }

    /**
     * @brief 获取喷墨音效
     *
     * 子类可重写以提供不同的喷墨音效。默认返回空（鱿鱼原版喷墨音效未在基类播放）。
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getSquirtSound() const
    {
        return SoundEvents::ENTITY_SQUID_SQUIRT;
    }

    // ========== 受伤 ==========

    /**
     * @brief 受伤处理
     *
     * 重写 LivingEntity::hurt，受击成功后触发喷墨（对应 MC Java Squid.hurtServer）。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    // ========== 移动向量 ==========

    /**
     * @brief 设置移动向量
     *
     * 鱿鱼使用自定义的移动向量系统进行游泳，
     * 而不是使用标准的导航系统。
     *
     * @param x X方向分量
     * @param y Y方向分量（垂直）
     * @param z Z方向分量
     */
    void setMovementVector(f32 x, f32 y, f32 z);

    /**
     * @brief 是否有移动向量
     * @return 如果移动向量不为零返回 true
     */
    [[nodiscard]] bool hasMovementVector() const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.4f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 移动向量
    f32 m_randomMotionVecX = 0.0f;
    f32 m_randomMotionVecY = 0.0f;
    f32 m_randomMotionVecZ = 0.0f;

    // 游泳状态
    bool m_swimming = false;
    f32 m_swimAngle = 0.0f;
    f32 m_targetSwimAngle = 0.0f;

    // 喷墨状态
    bool m_sprayingInk = false;
    i32 m_sprayTimer = 0;

    // 游泳计时器
    i32 m_swimTimer = 0;
    i32 m_changeDirectionTimer = 0;

    // 常量
    static constexpr i32 SWIM_DURATION = 40;      // 每次游泳持续时间
    static constexpr i32 SPRAY_INK_DURATION = 20; // 喷墨持续时间
};

} // namespace mc
