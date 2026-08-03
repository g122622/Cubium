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

#include "MobEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"

namespace mc {

/**
 * @brief 飞行生物基类
 *
 * 所有可以飞行的生物实体的基类。
 * 包括恶魂、幻翼、蜜蜂等。
 *
 * 特性：
 * - 不受重力影响
 * - 可以在空中自由移动
 * - 特殊的空中寻路逻辑
 */
class FlyingEntity : public MobEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    FlyingEntity(EntityInstanceId id);

    ~FlyingEntity() override = default;

    /// 本类继承链标识（parent = MobEntity::classInfo()）。见 Entity::classInfo()。
    // TODO(实体同步对齐, 见 entity-sync-alignment-decisions-2026-07): 本类是 1.16.5 遗留中间层，
    // vanilla 1.21.11 类树已调整（PathfinderMob/WaterAnimal/AgeableWaterCreature 等），本项目保留此层。
    // 若后期 vanilla 此层有同步字段须补 registerData+ClassRegisterGuard，当前仅占位 classInfo。
    static const entity::EntityClassInfo& classInfo();

    // 禁止拷贝
    FlyingEntity(const FlyingEntity&) = delete;
    FlyingEntity& operator=(const FlyingEntity&) = delete;

    // 允许移动
    FlyingEntity(FlyingEntity&&) = delete;
    FlyingEntity& operator=(FlyingEntity&&) = delete;

    // ========== 飞行属性 ==========

    /**
     * @brief 检查是否正在飞行
     * @return 如果正在飞行返回true
     */
    [[nodiscard]] bool isFlying() const { return m_flying; }

    /**
     * @brief 设置飞行状态
     * @param flying 是否飞行
     */
    void setFlying(bool flying) { m_flying = flying; }

    /**
     * @brief 获取最大飞行高度（相对地面）
     * @return 最大飞行高度
     */
    [[nodiscard]] f32 getMaxFlightHeight() const { return m_maxFlightHeight; }

    /**
     * @brief 设置最大飞行高度
     * @param height 最大高度
     */
    void setMaxFlightHeight(f32 height) { m_maxFlightHeight = height; }

    // ========== 重力 ==========

    /**
     * @brief 检查是否受重力影响
     * @return 飞行生物不受重力影响
     */
    [[nodiscard]] bool hasGravity() const { return false; }

    // ========== 移动 ==========

    /**
     * @brief 飞行移动
     * @param x X方向移动
     * @param y Y方向移动
     * @param z Z方向移动
     */
    void travel(f32 x, f32 y, f32 z) override;

protected:
    bool m_flying = true;
    f32 m_maxFlightHeight = 64.0f;
};

} // namespace mc
