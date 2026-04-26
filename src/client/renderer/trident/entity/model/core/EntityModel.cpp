#include "EntityModel.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cassert>

namespace mc::client::renderer::entity::model {

// ==================== EntityModel ====================

void EntityModel::render(f64 scale) {
    for (auto& part : m_parts) {
        if (part) {
            part->render(scale);
        }
    }
}

void EntityModel::setAngles(f64 /*limbSwing*/, f64 /*limbSwingAmount*/,
                            f64 /*ageInTicks*/, f64 /*netHeadYaw*/,
                            f64 /*headPitch*/, f64 /*scale*/) {
    // 基类不实现动画
}

void EntityModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/) {
    // 基类不实现动画状态设置
    // 子类可以重写此方法来设置模型状态
}

void EntityModel::copyAnglesTo(EntityModel& target) const {
    assert(m_parts.size() == target.m_parts.size());

    for (std::size_t index = 0; index < m_parts.size(); ++index) {
        m_parts[index]->copyModelAngles(*target.m_parts[index]);
    }
}

void EntityModel::copyAnglesFrom(const EntityModel& source) {
    assert(source.m_parts.size() == m_parts.size());

    for (std::size_t index = 0; index < source.m_parts.size(); ++index) {
        source.m_parts[index]->copyModelAngles(*m_parts[index]);
    }
}

void EntityModel::generateMesh(std::vector<ModelVertex>& vertices,
                                std::vector<u32>& indices,
                                f64 scale) const {
    for (const auto& part : m_parts) {
        if (part) {
            part->generateMesh(vertices, indices, scale);
        }
    }
}

} // namespace mc::client::renderer::entity::model
