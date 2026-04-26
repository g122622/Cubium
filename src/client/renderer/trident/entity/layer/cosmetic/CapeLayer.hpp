#pragma once

#include "../core/LayerRenderer.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>

namespace mc {
class Player;
struct TextureRegion;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
}

namespace mc::client::renderer::entity::layer::cosmetic {

/**
 * @brief 斗篷层渲染器
 *
 * 渲染玩家的斗篷。支持动态摆动动画。
 *
 * 参考 MC 1.16.5 CapeLayer
 */
class CapeLayer : public core::LayerRenderer<::mc::Player> {
public:
    CapeLayer() = default;
    ~CapeLayer() override = default;

    /**
     * @brief 渲染斗篷层（GPU管线路径）
     */
    void renderPipeline(
        ::mc::Player& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline
    ) override;

    /**
     * @brief 渲染斗篷层（CPU路径 - 已废弃）
     */
    void render(
        ::mc::Player& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) override;

    /**
     * @brief 检查是否应该渲染斗篷
     */
    [[nodiscard]] bool shouldRender(const ::mc::Player& entity) const override;

    /**
     * @brief 设置自定义斗篷纹理
     * @param region 纹理区域（可为 nullptr）
     */
    void setCapeTexture(const TextureRegion* region) { m_customCapeRegion = region; }

    /**
     * @brief 获取当前斗篷纹理
     */
    [[nodiscard]] const TextureRegion* getCapeTexture() const { return m_customCapeRegion; }

private:
    /**
     * @brief 计算斗篷摆动角度
     * @param entity 玩家实体
     * @param context 动画上下文
     * @param partialTicks 部分 tick
     * @return 斗篷旋转角度（度）
     */
    [[nodiscard]] f32 calculateCapeSwing(::mc::Player& entity, const mc::client::renderer::entity::core::AnimationContext& context, f32 partialTicks) const;

    /**
     * @brief 构建斗篷网格
     */
    void buildCapeMesh(
        f32 swingAngle,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices
    );

    /**
     * @brief 获取或创建斗篷网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateCapeMesh(
        f32 swingAngle,
        pipeline::EntityPipeline& pipeline
    );

    const TextureRegion* m_customCapeRegion = nullptr;

    // 斗篷网格缓存（按摆动角度离散化）
    // 使用有限的几个角度来避免每帧重建
    std::unordered_map<i32, pipeline::EntityMesh> m_capeMeshCache;
};

} // namespace mc::client::renderer::entity::layer::cosmetic
