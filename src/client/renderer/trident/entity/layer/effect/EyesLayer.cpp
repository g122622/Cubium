#include "EyesLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::effect {

template<typename TEntity>
void EyesLayer<TEntity>::renderPipeline(
    TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 获取眼睛纹理和颜色
    ResourceLocation texture = getEyesTexture(entity);
    Vector3f color = getEyesColor(entity);

    // TODO: 实际渲染眼睛网格
    // 眼睛层需要使用叠加混合模式（additive blending）
    // 关键步骤：
    // 1. 设置叠加混合模式
    // 2. 渲染头部部件的眼睛纹理
    // 3. 恢复正常混合模式

    // 由于眼睛层需要使用父模型的头部部件来渲染，
    // 完整实现需要访问父渲染器的模型
    // 这里先提供一个简化实现

    // 构建眼睛网格（简化的四边形）
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildEyesMesh(vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // 创建临时网格
    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("EyesLayer: Failed to create eyes mesh");
        return;
    }

    // 计算眼睛变换矩阵（位于头部）
    std::array<f64, 16> eyesTransform;
    eyesTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 眼睛位置在头部
    eyesTransform[3] = 0.0;           // X - 居中
    eyesTransform[7] = 1.5;           // Y - 头部高度
    eyesTransform[11] = 0.1;          // Z - 略微向前

    // 应用头部旋转
    f32 headYaw = static_cast<f32>(context.netHeadYaw);
    f32 headPitch = static_cast<f32>(context.headPitch);
    f32 yawRad = headYaw * 3.14159265f / 180.0f;
    f32 pitchRad = headPitch * 3.14159265f / 180.0f;

    f32 cosYaw = std::cos(yawRad);
    f32 sinYaw = std::sin(yawRad);
    f32 cosPitch = std::cos(pitchRad);
    f32 sinPitch = std::sin(pitchRad);

    // 应用旋转到变换矩阵
    eyesTransform[0] = cosYaw;
    eyesTransform[2] = -sinYaw;
    eyesTransform[5] = cosPitch;
    eyesTransform[6] = sinPitch;
    eyesTransform[8] = sinYaw;
    eyesTransform[9] = -sinPitch;
    eyesTransform[10] = cosYaw * cosPitch;

    // 获取实体位置
    Vector3f entityPos(
        static_cast<f32>(entity.x()),
        static_cast<f32>(entity.y()),
        static_cast<f32>(entity.z())
    );

    // 使用发光颜色作为叠加颜色
    Vector4f overlayColor(color.x, color.y, color.z, 1.0f);

    // TODO: 需要设置叠加混合模式
    // 目前先使用普通渲染，后续需要添加混合模式支持
    pipeline.drawMesh(cmd, result.value(), eyesTransform, entityPos, 1.0,
                      overlayColor, 0.0f, 0.0f);

    spdlog::trace("EyesLayer: Rendered eyes for entity");

    (void)texture;
    (void)cmd;
}

template<typename TEntity>
void EyesLayer<TEntity>::render(
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
bool EyesLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // 默认情况下眼睛层总是可见
    // 子类可以根据实体状态重写此方法
    // 例如：末影人在愤怒时眼睛发光
    (void)entity;
    return true;
}

template<typename TEntity>
void EyesLayer<TEntity>::buildEyesMesh(
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 眼睛是一个简单的四边形，位于头部前方
    // 参考 MC 1.16.5 的眼睛模型

    constexpr f32 EYE_WIDTH = 0.25f;
    constexpr f32 EYE_HEIGHT = 0.15f;
    constexpr f32 EYE_DEPTH = 0.05f;

    f32 hw = EYE_WIDTH / 2.0f;
    f32 hh = EYE_HEIGHT / 2.0f;

    vertices.clear();
    indices.clear();

    // 左眼
    f32 leftEyeX = -0.15f;
    // 顶点格式: ModelVertex(x, y, z, u, v, nx, ny, nz)
    vertices.push_back(model::ModelVertex(leftEyeX - hw, -hh, EYE_DEPTH, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX + hw, -hh, EYE_DEPTH, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX + hw,  hh, EYE_DEPTH, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(leftEyeX - hw,  hh, EYE_DEPTH, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f));

    // 右眼
    f32 rightEyeX = 0.15f;
    vertices.push_back(model::ModelVertex(rightEyeX - hw, -hh, EYE_DEPTH, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX + hw, -hh, EYE_DEPTH, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX + hw,  hh, EYE_DEPTH, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(rightEyeX - hw,  hh, EYE_DEPTH, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f));

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
template class EyesLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::effect
