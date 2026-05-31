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

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/core/IEntityRenderer.hpp"
#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include <memory>
#include <vector>

namespace mc {
class Player;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline; // 前向声明
}

namespace mc::client::renderer::entity::renderer::player {

/**
 * @brief 玩家渲染器
 *
 * 支持标准手臂和纤细手臂两种模式。
 *
 * 注意：Player 类不继承 LivingEntity，因此这里直接继承 EntityRenderer，
 * 但实现了层渲染器支持和 IEntityRenderer 接口。
 */
class PlayerRenderer : public core::EntityRenderer,
                       public core::IEntityRenderer<::mc::Player, model::player::PlayerModel> {
public:
    using core::EntityRenderer::computeAnimationContext;

    /**
     * @brief 构造函数
     * @param slimArms 是否使用纤细手臂
     */
    explicit PlayerRenderer(bool slimArms = false);
    ~PlayerRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 玩家渲染器支持动画
     */
    [[nodiscard]] bool supportsAnimation() const override { return true; }

    /**
     * @brief 玩家渲染器支持层渲染
     */
    [[nodiscard]] bool supportsLayers() const override { return true; }

    /**
     * @brief 渲染层（GPU管线路径）
     */
    void renderLayersPipeline(Entity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 渲染右手臂（第一人称）
     */
    void renderRightArm(::mc::Player& player, f64 partialTicks);

    /**
     * @brief 渲染左手臂（第一人称）
     */
    void renderLeftArm(::mc::Player& player, f64 partialTicks);

    /**
     * @brief 获取是否使用纤细手臂
     */
    [[nodiscard]] bool hasSlimArms() const { return m_slimArms; }

    /**
     * @brief 获取模型
     */
    [[nodiscard]] model::player::PlayerModel& getModel() override { return m_model; }
    [[nodiscard]] const model::player::PlayerModel& getModel() const override { return m_model; }

    /**
     * @brief 获取实体纹理（IEntityRenderer 接口）
     * @param entity 玩家实体
     * @return 皮肤纹理资源位置
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::Player& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::Player& entity) const override;

    /**
     * @brief 设置皮肤纹理区域
     * @param region 皮肤纹理区域（可为 nullptr 使用默认皮肤）
     */
    void setSkinTexture(const TextureRegion* region) { m_skinRegion = region; }

    /**
     * @brief 设置披风纹理区域
     * @param region 披风纹理区域（可为 nullptr）
     */
    void setCapeTexture(const TextureRegion* region) { m_capeRegion = region; }

    /**
     * @brief 设置鞘翅纹理区域
     * @param region 鞘翅纹理区域（可为 nullptr）
     */
    void setElytraTexture(const TextureRegion* region) { m_elytraRegion = region; }

    /**
     * @brief 获取皮肤纹理区域
     */
    [[nodiscard]] const TextureRegion* getSkinTexture() const { return m_skinRegion; }

    /**
     * @brief 获取披风纹理区域
     */
    [[nodiscard]] const TextureRegion* getCapeTexture() const { return m_capeRegion; }

    /**
     * @brief 获取鞘翅纹理区域
     */
    [[nodiscard]] const TextureRegion* getElytraTexture() const { return m_elytraRegion; }

    // ========== 层渲染器管理 ==========

    /**
     * @brief 添加层渲染器（使用 LayerRenderer<Player> 模板）
     */
    template <typename TLayer, typename... TArgs>
    void addLayer(TArgs&&... args)
    {
        m_layers.push_back(std::make_unique<TLayer>(std::forward<TArgs>(args)...));
    }

    /**
     * @brief 获取层渲染器数量
     */
    [[nodiscard]] size_t getLayerCount() const { return m_layers.size(); }

protected:
    /**
     * @brief 设置模型可见性
     *
     * 根据玩家设置显示/隐藏各部件。
     */
    void setModelVisibilities(::mc::Player& player);

    /**
     * @brief 确定手臂姿态
     */
    model::player::ArmPose determineArmPose(::mc::Player& player, bool mainHand);

    /**
     * @brief 计算动画上下文
     */
    void computeAnimationContext(::mc::Player& player, f64 partialTicks, core::AnimationContext& context);

    /**
     * @brief 计算步态动画周期
     */
    [[nodiscard]] f64 getLimbSwing(::mc::Player& player, f64 partialTicks) const;

    /**
     * @brief 计算步态动画强度
     */
    [[nodiscard]] f64 getLimbSwingAmount(::mc::Player& player, f64 partialTicks) const;

    /**
     * @brief 获取头部偏航角
     */
    [[nodiscard]] f64 getHeadYaw(::mc::Player& player, f64 partialTicks) const;

    /**
     * @brief 获取头部俯仰角
     */
    [[nodiscard]] f64 getHeadPitch(::mc::Player& player, f64 partialTicks) const;

    /**
     * @brief 获取年龄（tick）
     */
    [[nodiscard]] f64 getAgeInTicks(::mc::Player& player) const;

private:
    void _setupLayers();

    model::player::PlayerModel m_model;
    bool m_slimArms; // 是否使用纤细手臂

    // 层渲染器 - 使用 LayerRenderer<Player> 模板类型
    std::vector<std::unique_ptr<layer::core::LayerRenderer<::mc::Player>>> m_layers;

    // 皮肤纹理区域
    const TextureRegion* m_skinRegion = nullptr;
    const TextureRegion* m_capeRegion = nullptr;
    const TextureRegion* m_elytraRegion = nullptr;
};

} // namespace mc::client::renderer::entity::renderer::player
