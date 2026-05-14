#pragma once

#include "../../core/IEntityRenderer.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::effect {

/**
 * @brief 发光眼睛层渲染器
 *
 * 渲染实体的发光眼睛（如末影人、蜘蛛、幻翼等）。
 * 使用叠加混合模式实现发光效果。
 *
 * 参考 MC 1.16.5 AbstractEyesLayer
 *
 * 关键点:
 * 1. 使用固定光照值 15728640 (0xF00000) = 全亮
 * 2. 使用叠加混合模式 (additive blending)
 * 3. 颜色为半透明白色 (0.5, 0.5, 0.5, 1.0)
 * 4. 使用父模型的头部部件定位眼睛位置
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template <typename TEntity, typename TModel = model::EntityModel>
class EyesLayer : public core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     */
    explicit EyesLayer(mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer)
        : m_renderer(&renderer)
    {}

    ~EyesLayer() override = default;

    /**
     * @brief 渲染眼睛层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 渲染眼睛层（CPU路径 - 已废弃）
     */
    void render(TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale) override;

    /**
     * @brief 检查是否应该渲染眼睛
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* getRenderer()
    {
        return m_renderer;
    }

    /**
     * @brief 获取父模型
     */
    [[nodiscard]] TModel* getParentModel() { return m_renderer ? &m_renderer->getModel() : nullptr; }

    /**
     * @brief 获取眼睛发光纹理
     * 子类可以重写此方法以提供特定纹理
     */
    [[nodiscard]] virtual ResourceLocation getEyesTexture(const TEntity& entity) const
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/eyes.png");
    }

    /**
     * @brief 获取发光颜色
     * 参考 MC 1.16.5 AbstractEyesLayer: 颜色为 (1.0F, 1.0F, 1.0F, 1.0F)
     */
    [[nodiscard]] virtual Vector3f getEyesColor(const TEntity& entity) const
    {
        (void)entity;
        // MC 1.16.5: 眼睛使用纯白色，通过叠加混合实现发光效果
        return Vector3f(1.0f, 1.0f, 1.0f);
    }

    /**
     * @brief 构建眼睛网格
     * @param headTransform 头部变换矩阵
     * @param vertices 输出顶点
     * @param indices 输出索引
     */
    void buildEyesMesh(
        const std::array<f64, 16>& headTransform, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    // MC 1.16.5 固定光照值: 15728640 = 0xF00000 = 全亮
    // 用于眼睛层始终显示为发光状态
    static constexpr i32 FULL_LIGHT = 15728640;

private:
    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
};

} // namespace mc::client::renderer::entity::layer::effect
