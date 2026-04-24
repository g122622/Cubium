#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include <memory>

namespace mc {
class Entity;
}

namespace mc::client::renderer::entity::effect::glow {

/**
 * @brief 发光效果管理器
 *
 * 参考 MC 1.16.5 发光轮廓效果
 * 用于渲染实体的发光轮廓（如发光鱿鱼、发光药水效果）。
 */
class GlowEffect {
public:
    /**
     * @brief 初始化发光效果系统
     */
    static void initialize();

    /**
     * @brief 清理发光效果系统
     */
    static void cleanup();

    /**
     * @brief 检查实体是否有发光效果
     * @param entity 实体
     * @return 是否有发光效果
     */
    [[nodiscard]] static bool hasGlowEffect(Entity& entity);

    /**
     * @brief 获取发光颜色
     * @param entity 实体
     * @return 发光颜色 (RGBA)
     *
     * 默认颜色为白色 (1, 1, 1, 1)。
     * 特殊实体可能有不同颜色：
     * - 发光鱿鱼：青色
     * - 团队颜色：团队颜色
     */
    [[nodiscard]] static math::Vector4f getGlowColor(Entity& entity);

    /**
     * @brief 渲染发光轮廓
     * @param entity 实体
     * @param partialTicks 部分tick
     * @param color 发光颜色
     */
    static void renderGlow(Entity& entity, f64 partialTicks, const math::Vector4f& color);

    /**
     * @brief 渲染所有发光实体
     * @param partialTicks 部分tick
     *
     * 遍历所有发光实体并渲染轮廓。
     */
    static void renderAllGlowing(f64 partialTicks);

private:
    GlowEffect() = delete;
    ~GlowEffect() = delete;

    /**
     * @brief 生成发光轮廓网格
     */
    static void generateGlowMesh(
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        f64 scale
    );

    static bool s_initialized;
};

} // namespace mc::client::renderer::entity::effect::glow
