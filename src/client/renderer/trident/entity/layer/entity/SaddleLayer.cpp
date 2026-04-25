#include "SaddleLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::entity {

// 静态成员定义
template<typename TEntity>
std::unique_ptr<pipeline::EntityMesh> SaddleLayer<TEntity>::s_saddleMesh = nullptr;

template<typename TEntity>
void SaddleLayer<TEntity>::renderPipeline(
    TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 获取或创建鞍网格
    pipeline::EntityMesh* mesh = getOrCreateSaddleMesh(pipeline);
    if (!mesh || mesh->indexCount == 0) {
        return;
    }

    // 计算鞍的变换矩阵
    // 鞍位于实体背部
    std::array<f64, 16> saddleTransform;
    saddleTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 鞍的位置在背部中心
    // 参考 MC 1.16.5 的鞍模型位置
    saddleTransform[3] = 0.0;      // X - 居中
    saddleTransform[7] = 1.0;      // Y - 背部高度
    saddleTransform[11] = 0.0;     // Z - 略微向前

    // 应用步态动画（鞍跟随身体摆动）
    f64 limbSwing = context.limbSwing;
    f64 limbSwingAmount = context.limbSwingAmount;
    if (limbSwingAmount > 0.01) {
        f64 swingAngle = std::sin(limbSwing * 0.5) * limbSwingAmount * 0.1;
        // 轻微的侧向倾斜
        saddleTransform[2] = swingAngle * 0.1;
        saddleTransform[6] = -swingAngle * 0.1;
    }

    // 获取实体位置
    Vector3f entityPos(
        static_cast<f32>(entity.x()),
        static_cast<f32>(entity.y()),
        static_cast<f32>(entity.z())
    );

    // 使用实体的受伤时间
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    }

    pipeline.drawMesh(cmd, *mesh, saddleTransform, entityPos, 1.0,
                      Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);

    spdlog::trace("SaddleLayer: Rendered saddle on entity");

    (void)context;
}

template<typename TEntity>
void SaddleLayer<TEntity>::render(
    TEntity& entity,
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

template<typename TEntity>
bool SaddleLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // 检查实体是否装备了鞍
    // 完整实现需要检查实体的 isSaddled() 方法
    // 例如马、猪等可骑乘实体
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        // TODO: 检查实体是否有 isSaddled() 方法
        // 目前暂时返回 false，待实体类实现鞍装备后再完善
        (void)entity;
    }
    return false;
}

template<typename TEntity>
void SaddleLayer<TEntity>::buildSaddleMesh(
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 参考 MC 1.16.5 的鞍模型
    // 鞍是一个简单的扁平形状，位于实体背部

    // 鞍的尺寸（世界单位）
    constexpr f32 SADDLE_WIDTH = 0.3f;
    constexpr f32 SADDLE_HEIGHT = 0.1f;
    constexpr f32 SADDLE_DEPTH = 0.4f;

    f32 hw = SADDLE_WIDTH / 2.0f;
    f32 hh = SADDLE_HEIGHT / 2.0f;
    f32 hd = SADDLE_DEPTH / 2.0f;

    // 构建鞍的顶点（简单的立方体）
    // UV 坐标
    f32 u0 = 0.0f, u1 = 1.0f;
    f32 v0 = 0.0f, v1 = 1.0f;

    // 顶点格式: ModelVertex(x, y, z, u, v, nx, ny, nz)

    // 顶面
    vertices.push_back(model::ModelVertex(-hw, hh, -hd, u0, v0, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex( hw, hh, -hd, u1, v0, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex( hw, hh,  hd, u1, v1, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-hw, hh,  hd, u0, v1, 0.0f, 1.0f, 0.0f));

    // 底面
    vertices.push_back(model::ModelVertex(-hw, -hh, -hd, u0, v0, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex( hw, -hh, -hd, u1, v0, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex( hw, -hh,  hd, u1, v1, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-hw, -hh,  hd, u0, v1, 0.0f, -1.0f, 0.0f));

    // 前面
    vertices.push_back(model::ModelVertex(-hw, -hh, -hd, u0, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex( hw, -hh, -hd, u1, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex( hw,  hh, -hd, u1, v1, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(-hw,  hh, -hd, u0, v1, 0.0f, 0.0f, -1.0f));

    // 后面
    vertices.push_back(model::ModelVertex(-hw, -hh,  hd, u0, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex( hw, -hh,  hd, u1, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex( hw,  hh,  hd, u1, v1, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(-hw,  hh,  hd, u0, v1, 0.0f, 0.0f, 1.0f));

    // 左面
    vertices.push_back(model::ModelVertex(-hw, -hh, -hd, u0, v0, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-hw, -hh,  hd, u1, v0, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-hw,  hh,  hd, u1, v1, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-hw,  hh, -hd, u0, v1, -1.0f, 0.0f, 0.0f));

    // 右面
    vertices.push_back(model::ModelVertex( hw, -hh, -hd, u0, v0, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex( hw, -hh,  hd, u1, v0, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex( hw,  hh,  hd, u1, v1, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex( hw,  hh, -hd, u0, v1, 1.0f, 0.0f, 0.0f));

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

template<typename TEntity>
pipeline::EntityMesh* SaddleLayer<TEntity>::getOrCreateSaddleMesh(pipeline::EntityPipeline& pipeline) {
    if (s_saddleMesh && s_saddleMesh->indexCount > 0) {
        return s_saddleMesh.get();
    }

    // 构建鞍网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildSaddleMesh(vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("SaddleLayer: Failed to create saddle mesh");
        return nullptr;
    }

    s_saddleMesh = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    return s_saddleMesh.get();
}

// 显式实例化
template class SaddleLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::entity
