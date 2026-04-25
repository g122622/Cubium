#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <vulkan/vulkan.h>
#include <vector>

namespace mc {

class Entity;

namespace client::renderer::entity::pipeline {
class EntityPipeline;  // 前向声明
struct EntityMesh;     // 前向声明
}

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
     * @brief 初始化阴影渲染器
     *
     * @param pipeline 实体渲染管线
     * @return 成功或错误
     */
    static bool initialize(pipeline::EntityPipeline& pipeline);

    /**
     * @brief 清理阴影渲染器资源
     */
    static void cleanup();

    /**
     * @brief 检查阴影是否已初始化
     */
    [[nodiscard]] static bool isInitialized();

    /**
     * @brief 渲染实体阴影（GPU管线路径）
     *
     * @param cmd Vulkan 命令缓冲区
     * @param entity 实体
     * @param partialTicks 部分 tick
     * @param shadowRadius 阴影半径
     * @param shadowAlpha 阴影透明度
     * @param pipeline 实体渲染管线
     */
    static void renderShadow(
        VkCommandBuffer cmd,
        Entity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 shadowAlpha,
        pipeline::EntityPipeline& pipeline
    );

    /**
     * @brief 渲染实体阴影（CPU路径 - 已废弃）
     *
     * @deprecated 使用 renderShadow(cmd, entity, ...) 代替
     */
    static void renderShadow(
        Entity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 shadowAlpha
    );

private:
    /**
     * @brief 创建阴影网格
     */
    static bool createShadowMesh(pipeline::EntityPipeline& pipeline);

    /**
     * @brief 计算阴影透明度
     */
    [[nodiscard]] static f64 computeShadowAlpha(
        Entity& entity,
        f64 partialTicks,
        f64 shadowRadius,
        f64 baseAlpha
    );

    /**
     * @brief 获取阴影圆盘顶点
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
    static pipeline::EntityMesh* s_shadowMesh;
};

} // namespace mc::client::renderer::entity::util
} // namespace mc
