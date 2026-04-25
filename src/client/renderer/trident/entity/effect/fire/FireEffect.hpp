#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mc {
class Entity;

namespace client {
class ClientEntity;
}
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;  // 前向声明
}

namespace mc::client::renderer::entity::effect::fire {

/**
 * @brief 着火效果渲染器
 *
 * 参考 MC 1.16.5 EntityRenderer.renderFire()
 * 用于渲染实体身上的火焰效果。
 */
class FireEffect {
public:
    /**
     * @brief 初始化着火效果系统
     */
    static void initialize();

    /**
     * @brief 清理着火效果系统
     */
    static void cleanup();

    /**
     * @brief 检查实体是否在燃烧（Entity 版本）
     * @param entity 实体
     * @return 是否在燃烧
     */
    [[nodiscard]] static bool isBurning(Entity& entity);

    /**
     * @brief 检查实体是否在燃烧（ClientEntity 版本）
     * @param entity 客户端实体
     * @return 是否在燃烧
     */
    [[nodiscard]] static bool isBurningClient(::mc::client::ClientEntity& entity);

    /**
     * @brief 渲染实体着火效果（Entity 版本，用于旧路径）
     * @param entity 实体
     * @param partialTicks 部分tick
     */
    static void renderFire(Entity& entity, f64 partialTicks);

    /**
     * @brief 渲染实体着火效果（ClientEntity + Vulkan 版本）
     * @param cmd Vulkan 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     * @param pipeline 实体渲染管线
     */
    static void renderFire(
        VkCommandBuffer cmd,
        ::mc::client::ClientEntity& entity,
        f64 partialTicks,
        pipeline::EntityPipeline& pipeline
    );

    /**
     * @brief 渲染火焰四边形
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @param width 宽度
     * @param height 高度
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     */
    static void generateFireQuad(
        f64 x, f64 y, f64 z,
        f64 width, f64 height,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

private:
    FireEffect() = delete;
    ~FireEffect() = delete;

    /**
     * @brief 计算火焰偏移（用于动画）
     */
    [[nodiscard]] static f64 computeFireOffset(f64 time, f64 seed);

    static bool s_initialized;
    static pipeline::EntityPipeline* s_pipeline;  // 当前使用的管线
};

} // namespace mc::client::renderer::entity::effect::fire
