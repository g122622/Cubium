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
 * 参考 MC 1.16.5 LivingRenderer.getPackedOverlay()
 * 用于渲染实体受伤时的红色闪烁效果。
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
     * MC 1.16.5 使用 16 位打包 UV 值：
     * - U = hurtTime / 10.0F * 16.0F
     * - V = 0
     * - 道德影响时 U 固定为 3.0F
     */
    [[nodiscard]] static i32 getPackedOverlay(::mc::LivingEntity& entity, bool whiteFlash = false);

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

    static constexpr i32 OVERLAY_PACKING = 16;  // UV 打包位数
    static bool s_initialized;
};

} // namespace mc::client::renderer::entity::effect::hurt
