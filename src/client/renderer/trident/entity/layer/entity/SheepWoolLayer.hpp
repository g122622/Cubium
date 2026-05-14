#pragma once

#include "../../core/IEntityRenderer.hpp"
#include "../../model/base/BipedModel.hpp"
#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class LivingEntity;
class SheepEntity;
} // namespace mc

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 羊毛层渲染器
 *
 * 渲染羊身上的羊毛。支持染色羊毛的颜色和 jeb_ 彩虹羊。
 *
 * 参考 MC 1.16.5 SheepWoolLayer
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template <typename TEntity = ::mc::LivingEntity, typename TModel = ::mc::client::renderer::entity::model::BipedModel>
class SheepWoolLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     * @param woolModel 羊毛模型（可选）
     */
    explicit SheepWoolLayer(mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer,
        std::shared_ptr<TModel> woolModel = nullptr)
        : m_renderer(&renderer)
        , m_woolModel(std::move(woolModel))
    {}

    ~SheepWoolLayer() override = default;

    /**
     * @brief 渲染羊毛层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 渲染羊毛层（CPU路径 - 已废弃）
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
     * @brief 检查是否应该渲染羊毛
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
     * @brief 获取羊毛模型
     */
    [[nodiscard]] TModel* getWoolModel() { return m_woolModel.get(); }

    /**
     * @brief 获取羊毛颜色
     * @param entity 羊实体
     * @param ticksExisted 实体存活时间（用于彩虹羊）
     * @return RGB 颜色值
     */
    [[nodiscard]] static Vector3f getWoolColor(const TEntity& entity, u32 ticksExisted);

    /**
     * @brief 计算 jeb_ 彩虹羊颜色
     * @param ticksExisted 实体存活时间
     * @return RGB 颜色值
     */
    [[nodiscard]] static Vector3f computeRainbowColor(u32 ticksExisted);

    /**
     * @brief 检查是否为 jeb_ 彩虹羊
     * @param entity 羊实体
     * @return 如果是彩虹羊返回 true
     */
    [[nodiscard]] static bool isRainbowSheep(const TEntity& entity);

private:
    /**
     * @brief 构建羊毛网格
     */
    void buildWoolMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建羊毛网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateWoolMesh(pipeline::EntityPipeline& pipeline);

    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
    std::shared_ptr<TModel> m_woolModel;

    // 羊毛网格缓存
    static std::unique_ptr<pipeline::EntityMesh> s_woolMesh;
};

} // namespace mc::client::renderer::entity::layer::entity
