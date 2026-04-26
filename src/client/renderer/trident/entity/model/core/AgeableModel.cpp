#include "AgeableModel.hpp"

namespace mc::client::renderer::entity::model {

AgeableModel::AgeableModel()
    : EntityModel()
    , m_isChildHeadScaled(false)
    , m_childHeadOffsetY(5.0f)
    , m_childHeadOffsetZ(2.0f)
    , m_childHeadScale(2.0f)
    , m_childBodyScale(2.0f)
    , m_childBodyOffsetY(24.0f)
{
}

AgeableModel::AgeableModel(bool isChildHeadScaled, f32 childHeadOffsetY, f32 childHeadOffsetZ)
    : EntityModel()
    , m_isChildHeadScaled(isChildHeadScaled)
    , m_childHeadOffsetY(childHeadOffsetY)
    , m_childHeadOffsetZ(childHeadOffsetZ)
    , m_childHeadScale(2.0f)
    , m_childBodyScale(2.0f)
    , m_childBodyOffsetY(24.0f)
{
}

AgeableModel::AgeableModel(bool isChildHeadScaled, f32 childHeadOffsetY, f32 childHeadOffsetZ,
                           f32 childHeadScale, f32 childBodyScale, f32 childBodyOffsetY)
    : EntityModel()
    , m_isChildHeadScaled(isChildHeadScaled)
    , m_childHeadOffsetY(childHeadOffsetY)
    , m_childHeadOffsetZ(childHeadOffsetZ)
    , m_childHeadScale(childHeadScale)
    , m_childBodyScale(childBodyScale)
    , m_childBodyOffsetY(childBodyOffsetY)
{
}

void AgeableModel::render(f64 scale) {
    // 参考 MC 1.16.5 AgeableModel.render()
    // 幼体渲染需要分别处理头部和身体
    // Java: 头部缩放 1.5F / childHeadScale，身体缩放 1.0F / childBodyScale

    if (m_isChild) {
        // 渲染头部部件
        auto headParts = getHeadParts();
        if (!headParts.empty()) {
            // 头部缩放
            f32 headScale = m_isChildHeadScaled ? (1.5f / m_childHeadScale) : 1.0f;
            f64 headOffsetY = static_cast<f64>(m_childHeadOffsetY) / 16.0;
            f64 headOffsetZ = static_cast<f64>(m_childHeadOffsetZ) / 16.0;

            // 对每个头部部件应用缩放和偏移
            for (auto& part : headParts) {
                if (part) {
                    // 保存原始状态
                    f64 origRotX = part->rotationPointX();
                    f64 origRotY = part->rotationPointY();
                    f64 origRotZ = part->rotationPointZ();

                    // 应用偏移
                    part->setRotationPoint(origRotX, origRotY + headOffsetY * 16.0, origRotZ + headOffsetZ * 16.0);

                    // 渲染
                    part->render(scale * headScale);

                    // 恢复原始状态
                    part->setRotationPoint(origRotX, origRotY, origRotZ);
                }
            }
        }

        // 渲染身体部件
        auto bodyParts = getBodyParts();
        if (!bodyParts.empty()) {
            // 身体缩放
            f32 bodyScale = 1.0f / m_childBodyScale;
            f64 bodyOffsetY = static_cast<f64>(m_childBodyOffsetY) / 16.0;

            // 对每个身体部件应用缩放和偏移
            for (auto& part : bodyParts) {
                if (part) {
                    // 保存原始状态
                    f64 origRotY = part->rotationPointY();

                    // 应用偏移
                    part->setRotationPoint(part->rotationPointX(), origRotY + bodyOffsetY * 16.0, part->rotationPointZ());

                    // 渲染
                    part->render(scale * bodyScale);

                    // 恢复原始状态
                    part->setRotationPoint(part->rotationPointX(), origRotY, part->rotationPointZ());
                }
            }
        }
    } else {
        // 成年体：正常渲染
        EntityModel::render(scale);
    }
}

void AgeableModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    EntityModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
}

void AgeableModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/) {
    // 默认实现为空，子类可以重写
    // 参考 MC 1.16.5 EntityModel.setLivingAnimations
}

} // namespace mc::client::renderer::entity::model
