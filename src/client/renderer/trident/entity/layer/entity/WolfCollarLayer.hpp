#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/animal/WolfModel.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mc {
class WolfEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 狼项圈层渲染器
 *
 * 渲染驯服狼的项圈。支持不同颜色的项圈。
 *
 * 参考 MC 1.16.5 WolfCollarLayer
 */
class WolfCollarLayer : public core::LayerRenderer<::mc::WolfEntity> {
public:
    WolfCollarLayer() = default;
    ~WolfCollarLayer() override = default;

    /**
     * @brief 渲染项圈层（GPU管线路径）
     */
    void renderPipeline(
        ::mc::WolfEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染项圈层（CPU路径 - 已废弃）
     */
    void render(
        ::mc::WolfEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) override;

    /**
     * @brief 检查是否应该渲染项圈
     */
    [[nodiscard]] bool shouldRender(const ::mc::WolfEntity& entity) const override;

private:
    /**
     * @brief 获取项圈颜色
     * @param entity 狼实体
     * @return RGB颜色值
     */
    [[nodiscard]] static Vector3f getCollarColor(const ::mc::WolfEntity& entity);

    /**
     * @brief 构建项圈网格
     */
    void buildCollarMesh(
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

    /**
     * @brief 获取或创建项圈网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateCollarMesh(pipeline::EntityPipeline& pipeline);

    // 项圈网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_collarMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
