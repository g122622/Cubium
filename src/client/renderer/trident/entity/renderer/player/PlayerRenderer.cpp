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
#include "client/renderer/trident/entity/layer/cosmetic/CapeLayer.hpp"
#include "client/renderer/trident/entity/layer/cosmetic/ElytraLayer.hpp"
#include "client/renderer/trident/entity/layer/equipment/HeadLayer.hpp"
#include "client/renderer/trident/entity/layer/equipment/HeldItemLayer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/item/tag/ItemTags.hpp"
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
    _setupLayers();
}

void PlayerRenderer::render(Entity& entity, f64 partialTicks)
{
    auto& player = static_cast<::mc::Player&>(entity);

    // 设置模型可见性
    setModelVisibilities(player);

    // 计算动画参数
    f64 limbSwing = getLimbSwing(player, partialTicks);
    f64 limbSwingAmount = getLimbSwingAmount(player, partialTicks);
    f64 ageInTicks = getAgeInTicks(player) + partialTicks;
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
    m_model.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);
    m_model.setSwingProgress(0.0f);
    m_model.setCrouching(false);
    m_model.setSwimming(false);
    // 重置游泳动画（来自 BipedModel）
    m_model.setSwimAnimation(0.0f);

    // 仅渲染右臂和右袖
    m_model.renderRightArm(1.0 / 16.0);

    (void)player;
    (void)partialTicks;
}

void PlayerRenderer::renderLeftArm(::mc::Player& player, f64 partialTicks)
{
    // 设置模型可见性
    setModelVisibilities(player);

    // 重置动画状态
    m_model.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);
    m_model.setSwingProgress(0.0f);
    m_model.setCrouching(false);
    m_model.setSwimming(false);
    // 重置游泳动画（来自 BipedModel）
    m_model.setSwimAnimation(0.0f);

    // 仅渲染左臂和左袖
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
    // 默认显示所有部件
    m_model.setAllVisible(true);

    // 计算主手和副手的手臂姿态
    // 参考 MC 1.21.11 AvatarRenderer.getArmPose(Avatar, HumanoidArm)
    const ::mc::Hand mainHandSlot = ::mc::Hand::MainHand;
    const ::mc::Hand offHandSlot = ::mc::Hand::OffHand;
    auto mainArmPose = determineArmPose(player, mainHandSlot);
    auto offArmPose = determineArmPose(player, offHandSlot);

    // 双手姿态协调：若主手姿态为双手动作（弓、弩装填、弩持握），
    // 副手姿态降级为 Empty（副手空）或 Item（副手非空）
    if (mainArmPose == model::player::ArmPose::BowAndArrow || mainArmPose == model::player::ArmPose::CrossbowCharge ||
        mainArmPose == model::player::ArmPose::CrossbowHold) {
        const ::mc::ItemStack& offHandStack = player.getHeldItem(offHandSlot);
        offArmPose = offHandStack.isEmpty() ? model::player::ArmPose::Empty : model::player::ArmPose::Item;
    }

    // 根据玩家主手偏好映射到模型右臂/左臂
    // 右撇子：主手姿态 → 右臂，副手姿态 → 左臂
    // 左撇子：主手姿态 → 左臂，副手姿态 → 右臂
    if (player.isRightHanded()) {
        m_model.setArmPose(offArmPose, mainArmPose); // (left, right)
    } else {
        m_model.setArmPose(mainArmPose, offArmPose); // (left, right)
    }

    // 设置主手和挥动手，供 BipedModel::setAngles 双臂协调逻辑使用
    m_model.setMainHand(player.isRightHanded() ? model::HandSide::Right : model::HandSide::Left);
    if (player.isSwingInProgress()) {
        m_model.setSwingingHand(player.swingingHand() == ::mc::Hand::MainHand
                ? (player.isRightHanded() ? model::HandSide::Right : model::HandSide::Left)
                : (player.isRightHanded() ? model::HandSide::Left : model::HandSide::Right));
    }

    // 根据 PlayerModelPart 设置外层皮肤部件可见性
    // 注意：Cape 由 CapeLayer 单独处理

    // 使用 PlayerModel::setModelVisibilitiesFromFlags 设置所有外层皮肤部件
    m_model.setModelVisibilitiesFromFlags(player.playerModelParts());

    // 设置蹲伏状态
    m_model.setCrouching(player.isSneaking());

    // 设置游泳状态
    m_model.setSwimming(player.isSwimming());
}

model::player::ArmPose PlayerRenderer::determineArmPose(::mc::Player& player, ::mc::Hand hand)
{
    // 参考 MC 1.21.11 AvatarRenderer.getArmPose(Avatar, ItemStack, InteractionHand)

    // 获取手持物品
    const ::mc::ItemStack& heldStack = player.getHeldItem(hand);
    if (heldStack.isEmpty()) {
        return model::player::ArmPose::Empty;
    }

    const ::mc::Item* item = heldStack.getItem();
    if (item == nullptr) {
        return model::player::ArmPose::Item;
    }

    // 1) 已装填的弩：未挥动时返回 CrossbowHold
    //    匹配 MC: !swinging && stack.is(Items.CROSSBOW) && CrossbowItem.isCharged(stack)
    if (!player.isSwingInProgress() && item == ::mc::Items::CROSSBOW) {
        if (::mc::item::CrossbowItem::isCharged(heldStack)) {
            return model::player::ArmPose::CrossbowHold;
        }
    }

    // 2) 正在使用物品且使用的手就是当前判断的手
    //    匹配 MC: entity.getUsedItemHand() == hand && entity.getUseItemRemainingTicks() > 0
    if (player.isUsingItem() && player.getActiveHand() == hand && player.getItemInUseCount() > 0) {
        const ::mc::UseAction useAction = item->getUseAction(heldStack);
        switch (useAction) {
            case ::mc::UseAction::Block:
                return model::player::ArmPose::Block;
            case ::mc::UseAction::Bow:
                return model::player::ArmPose::BowAndArrow;
            case ::mc::UseAction::Spear:
                // Trident 是 Spear 的别名（同枚举值），三叉戟与长矛共用 ThrowSpear 姿态
                return model::player::ArmPose::ThrowSpear;
            case ::mc::UseAction::Crossbow:
                return model::player::ArmPose::CrossbowCharge;
            case ::mc::UseAction::Spyglass:
                // TODO: 望远镜暂无第三人称特殊姿态枚举，临时降级为 Item
                return model::player::ArmPose::Item;
            case ::mc::UseAction::Brush:
                // TODO: 刷子暂无第三人称特殊姿态枚举，临时降级为 Item
                return model::player::ArmPose::Item;
            default:
                break;
        }
    }

    // 3) 长矛类物品（通过 ItemTags::SPEARS 标签判断）返回 ThrowSpear
    //    匹配 MC: stack.is(ItemTags.SPEARS) ? SPEAR : ITEM
    if (::mc::item::tag::ItemTags::isInitialized()) {
        if (item->isIn(::mc::item::tag::ItemTags::SPEARS())) {
            return model::player::ArmPose::ThrowSpear;
        }
    }

    // 4) 默认持有物品姿态
    return model::player::ArmPose::Item;
}

f64 PlayerRenderer::getLimbSwing(::mc::Player& player, f64 partialTicks) const
{
    f64 limbSwingAmount = static_cast<f64>(player.limbSwingAmount());
    f64 result = static_cast<f64>(player.limbSwing()) - limbSwingAmount * (1.0 - partialTicks);
    return result;
}

f64 PlayerRenderer::getLimbSwingAmount(::mc::Player& player, f64 partialTicks) const
{
    f64 prevAmount = static_cast<f64>(player.prevLimbSwingAmount());
    f64 amount = static_cast<f64>(player.limbSwingAmount());
    f64 result = prevAmount + (amount - prevAmount) * partialTicks;

    // 限制最大值为 1.0
    if (result > 1.0) {
        result = 1.0;
    }

    return result;
}

f64 PlayerRenderer::getHeadYaw(::mc::Player& player, f64 partialTicks) const
{
    // 头部偏航角（相对于身体）
    f64 bodyYaw = static_cast<f64>(player.prevRenderYawOffset()) +
        (static_cast<f64>(player.renderYawOffset()) - static_cast<f64>(player.prevRenderYawOffset())) * partialTicks;
    f64 headYaw = static_cast<f64>(player.prevRotationYawHead()) +
        (static_cast<f64>(player.rotationYawHead()) - static_cast<f64>(player.prevRotationYawHead())) * partialTicks;
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
    // 年龄（用于空闲动画），加上 partialTicks 用于帧间插值
    return static_cast<f64>(player.ticksExisted());
}

ResourceLocation PlayerRenderer::getEntityTexture(::mc::Player& entity)
{
    // 返回玩家皮肤纹理位置
    // 如果设置了自定义纹理，使用纹理区域的位置
    // 否则返回规范默认皮肤 (slim/steve)
    (void)entity;
    if (m_skinRegion) {
        // 使用皮肤纹理区域的位置
        return ResourceLocation("minecraft:textures/entity/player/custom_skin.png");
    }
    // 返回规范默认皮肤 (MC 1.21.1: slim/steve，索引 6)
    return ResourceLocation("minecraft:textures/entity/player/slim/steve.png");
}

ResourceLocation PlayerRenderer::getEntityTexture(const ::mc::Player& entity) const
{
    (void)entity;
    if (m_skinRegion) {
        return ResourceLocation("minecraft:textures/entity/player/custom_skin.png");
    }
    return ResourceLocation("minecraft:textures/entity/player/slim/steve.png");
}

void PlayerRenderer::_setupLayers()
{
    // 添加层渲染器

    // 创建手持物品层渲染器（主手和副手）
    m_layers.push_back(std::make_unique<equipment::HeldItemLayer<::mc::Player>>());

    // 头部物品层（头盔等）
    m_layers.push_back(std::make_unique<equipment::HeadLayer<::mc::Player, model::player::PlayerModel>>(*this));

    // 披风层
    m_layers.push_back(std::make_unique<cosmetic::CapeLayer>());

    // 鞘翅层
    m_layers.push_back(std::make_unique<cosmetic::ElytraLayer<::mc::Player>>());

    spdlog::info("PlayerRenderer: Layer setup complete ({} layers registered)", m_layers.size());
}

} // namespace mc::client::renderer::entity::renderer::player
