#include "BlazeModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
    // 烟雾棒的旋转偏移（每根棒间隔30度）
    constexpr f64 ROD_ANGLE_OFFSET = PI / 6.0;
    // 棒的浮动偏移角度增量
    constexpr f64 ROD_FLOAT_SPEED = 0.5;
}

BlazeModel::BlazeModel() {
    setTextureSize(64, 64);
    setupParts();
}

void BlazeModel::setupParts() {
    // 参考 MC 1.16.5 BlazeModel
    // 头部（主体）
    m_head = std::make_shared<model::ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0.0);
    m_head->setRotationPoint(0.0f, 16.0f, 0.0f);
    m_parts.push_back(m_head);

    // 烟雾棒（12根）
    // 参考 MC 1.16.5：棒围绕头部排列，随机浮动
    for (i32 i = 0; i < SMOKE_ROD_COUNT; ++i) {
        auto& rod = m_smokeRods[i];
        rod = std::make_shared<model::ModelRenderer>("smokeRod" + std::to_string(i));

        // 棒尺寸：2x6x2
        rod->setTextureOffset(0, 16);
        rod->addBox(-1.0f, -3.0f, -1.0f, 2.0f, 6.0f, 2.0f, 0.0);

        // 初始位置在头部周围
        // MC 1.16.5: 棒的位置根据索引不同而变化
        f64 angle = i * ROD_ANGLE_OFFSET;
        f64 radius = 6.0; // 距离中心的半径

        f64 x = std::sin(angle) * radius;
        f64 z = std::cos(angle) * radius;

        // Y位置根据索引分层
        f64 y = 8.0 + (i % 3) * 3.0;

        rod->setRotationPoint(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        m_parts.push_back(rod);
    }
}

void BlazeModel::render(f64 scale) {
    // 渲染头部
    if (m_head) {
        m_head->render(scale);
    }

    // 渲染烟雾棒
    for (auto& rod : m_smokeRods) {
        if (rod) {
            rod->render(scale);
        }
    }
}

void BlazeModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                           f64 ageInTicks, f64 netHeadYaw,
                           f64 headPitch, f64 /*scale*/) {
    m_ageInTicks = ageInTicks;

    // 头部旋转
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // 烟雾棒动画：每根棒有不同的浮动
    for (i32 i = 0; i < SMOKE_ROD_COUNT; ++i) {
        auto& rod = m_smokeRods[i];
        if (!rod) continue;

        // 每根棒有相位偏移
        f64 phase = i * ROD_ANGLE_OFFSET;
        f64 floatOffset = std::sin(ageInTicks * ROD_FLOAT_SPEED + phase) * 2.0;

        // 更新Y位置
        f64 baseY = 8.0 + (i % 3) * 3.0;
        rod->setRotationPointY(static_cast<f32>(baseY + floatOffset));

        // 棒自转
        rod->setRotateAngleY(static_cast<f32>(std::sin(ageInTicks * 0.1 + phase) * 0.5));
    }
}

} // namespace mc::client::renderer::entity::model::monster
