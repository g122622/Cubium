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
 * The above copyright notice shall this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/util/math/Vector4.hpp"

namespace mc {

class ExperienceOrbEntity;

namespace client::renderer::entity::renderer::projectile {

/**
 * @brief 经验球渲染器
 *
 * 渲染世界中的经验球实体。经验球使用 billboard 方式渲染，
 * 始终面向摄像机，带有绿色发光效果和浮动动画。
 *
 * 特性：
 * - 11 种大小等级（对应不同经验值，决定纹理图标）
 * - 绿色↔黄色循环颜色动画（基于时间的正弦波）
 * - 上下浮动动画
 * - 半透明渲染（alpha = 128/255）
 * - 固定光照 (blockLight = max(worldLight, 7))
 *
 * 经验球纹理为 64x64 像素的精灵图集，包含 11 个 16x16 的图标：
 * - 4 列 × 3 行布局
 * - 图标索引 = getOrbSize(xpValue)，范围 0-10
 * - UV 计算: u = (icon % 4) * 16 / 64, v = (icon / 4) * 16 / 64
 *
 * 颜色动画（对齐 MC Java 版 ExperienceOrbRenderer.submit）：
 * - Red:   (sin(ageInTicks / 2) + 1) * 0.5 * 255
 * - Green: 255（固定）
 * - Blue:  (sin(ageInTicks / 2 + 4π/3) + 1) * 0.1 * 255
 * - Alpha: 128
 *
 * 关键实现细节：
 * - 浮动偏移: sin((age + partialTick) / 20.0) * 0.1 + 0.05
 * - Billboard: 始终面向摄像机
 * - 光照: getBlockLight() = max(worldLight, 7)，通过 fullbright 因子 7/15 ≈ 0.467 实现
 * - 阴影: shadowRadius = 0.15, shadowStrength = 0.75
 */
class ExperienceOrbRenderer : public core::EntityRenderer {
public:
    ExperienceOrbRenderer();
    ~ExperienceOrbRenderer() override = default;

    // 禁止拷贝
    ExperienceOrbRenderer(const ExperienceOrbRenderer&) = delete;
    ExperienceOrbRenderer& operator=(const ExperienceOrbRenderer&) = delete;

    /**
     * @brief 渲染经验球
     *
     * 实际渲染由 EntityRendererManager::renderWithPipeline() 中的经验球特殊路径处理，
     * 此方法当前为空操作。颜色动画、浮动、缩放和 UV 映射均由管线管理器完成。
     *
     * @param entity 实体（必须是 ExperienceOrbEntity）
     * @param partialTicks 部分 tick
     */
    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 渲染阴影（经验球有阴影）
     * @param entity 实体
     * @param partialTicks 部分 tick
     */
    void renderShadow(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 根据经验值获取大小等级
     *
     * 大小等级决定经验球的纹理图标和渲染缩放。
     *
     * - 0-2: size 0 (最小)
     * - 3-6: size 1
     * - 7-16: size 2
     * - 17-36: size 3
     * - 37-72: size 4
     * - 73-148: size 5
     * - 149-306: size 6
     * - 307-616: size 7
     * - 617-1236: size 8
     * - 1237-2476: size 9
     * - 2477+: size 10 (最大)
     *
     * @param xpValue 经验值
     * @return 大小等级 (0-10)
     */
    [[nodiscard]] static i32 getSizeByValue(i32 xpValue);

    /**
     * @brief 计算浮动偏移
     *
     * 经验球在 Y 轴上下浮动，频率比 ItemEntity 慢（/20.0 而非 /10.0），
     * 基础高度偏移 0.05（billboard 不做 Y 翻转，故基线较低）。
     *
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] static f64 calculateBobOffset(u32 ticksExisted, f64 partialTick);

    /**
     * @brief 计算经验球颜色
     *
     * 基于时间的正弦波颜色动画，颜色在绿色和黄色之间循环。
     * 对齐 MC Java 版 ExperienceOrbRenderer.submit() 的颜色计算。
     *
     * @param time 时间参数 (ticksExisted + partialTick)
     * @return RGBA 颜色向量 (范围 0.0-1.0)
     */
    [[nodiscard]] static math::Vector4f calculateColor(f64 time);

    /**
     * @brief 根据图标索引计算纹理 UV 坐标
     *
     * 经验球纹理为 64x64 精灵图集，4列×3行布局，每个图标 16x16 像素。
     *
     * @param iconIndex 图标索引 (0-10，由 getOrbSize 返回)
     * @param[out] u0 UV 左边界
     * @param[out] v0 UV 上边界
     * @param[out] u1 UV 右边界
     * @param[out] v1 UV 下边界
     */
    static void calculateIconUV(i32 iconIndex, f64& u0, f64& v0, f64& u1, f64& v1);

    // 动画常量
    static constexpr f64 BOB_FREQUENCY = 0.05; // 浮动速度（1/20 弧度/tick）
    static constexpr f64 BOB_AMPLITUDE = 0.1;  // 浮动高度幅度
    // 基础高度偏移：billboard 不做 Y 翻转（顶点 y∈[0.25,0.5] 已在地面以上），
    // 此值配合 BOB_AMPLITUDE 让经验球浮在地面附近。
    static constexpr f64 BOB_BASE = 0.05;
    static constexpr f64 BASE_SIZE = 0.25;       // 基础大小
    static constexpr f64 SIZE_INCREMENT = 0.015; // 每级大小增量

    // 纹理图集常量
    static constexpr i32 ATLAS_SIZE = 64;   // 图集总尺寸（像素）
    static constexpr i32 ICON_SIZE = 16;    // 单个图标尺寸（像素）
    static constexpr i32 ICONS_PER_ROW = 4; // 每行图标数
};

} // namespace client::renderer::entity::renderer::projectile
} // namespace mc
