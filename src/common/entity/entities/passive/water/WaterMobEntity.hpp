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

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "entity/core/CreatureEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 水生生物基类
 *
 * 生活在水中的生物的基类。
 *
 * 特性：
 * - 水下呼吸：可以在水下呼吸
 * - 水外窒息：离开水会逐渐窒息
 * - 游泳行为：在水中游泳
 * - 陆地挣扎：在陆地上会扑腾
 */
class WaterMobEntity : public CreatureEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    WaterMobEntity(EntityInstanceId id);
    ~WaterMobEntity() override = default;

    /// 本类继承链标识（parent = CreatureEntity::classInfo()）。见 Entity::classInfo()。
    // TODO(实体同步对齐, 见 entity-sync-alignment-decisions-2026-07): 本类是 1.16.5 遗留中间层，
    // vanilla 1.21.11 类树已调整（PathfinderMob/WaterAnimal/AgeableWaterCreature 等），本项目保留此层。
    // 若后期 vanilla 此层有同步字段须补 registerData+ClassRegisterGuard，当前仅占位 classInfo。
    static const entity::EntityClassInfo& classInfo();

    // 禁止拷贝
    WaterMobEntity(const WaterMobEntity&) = delete;
    WaterMobEntity& operator=(const WaterMobEntity&) = delete;

    // 允许移动
    WaterMobEntity(WaterMobEntity&&) = delete;
    WaterMobEntity& operator=(WaterMobEntity&&) = delete;

    // ========== 水下状态 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 是否在水中或气泡中
     */
    [[nodiscard]] bool isInWaterOrBubble() const;

    /**
     * @brief 是否可以生成
     * 检查是否在适合生成的水域
     */
    [[nodiscard]] virtual bool canSpawnInWater() const { return true; }

    // ========== 呼吸系统 ==========

    /**
     * @brief 获取空气供应量
     * 水生生物使用基类的air()方法
     */
    [[nodiscard]] i32 getAirSupply() const { return air(); }

    /**
     * @brief 设置空气供应量
     */
    void setAirSupply(i32 supply) { setAir(supply); }

    /**
     * @brief 获取最大空气供应量
     */
    [[nodiscard]] i32 getMaxAirSupply() const { return maxAir(); }

    /**
     * @brief 是否在窒息
     */
    [[nodiscard]] bool isDrowning() const { return air() <= 0; }

    // ========== 行为 ==========

    /**
     * @brief 是否可以游泳
     */
    [[nodiscard]] virtual bool canSwim() const { return true; }

    /**
     * @brief 是否会被水流推动
     */
    [[nodiscard]] virtual bool canBePushedByWater() const { return true; }

    // ========== 寻路权重 ==========

    /**
     * @brief 获取路径权重
     *
     * 水生生物偏好水中位置：在水中返回10.0f，否则返回0.0f。
     * 对应 MC WaterAnimal.getWalkTargetValue 中不重写此方法（返回0.0f），
     * 但水生生物的子类（如守卫者）会重写以增加水中偏好。
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 更新空气供应
     * 水生生物在陆地上消耗空气，在水中恢复空气
     */
    void updateAirSupply() override;

    /**
     * @brief 当离开水时调用
     */
    virtual void onLeaveWater() {}

    /**
     * @brief 当进入水时调用
     */
    virtual void onEnterWater() {}

private:
    // 水状态追踪（用于 onEnterWater/onLeaveWater 回调）
    bool m_wasInWater = false;
};

} // namespace mc
