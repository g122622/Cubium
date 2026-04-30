#include "BlockEntityModel.hpp"

namespace mc::client::renderer::blockentity::model {

BlockEntityModel::BlockEntityModel() = default;

std::shared_ptr<entity::model::ModelRenderer> BlockEntityModel::createPart(
    const String& name,
    i32 textureWidth,
    i32 textureHeight)
{
    auto part = std::make_shared<entity::model::ModelRenderer>(name);
    part->setTextureSize(textureWidth, textureHeight);
    m_parts.push_back(part);
    return part;
}

void BlockEntityModel::addPart(std::shared_ptr<entity::model::ModelRenderer> part) {
    if (part) {
        m_parts.push_back(part);
    }
}

void BlockEntityModel::generateMesh(
    std::vector<entity::model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    f64 scale) const
{
    for (const auto& part : m_parts) {
        if (part) {
            part->generateMesh(vertices, indices, scale);
        }
    }
}

void BlockEntityModel::setAllVisible(bool visible) {
    for (auto& part : m_parts) {
        if (part) {
            part->setVisible(visible);
        }
    }
}

} // namespace mc::client::renderer::blockentity::model
