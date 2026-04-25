#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::layer::effect {

/**
 * @brief 发光眼睛层渲染器基类
 *
 * 渲染实体的发光眼睛（如末影人、蜘蛛、幻翼等）。
 * 使用叠加混合模式实现发光效果。
 *
 * 参考 MC 1.16.5 AbstractEyesLayer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class EyesLayer : public core::LayerRenderer<TEntity> {
public:
    EyesLayer() = default;
    ~EyesLayer() override = default;

    /**
     * @brief 渲染眼睛层（GPU管线路径）
     */
    void renderPipeline(
        TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染眼睛层（CPU路径 - 已废弃）
     */
    void render(
        TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) override;

    /**
     * @brief 检查是否应该渲染眼睛
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 获取眼睛发光纹理
     * 子类可以重写此方法以提供特定纹理
     */
    [[nodiscard]] virtual ResourceLocation getEyesTexture(const TEntity& entity) const {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/eyes.png");
    }

    /**
     * @brief 获取发光颜色
     */
    [[nodiscard]] virtual Vector3f getEyesColor(const TEntity& entity) const {
        (void)entity;
        return Vector3f(1.0f, 1.0f, 1.0f);
    }

    /**
     * @brief 构建眼睛网格
     */
    void buildEyesMesh(
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );
};

} // namespace mc::client::renderer::entity::layer::effect
