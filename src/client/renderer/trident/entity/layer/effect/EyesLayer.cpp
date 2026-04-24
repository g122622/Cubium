#include "EyesLayer.hpp"

namespace mc::client::renderer::entity::layer::effect {

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
    // 参考 MC 1.16.5 AbstractEyesLayer.render()
    // 使用叠加混合模式渲染眼睛纹理
    ResourceLocation texture = getEyesTexture(entity);
    Vector3f color = getEyesColor(entity);

    // TODO: 实际渲染眼睛网格
    // 需要使用父模型的头部部件，应用纹理和颜色渲染
    // 关键步骤：
    // 1. 绑定眼睛纹理
    // 2. 设置发光混合模式（additive blending）
    // 3. 渲染头部部件
    // 4. 恢复正常混合模式

    (void)texture;
    (void)color;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
    (void)entity;
}

template<typename TEntity>
bool EyesLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // 默认情况下眼睛层总是可见
    // 子类可以根据实体状态重写此方法
    (void)entity;
    return true;
}

// 显式实例化常用类型
template class EyesLayer<::mc::LivingEntity>;

} // namespace mc::client::renderer::entity::layer::effect
