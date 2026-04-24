#pragma once

#include "../../model/core/EntityModel.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::layer::core {

/**
 * @brief 层渲染器基类
 *
 * 用于在基础实体模型上添加额外渲染层（盔甲、鞍、发光效果等）。
 * 参考 MC 1.16.5 LayerRenderer
 *
 * @tparam TEntity 实体类型
 */
template<typename TEntity>
class LayerRenderer {
public:
    virtual ~LayerRenderer() = default;

    /**
     * @brief 渲染层
     *
     * @param entity 实体
     * @param limbSwing 步态动画周期
     * @param limbSwingAmount 步态动画强度
     * @param partialTicks 部分tick
     * @param ageInTicks 年龄tick（用于空闲动画）
     * @param netHeadYaw 头部偏航角（相对身体）
     * @param headPitch 头部俯仰角
     * @param scale 缩放因子
     */
    virtual void render(
        TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale
    ) = 0;

    /**
     * @brief 检查是否应该渲染此层
     * @param entity 实体
     * @return 是否应该渲染
     */
    [[nodiscard]] virtual bool shouldRender(const TEntity& entity) const {
        (void)entity;
        return true;
    }
};

} // namespace mc::client::renderer::entity::layer::core
