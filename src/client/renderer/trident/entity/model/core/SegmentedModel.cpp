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

#include "SegmentedModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "common/core/Types.hpp"
#include <string>
#include <utility>

namespace mc::client::renderer::entity::model {

void SegmentedModel::render(f64 scale)
{
    // 分段模型默认渲染所有部件
    EntityModel::render(scale);
}

void SegmentedModel::setSegmentRenderer(const std::string& partName, SegmentFunc func)
{
    m_segmentRenderers[partName] = std::move(func);
}

void SegmentedModel::renderSegment(const std::string& partName, f64 scale)
{
    auto it = m_segmentRenderers.find(partName);
    if (it != m_segmentRenderers.end()) {
        // 查找对应的 ModelRenderer
        for (auto& part : m_parts) {
            if (part && part->name() == partName) {
                it->second(*part, scale);
                break;
            }
        }
    }
}

bool SegmentedModel::hasSegment(const std::string& partName) const
{
    return m_segmentRenderers.find(partName) != m_segmentRenderers.end();
}

} // namespace mc::client::renderer::entity::model
