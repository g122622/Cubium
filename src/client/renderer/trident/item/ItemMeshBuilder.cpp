#include "ItemMeshBuilder.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "client/renderer/api/texture/TextureRegion.hpp"
#include "client/resource/ItemModelCache.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "client/resource/ItemModelLoader.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::item {

// 引入 ModelElement 和 Direction 类型
using ::mc::ModelElement;
using ::mc::ModelFace;
using ::mc::Direction;

namespace {

// 将 ItemTransformType 转换为 ItemDisplayContext
resource::ItemDisplayContext toDisplayContext(ItemTransformType type) {
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

// 将 glm::mat4 转换为 std::array<f64, 16>
std::array<f64, 16> mat4ToArray(const glm::mat4& mat) {
    std::array<f64, 16> result;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result[col * 4 + row] = static_cast<f64>(mat[col][row]);
        }
    }
    return result;
}

// 将 std::array<f64, 16> 转换为 glm::mat4
glm::mat4 arrayToMat4(const std::array<f64, 16>& arr) {
    glm::mat4 mat;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            mat[col][row] = static_cast<float>(arr[col * 4 + row]);
        }
    }
    return mat;
}

} // namespace

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildHeldItemMesh(
    const ::mc::ItemStack& itemStack,
    ItemTransformType transformType)
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

    // 获取物品变换
    std::array<f64, 16> transform = getItemTransform(transformType, 0.0f, 0.0f, true);

    // 构建 3D 物品网格
    build3DItemMesh(*item, transformType, vertices, indices);

    // 应用变换
    if (!vertices.empty()) {
        transformVertices(vertices, transform);
    }

    return {vertices, indices};
}

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildArmorMesh(
    const ::mc::ItemStack& itemStack,
    u32 slot,
    const std::array<f64, 16>& bodyPartTransform)
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
    build3DItemMesh(*item, ItemTransformType::ThirdPersonRightHand, vertices, indices);

    // 应用身体部件变换
    if (!vertices.empty()) {
        transformVertices(vertices, bodyPartTransform);
    }

    (void)slot;

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
    build3DItemMesh(*item, ItemTransformType::Head, vertices, indices);

    return {vertices, indices};
}

std::pair<std::vector<model::ModelVertex>, std::vector<u32>> ItemMeshBuilder::buildGroundItemMesh(
    const ::mc::ItemStack& itemStack,
    f64 rotation)
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
    build3DItemMesh(*item, ItemTransformType::Ground, vertices, indices);

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
    const ::mc::client::renderer::api::TextureRegion& region,
    f64 size)
{
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    buildItemQuad(region, size, vertices, indices);

    return {vertices, indices};
}

std::array<f64, 16> ItemMeshBuilder::getItemTransform(
    ItemTransformType transformType,
    f32 limbSwing,
    f32 swingProgress,
    bool isRightHand)
{
    // 初始化为单位矩阵
    std::array<f64, 16> transform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 参考 MC 1.16.5 ItemCameraTransforms
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

    (void)limbSwing;
    (void)isRightHand;

    return transform;
}

void ItemMeshBuilder::buildItemQuad(
    const ::mc::client::renderer::api::TextureRegion& region,
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

void ItemMeshBuilder::build3DItemMesh(
    const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 从 ItemModelCache 获取模型
    auto& cache = resource::ItemModelCache::instance();
    const resource::BakedItemModel* model = cache.getItemModel(item);

    if (model == nullptr) {
        // 回退：使用简单四边形
        buildFallbackMesh(item, vertices, indices);
        return;
    }

    // 根据模型类型构建网格
    switch (model->type) {
        case resource::ItemModelType::Generated:
        case resource::ItemModelType::Handheld:
            buildGeneratedMesh(*model, item, transformType, vertices, indices);
            break;

        case resource::ItemModelType::Block:
            buildBlockItemMesh(*model, item, transformType, vertices, indices);
            break;

        case resource::ItemModelType::Custom:
            buildCustomMesh(*model, item, transformType, vertices, indices);
            break;

        default:
            buildFallbackMesh(item, vertices, indices);
            break;
    }
}

void ItemMeshBuilder::buildGeneratedMesh(
    const resource::BakedItemModel& model,
    const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 获取纹理层
    auto layers = model.textureLayers;
    if (layers.empty()) {
        buildFallbackMesh(item, vertices, indices);
        return;
    }

    // 获取 ItemTextureAtlas
    // 注意：这里假设 ItemTextureAtlas 已初始化，实际使用时需要传入
    // 暂时使用简单纹理坐标

    f64 size = ITEM_SCALE * 16.0;
    f64 halfSize = size * 0.5;

    // 为每一层生成 billboard 四边形
    for (size_t layer = 0; layer < layers.size(); ++layer) {
        f32 zOffset = static_cast<f32>(layer * 0.001);  // 防止 z-fighting

        // 占位符 UV（实际应从 ItemTextureAtlas 获取）
        f32 u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;

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
        zOffset = -zOffset;
        baseIndex = static_cast<u32>(vertices.size());
        vertices.push_back(model::ModelVertex(halfSize, -halfSize, zOffset, u0, v1, 0.0, 0.0, -1.0));
        vertices.push_back(model::ModelVertex(-halfSize, -halfSize, zOffset, u1, v1, 0.0, 0.0, -1.0));
        vertices.push_back(model::ModelVertex(-halfSize, halfSize, zOffset, u1, v0, 0.0, 0.0, -1.0));
        vertices.push_back(model::ModelVertex(halfSize, halfSize, zOffset, u0, v0, 0.0, 0.0, -1.0));

        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }

    // 应用模型变换
    if (!vertices.empty()) {
        auto displayContext = toDisplayContext(transformType);
        const auto& transform = model.getTransform(displayContext);
        glm::mat4 mat = transform.toMatrix();
        applyMatrixToVertices(vertices, mat);
    }

    (void)item;
}

void ItemMeshBuilder::buildBlockItemMesh(
    const resource::BakedItemModel& model,
    const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 方块物品：从 elements 构建 3D 网格
    if (model.elements.empty()) {
        buildFallbackMesh(item, vertices, indices);
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

        u32 baseIndex = static_cast<u32>(vertices.size());

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
                        glm::vec3(x1, y1, z1),
                        glm::vec3(x2, y1, z1),
                        glm::vec3(x2, y2, z1),
                        glm::vec3(x1, y2, z1)
                    };
                    break;
                case Direction::South: // Z+
                    normal = glm::vec3(0.0f, 0.0f, 1.0f);
                    corners = {
                        glm::vec3(x2, y1, z2),
                        glm::vec3(x1, y1, z2),
                        glm::vec3(x1, y2, z2),
                        glm::vec3(x2, y2, z2)
                    };
                    break;
                case Direction::West: // X-
                    normal = glm::vec3(-1.0f, 0.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y1, z2),
                        glm::vec3(x1, y1, z1),
                        glm::vec3(x1, y2, z1),
                        glm::vec3(x1, y2, z2)
                    };
                    break;
                case Direction::East: // X+
                    normal = glm::vec3(1.0f, 0.0f, 0.0f);
                    corners = {
                        glm::vec3(x2, y1, z1),
                        glm::vec3(x2, y1, z2),
                        glm::vec3(x2, y2, z2),
                        glm::vec3(x2, y2, z1)
                    };
                    break;
                case Direction::Down: // Y-
                    normal = glm::vec3(0.0f, -1.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y1, z2),
                        glm::vec3(x2, y1, z2),
                        glm::vec3(x2, y1, z1),
                        glm::vec3(x1, y1, z1)
                    };
                    break;
                case Direction::Up: // Y+
                    normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y2, z1),
                        glm::vec3(x2, y2, z1),
                        glm::vec3(x2, y2, z2),
                        glm::vec3(x1, y2, z2)
                    };
                    break;
                default:
                    continue;
            }

            // 添加顶点
            u32 faceBase = static_cast<u32>(vertices.size());
            vertices.push_back(model::ModelVertex(
                corners[0].x, corners[0].y, corners[0].z, u0, v1,
                normal.x, normal.y, normal.z));
            vertices.push_back(model::ModelVertex(
                corners[1].x, corners[1].y, corners[1].z, u1, v1,
                normal.x, normal.y, normal.z));
            vertices.push_back(model::ModelVertex(
                corners[2].x, corners[2].y, corners[2].z, u1, v0,
                normal.x, normal.y, normal.z));
            vertices.push_back(model::ModelVertex(
                corners[3].x, corners[3].y, corners[3].z, u0, v0,
                normal.x, normal.y, normal.z));

            // 添加索引
            indices.push_back(faceBase + 0);
            indices.push_back(faceBase + 1);
            indices.push_back(faceBase + 2);
            indices.push_back(faceBase + 0);
            indices.push_back(faceBase + 2);
            indices.push_back(faceBase + 3);
        }
    }

    // 应用模型变换
    if (!vertices.empty()) {
        auto displayContext = toDisplayContext(transformType);
        const auto& transform = model.getTransform(displayContext);
        glm::mat4 mat = transform.toMatrix();
        applyMatrixToVertices(vertices, mat);
    }
}

void ItemMeshBuilder::buildCustomMesh(
    const resource::BakedItemModel& model,
    const ::mc::Item& item,
    ItemTransformType transformType,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 自定义模型：与方块物品类似
    buildBlockItemMesh(model, item, transformType, vertices, indices);
}

void ItemMeshBuilder::buildFallbackMesh(
    const ::mc::Item& item,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 简单的立方体作为回退
    f64 size = ITEM_SCALE * 16.0;
    f64 halfSize = size * 0.5;

    f32 u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;

    // 前面 (Z+)
    vertices.push_back(model::ModelVertex(-halfSize, -halfSize, halfSize, u0, v1, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(halfSize, -halfSize, halfSize, u1, v1, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(halfSize, halfSize, halfSize, u1, v0, 0.0, 0.0, 1.0));
    vertices.push_back(model::ModelVertex(-halfSize, halfSize, halfSize, u0, v0, 0.0, 0.0, 1.0));

    // 后面 (Z-)
    vertices.push_back(model::ModelVertex(halfSize, -halfSize, -halfSize, u0, v1, 0.0, 0.0, -1.0));
    vertices.push_back(model::ModelVertex(-halfSize, -halfSize, -halfSize, u1, v1, 0.0, 0.0, -1.0));
    vertices.push_back(model::ModelVertex(-halfSize, halfSize, -halfSize, u1, v0, 0.0, 0.0, -1.0));
    vertices.push_back(model::ModelVertex(halfSize, halfSize, -halfSize, u0, v0, 0.0, 0.0, -1.0));

    // 前面三角形
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(0); indices.push_back(2); indices.push_back(3);

    // 后面三角形
    indices.push_back(4); indices.push_back(5); indices.push_back(6);
    indices.push_back(4); indices.push_back(6); indices.push_back(7);

    (void)item;
}

void ItemMeshBuilder::applyMatrixToVertices(
    std::vector<model::ModelVertex>& vertices,
    const glm::mat4& matrix)
{
    for (auto& vertex : vertices) {
        glm::vec4 pos(vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);
        pos = matrix * pos;
        vertex.position.x = pos.x;
        vertex.position.y = pos.y;
        vertex.position.z = pos.z;

        // 法线也需要变换（使用逆转置矩阵，但这里简化处理）
        glm::vec4 normal(vertex.normal.x, vertex.normal.y, vertex.normal.z, 0.0f);
        normal = matrix * normal;
        if (glm::length(glm::vec3(normal)) > 0.0001f) {
            normal = glm::normalize(normal);
        }
        vertex.normal.x = normal.x;
        vertex.normal.y = normal.y;
        vertex.normal.z = normal.z;
    }
}

void ItemMeshBuilder::applyHeldItemTransform(
    std::vector<model::ModelVertex>& vertices,
    ItemTransformType transformType,
    f32 limbSwing,
    f32 swingProgress,
    bool isRightHand)
{
    std::array<f64, 16> transform = getItemTransform(transformType, limbSwing, swingProgress, isRightHand);
    transformVertices(vertices, transform);
}

void ItemMeshBuilder::transformVertices(
    std::vector<model::ModelVertex>& vertices,
    const std::array<f64, 16>& matrix)
{
    for (auto& vertex : vertices) {
        f64 x = static_cast<f64>(vertex.position.x);
        f64 y = static_cast<f64>(vertex.position.y);
        f64 z = static_cast<f64>(vertex.position.z);
        f64 w = 1.0;

        vertex.position.x = static_cast<f32>(
            matrix[0] * x + matrix[1] * y + matrix[2] * z + matrix[3] * w);
        vertex.position.y = static_cast<f32>(
            matrix[4] * x + matrix[5] * y + matrix[6] * z + matrix[7] * w);
        vertex.position.z = static_cast<f32>(
            matrix[8] * x + matrix[9] * y + matrix[10] * z + matrix[11] * w);
    }
}

} // namespace mc::client::renderer::entity::item
