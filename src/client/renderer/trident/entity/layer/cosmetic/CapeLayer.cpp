#include "CapeLayer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <cmath>

namespace mc::client::renderer::entity::layer::cosmetic {

namespace {
    constexpr f32 PI = 3.14159265f;
}

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

    // 检查是否有斗篷纹理
    // 需要从 ClientSkinManager::getCapeRegion() 获取
    // 斗篷渲染需要渲染管线支持
    if (!m_customCapeRegion) {
        return;
    }

    // 计算斗篷摆动
    f32 swing = calculateCapeSwing(entity, partialTicks);

    // 渲染斗篷网格
    // MC 1.16.5 使用 CapeModel 渲染
    // 需要渲染管线支持

    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
    (void)swing;
}

bool CapeLayer::shouldRender(const ::mc::Player& entity) const {
    // 参考 MC 1.16.5 CapeLayer
    // 检查玩家是否有斗篷：
    // 1. 玩家档案中有 capeUrl
    // 2. 存在对应的披风纹理
    // 需要从 ClientSkinManager::getCapeRegion() 获取
    (void)entity;
    return m_customCapeRegion != nullptr;
}

f32 CapeLayer::calculateCapeSwing(::mc::Player& entity, f32 partialTicks) const {
    // 参考 MC 1.16.5 CapeLayer 中的摆动计算
    // MC源码: MathHelper.sin(entity.prevChasingPosX + (entity.chasingPosX - entity.prevChasingPosX) * partialTicks)
    //         * (1.0F - MathHelper.cos(entity.prevRenderYawOffset + (entity.renderYawOffset - entity.prevRenderYawOffset) * partialTicks))

    f64 prevX = static_cast<f64>(entity.prevX());
    f64 prevZ = static_cast<f64>(entity.prevZ());
    f64 x = static_cast<f64>(entity.x());
    f64 z = static_cast<f64>(entity.z());

    // 计算移动向量
    f32 moveX = static_cast<f32>(x - prevX);
    f32 moveZ = static_cast<f32>(z - prevZ);

    // 计算摆动角度
    f32 time = static_cast<f32>(entity.ticksExisted()) + partialTicks;
    f32 swing = std::sin(time * 0.1f);
    f32 moveSq = moveX * moveX + moveZ * moveZ;

    return swing * std::sqrt(moveSq) * 0.5f;
}

} // namespace mc::client::renderer::entity::layer::cosmetic
