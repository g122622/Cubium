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

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/core/IEntityRenderer.hpp"
#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline; // 前向声明
}

namespace mc::client::renderer::entity::renderer::player {

/**
 * @brief 玩家渲染器
 *
 * 第三人称玩家走 GPU 管线路径（EntityRendererManager::renderWithPipeline）：
 * 1. supportsAnimation() 返回 true → Path B（ModelFactory + AnimatedMeshCache）
 *    主模型由 ModelFactory::createModel("minecraft:player") 创建，动画状态由
 *    EntityRendererManager::_createModelForEntity + _applyPlayerCrossbowState 统一填充
 *    （本地玩家通过 m_localPlayerAccessor 回填 use-item/ArmPose/主手/蹲伏/游泳）。
 * 2. supportsLayers() 返回 true → 主模型绘制后调用 renderLayersPipelineClient，
 *    分发到本渲染器注册的层（HeldItemLayer/HeadLayer，均基于 ClientEntity）。
 *
 * 皮肤区域选择不在本渲染器完成：AnimatedMeshCache 的 UvRemapFunc 按 entityId
 * 经 PlayerSkinRegionProvider 查询动态图集区域（见 EntityRendererManager），
 * 因此 PlayerRenderer 是无每玩家实例状态的单例——所有皮肤/装备状态都从
 * ClientEntity 每帧读取，符合"单例不能存每玩家状态"的约束。
 *
 * 披风(CapeLayer)/鞘翅(ElytraLayer)本阶段不接入：项目无 cape/elytra 默认资源，
 * 两层的 shouldRender 恒为 false，接入亦不渲染。待资源就绪后再注册并补 ClientEntity
 * 的 isWearingCape/elytra 纹理区域查询。
 */
class PlayerRenderer : public core::EntityRenderer,
                       public core::IEntityRenderer<::mc::client::ClientEntity, model::player::PlayerModel> {
public:
    /**
     * @brief 构造函数
     * @param slimArms 是否使用纤细手臂
     */
    explicit PlayerRenderer(bool slimArms);
    ~PlayerRenderer() override = default;

    /**
     * @brief CPU 渲染路径入口（已弃用，保留满足基类纯虚）
     *
     * 第三人称玩家实际由 renderWithPipeline → GPU 管线渲染，不经此入口。
     */
    void render(Entity& entity, f64 partialTicks) override;

    /**
     * @brief 玩家渲染器支持动画（进入 GPU 管线 Path B）
     */
    [[nodiscard]] bool supportsAnimation() const override { return true; }

    /**
     * @brief 玩家渲染器支持层渲染
     */
    [[nodiscard]] bool supportsLayers() const override { return true; }

    /**
     * @brief 渲染层（GPU管线路径，ClientEntity 版本）
     *
     * 分发到已注册的层（HeldItemLayer/HeadLayer）。
     * 对应 MC 1.21.11 RenderLayer 遍历调用 submit() 的逻辑。
     */
    void renderLayersPipelineClient(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 获取是否使用纤细手臂
     */
    [[nodiscard]] bool hasSlimArms() const { return m_slimArms; }

    /**
     * @brief 获取模型（IEntityRenderer 接口，供层渲染器获取父模型）
     */
    [[nodiscard]] model::player::PlayerModel& getModel() override { return m_model; }
    [[nodiscard]] const model::player::PlayerModel& getModel() const override { return m_model; }

    /**
     * @brief 获取实体纹理（IEntityRenderer 接口契约实现）
     *
     * 玩家皮肤区域由 EntityRendererManager 的 UvRemapFunc 按 entityId 经
     * PlayerSkinRegionProvider 解析，不经此方法。返回规范默认皮肤位置仅为满足接口契约。
     */
    [[nodiscard]] ResourceLocation getEntityTexture(::mc::client::ClientEntity& entity) override;
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::client::ClientEntity& entity) const override;

private:
    void _setupLayers();

    model::player::PlayerModel m_model;
    bool m_slimArms; // 是否使用纤细手臂

    // 层渲染器 - 基于 ClientEntity，第三人称玩家图层（手持物品/头盔）由此分发
    std::vector<std::unique_ptr<layer::core::LayerRenderer<::mc::client::ClientEntity>>> m_layers;
};

} // namespace mc::client::renderer::entity::renderer::player
