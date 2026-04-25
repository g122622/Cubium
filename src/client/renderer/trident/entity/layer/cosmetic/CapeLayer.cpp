#include "CapeLayer.hpp"

namespace mc::client::renderer::entity::layer::cosmetic {

void CapeLayer::render(
    ::mc::Player& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // 参考 MC 1.16.5 CapeLayer.render()
    // 1. 获取玩家斗篷纹理
    // 2. 计算斗篷摆动动画
    // 3. 渲染斗篷网格

    // 检查是否有自定义斗篷纹理
    if (!m_customCapeRegion) {
        // TODO: 从 ClientSkinManager 获取玩家的斗篷纹理
        // 目前没有斗篷，不渲染
        return;
    }

    // TODO: 渲染斗篷网格
    // 需要使用 m_customCapeRegion 作为纹理
    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

bool CapeLayer::shouldRender(const ::mc::Player& entity) const {
    // 检查玩家是否有斗篷
    // TODO: 从 PlayerSkinInfo 获取斗篷状态
    (void)entity;
    return m_customCapeRegion != nullptr;
}

f32 CapeLayer::calculateCapeSwing(::mc::Player& entity, f32 partialTicks) const {
    // 参考 MC 1.16.5 CapeLayer 中的摆动计算
    // 使用玩家前一帧位置和当前位置计算摆动角度
    (void)entity;
    (void)partialTicks;
    return 0.0f;
}

} // namespace mc::client::renderer::entity::layer::cosmetic
