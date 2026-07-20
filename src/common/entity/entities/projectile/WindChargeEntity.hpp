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
 * The above copyright notice shall be included in all
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

#include "ThrowableEntity.hpp"

namespace mc {

namespace test {
class WindChargeEntityTestAccessor; // 测试访问器，声明为 friend 以访问 private 成员
} // namespace test

namespace entity {

/**
 * @brief 风弹弹射物实体
 *
 * 由旋风人投掷或玩家使用风弹物品投掷。
 * 命中实体或方块时产生风爆效果，推开周围实体和弹射物。
 *
 * 风爆效果通过创建一个 TRIGGER 模式的爆炸实现：
 * - 不破坏方块，不造成爆炸伤害
 * - 推开范围内的实体，方向从爆炸中心指向实体
 * - 考虑实体与爆炸中心之间的方块遮挡
 * - 对弹射物进行特殊处理（可被偏转）
 *
 * 玩家风弹和旋风人风弹参数不同：
 * | 参数         | 玩家投掷     | 旋风人投掷   |
 * |-------------|-------------|-------------|
 * | 爆炸半径      | 1.2         | 3.0         |
 * | 击退乘数      | 1.22        | 1.0 (默认)  |
 * | 伤害         | 1.0         | 1.0         |
 *
 * 命名空间ID: minecraft:wind_charge
 */
class WindChargeEntity final : public ThrowableEntity {
public:
    /// 玩家投掷风弹的基础伤害
    static constexpr f32 PLAYER_DAMAGE = 1.0f;

    /// 旋风人投掷风弹的基础伤害
    static constexpr f32 BREEZE_DAMAGE = 1.0f;

    /**
     * @brief 构造风弹弹射物
     * @param id 实体ID
     */
    explicit WindChargeEntity(EntityInstanceId id);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取重力加速度
     */
    f32 getGravity() const override { return 0.03f; }

protected:
    /**
     * @brief 命中实体时处理
     *
     * 对命中的实体造成 1 点风爆伤害，然后触发风爆效果。
     */
    void onEntityHit(const RayTraceResult& result) override;

    /**
     * @brief 命中方块时处理
     *
     * 将爆炸中心沿命中面法线偏移 0.25 格，然后触发风爆效果。
     */
    void onBlockHit(const RayTraceResult& result) override;

    /**
     * @brief 冲击处理（风爆效果由 onEntityHit/onBlockHit 处理）
     */
    void onImpact(const RayTraceResult& result) override;

private:
    /**
     * @brief 产生风爆效果
     *
     * 参考 MC 1.21 的 AbstractWindCharge.explode() 和 ServerExplosion：
     * 1. 使用 Explosion 系统（TRIGGER 模式）实现推力效果
     * 2. 不破坏方块，不造成爆炸伤害（伤害已在 onEntityHit 中单独处理）
     * 3. 对范围内实体施加方向性推力
     * 4. 推力受方块遮挡衰减
     * 5. 播放风爆音效和粒子效果
     */
    void applyWindBurst();

    /**
     * @brief 获取风爆爆炸半径
     *
     * 玩家风弹：1.2，旋风人风弹：3.0。
     * 通过检查发射者类型判断。
     */
    f32 getExplosionRadius() const;

    /**
     * @brief 获取风爆击退乘数
     *
     * 玩家风弹：1.22，旋风人风弹：1.0（默认）。
     */
    f32 getKnockbackMultiplier() const;

    /**
     * @brief 获取风爆爆炸中心位置
     *
     * 命中方块时沿命中面法线偏移 0.25 格，
     * 命中实体或默认情况使用风弹自身位置。
     */
    Vector3 getBurstCenter() const;

    /// 是否已产生风爆（防止重复触发）
    bool m_hasBurst = false;

    /// 风爆爆炸中心（命中方块时偏移）
    Vector3 m_burstCenter{0.0f, 0.0f, 0.0f};

    /// 是否已设置风爆中心
    bool m_hasBurstCenter = false;

    /**
     * @brief 计算实体碰撞箱对爆炸中心的可见比例
     *
     * 参考 MC Explosion.getBlockDensity，采样实体碰撞箱内点，
     * 通过射线检测可见比例，用于计算推力衰减。
     *
     * @param entityBox 实体碰撞箱
     * @param center 爆炸中心位置
     * @return 可见比例 (0.0 - 1.0)，1.0 表示完全无遮挡
     */
    f32 _calculateSeenPercent(const AxisAlignedBB& entityBox, const Vector3& center) const;

    /**
     * @brief 播放风爆音效
     */
    void _playWindBurstSound(const Vector3& pos) const;

    /**
     * @brief 生成风爆粒子效果
     */
    void _spawnWindBurstParticles(const Vector3& pos) const;

    // 测试访问器通过 friend 声明访问 private applyWindBurst，
    // 避免修改生产代码的可见性。对应 BreezeEntity 的 test::BreezeEntityTestAccessor 模式。
    friend class test::WindChargeEntityTestAccessor;
};

} // namespace entity
} // namespace mc
