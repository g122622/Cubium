#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>

namespace mc {

namespace client::renderer::entity::core {

/**
 * @brief 实体渲染器接口
 *
 * 定义实体渲染器的基本契约，将渲染器与模型类型解耦。
 * 参考 MC 1.16.5 IEntityRenderer
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template<typename TEntity, typename TModel>
class IEntityRenderer {
public:
    virtual ~IEntityRenderer() = default;

    /**
     * @brief 获取实体模型
     * @return 模型引用
     */
    [[nodiscard]] virtual TModel& getModel() = 0;

    /**
     * @brief 获取实体模型（const版本）
     * @return 模型const引用
     */
    [[nodiscard]] virtual const TModel& getModel() const = 0;

    /**
     * @brief 获取实体纹理
     * @param entity 实体引用
     * @return 纹理资源位置
     */
    [[nodiscard]] virtual ResourceLocation getEntityTexture(TEntity& entity) = 0;

    /**
     * @brief 获取实体纹理（const版本）
     * @param entity 实体const引用
     * @return 纹理资源位置
     */
    [[nodiscard]] virtual ResourceLocation getEntityTexture(const TEntity& entity) const = 0;
};

} // namespace mc::client::renderer::entity::core
} // namespace mc
