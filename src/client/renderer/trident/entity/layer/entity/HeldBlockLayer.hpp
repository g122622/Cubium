#pragma once

#include "../../model/core/ModelRenderer.hpp"
#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector4.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class LivingEntity;
class BlockState;
} // namespace mc

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 方块持有层渲染器
 *
 * 渲染末影人手持的方块。
 *
 * 类型安全的实现：
 * - 使用 `if constexpr` + `std::is_base_of_v` 进行编译时类型检查
 * - 只有 `EndermanEntity` 有手持方块功能
 * - 从 `EndermanEntity::getHeldBlockState()` 获取方块状态
 * - 从 `EndermanEntity::isHoldingBlock()` 判断是否渲染
 *
 * 参考 MC 1.16.5 HeldBlockLayer (for Enderman)
 *
 * @tparam TEntity 实体类型
 */
template <typename TEntity>
class HeldBlockLayer : public core::LayerRenderer<TEntity> {
public:
    HeldBlockLayer() = default;
    ~HeldBlockLayer() override = default;

    /**
     * @brief 渲染方块持有层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 渲染方块持有层（CPU路径 - 已废弃）
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
     * @brief 检查是否应该渲染持有的方块
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

private:
    /**
     * @brief 获取实体持有的方块状态
     */
    [[nodiscard]] const ::mc::BlockState* getHeldBlock(const TEntity& entity) const;

    /**
     * @brief 渲染持有的方块（GPU管线路径）
     */
    void renderBlockPipeline(const ::mc::BlockState& blockState,
        f32 x,
        f32 y,
        f32 z,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 构建简单方块网格
     */
    void buildBlockMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建方块网格
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateBlockMesh(pipeline::EntityPipeline& pipeline);
};

} // namespace mc::client::renderer::entity::layer::entity
