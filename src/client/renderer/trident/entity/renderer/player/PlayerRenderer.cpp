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
#include "../../layer/equipment/HeldItemLayer.hpp"
#include "common/entity/entities/player/Player.hpp"
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

    for (auto& layer : m_layers) {
        if (layer && layer->shouldRender(player)) {
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

    // 根据玩家设置显示/隐藏外层皮肤
    // PlayerModelPart 枚举值：
    // - Cape (0x01) - 斗篷
    // - Jacket (0x02) - 外套
    // - LeftSleeve (0x04) - 左袖
    // - RightSleeve (0x08) - 右袖
    // - LeftPants (0x10) - 左裤腿
    // - RightPants (0x20) - 右裤腿
    // - Hat (0x40) - 帽子
    // TODO: 当 PlayerModel 支持按部件名称设置可见性时实现
    // 目前只能通过 setAllVisible 设置整体可见性

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

void PlayerRenderer::setupLayers()
{
    // 参考 MC 1.16.5 PlayerRenderer 构造函数
    // 添加层渲染器

    // 创建手持物品层渲染器（主手和副手）
    m_layers.push_back(std::make_unique<equipment::HeldItemLayer<::mc::Player>>());

    // 头部物品层（头盔等）- TODO 暂时注释，等待完整实现
    // m_layers.push_back(std::make_unique<equipment::HeadLayer<::mc::Player>>());

    // 披风层 - TODO 暂时注释，等待完整实现
    // m_layers.push_back(std::make_unique<cosmetic::CapeLayer<::mc::Player>>());

    // 鞘翅层 - TODO 暂时注释，等待完整实现
    // m_layers.push_back(std::make_unique<cosmetic::ElytraLayer<::mc::Player>>());

    spdlog::debug("PlayerRenderer: Layer setup complete ({} layers registered)", m_layers.size());
}

void registerPlayerRenderers(EntityRendererManager& manager)
{
    // 注册标准手臂玩家渲染器
    manager.registerRenderer("minecraft:player",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PlayerRenderer>(false); });

    // 注册纤细手臂玩家渲染器（通过不同的实体类型 ID 或运行时切换）
    manager.registerRenderer("minecraft:player_slim",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PlayerRenderer>(true); });
}

} // namespace mc::client::renderer::entity::renderer::player
