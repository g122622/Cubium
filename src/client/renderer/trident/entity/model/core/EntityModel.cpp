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

#include "EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cstddef>
#include <vector>

namespace mc::client::renderer::entity::model {

// ==================== EntityModel ====================

void EntityModel::render(f64 scale)
{
    for (auto& part : m_parts) {
        if (part) {
            part->render(scale);
        }
    }
}

void EntityModel::setAngles(f64 /*limbSwing*/,
    f64 /*limbSwingAmount*/,
    f64 /*ageInTicks*/,
    f64 /*netHeadYaw*/,
    f64 /*headPitch*/,
    f64 /*scale*/)
{
    // 基类不实现动画
}

void EntityModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 基类不实现动画状态设置
    // 子类可以重写此方法来设置模型状态
}

void EntityModel::copyAnglesTo(EntityModel& target) const
{
    MC_ASSERT_RELEASE(m_parts.size() == target.m_parts.size());

    for (std::size_t index = 0; index < m_parts.size(); ++index) {
        target.m_parts[index]->copyModelAngles(*m_parts[index]);
    }
}

void EntityModel::copyAnglesFrom(const EntityModel& source)
{
    MC_ASSERT_RELEASE(source.m_parts.size() == m_parts.size());

    for (std::size_t index = 0; index < source.m_parts.size(); ++index) {
        m_parts[index]->copyModelAngles(*source.m_parts[index]);
    }
}

void EntityModel::generateMesh(std::vector<ModelVertex>& vertices, std::vector<u32>& indices, f64 scale) const
{
    for (const auto& part : m_parts) {
        if (part) {
            part->generateMesh(vertices, indices, scale);
        }
    }
}

} // namespace mc::client::renderer::entity::model
