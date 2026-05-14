#include "SegmentedModel.hpp"

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
