#include "HeldBlockLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/world/block/Block.hpp"
#include <cmath>
#include <type_traits>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::entity {

template <typename TEntity>
void HeldBlockLayer<TEntity>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 获取持有的方块
    const ::mc::BlockState* blockState = getHeldBlock(entity);
    if (!blockState) {
        return;
    }

    // 末影人持有方块的位置
    // 参考 MC 1.16.5: 方块在头部附近
    renderBlockPipeline(*blockState, 0.0f, 0.6875f, 0.0f, cmd, context, pipeline);

    (void)cmd;
}

template <typename TEntity>
void HeldBlockLayer<TEntity>::render(TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // CPU 路径已废弃
    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

template <typename TEntity>
bool HeldBlockLayer<TEntity>::shouldRender(const TEntity& entity) const
{
    // 使用编译时类型检查
    if constexpr (std::is_base_of_v<::mc::EndermanEntity, TEntity>) {
        return entity.isHoldingBlock();
    }
    return false;
}

template <typename TEntity>
const ::mc::BlockState* HeldBlockLayer<TEntity>::getHeldBlock(const TEntity& entity) const
{
    // 使用编译时类型检查：只有 EndermanEntity 有手持方块功能
    // 参考 MC 1.16.5: EndermanEntity.getHeldBlockState()
    if constexpr (std::is_base_of_v<::mc::EndermanEntity, TEntity>) {
        return entity.getHeldBlockState();
    }
    return nullptr;
}

template <typename TEntity>
void HeldBlockLayer<TEntity>::renderBlockPipeline(const ::mc::BlockState& blockState,
    f32 x,
    f32 y,
    f32 z,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 获取或创建方块网格
    pipeline::EntityMesh* mesh = getOrCreateBlockMesh(pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算方块变换矩阵
    std::array<f64, 16> blockTransform;
    blockTransform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 方块位置偏移
    blockTransform[3] = static_cast<f64>(x);
    blockTransform[7] = static_cast<f64>(y);
    blockTransform[11] = static_cast<f64>(z);

    // 方块略微缩放
    const f32 blockScale = 0.5f; // 方块大小
    blockTransform[0] = blockScale;
    blockTransform[5] = blockScale;
    blockTransform[10] = blockScale;

    // 获取实体位置（假设是末影人）
    // 在完整实现中，应该从参数获取实体位置
    Vector3f entityPos(0.0f, 0.0f, 0.0f);

    // TODO: 从方块状态获取正确的纹理颜色
    Vector4f overlayColor(1.0f, 1.0f, 1.0f, 1.0f);

    pipeline.drawMesh(cmd, *mesh, blockTransform, entityPos, 1.0, overlayColor, 0.0f, 0.0f);

    spdlog::trace("HeldBlockLayer: Rendered held block at ({}, {}, {})", x, y, z);

    (void)blockState;
    (void)context;
}

template <typename TEntity>
void HeldBlockLayer<TEntity>::buildBlockMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 构建简单的立方体方块网格
    // 实际实现应该从方块模型获取网格数据

    constexpr f32 HALF = 0.5f;

    vertices.clear();
    indices.clear();

    // UV 坐标
    f32 u0 = 0.0f, u1 = 1.0f;
    f32 v0 = 0.0f, v1 = 1.0f;

    // 顶点格式: ModelVertex(x, y, z, u, v, nx, ny, nz)

    // 前面 (+Z)
    vertices.push_back(model::ModelVertex(-HALF, -HALF, HALF, u0, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(HALF, -HALF, HALF, u1, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(HALF, HALF, HALF, u1, v1, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(-HALF, HALF, HALF, u0, v1, 0.0f, 0.0f, 1.0f));

    // 后面 (-Z)
    vertices.push_back(model::ModelVertex(HALF, -HALF, -HALF, u0, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(-HALF, -HALF, -HALF, u1, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(-HALF, HALF, -HALF, u1, v1, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(HALF, HALF, -HALF, u0, v1, 0.0f, 0.0f, -1.0f));

    // 顶面 (+Y)
    vertices.push_back(model::ModelVertex(-HALF, HALF, HALF, u0, v0, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(HALF, HALF, HALF, u1, v0, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(HALF, HALF, -HALF, u1, v1, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-HALF, HALF, -HALF, u0, v1, 0.0f, 1.0f, 0.0f));

    // 底面 (-Y)
    vertices.push_back(model::ModelVertex(-HALF, -HALF, -HALF, u0, v0, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(HALF, -HALF, -HALF, u1, v0, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(HALF, -HALF, HALF, u1, v1, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-HALF, -HALF, HALF, u0, v1, 0.0f, -1.0f, 0.0f));

    // 右面 (+X)
    vertices.push_back(model::ModelVertex(HALF, -HALF, HALF, u0, v0, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(HALF, -HALF, -HALF, u1, v0, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(HALF, HALF, -HALF, u1, v1, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(HALF, HALF, HALF, u0, v1, 1.0f, 0.0f, 0.0f));

    // 左面 (-X)
    vertices.push_back(model::ModelVertex(-HALF, -HALF, -HALF, u0, v0, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-HALF, -HALF, HALF, u1, v0, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-HALF, HALF, HALF, u1, v1, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-HALF, HALF, -HALF, u0, v1, -1.0f, 0.0f, 0.0f));

    // 索引（每个面两个三角形）
    for (u32 face = 0; face < 6; ++face) {
        u32 base = face * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
}

template <typename TEntity>
pipeline::EntityMesh* HeldBlockLayer<TEntity>::getOrCreateBlockMesh(pipeline::EntityPipeline& pipeline)
{
    // 使用静态缓存
    static std::unique_ptr<pipeline::EntityMesh> s_blockMesh;

    if (s_blockMesh && s_blockMesh->indexCount > 0) {
        return s_blockMesh.get();
    }

    // 构建方块网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildBlockMesh(vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("HeldBlockLayer: Failed to create block mesh");
        return nullptr;
    }

    s_blockMesh = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    return s_blockMesh.get();
}

// 显式实例化
template class HeldBlockLayer<::mc::LivingEntity>;
template class HeldBlockLayer<::mc::EndermanEntity>;

} // namespace mc::client::renderer::entity::layer::entity
