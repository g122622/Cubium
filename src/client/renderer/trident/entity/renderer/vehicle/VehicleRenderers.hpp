/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <array>
#include <memory>

// Forward declarations
namespace mc::entity {
class BoatEntity;
class AbstractMinecartEntity;
} // namespace mc::entity

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
    DarkOak = 5,
    Mangrove = 6,
    Cherry = 7,
    PaleOak = 8,
    Bamboo = 9
};

/**
 * @brief 船模型
 */
class BoatModel {
public:
    BoatModel();
    ~BoatModel() = default;

    void render(f64 scale);
    void renderNoWater(f64 scale);

    /**
     * @brief 设置桨的角度
     * @param paddleIndex 0=左桨, 1=右桨
     * @param angle X轴旋转角度
     */
    void setPaddleAngle(i32 paddleIndex, f32 angle);

private:
    void _setupParts();

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
 */
class BoatRenderer : public core::EntityRenderer {
public:
    explicit BoatRenderer(BoatType type);
    ~BoatRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] ResourceLocation getTexture() const;

private:
    BoatType m_type;
    std::unique_ptr<BoatModel> m_model;

    /**
     * @brief 计算船的摇晃角度
     * TODO: 尚未实现完整的摇晃角度计算逻辑
     */
    [[nodiscard]] f64 _calculateRockingAngle(::mc::entity::BoatEntity& boat, f64 partialTicks) const;
};

/**
 * @brief 矿车模型
 */
class MinecartModel {
public:
    MinecartModel();
    ~MinecartModel() = default;

    void render(f64 scale);

    /**
     * @brief 设置内部底板Y偏移
     * 用于乘客乘坐时的动画
     */
    void setInsideOffset(f32 yOffset);

private:
    void _setupParts();

    // 6个面：底部、左、右、后、前、内部底
    std::array<std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer>, 6> m_sides;
};

/**
 * @brief 矿车渲染器
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
     * TODO: 尚未实现完整的矿车方向计算逻辑
     */
    void _calculateCartDirection(::mc::entity::AbstractMinecartEntity& minecart, f64 partialTicks);
};

} // namespace mc::client::renderer::entity::renderer::vehicle
