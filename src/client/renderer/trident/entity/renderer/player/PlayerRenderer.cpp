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

#include "PlayerRenderer.hpp"
#include "../../layer/cosmetic/CapeLayer.hpp"
#include "../../layer/cosmetic/ElytraLayer.hpp"
#include "../../layer/equipment/HeadLayer.hpp"
#include "../../layer/equipment/HeldItemLayer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <spdlog/spdlog.h>

// 使用命名空间简化代码
using namespace mc::client::renderer::entity::layer;

namespace mc::client::renderer::entity::renderer::player {

PlayerRenderer::PlayerRenderer(bool slimArms)
    : m_model(0.0f, slimArms)
    , m_slimArms(slimArms)
{
    // 设置阴影
    setShadowSize(0.5);
    setShadowAlpha(0.8);

    // 设置层渲染器
    setupLayers();
}

void PlayerRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& player = static_cast<::mc::Player&>(entity);

    // 设置模型可见性
    setModelVisibilities(player);

    // 计算动画参数
    f64 limbSwing = getLimbSwing(player, partialTicks);
    f64 limbSwingAmount = getLimbSwingAmount(player, partialTicks);
    f64 ageInTicks = getAgeInTicks(player);
    f64 headYaw = getHeadYaw(player, partialTicks);
    f64 headPitch = getHeadPitch(player, partialTicks);
    f64 scale = 1.0 / 16.0;

    // 设置模型动画参数
    m_model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);

    // 渲染模型
    m_model.render(scale);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

void PlayerRenderer::renderLayersPipeline(
    Entity& entity, VkCommandBuffer cmd, const core::AnimationContext& context, pipeline::EntityPipeline& pipeline)
{
    auto& player = static_cast<::mc::Player&>(entity);

    // 在渲染层之前，将纹理传递给需要的层渲染器
    // 由于层渲染器存储在基类指针向量中，我们需要使用 dynamic_cast 来设置纹理
    for (auto& layer : m_layers) {
        if (layer && layer->shouldRender(player)) {
            // 尝试设置纹理（如果层支持）
            // CapeLayer
            if (m_capeRegion) {
                auto* capeLayer = dynamic_cast<cosmetic::CapeLayer*>(layer.get());
                if (capeLayer) {
                    capeLayer->setCapeTexture(m_capeRegion);
                }
            }
            // ElytraLayer
            if (m_elytraRegion || m_capeRegion) {
                auto* elytraLayer = dynamic_cast<cosmetic::ElytraLayer<::mc::Player>*>(layer.get());
                if (elytraLayer) {
                    if (m_elytraRegion) {
                        elytraLayer->setElytraTexture(m_elytraRegion);
                    }
                    if (m_capeRegion) {
                        elytraLayer->setCapeTexture(m_capeRegion);
                    }
                }
            }

            layer->renderPipeline(player, cmd, context, pipeline);
        }
    }
}

void PlayerRenderer::renderRightArm(::mc::Player& player, f64 partialTicks)
{
    // 设置模型可见性
    setModelVisibilities(player);

    // 重置动画状态
    // 参考 MC 1.16.5 PlayerRenderer.renderItem:
    // playermodel.swingProgress = 0.0F;
    // playermodel.isSneak = false;
    // playermodel.swimAnimation = 0.0F;
    // playermodel.setRotationAngles(player, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F);
    m_model.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);
    m_model.setSwingProgress(0.0f);
    m_model.setCrouching(false);
    m_model.setSwimming(false);
    // 重置游泳动画（来自 BipedModel）
    m_model.setSwimAnimation(0.0f);

    // 仅渲染右臂和右袖
    // 参考 MC 1.16.5 PlayerRenderer.renderRightArm
    m_model.renderRightArm(1.0 / 16.0);

    (void)player;
    (void)partialTicks;
}

void PlayerRenderer::renderLeftArm(::mc::Player& player, f64 partialTicks)
{
    // 设置模型可见性
    setModelVisibilities(player);

    // 重置动画状态
    // 参考 MC 1.16.5 PlayerRenderer.renderItem:
    // playermodel.swingProgress = 0.0F;
    // playermodel.isSneak = false;
    // playermodel.swimAnimation = 0.0F;
    // playermodel.setRotationAngles(player, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F);
    m_model.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);
    m_model.setSwingProgress(0.0f);
    m_model.setCrouching(false);
    m_model.setSwimming(false);
    // 重置游泳动画（来自 BipedModel）
    m_model.setSwimAnimation(0.0f);

    // 仅渲染左臂和左袖
    // 参考 MC 1.16.5 PlayerRenderer.renderLeftArm
    m_model.renderLeftArm(1.0 / 16.0);

    (void)player;
    (void)partialTicks;
}

void PlayerRenderer::computeAnimationContext(::mc::Player& player, f64 partialTicks, core::AnimationContext& context)
{
    context.partialTicks = partialTicks;
    context.limbSwing = getLimbSwing(player, partialTicks);
    context.limbSwingAmount = getLimbSwingAmount(player, partialTicks);
    context.ageInTicks = getAgeInTicks(player);
    context.netHeadYaw = getHeadYaw(player, partialTicks);
    context.headPitch = getHeadPitch(player, partialTicks);
    context.scale = 1.0 / 16.0;
    context.computeHash();
}

void PlayerRenderer::setModelVisibilities(::mc::Player& player)
{
    // 参考 MC 1.16.5 PlayerRenderer.setModelVisibilities
    // 默认显示所有部件
    m_model.setAllVisible(true);

    // 设置手臂姿态
    auto rightArmPose = determineArmPose(player, true);
    auto leftArmPose = determineArmPose(player, false);
    m_model.setArmPose(leftArmPose, rightArmPose);

    // 根据 PlayerModelPart 设置外层皮肤部件可见性
    // 参考 MC 1.16.5 PlayerRenderer.setModelVisibilities:
    // playermodel.bipedHeadwear.showModel = clientPlayer.isWearing(PlayerModelPart.HAT);
    // playermodel.bipedBodyWear.showModel = clientPlayer.isWearing(PlayerModelPart.JACKET);
    // playermodel.bipedLeftLegwear.showModel = clientPlayer.isWearing(PlayerModelPart.LEFT_PANTS_LEG);
    // playermodel.bipedRightLegwear.showModel = clientPlayer.isWearing(PlayerModelPart.RIGHT_PANTS_LEG);
    // playermodel.bipedLeftArmwear.showModel = clientPlayer.isWearing(PlayerModelPart.LEFT_SLEEVE);
    // playermodel.bipedRightArmwear.showModel = clientPlayer.isWearing(PlayerModelPart.RIGHT_SLEEVE);
    // 注意：Cape 由 CapeLayer 单独处理

    // 使用 PlayerModel::setModelVisibilitiesFromFlags 设置所有外层皮肤部件
    m_model.setModelVisibilitiesFromFlags(player.playerModelParts());

    // 设置蹲伏状态
    m_model.setCrouching(player.isSneaking());

    // 设置游泳状态
    m_model.setSwimming(player.isSwimming());
}

model::player::ArmPose PlayerRenderer::determineArmPose(::mc::Player& player, bool mainHand)
{
    // 参考 MC 1.16.5 PlayerRenderer.func_241741_a_
    // 从玩家获取手持物品和使用状态
    // 目前返回默认值，等待物品系统完善后实现
    (void)player;
    (void)mainHand;

    // 默认返回空手
    return model::player::ArmPose::Empty;
}

f64 PlayerRenderer::getLimbSwing(::mc::Player& player, f64 partialTicks) const
{
    // MC 1.16.5 LivingRenderer.java:100
    // f5 = entity.limbSwing - entity.limbSwingAmount * (1.0F - partialTicks);
    // 注意：Player 继承自 Entity，没有 limbSwing 字段
    // 需要从移动距离计算
    f64 dx = static_cast<f64>(player.x() - player.prevX());
    f64 dz = static_cast<f64>(player.z() - player.prevZ());
    f64 distance = std::sqrt(dx * dx + dz * dz);

    // 计算动画周期
    f64 limbSwing = static_cast<f64>(player.ticksExisted()) * distance;
    f64 limbSwingAmount = std::min(distance * 4.0, 1.0);

    // MC 1.16.5 公式
    return limbSwing - limbSwingAmount * (1.0 - partialTicks);
}

f64 PlayerRenderer::getLimbSwingAmount(::mc::Player& player, f64 partialTicks) const
{
    // MC 1.16.5 LivingRenderer.java:99
    // f8 = MathHelper.lerp(partialTicks, prevLimbSwingAmount, limbSwingAmount);
    // 限制最大值为 1.0
    f64 dx = static_cast<f64>(player.x() - player.prevX());
    f64 dz = static_cast<f64>(player.z() - player.prevZ());
    f64 speed = std::sqrt(dx * dx + dz * dz) * 4.0;

    // 插值计算
    f64 prevAmount = speed * 0.7; // 近似前一帧的值
    f64 amount = speed;
    f64 result = prevAmount + (amount - prevAmount) * partialTicks;

    // MC 1.16.5 LivingRenderer.java:105-107
    if (result > 1.0) {
        result = 1.0;
    }

    return result;
}

f64 PlayerRenderer::getHeadYaw(::mc::Player& player, f64 partialTicks) const
{
    // 头部偏航角（相对于身体）
    f64 bodyYaw = player.prevYaw() + (player.yaw() - player.prevYaw()) * partialTicks;
    f64 headYaw = player.prevYaw() + (player.yaw() - player.prevYaw()) * partialTicks;
    f64 diff = headYaw - bodyYaw;

    // 归一化到 -180 到 180
    while (diff < -180.0)
        diff += 360.0;
    while (diff > 180.0)
        diff -= 360.0;

    return diff;
}

f64 PlayerRenderer::getHeadPitch(::mc::Player& player, f64 partialTicks) const
{
    // 头部俯仰角
    f64 prevPitch = player.prevPitch();
    f64 pitch = player.pitch();
    return prevPitch + (pitch - prevPitch) * partialTicks;
}

f64 PlayerRenderer::getAgeInTicks(::mc::Player& player) const
{
    // 年龄（用于空闲动画）
    return static_cast<f64>(player.ticksExisted());
}

ResourceLocation PlayerRenderer::getEntityTexture(::mc::Player& entity)
{
    // 返回玩家皮肤纹理位置
    // 如果设置了自定义纹理，使用纹理区域的位置
    // 否则返回默认 Steve 皮肤
    (void)entity;
    if (m_skinRegion) {
        // 使用皮肤纹理区域的位置
        return ResourceLocation("minecraft:textures/entity/player/custom_skin.png");
    }
    // 返回默认 Steve 皮肤
    return ResourceLocation("minecraft:textures/entity/steve.png");
}

ResourceLocation PlayerRenderer::getEntityTexture(const ::mc::Player& entity) const
{
    (void)entity;
    if (m_skinRegion) {
        return ResourceLocation("minecraft:textures/entity/player/custom_skin.png");
    }
    return ResourceLocation("minecraft:textures/entity/steve.png");
}

void PlayerRenderer::setupLayers()
{
    // 参考 MC 1.16.5 PlayerRenderer 构造函数
    // 添加层渲染器

    // 创建手持物品层渲染器（主手和副手）
    m_layers.push_back(std::make_unique<equipment::HeldItemLayer<::mc::Player>>());

    // 头部物品层（头盔等）
    // 参考 MC 1.16.5: this.addLayer(new HeadLayer<>(this));
    // HeadLayer 需要匹配 IEntityRenderer<Player, PlayerModel> 接口
    m_layers.push_back(std::make_unique<equipment::HeadLayer<::mc::Player, model::player::PlayerModel>>(*this));

    // 披风层
    // 参考 MC 1.16.5: this.addLayer(new CapeLayer(this));
    m_layers.push_back(std::make_unique<cosmetic::CapeLayer>());

    // 鞘翅层
    // 参考 MC 1.16.5: this.addLayer(new ElytraLayer<>(this));
    m_layers.push_back(std::make_unique<cosmetic::ElytraLayer<::mc::Player>>());

    spdlog::debug("PlayerRenderer: Layer setup complete ({} layers registered)", m_layers.size());
}

} // namespace mc::client::renderer::entity::renderer::player
