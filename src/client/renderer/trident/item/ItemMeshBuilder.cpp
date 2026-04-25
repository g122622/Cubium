#include "ItemMeshBuilder.hpp"
#include "common/item/ItemStack.hpp"
#include "common/item/Item.hpp"
#include "client/renderer/MeshTypes.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::item {

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

    // TODO: 从物品模型获取纹理区域
    // 当前实现：创建简单的图标四边形

    // 获取物品变换
    std::array<f64, 16> transform = getItemTransform(transformType, 0.0f, 0.0f, true);

    // 构建简单四边形
    // TODO: 根据物品类型（工具、方块等）选择不同的渲染方式
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

    // TODO: 根据 slot 确定盔甲类型和变换
    // EquipmentSlot::Head, Chest, Legs, Feet

    (void)slot;

    // 构建盔甲网格
    // TODO: 从盔甲模型获取几何数据
    build3DItemMesh(*item, ItemTransformType::ThirdPersonRightHand, vertices, indices);

    // 应用身体部件变换
    if (!vertices.empty()) {
        transformVertices(vertices, bodyPartTransform);
    }

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
    // TODO: 头部物品通常是一个立方体（如南瓜、头盔）
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
        f64 rotRad = rotation * 3.14159265359 / 180.0;
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
    const ::mc::TextureRegion& region,
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
            // 第一人称右手
            // 参考 MC: 平移 (1.13, 3.2, 1.13), 旋转 (0, 45, 0), 缩放 0.4
            transform[0] = 0.4;   // scale X
            transform[5] = 0.4;   // scale Y (翻转)
            transform[10] = 0.4;  // scale Z
            transform[3] = 0.56;  // X 平移
            transform[7] = -0.52; // Y 平移 (负值因为 Y 翻转)
            transform[11] = -0.72; // Z 平移

            // 旋转 Y 45 度
            f64 rotRad = 45.0 * 3.14159265359 / 180.0;
            f64 cosR = std::cos(rotRad);
            f64 sinR = std::sin(rotRad);
            transform[0] = static_cast<f64>(0.4 * cosR);
            transform[2] = static_cast<f64>(0.4 * sinR);
            transform[8] = static_cast<f64>(-0.4 * sinR);
            transform[10] = static_cast<f64>(0.4 * cosR);
            break;
        }

        case ItemTransformType::FirstPersonLeftHand: {
            // 第一人称左手
            // 镜像右手
            transform[0] = -0.4;  // scale X (镜像)
            transform[5] = 0.4;   // scale Y (翻转)
            transform[10] = 0.4;  // scale Z
            transform[3] = -0.56; // X 平移
            transform[7] = -0.52; // Y 平移
            transform[11] = -0.72; // Z 平移

            // 旋转 Y -45 度
            f64 rotRad = -45.0 * 3.14159265359 / 180.0;
            f64 cosR = std::cos(rotRad);
            f64 sinR = std::sin(rotRad);
            transform[0] = static_cast<f64>(-0.4 * cosR);
            transform[2] = static_cast<f64>(0.4 * sinR);
            transform[8] = static_cast<f64>(0.4 * sinR);
            transform[10] = static_cast<f64>(0.4 * cosR);
            break;
        }

        case ItemTransformType::ThirdPersonRightHand: {
            // 第三人称右手（手持）
            // 参考 MC: 平移 (0, 3, 1), 旋转 (0, 0, -50), 缩放 0.55
            f64 scale = 0.55;

            // 先缩放
            transform[0] = scale;
            transform[5] = -scale;  // Y 翻转
            transform[10] = scale;

            // 应用挥动动画
            if (swingProgress > 0.0f) {
                f64 swingAngle = static_cast<f64>(swingProgress) * ARM_SWING_ANGLE;
                f64 rotRad = swingAngle * 3.14159265359 / 180.0;
                f64 cosR = std::cos(rotRad);
                f64 sinR = std::sin(rotRad);

                f64 orig0 = transform[0];
                f64 orig2 = transform[2];
                transform[0] = orig0 * cosR - transform[8] * sinR;
                transform[2] = orig2 * cosR - transform[10] * sinR;
                transform[8] = orig0 * sinR + transform[8] * cosR;
                transform[10] = orig2 * sinR + transform[10] * cosR;
            }

            // 平移
            transform[3] = 0.06;   // X
            transform[7] = 0.16;   // Y (考虑翻转)
            transform[11] = -0.22; // Z
            break;
        }

        case ItemTransformType::ThirdPersonLeftHand: {
            // 第三人称左手
            f64 scale = 0.55;
            transform[0] = -scale;  // X 镜像
            transform[5] = -scale;  // Y 翻转
            transform[10] = scale;

            transform[3] = -0.06;
            transform[7] = 0.16;
            transform[11] = -0.22;
            break;
        }

        case ItemTransformType::Head: {
            // 头部（头盔等）
            // 缩放 0.625, 平移 (0, 0, 0)
            f64 scale = 0.625;
            transform[0] = scale;
            transform[5] = -scale;
            transform[10] = scale;
            // 头盔位置调整
            transform[7] = 0.75;  // Y 偏移
            break;
        }

        case ItemTransformType::Gui: {
            // GUI 界面
            transform[0] = ITEM_GUI_SCALE;
            transform[5] = -ITEM_GUI_SCALE;
            transform[10] = ITEM_GUI_SCALE;
            break;
        }

        case ItemTransformType::Ground: {
            // 地面（掉落物）
            transform[0] = 0.25;
            transform[5] = -0.25;
            transform[10] = 0.25;
            // 轻微向上偏移
            transform[7] = 0.1;
            break;
        }

        case ItemTransformType::Fixed:
        case ItemTransformType::None:
        default:
            // 无变换
            break;
    }

    (void)limbSwing;
    (void)isRightHand;

    return transform;
}

void ItemMeshBuilder::buildItemQuad(
    const ::mc::TextureRegion& region,
    f64 size,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices)
{
    // 构建一个面向相机的四边形
    f64 halfSize = size * 0.5;

    // UV 坐标
    f32 u0 = static_cast<f32>(region.u0);
    f32 v0 = static_cast<f32>(region.v0);
    f32 u1 = static_cast<f32>(region.u1);
    f32 v1 = static_cast<f32>(region.v1);

    // 四个顶点
    model::ModelVertex v0, v1, v2, v3;

    // 左上
    v0.position = Vector3f(static_cast<f32>(-halfSize), static_cast<f32>(-halfSize), 0.0f);
    v0.texCoord = Vector2f(u0, v0);
    v0.normal = Vector3f(0.0f, 0.0f, 1.0f);
    v0.color = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

    // 右上
    v1.position = Vector3f(static_cast<f32>(halfSize), static_cast<f32>(-halfSize), 0.0f);
    v1.texCoord = Vector2f(u1, v0);
    v1.normal = Vector3f(0.0f, 0.0f, 1.0f);
    v1.color = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

    // 右下
    v2.position = Vector3f(static_cast<f32>(halfSize), static_cast<f32>(halfSize), 0.0f);
    v2.texCoord = Vector2f(u1, v1);
    v2.normal = Vector3f(0.0f, 0.0f, 1.0f);
    v2.color = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

    // 左下
    v3.position = Vector3f(static_cast<f32>(-halfSize), static_cast<f32>(halfSize), 0.0f);
    v3.texCoord = Vector2f(u0, v1);
    v3.normal = Vector3f(0.0f, 0.0f, 1.0f);
    v3.color = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    // 两个三角形
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
    // TODO: 从物品模型系统获取 3D 网格
    // 当前实现：创建简单的图标四边形

    // 获取物品位置（用于构造纹理区域）
    const auto& itemLocation = item.itemLocation();

    // 创建简单的四边形表示
    // 真实实现需要从物品模型获取几何数据
    f64 size = ITEM_SCALE * 16.0;  // 1 格大小

    // 占位符：创建简单的立方体（8 个顶点，12 个三角形）
    f64 halfSize = size * 0.5;

    // UV 坐标（占位符）
    f32 u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;

    // 立方体顶点
    model::ModelVertex cubeVertices[24];  // 6 面 * 4 顶点

    // 前面 (Z+)
    cubeVertices[0] = {Vector3f(static_cast<f32>(-halfSize), static_cast<f32>(-halfSize), static_cast<f32>(halfSize)),
                       Vector2f(u0, v1), Vector3f(0.0f, 0.0f, 1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};
    cubeVertices[1] = {Vector3f(static_cast<f32>(halfSize), static_cast<f32>(-halfSize), static_cast<f32>(halfSize)),
                       Vector2f(u1, v1), Vector3f(0.0f, 0.0f, 1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};
    cubeVertices[2] = {Vector3f(static_cast<f32>(halfSize), static_cast<f32>(halfSize), static_cast<f32>(halfSize)),
                       Vector2f(u1, v0), Vector3f(0.0f, 0.0f, 1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};
    cubeVertices[3] = {Vector3f(static_cast<f32>(-halfSize), static_cast<f32>(halfSize), static_cast<f32>(halfSize)),
                       Vector2f(u0, v0), Vector3f(0.0f, 0.0f, 1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};

    // 后面 (Z-)
    cubeVertices[4] = {Vector3f(static_cast<f32>(halfSize), static_cast<f32>(-halfSize), static_cast<f32>(-halfSize)),
                       Vector2f(u0, v1), Vector3f(0.0f, 0.0f, -1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};
    cubeVertices[5] = {Vector3f(static_cast<f32>(-halfSize), static_cast<f32>(-halfSize), static_cast<f32>(-halfSize)),
                       Vector2f(u1, v1), Vector3f(0.0f, 0.0f, -1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};
    cubeVertices[6] = {Vector3f(static_cast<f32>(-halfSize), static_cast<f32>(halfSize), static_cast<f32>(-halfSize)),
                       Vector2f(u1, v0), Vector3f(0.0f, 0.0f, -1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};
    cubeVertices[7] = {Vector3f(static_cast<f32>(halfSize), static_cast<f32>(halfSize), static_cast<f32>(-halfSize)),
                       Vector2f(u0, v0), Vector3f(0.0f, 0.0f, -1.0f), Vector4f(1.0f, 1.0f, 1.0f, 1.0f)};

    // 上下面、左右面
    // 简化：只添加前后面
    for (int i = 0; i < 8; ++i) {
        vertices.push_back(cubeVertices[i]);
    }

    // 前面三角形
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(0); indices.push_back(2); indices.push_back(3);

    // 后面三角形
    indices.push_back(4); indices.push_back(5); indices.push_back(6);
    indices.push_back(4); indices.push_back(6); indices.push_back(7);

    (void)transformType;
    (void)itemLocation;
}

void ItemMeshBuilder::applyHeldItemTransform(
    std::vector<model::ModelVertex>& vertices,
    ItemTransformType transformType,
    f32 limbSwing,
    f32 swingProgress,
    bool isRightHand)
{
    // 获取基础变换
    std::array<f64, 16> transform = getItemTransform(transformType, limbSwing, swingProgress, isRightHand);

    // 应用到所有顶点
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

        // 矩阵乘法
        vertex.position.x = static_cast<f32>(
            matrix[0] * x + matrix[1] * y + matrix[2] * z + matrix[3] * w);
        vertex.position.y = static_cast<f32>(
            matrix[4] * x + matrix[5] * y + matrix[6] * z + matrix[7] * w);
        vertex.position.z = static_cast<f32>(
            matrix[8] * x + matrix[9] * y + matrix[10] * z + matrix[11] * w);
    }
}

} // namespace mc::client::renderer::entity::item
