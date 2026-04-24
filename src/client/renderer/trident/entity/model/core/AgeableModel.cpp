#include "AgeableModel.hpp"

namespace mc::client::renderer::entity::model {

AgeableModel::AgeableModel()
    : EntityModel()
{
}

void AgeableModel::render(f64 scale) {
    // 如果是幼体，应用缩放
    f64 renderScale = scale;
    if (m_isChild) {
        renderScale = getChildScale(scale);
    }

    EntityModel::render(renderScale);
}

void AgeableModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    // 如果是幼体，可以在这里调整动画参数
    // 例如：幼体的动作可能更快或更夸张
    EntityModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
}

f64 AgeableModel::getChildScale(f64 baseScale) const {
    if (m_isChild) {
        // 幼体整体缩放
        return baseScale * m_childBodyScale;
    }
    return baseScale;
}

void AgeableModel::setupChildModel() {
    // 子类可以重写此方法来调整幼体模型参数
    // 默认实现为空，由子类决定具体的调整方式
}

} // namespace mc::client::renderer::entity::model
