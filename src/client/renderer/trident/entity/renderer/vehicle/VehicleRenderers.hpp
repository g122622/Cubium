#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <array>

// Forward declarations
namespace mc {
class BoatEntity;
class AbstractMinecartEntity;
}

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::vehicle {

/**
 * @brief 船类型枚举
 */
enum class BoatType : u8 {
    Oak = 0,
    Spruce = 1,
    Birch = 2,
    Jungle = 3,
    Acacia = 4,
    DarkOak = 5
};

/**
 * @brief 船模型
 *
 * 参考 MC 1.16.5 BoatModel
 */
class BoatModel {
public:
    BoatModel();
    ~BoatModel() = default;

    void render(f64 scale = 1.0f / 16.0f);
    void renderNoWater(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 设置桨的角度
     * @param paddleIndex 0=左桨, 1=右桨
     * @param angle X轴旋转角度
     */
    void setPaddleAngle(i32 paddleIndex, f32 angle);

private:
    void setupParts();

    i32 m_textureWidth = 128;
    i32 m_textureHeight = 64;

    // 船体部件
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bottom;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_back;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_front;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_left;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_right;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_paddleLeft;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_paddleRight;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_noWater;
};

/**
 * @brief 船渲染器
 *
 * 参考 MC 1.16.5 BoatRenderer
 */
class BoatRenderer : public core::EntityRenderer {
public:
    explicit BoatRenderer(BoatType type = BoatType::Oak);
    ~BoatRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] ResourceLocation getTexture() const;

private:
    BoatType m_type;
    std::unique_ptr<BoatModel> m_model;

    /**
     * @brief 计算船的摇晃角度
     */
    [[nodiscard]] f64 calculateRockingAngle(::mc::BoatEntity& boat, f64 partialTicks) const;
};

/**
 * @brief 矿车模型
 *
 * 参考 MC 1.16.5 MinecartModel
 */
class MinecartModel {
public:
    MinecartModel();
    ~MinecartModel() = default;

    void render(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 设置内部底板Y偏移
     * 用于乘客乘坐时的动画
     */
    void setInsideOffset(f32 yOffset);

private:
    void setupParts();

    // 6个面：底部、左、右、后、前、内部底
    std::array<std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer>, 6> m_sides;
};

/**
 * @brief 矿车渲染器
 *
 * 参考 MC 1.16.5 MinecartRenderer
 */
class MinecartRenderer : public core::EntityRenderer {
public:
    MinecartRenderer();
    ~MinecartRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] static ResourceLocation getMinecartTexture();

private:
    std::unique_ptr<MinecartModel> m_model;

    /**
     * @brief 计算矿车方向
     */
    void calculateCartDirection(::mc::AbstractMinecartEntity& minecart, f64 partialTicks);
};

/**
 * @brief 注册载具渲染器
 */
void registerVehicleRenderers(::mc::client::renderer::entity::EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::vehicle
