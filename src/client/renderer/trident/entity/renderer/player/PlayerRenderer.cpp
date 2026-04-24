#include "PlayerRenderer.hpp"
#include "../../layer/equipment/ArmorLayer.hpp"
#include "../../layer/equipment/HeldItemLayer.hpp"
#include "../../layer/equipment/HeadLayer.hpp"
#include "../../layer/cosmetic/CapeLayer.hpp"
#include "../../layer/cosmetic/ElytraLayer.hpp"
#include "../../layer/entity/ArrowLayer.hpp"
#include "common/entity/entities/player/Player.hpp"

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

void PlayerRenderer::render(Entity& entity, f64 partialTicks) {
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

void PlayerRenderer::renderRightArm(::mc::Player& player, f64 partialTicks) {
    // 设置模型可见性
    setModelVisibilities(player);

    // 重置动画状态
    m_model.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);

    // 仅渲染右臂和右袖外层
    // 参考 MC 1.16.5 PlayerRenderer.renderRightArm
    // TODO: 需要在 PlayerModel 中添加单独渲染手臂的方法
    (void)player;
    (void)partialTicks;
}

void PlayerRenderer::renderLeftArm(::mc::Player& player, f64 partialTicks) {
    // 设置模型可见性
    setModelVisibilities(player);

    // 重置动画状态
    m_model.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);

    // 仅渲染左臂和左袖外层
    // 参考 MC 1.16.5 PlayerRenderer.renderLeftArm
    // TODO: 需要在 PlayerModel 中添加单独渲染手臂的方法
    (void)player;
    (void)partialTicks;
}

void PlayerRenderer::setModelVisibilities(::mc::Player& player) {
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

model::player::ArmPose PlayerRenderer::determineArmPose(::mc::Player& player, bool mainHand) {
    // 参考 MC 1.16.5 PlayerRenderer.func_241741_a_
    // 从玩家获取手持物品和使用状态
    // 目前返回默认值，等待物品系统完善后实现
    (void)player;
    (void)mainHand;

    // 默认返回空手
    return model::player::ArmPose::Empty;

    // 实际实现需要：
    // 1. 获取手持物品
    // 2. 检查物品使用状态
    // 3. 根据 UseAction 确定姿态
    // - UseAction::Block -> ArmPose::Block
    // - UseAction::Bow -> ArmPose::BowAndArrow
    // - UseAction::Spear -> ArmPose::ThrowSpear
    // - UseAction::Crossbow (正在使用) -> ArmPose::CrossbowCharge
    // - UseAction::Crossbow (已装填) -> ArmPose::CrossbowHold
    // - 其他非空物品 -> ArmPose::Item
}

f64 PlayerRenderer::getLimbSwing(::mc::Player& player, f64 partialTicks) const {
    // 步态动画周期
    // Player 继承自 Entity，有类似的动画属性
    // 目前返回默认值，等待动画系统集成
    (void)player;
    (void)partialTicks;
    return 0.0;
}

f64 PlayerRenderer::getLimbSwingAmount(::mc::Player& player, f64 partialTicks) const {
    // 步态动画强度
    (void)player;
    (void)partialTicks;
    return 0.0;
}

f64 PlayerRenderer::getHeadYaw(::mc::Player& player, f64 partialTicks) const {
    // 头部偏航角（相对于身体）
    f64 bodyYaw = player.prevYaw() + (player.yaw() - player.prevYaw()) * partialTicks;
    f64 headYaw = player.prevYaw() + (player.yaw() - player.prevYaw()) * partialTicks;
    f64 diff = headYaw - bodyYaw;

    // 归一化到 -180 到 180
    while (diff < -180.0) diff += 360.0;
    while (diff > 180.0) diff -= 360.0;

    return diff;
}

f64 PlayerRenderer::getHeadPitch(::mc::Player& player, f64 partialTicks) const {
    // 头部俯仰角
    f64 prevPitch = player.prevPitch();
    f64 pitch = player.pitch();
    return prevPitch + (pitch - prevPitch) * partialTicks;
}

f64 PlayerRenderer::getAgeInTicks(::mc::Player& player) const {
    // 年龄（用于空闲动画）
    return static_cast<f64>(player.ticksExisted());
}

void PlayerRenderer::setupLayers() {
    // 参考 MC 1.16.5 PlayerRenderer 构造函数
    // 添加层渲染器

    // 盔甲层 - 需要模板参数
    // addLayer(equipment::ArmorLayer<::mc::Player, model::player::PlayerModel>(*this));

    // 手持物品层
    // addLayer(equipment::HeldItemLayer<::mc::Player>());

    // 箭矢附着层
    // addLayer(entity::ArrowLayer<::mc::Player>());

    // 斗篷层
    // addLayer(cosmetic::CapeLayer());

    // 头部物品层
    // addLayer(equipment::HeadLayer<::mc::Player>());

    // 鞘翅层
    // addLayer(cosmetic::ElytraLayer<::mc::Player>());

    // 注意：层渲染器需要 LivingRenderer 提供 getModel() 方法
    // PlayerRenderer 目前继承自 EntityRenderer，不继承 LivingRenderer
    // 需要调整架构或将层渲染器的添加延迟到渲染系统完善后
}

void registerPlayerRenderers(EntityRendererManager& manager) {
    // 注册标准手臂玩家渲染器
    manager.registerRenderer("minecraft:player", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<PlayerRenderer>(false);
    });

    // 注册纤细手臂玩家渲染器（通过不同的实体类型 ID 或运行时切换）
    manager.registerRenderer("minecraft:player_slim", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<PlayerRenderer>(true);
    });
}

} // namespace mc::client::renderer::entity::renderer::player
