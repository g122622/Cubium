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

#include "ItemMeshBuilder.hpp"
#include "client/renderer/api/texture/TextureRegion.hpp"
#include "client/resource/ItemModelCache.hpp"
#include "client/resource/ItemModelLoader.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace mc::client::renderer::entity::item {

// 引入 ModelElement 和 Direction 类型
using ::mc::Direction;
using ::mc::ModelElement;
using ::mc::ModelFace;

// 静态成员初始化
const ::mc::client::ItemTextureAtlas* ItemMeshBuilder::s_itemTextureAtlas = nullptr;

void ItemMeshBuilder::setItemTextureAtlas(const ::mc::client::ItemTextureAtlas* atlas)
{
    s_itemTextureAtlas = atlas;
}

namespace {

// 将 ItemTransformType 转换为 ItemDisplayContext
resource::ItemDisplayContext toDisplayContext(ItemTransformType type)
{
    using namespace resource;
    switch (type) {
        case ItemTransformType::ThirdPersonRightHand:
            return ItemDisplayContext::ThirdPersonRightHand;
        case ItemTransformType::ThirdPersonLeftHand:
            return ItemDisplayContext::ThirdPersonLeftHand;
        case ItemTransformType::FirstPersonRightHand:
            return ItemDisplayContext::FirstPersonRightHand;
        case ItemTransformType::FirstPersonLeftHand:
            return ItemDisplayContext::FirstPersonLeftHand;
        case ItemTransformType::Head:
            return ItemDisplayContext::Head;
        case ItemTransformType::Gui:
            return ItemDisplayContext::Gui;
        case ItemTransformType::Ground:
            return ItemDisplayContext::Ground;
        case ItemTransformType::Fixed:
            return ItemDisplayContext::Fixed;
        default:
            return ItemDisplayContext::Gui;
    }
}

} // namespace

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildHeldItemMesh(
    const ::mc::ItemStack& itemStack, ItemTransformType transformType, bool bakeTransforms)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    if (itemStack.isEmpty()) {
        return {vertices, indices};
    }

    const ::mc::Item* item = itemStack.getItem();
    if (!item) {
        return {vertices, indices};
    }

    // 构建 3D 物品网格。bakeTransforms=false 时，_build3DItemMesh 不烘焙 display 变换，
    // 由调用方在矩阵栈上单独施加（第一人称路径由 ItemInHandRenderer::applyTransform 拥有）。
    _build3DItemMesh(*item, transformType, vertices, indices, bakeTransforms);

    // getItemTransform 摄像机矩阵仅在 bakeTransforms=true 时烘焙进顶点。
    // bakeTransforms=false 时跳过，避免与调用方栈上的 display 变换双重施加。
    if (bakeTransforms && !vertices.empty()) {
        std::array<f64, 16> transform = getItemTransform(transformType, 0.0f, 0.0f, true);
        _transformVertices(vertices, transform);
    }

    return {vertices, indices};
}

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildArmorMesh(
    const ::mc::ItemStack& itemStack, u32 slot, const std::array<f64, 16>& bodyPartTransform)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    if (itemStack.isEmpty()) {
        return {vertices, indices};
    }

    const ::mc::Item* item = itemStack.getItem();
    if (!item) {
        return {vertices, indices};
    }

    // 构建盔甲网格
    _build3DItemMesh(*item, ItemTransformType::ThirdPersonRightHand, vertices, indices, true);

    // 应用身体部件变换
    if (!vertices.empty()) {
        _transformVertices(vertices, bodyPartTransform);
    }

    MC_UNUSED(slot);

    return {vertices, indices};
}

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildHeadMesh(
    const ::mc::ItemStack& itemStack)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    if (itemStack.isEmpty()) {
        return {vertices, indices};
    }

    const ::mc::Item* item = itemStack.getItem();
    if (!item) {
        return {vertices, indices};
    }

    // 构建头部物品网格
    _build3DItemMesh(*item, ItemTransformType::Head, vertices, indices, true);

    return {vertices, indices};
}

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildGroundItemMesh(
    const ::mc::ItemStack& itemStack, f64 rotation)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    if (itemStack.isEmpty()) {
        return {vertices, indices};
    }

    const ::mc::Item* item = itemStack.getItem();
    if (!item) {
        return {vertices, indices};
    }

    // 构建地面物品网格
    _build3DItemMesh(*item, ItemTransformType::Ground, vertices, indices, true);

    // 应用旋转
    if (!vertices.empty() && rotation != 0.0) {
        f64 rotRad = rotation * mc::math::PI_DOUBLE / 180.0;
        f64 cosR = std::cos(rotRad);
        f64 sinR = std::sin(rotRad);

        for (auto& vertex : vertices) {
            f32 x = vertex.position.x;
            f32 z = vertex.position.z;
            vertex.position.x = static_cast<f32>(x * cosR - z * sinR);
            vertex.position.z = static_cast<f32>(x * sinR + z * cosR);
        }
    }

    return {vertices, indices};
}

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildIconMesh(
    const ::mc::client::renderer::api::TextureRegion& region, f64 size)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    _buildItemQuad(region, size, vertices, indices);

    return {vertices, indices};
}

std::array<f64, 16> ItemMeshBuilder::getItemTransform(
    ItemTransformType transformType, f32 limbSwing, f32 swingProgress, bool isRightHand)
{
    // 初始化为单位矩阵
    std::array<f64, 16> transform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 物品相机变换
    switch (transformType) {
        case ItemTransformType::FirstPersonRightHand: {
            transform[0] = 0.4;
            transform[5] = 0.4;
            transform[10] = 0.4;
            transform[3] = 0.56;
            transform[7] = -0.52;
            transform[11] = -0.72;

            f64 rotRad = 45.0 * mc::math::PI_DOUBLE / 180.0;
            f64 cosR = std::cos(rotRad);
            f64 sinR = std::sin(rotRad);
            transform[0] = static_cast<f64>(0.4 * cosR);
            transform[2] = static_cast<f64>(0.4 * sinR);
            transform[8] = static_cast<f64>(-0.4 * sinR);
            transform[10] = static_cast<f64>(0.4 * cosR);
            break;
        }

        case ItemTransformType::FirstPersonLeftHand: {
            transform[0] = -0.4;
            transform[5] = 0.4;
            transform[10] = 0.4;
            transform[3] = -0.56;
            transform[7] = -0.52;
            transform[11] = -0.72;

            f64 rotRad = -45.0 * mc::math::PI_DOUBLE / 180.0;
            f64 cosR = std::cos(rotRad);
            f64 sinR = std::sin(rotRad);
            transform[0] = static_cast<f64>(-0.4 * cosR);
            transform[2] = static_cast<f64>(0.4 * sinR);
            transform[8] = static_cast<f64>(0.4 * sinR);
            transform[10] = static_cast<f64>(0.4 * cosR);
            break;
        }

        case ItemTransformType::ThirdPersonRightHand: {
            f64 scale = 0.55;
            transform[0] = scale;
            transform[5] = -scale;
            transform[10] = scale;

            if (swingProgress > 0.0f) {
                f64 swingAngle = static_cast<f64>(swingProgress) * ARM_SWING_ANGLE;
                f64 rotRad = swingAngle * mc::math::PI_DOUBLE / 180.0;
                f64 cosR = std::cos(rotRad);
                f64 sinR = std::sin(rotRad);

                f64 orig0 = transform[0];
                f64 orig2 = transform[2];
                transform[0] = orig0 * cosR - transform[8] * sinR;
                transform[2] = orig2 * cosR - transform[10] * sinR;
                transform[8] = orig0 * sinR + transform[8] * cosR;
                transform[10] = orig2 * sinR + transform[10] * cosR;
            }

            transform[3] = 0.06;
            transform[7] = 0.16;
            transform[11] = -0.22;
            break;
        }

        case ItemTransformType::ThirdPersonLeftHand: {
            f64 scale = 0.55;
            transform[0] = -scale;
            transform[5] = -scale;
            transform[10] = scale;

            transform[3] = -0.06;
            transform[7] = 0.16;
            transform[11] = -0.22;
            break;
        }

        case ItemTransformType::Head: {
            f64 scale = 0.625;
            transform[0] = scale;
            transform[5] = -scale;
            transform[10] = scale;
            transform[7] = 0.75;
            break;
        }

        case ItemTransformType::Gui: {
            transform[0] = ITEM_GUI_SCALE;
            transform[5] = -ITEM_GUI_SCALE;
            transform[10] = ITEM_GUI_SCALE;
            break;
        }

        case ItemTransformType::Ground: {
            transform[0] = 0.25;
            transform[5] = -0.25;
            transform[10] = 0.25;
            transform[7] = 0.1;
            break;
        }

        case ItemTransformType::Fixed:
        case ItemTransformType::None:
        default:
            break;
    }

    MC_UNUSED(limbSwing);
    MC_UNUSED(isRightHand);

    return transform;
}

void ItemMeshBuilder::_buildItemQuad(const ::mc::client::renderer::api::TextureRegion& region,
    f64 size,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    f64 halfSize = size * 0.5;

    f32 u0 = static_cast<f32>(region.u0);
    f32 v0 = static_cast<f32>(region.v0);
    f32 u1 = static_cast<f32>(region.u1);
    f32 v1 = static_cast<f32>(region.v1);

    vertices.push_back(model::ModelVertex(-halfSize, -halfSize, 0.0, u0, v0, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(halfSize, -halfSize, 0.0, u1, v0, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(halfSize, halfSize, 0.0, u1, v1, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(-halfSize, halfSize, 0.0, u0, v1, 0.0, 0.0, 1.0));

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);
}

void ItemMeshBuilder::_build3DItemMesh(const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    bool bakeDisplayTransform)
{
    // 从 ItemModelCache 获取模型
    auto& cache = resource::ItemModelCache::instance();
    const resource::BakedItemModel* model = cache.getItemModel(item);

    if (model == nullptr) {
        // 回退：使用简单四边形
        _buildFallbackMesh(item, vertices, indices);
        return;
    }

    // 根据模型类型构建网格
    switch (model->type) {
        case resource::ItemModelType::Generated:
        case resource::ItemModelType::Handheld:
            _buildGeneratedMesh(*model, item, transformType, vertices, indices, bakeDisplayTransform);
            break;

        case resource::ItemModelType::Block:
            _buildBlockItemMesh(*model, item, transformType, vertices, indices, bakeDisplayTransform);
            break;

        case resource::ItemModelType::Custom:
            _buildCustomMesh(*model, item, transformType, vertices, indices, bakeDisplayTransform);
            break;

        default:
            _buildFallbackMesh(item, vertices, indices);
            break;
    }
}

void ItemMeshBuilder::_buildGeneratedMesh(const resource::BakedItemModel& model,
    const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    bool bakeDisplayTransform)
{
    // 获取纹理层
    auto layers = model.textureLayers;
    if (layers.empty()) {
        _buildFallbackMesh(item, vertices, indices);
        return;
    }

    // 从 ItemTextureAtlas 解析各层的纹理坐标
    std::vector<::mc::TextureRegion> textureRegions;
    if (s_itemTextureAtlas != nullptr) {
        textureRegions = s_itemTextureAtlas->getItemTextureLayers(layers);
    }

    f64 size = ITEM_SCALE * 16.0;
    f64 halfSize = size * 0.5;

    // 为每一层生成 billboard 四边形
    for (size_t layer = 0; layer < layers.size(); ++layer) {
        f32 zOffset = static_cast<f32>(layer * 0.001); // 防止 z-fighting

        // 获取该层的纹理坐标，无纹理时跳过该层
        f32 u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
        if (layer < textureRegions.size()) {
            const auto& region = textureRegions[layer];
            u0 = static_cast<f32>(region.u0);
            v0 = static_cast<f32>(region.v0);
            u1 = static_cast<f32>(region.u1);
            v1 = static_cast<f32>(region.v1);
        } else if (s_itemTextureAtlas != nullptr) {
            // 图集中未找到该层纹理，跳过
            continue;
        }

        // 前面
        u32 baseIndex = static_cast<u32>(vertices.size());
        vertices.push_back(model::ModelVertex(-halfSize, -halfSize, zOffset, u0, v1, 0.0, 0.0, 1.0));
        vertices.push_back(model::ModelVertex(halfSize, -halfSize, zOffset, u1, v1, 0.0, 0.0, 1.0));
        vertices.push_back(model::ModelVertex(halfSize, halfSize, zOffset, u1, v0, 0.0, 0.0, 1.0));
        vertices.push_back(model::ModelVertex(-halfSize, halfSize, zOffset, u0, v0, 0.0, 0.0, 1.0));

        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);

        // 后面（镜像）
        f32 backZ = -zOffset;
        baseIndex = static_cast<u32>(vertices.size());
        vertices.push_back(model::ModelVertex(halfSize, -halfSize, backZ, u0, v1, 0.0, 0.0, -1.0));
        vertices.push_back(model::ModelVertex(-halfSize, -halfSize, backZ, u1, v1, 0.0, 0.0, -1.0));
        vertices.push_back(model::ModelVertex(-halfSize, halfSize, backZ, u1, v0, 0.0, 0.0, -1.0));
        vertices.push_back(model::ModelVertex(halfSize, halfSize, backZ, u0, v0, 0.0, 0.0, -1.0));

        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }

    // 应用模型 display 变换。bakeDisplayTransform=false 时跳过，由调用方在矩阵栈上施加。
    if (bakeDisplayTransform && !vertices.empty()) {
        auto displayContext = toDisplayContext(transformType);
        const auto& transform = model.getTransform(displayContext);
        glm::mat4 mat = transform.toMatrix();
        _applyMatrixToVertices(vertices, mat);
    }

    MC_UNUSED(item);
}

void ItemMeshBuilder::_buildBlockItemMesh(const resource::BakedItemModel& model,
    const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    bool bakeDisplayTransform)
{
    // 方块物品：从 elements 构建 3D 网格
    if (model.elements.empty()) {
        _buildFallbackMesh(item, vertices, indices);
        return;
    }

    f64 scale = ITEM_SCALE;

    for (const auto& element : model.elements) {
        f64 x1 = element.from.x * scale;
        f64 y1 = element.from.y * scale;
        f64 z1 = element.from.z * scale;
        f64 x2 = element.to.x * scale;
        f64 y2 = element.to.y * scale;
        f64 z2 = element.to.z * scale;

        // 构建元素旋转矩阵（若有旋转），参考 MC FaceBakery.bakeQuad()
        bool hasRotation = !element.rotation.isIdentity();
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        if (hasRotation) {
            rotationMatrix = buildElementRotationMatrix(element.rotation, scale);
        }

        // 为每个面生成顶点
        for (const auto& [dir, face] : element.faces) {
            f32 u0 = face.uv.u0 / 16.0f;
            f32 v0 = face.uv.v0 / 16.0f;
            f32 u1 = face.uv.u1 / 16.0f;
            f32 v1 = face.uv.v1 / 16.0f;

            glm::vec3 normal;
            std::array<glm::vec3, 4> corners;

            switch (dir) {
                case Direction::North: // Z-
                    normal = glm::vec3(0.0f, 0.0f, -1.0f);
                    corners = {
                        glm::vec3(x1, y1, z1), glm::vec3(x2, y1, z1), glm::vec3(x2, y2, z1), glm::vec3(x1, y2, z1)};
                    break;
                case Direction::South: // Z+
                    normal = glm::vec3(0.0f, 0.0f, 1.0f);
                    corners = {
                        glm::vec3(x2, y1, z2), glm::vec3(x1, y1, z2), glm::vec3(x1, y2, z2), glm::vec3(x2, y2, z2)};
                    break;
                case Direction::West: // X-
                    normal = glm::vec3(-1.0f, 0.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y1, z2), glm::vec3(x1, y1, z1), glm::vec3(x1, y2, z1), glm::vec3(x1, y2, z2)};
                    break;
                case Direction::East: // X+
                    normal = glm::vec3(1.0f, 0.0f, 0.0f);
                    corners = {
                        glm::vec3(x2, y1, z1), glm::vec3(x2, y1, z2), glm::vec3(x2, y2, z2), glm::vec3(x2, y2, z1)};
                    break;
                case Direction::Down: // Y-
                    normal = glm::vec3(0.0f, -1.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y1, z2), glm::vec3(x2, y1, z2), glm::vec3(x2, y1, z1), glm::vec3(x1, y1, z1)};
                    break;
                case Direction::Up: // Y+
                    normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y2, z1), glm::vec3(x2, y2, z1), glm::vec3(x2, y2, z2), glm::vec3(x1, y2, z2)};
                    break;
                default:
                    continue;
            }

            // 应用元素旋转到顶点位置
            if (hasRotation) {
                for (auto& corner : corners) {
                    glm::vec4 pos(corner, 1.0f);
                    pos = rotationMatrix * pos;
                    corner = glm::vec3(pos);
                }
            }

            // 旋转后重算法线（参考 MC FaceBakery.calculateFacing()）
            if (hasRotation) {
                glm::vec3 edge1 = corners[1] - corners[0];
                glm::vec3 edge2 = corners[2] - corners[0];
                normal = glm::normalize(glm::cross(edge1, edge2));
            }

            // 添加顶点，使用UV旋转排列
            u32 faceBase = static_cast<u32>(vertices.size());
            for (int i = 0; i < 4; ++i) {
                auto [u, v] = getRotatedUV(i, face.uv.rotation, u0, v0, u1, v1);
                vertices.push_back(
                    model::ModelVertex(corners[i].x, corners[i].y, corners[i].z, u, v, normal.x, normal.y, normal.z));
            }

            // 添加索引
            indices.push_back(faceBase + 0);
            indices.push_back(faceBase + 1);
            indices.push_back(faceBase + 2);
            indices.push_back(faceBase + 0);
            indices.push_back(faceBase + 2);
            indices.push_back(faceBase + 3);
        }
    }

    // 应用模型 display 变换。bakeDisplayTransform=false 时跳过，由调用方在矩阵栈上施加。
    if (bakeDisplayTransform && !vertices.empty()) {
        auto displayContext = toDisplayContext(transformType);
        const auto& transform = model.getTransform(displayContext);
        glm::mat4 mat = transform.toMatrix();
        _applyMatrixToVertices(vertices, mat);
    }
}

void ItemMeshBuilder::_buildCustomMesh(const resource::BakedItemModel& model,
    const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    bool bakeDisplayTransform)
{
    // 自定义物品模型使用与方块物品相同的 elements 系统，直接复用方块物品网格构建
    _buildBlockItemMesh(model, item, transformType, vertices, indices, bakeDisplayTransform);
}

void ItemMeshBuilder::_buildFallbackMesh(
    const ::mc::Item& item, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    f64 size = ITEM_SCALE * 16.0;
    f64 x1 = -size * 0.5;
    f64 y1 = -size * 0.5;
    f64 z1 = -size * 0.5;
    f64 x2 = size * 0.5;
    f64 y2 = size * 0.5;
    f64 z2 = size * 0.5;

    f32 u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;

    // 北面 (Z-) - 法线 (0, 0, -1)
    u32 baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y1, z1, u0, v1, 0.0, 0.0, -1.0));
    vertices.push_back(model::ModelVertex(x2, y1, z1, u1, v1, 0.0, 0.0, -1.0));
    vertices.push_back(model::ModelVertex(x2, y2, z1, u1, v0, 0.0, 0.0, -1.0));
    vertices.push_back(model::ModelVertex(x1, y2, z1, u0, v0, 0.0, 0.0, -1.0));
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    // 南面 (Z+) - 法线 (0, 0, 1)
    baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x2, y1, z2, u0, v1, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(x1, y1, z2, u1, v1, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(x1, y2, z2, u1, v0, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(x2, y2, z2, u0, v0, 0.0, 0.0, 1.0));
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    // 西面 (X-) - 法线 (-1, 0, 0)
    baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y1, z2, u0, v1, -1.0, 0.0, 0.0));
    vertices.push_back(model::ModelVertex(x1, y1, z1, u1, v1, -1.0, 0.0, 0.0));
    vertices.push_back(model::ModelVertex(x1, y2, z1, u1, v0, -1.0, 0.0, 0.0));
    vertices.push_back(model::ModelVertex(x1, y2, z2, u0, v0, -1.0, 0.0, 0.0));
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    // 东面 (X+) - 法线 (1, 0, 0)
    baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x2, y1, z1, u0, v1, 1.0, 0.0, 0.0));
    vertices.push_back(model::ModelVertex(x2, y1, z2, u1, v1, 1.0, 0.0, 0.0));
    vertices.push_back(model::ModelVertex(x2, y2, z2, u1, v0, 1.0, 0.0, 0.0));
    vertices.push_back(model::ModelVertex(x2, y2, z1, u0, v0, 1.0, 0.0, 0.0));
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    // 底面 (Y-) - 法线 (0, -1, 0)
    baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y1, z2, u0, v1, 0.0, -1.0, 0.0));
    vertices.push_back(model::ModelVertex(x2, y1, z2, u1, v1, 0.0, -1.0, 0.0));
    vertices.push_back(model::ModelVertex(x2, y1, z1, u1, v0, 0.0, -1.0, 0.0));
    vertices.push_back(model::ModelVertex(x1, y1, z1, u0, v0, 0.0, -1.0, 0.0));
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    // 顶面 (Y+) - 法线 (0, 1, 0)
    baseIndex = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y2, z1, u0, v1, 0.0, 1.0, 0.0));
    vertices.push_back(model::ModelVertex(x2, y2, z1, u1, v1, 0.0, 1.0, 0.0));
    vertices.push_back(model::ModelVertex(x2, y2, z2, u1, v0, 0.0, 1.0, 0.0));
    vertices.push_back(model::ModelVertex(x1, y2, z2, u0, v0, 0.0, 1.0, 0.0));
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);

    MC_UNUSED(item);
}

void ItemMeshBuilder::_applyMatrixToVertices(std::vector<model::ModelVertex>& vertices, const glm::mat4& matrix)
{
    // 计算逆转置矩阵用于法线变换，确保非均匀缩放时法线正确
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(matrix)));

    for (auto& vertex : vertices) {
        glm::vec4 pos(vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);
        pos = matrix * pos;
        vertex.position.x = pos.x;
        vertex.position.y = pos.y;
        vertex.position.z = pos.z;

        glm::vec3 normal(vertex.normal.x, vertex.normal.y, vertex.normal.z);
        normal = glm::normalize(normalMatrix * normal);
        vertex.normal.x = normal.x;
        vertex.normal.y = normal.y;
        vertex.normal.z = normal.z;
    }
}

void ItemMeshBuilder::_applyHeldItemTransform(std::vector<model::ModelVertex>& vertices,
    ItemTransformType transformType,
    f32 limbSwing,
    f32 swingProgress,
    bool isRightHand)
{
    std::array<f64, 16> transform = getItemTransform(transformType, limbSwing, swingProgress, isRightHand);
    _transformVertices(vertices, transform);
}

void ItemMeshBuilder::_transformVertices(std::vector<model::ModelVertex>& vertices, const std::array<f64, 16>& matrix)
{
    for (auto& vertex : vertices) {
        f64 x = static_cast<f64>(vertex.position.x);
        f64 y = static_cast<f64>(vertex.position.y);
        f64 z = static_cast<f64>(vertex.position.z);
        f64 w = 1.0;

        vertex.position.x = static_cast<f32>(matrix[0] * x + matrix[1] * y + matrix[2] * z + matrix[3] * w);
        vertex.position.y = static_cast<f32>(matrix[4] * x + matrix[5] * y + matrix[6] * z + matrix[7] * w);
        vertex.position.z = static_cast<f32>(matrix[8] * x + matrix[9] * y + matrix[10] * z + matrix[11] * w);
    }
}

} // namespace mc::client::renderer::entity::item
