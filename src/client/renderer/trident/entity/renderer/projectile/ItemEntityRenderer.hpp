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

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "common/core/Types.hpp"

namespace mc {

namespace client::renderer::entity::renderer::projectile {

/**
 * @brief ItemEntity 渲染器
 *
 * 渲染掉落在世界中的物品实体。
 * 物品以 3D 方式浮动渲染，具有上下浮动和旋转动画。
 *
 * 关键实现细节：
 * - 浮动偏移: sin((age + partialTick) / BOB_PERIOD + hoverStart) * BOB_AMPLITUDE + BOB_BASE + GROUND_OFFSET
 * - 旋转: ((age + partialTick) / ROTATION_PERIOD + hoverStart) * RAD_TO_DEG
 * - 多物品堆叠: 根据数量 1-5 个物品
 */
class ItemEntityRenderer : public core::EntityRenderer {
public:
    ItemEntityRenderer();
    ~ItemEntityRenderer() override = default;

    // 禁止拷贝
    ItemEntityRenderer(const ItemEntityRenderer&) = delete;
    ItemEntityRenderer& operator=(const ItemEntityRenderer&) = delete;

    /**
     * @brief 渲染 ItemEntity
     * @param entity 实体（必须是 ClientEntity）
     * @param partialTicks 部分 tick
     */
    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染阴影（ItemEntity 有小阴影）
     * @param entity 实体
     * @param partialTicks 部分 tick
     */
    void renderShadow(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] static f64 calculateBobOffset(u32 ticksExisted, f64 partialTick, f32 hoverStart);

    /**
     * @brief 计算旋转角度
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @param hoverStart 悬浮起始偏移
     * @return 旋转角度（度）
     */
    [[nodiscard]] static f64 calculateRotation(u32 ticksExisted, f64 partialTick, f32 hoverStart);

    /**
     * @brief 计算物品堆叠数量对应的渲染数量
     *
     * - 1 个物品: 1 个模型
     * - 2-16: 2 个模型
     * - 17-32: 3 个模型
     * - 33-48: 4 个模型
     * - 49+: 5 个模型
     *
     * @param count 物品数量
     * @return 渲染的模型数量 (1-5)
     */
    [[nodiscard]] static i32 getItemCountForRender(i32 count);

private:
    /// 浮动动画周期除数
    static constexpr f64 BOB_PERIOD = 10.0;
    /// 浮动振幅
    static constexpr f64 BOB_AMPLITUDE = 0.1;
    /// 浮动基础偏移
    static constexpr f64 BOB_BASE = 0.1;
    /// 旋转动画周期除数
    static constexpr f64 ROTATION_PERIOD = 20.0;
    // 地面变换Y偏移：billboard 不做 Y 翻转（顶点已在地面以上），此偏移设为 0；
    // 物品悬浮高度由 BOB_BASE 提供，配合 BOB_AMPLITUDE 形成上下浮动。
    static constexpr f64 GROUND_TRANSFORM_Y_OFFSET = 0.0;
};

} // namespace client::renderer::entity::renderer::projectile
} // namespace mc
