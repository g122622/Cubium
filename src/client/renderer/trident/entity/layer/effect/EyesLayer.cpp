#include "EyesLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "../../model/base/BipedModel.hpp"
#include "../../model/monster/SpiderModel.hpp"
#include "../../model/monster/EndermanModel.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::effect {

template<typename TEntity, typename TModel>
void EyesLayer<TEntity, TModel>::renderPipeline(
    TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 获取父模型
    TModel* parentModel = getParentModel();
    if (!parentModel) {
        return;
    }

    // 获取眼睛纹理和颜色
    ResourceLocation texture = getEyesTexture(entity);
    Vector3f color = getEyesColor(entity);

    // 获取头部部件变换
    // 参考 MC 1.16.5 AbstractEyesLayer: this.getEntityModel().getModelHead().translateRotate(matrixStack)
    std::shared_ptr<model::ModelRenderer> headPart = parentModel->getModelHead();
    if (!headPart) {
        return;
    }

    // 从头部部件获取变换矩阵
    std::array<f64, 16> headTransform;
    headPart->getTransformMatrix(headTransform);

    // 构建眼睛网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildEyesMesh(headTransform, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // 创建临时网格
    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("EyesLayer: Failed to create eyes mesh");
        return;
    }

    // 获取实体位置
    Vector3f entityPos(
        static_cast<f32>(entity.x()),
        static_cast<f32>(entity.y()),
        static_cast<f32>(entity.z())
    );

    // 使用发光颜色作为叠加颜色
    Vector4f overlayColor(color.x, color.y, color.z, 1.0f);

    // 切换到叠加混合模式（用于眼睛发光效果）
    // 参考 MC 1.16.5 EyesLayer: GlintResourceManager.RenderTypes.entityGlintDirect()
    pipeline.bind(cmd, pipeline::BlendMode::Additive);

    pipeline.drawMesh(cmd, result.value(), headTransform, entityPos, 1.0,
                      overlayColor, 0.0f, 0.0f);

    // 恢复 Alpha 混合模式
    pipeline.bind(cmd, pipeline::BlendMode::Alpha);

    spdlog::trace("EyesLayer: Rendered eyes for entity");

    (void)texture;
    (void)cmd;
}

template<typename TEntity, typename TModel>
void EyesLayer<TEntity, TModel>::render(
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

template<typename TEntity, typename TModel>
bool EyesLayer<TEntity, TModel>::shouldRender(const TEntity& entity) const {
    // 默认情况下眼睛层总是可见
    // 子类可以根据实体状态重写此方法
    // 例如：末影人在愤怒时眼睛发光
    (void)entity;
    return true;
}

template<typename TEntity, typename TModel>
void EyesLayer<TEntity, TModel>::buildEyesMesh(
    const std::array<f64, 16>& headTransform,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 眼睛是一个简单的四边形，位于头部前方
    // 参考 MC 1.16.5 的眼睛模型
    // 眼睛位置相对于头部中心

    constexpr f32 EYE_WIDTH = 0.25f;
    constexpr f32 EYE_HEIGHT = 0.15f;
    constexpr f32 EYE_DEPTH = 0.05f;

    f32 hw = EYE_WIDTH / 2.0f;
    f32 hh = EYE_HEIGHT / 2.0f;

    vertices.clear();
    indices.clear();

    // 提取头部位置（变换矩阵的平移部分）
    f64 headX = headTransform[3];   // X 平移
    f64 headY = headTransform[7];   // Y 平移
    f64 headZ = headTransform[11];  // Z 平移

    // 提取旋转信息（简化：使用矩阵中的旋转分量）
    // 对于简单的眼睛层，我们使用固定的偏移位置
    // 实际上眼睛应该跟随头部旋转

    // 左眼位置（相对于头部中心）
    f32 leftEyeX = static_cast<f32>(headX) - 0.15f;
    f32 rightEyeX = static_cast<f32>(headX) + 0.15f;
    f32 eyeY = static_cast<f32>(headY);
    f32 eyeZ = static_cast<f32>(headZ) + static_cast<f32>(EYE_DEPTH);

    // 左眼顶点
    // 顶点格式: ModelVertex(x, y, z, u, v, nx, ny, nz)
    vertices.push_back(model::ModelVertex(leftEyeX - hw, eyeY - hh, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX + hw, eyeY - hh, eyeZ, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX + hw, eyeY + hh, eyeZ, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX - hw, eyeY + hh, eyeZ, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f));

    // 右眼顶点
    vertices.push_back(model::ModelVertex(rightEyeX - hw, eyeY - hh, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX + hw, eyeY - hh, eyeZ, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX + hw, eyeY + hh, eyeZ, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX - hw, eyeY + hh, eyeZ, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f));

    // 左眼索引
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    // 右眼索引
    indices.push_back(4);
    indices.push_back(5);
    indices.push_back(6);
    indices.push_back(4);
    indices.push_back(6);
    indices.push_back(7);
}

// 显式实例化常用类型
template class EyesLayer< ::mc::LivingEntity, ::mc::client::renderer::entity::model::BipedModel>;
template class EyesLayer< ::mc::LivingEntity, ::mc::client::renderer::entity::model::monster::SpiderModel>;
template class EyesLayer< ::mc::LivingEntity, ::mc::client::renderer::entity::model::monster::EndermanModel>;

} // namespace mc::client::renderer::entity::layer::effect
