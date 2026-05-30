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
#include "common/util/math/Vector4.hpp"

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::effect::hurt {

/**
 * @brief 受伤闪烁效果
 *
 * 用于渲染实体受伤时的红色闪烁效果。
 *
 * OverlayTexture 格式:
 * - U = getU(uIn) = (int)(uIn * 15.0F) - hurtTime/10.0 决定 U 值
 * - V = getV(hurtIn) = hurtIn ? 3 : 10 - 受伤时 V=3，正常时 V=10
 * - packedUV = u | (v << 16) - U 在低 16 位，V 在高 16 位
 */
class HurtFlashEffect {
public:
    /**
     * @brief 初始化受伤闪烁系统
     */
    static void initialize();

    /**
     * @brief 清理受伤闪烁系统
     */
    static void cleanup();

    /**
     * @brief 计算覆盖层UV
     *
     * @param entity 生物实体
     * @param whiteFlash 是否为白色闪烁（道德影响）
     * @return 打包的UV值
     *
     * U = (int)(uIn * 15.0F) 范围 0-15
     * V = hurtTime > 0 || deathTime > 0 ? 3 : 10
     */
    [[nodiscard]] static i32 getPackedOverlay(::mc::LivingEntity& entity, bool whiteFlash);

    /**
     * @brief 获取受伤时间进度
     * @param entity 生物实体
     * @return 受伤时间进度 (0.0 - 1.0)
     */
    [[nodiscard]] static f64 getHurtProgress(::mc::LivingEntity& entity);

    /**
     * @brief 检查实体是否受伤
     * @param entity 生物实体
     * @return 是否受伤
     */
    [[nodiscard]] static bool isHurt(::mc::LivingEntity& entity);

    /**
     * @brief 应用受伤闪烁效果
     * @param entity 生物实体
     * @param baseColor 基础颜色 (RGBA)
     * @return 应用闪烁后的颜色 (RGBA)
     */
    [[nodiscard]] static math::Vector4f applyHurtFlash(::mc::LivingEntity& entity, const math::Vector4f& baseColor);

private:
    HurtFlashEffect() = delete;
    ~HurtFlashEffect() = delete;

    // OverlayTexture 使用 15 作为打包因子
    // getU(uIn) = (int)(uIn * 15.0F)
    static constexpr i32 OVERLAY_PACKING = 15;

    // V 值常量
    static constexpr i32 OVERLAY_V_HURT = 3;    // 受伤时 V=3
    static constexpr i32 OVERLAY_V_NORMAL = 10; // 正常时 V=10

    static bool s_initialized;
};

} // namespace mc::client::renderer::entity::effect::hurt
