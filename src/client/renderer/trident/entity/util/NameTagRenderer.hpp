#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <string>

namespace mc {

class Entity;

namespace client::renderer::entity::util {

/**
 * @brief 名称标签渲染器
 *
 * 负责在实体上方渲染名称标签。
 * 支持自定义颜色、背景和可见性控制。
 *
 * 参考 MC 1.16.5 EntityRenderer.renderNameTag()
 */
class NameTagRenderer {
public:
    /**
     * @brief 渲染实体名称标签
     *
     * @param entity 实体
     * @param displayName 显示名称
     * @param partialTicks 部分 tick
     */
    static void renderNameTag(
        Entity& entity,
        const String& displayName,
        f64 partialTicks
    );

    /**
     * @brief 检查是否应该渲染名称标签
     *
     * @param entity 实体
     * @param distanceToCamera 到相机的距离
     * @return 是否应该渲染
     */
    [[nodiscard]] static bool shouldRenderNameTag(
        Entity& entity,
        f64 distanceToCamera
    );

    /**
     * @brief 设置最大可见距离
     * @param distance 最大距离
     */
    static void setMaxDistance(f64 distance);

    /**
     * @brief 获取最大可见距离
     */
    [[nodiscard]] static f64 maxDistance();

    // ========== 样式设置 ==========

    /**
     * @brief 设置名称标签缩放
     * @param scale 缩放因子
     */
    static void setScale(f64 scale);

    /**
     * @brief 设置背景颜色
     * @param r 红色分量 (0-255)
     * @param g 绿色分量 (0-255)
     * @param b 蓝色分量 (0-255)
     * @param a 透明度 (0-255)
     */
    static void setBackgroundColor(u8 r, u8 g, u8 b, u8 a);

    /**
     * @brief 设置是否显示背景
     * @param show 是否显示
     */
    static void setShowBackground(bool show);

private:
    /**
     * @brief 计算名称标签位置
     *
     * @param entity 实体
     * @param partialTicks 部分 tick
     * @return 标签位置（世界坐标）
     */
    [[nodiscard]] static Vector3d calculateNameTagPosition(
        Entity& entity,
        f64 partialTicks
    );

    /**
     * @brief 计算名称标签缩放
     *
     * @param distanceToCamera 到相机的距离
     * @return 缩放因子
     */
    [[nodiscard]] static f64 calculateScale(f64 distanceToCamera);

    static f64 s_maxDistance;       // 最大可见距离
    static f64 s_scale;             // 缩放因子
    static bool s_showBackground;   // 是否显示背景
    static u8 s_bgColorR;           // 背景红色
    static u8 s_bgColorG;           // 背景绿色
    static u8 s_bgColorB;           // 背景蓝色
    static u8 s_bgColorA;           // 背景透明度

    // 常量
    static constexpr f64 DEFAULT_MAX_DISTANCE = 64.0;
    static constexpr f64 DEFAULT_SCALE = 0.025;
    static constexpr f64 BACKGROUND_PADDING = 0.25;
    static constexpr f64 HEIGHT_OFFSET = 0.3;  // 实体高度之上的偏移
};

} // namespace mc::client::renderer::entity::util
} // namespace mc
