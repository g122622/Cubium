#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <vector>

namespace mc {

class Entity;

namespace client::renderer::entity::util {

/**
 * @brief 阴影渲染器
 *
 * 负责在实体下方渲染阴影圆盘。
 * 阴影大小根据实体尺寸和与地面的距离动态调整。
 *
 * 参考 MC 1.16.5 EntityRenderer.renderShadow()
 */
class ShadowRenderer {
public:
    /**
     * @brief 渲染实体阴影
     *
     * @param entity 实体
     * @param partialTicks 部分 tick
     * @param shadowRadius 阴影半径
     * @param shadowAlpha 阴影透明度
     */
    static void renderShadow(
        Entity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 shadowAlpha
    );

    /**
     * @brief 设置阴影网格
     *
     * 初始化阴影圆盘网格数据。
     *
     * @param segments 圆盘分段数（默认 16）
     */
    static void initialize(u32 segments = 16);

    /**
     * @brief 清理阴影网格资源
     */
    static void cleanup();

    /**
     * @brief 检查阴影是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

private:
    /**
     * @brief 计算阴影透明度
     *
     * 根据实体高度和阴影半径计算透明度衰减。
     *
     * @param entity 实体
     * @param partialTicks 部分 tick
     * @param shadowRadius 阴影半径
     * @param baseAlpha 基础透明度
     * @return 调整后的透明度
     */
    [[nodiscard]] static f64 computeShadowAlpha(
        Entity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 baseAlpha
    );

    /**
     * @brief 获取阴影圆盘顶点
     *
     * @param radius 半径
     * @param segments 分段数
     * @param vertices 输出顶点
     * @param indices 输出索引
     */
    static void getShadowVertices(
        f64 radius,
        u32 segments,
        std::vector<f32>& vertices,
        std::vector<u32>& indices
    );

    static bool s_initialized;
    static u32 s_segments;
    static std::vector<f32> s_shadowVertices;
    static std::vector<u32> s_shadowIndices;
};

} // namespace mc::client::renderer::entity::util
} // namespace mc
