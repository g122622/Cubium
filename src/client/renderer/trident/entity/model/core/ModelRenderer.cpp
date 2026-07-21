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

#include "ModelRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <glm/glm.hpp>

namespace mc::client::renderer::entity::model {

// ============================================================================
// TexturedQuad
// ============================================================================

TexturedQuad::TexturedQuad(const std::array<Vector3f, 4>& positions,
    f64 u1,
    f64 v1,
    f64 u2,
    f64 v2,
    f64 texWidth,
    f64 texHeight,
    const Vector3f& faceNormal,
    bool mirror)
    : normal(faceNormal)
{
    // 计算UV坐标（归一化到0-1范围）
    f32 u1n = static_cast<f32>(u1 / texWidth);
    f32 v1n = static_cast<f32>(v1 / texHeight);
    f32 u2n = static_cast<f32>(u2 / texWidth);
    f32 v2n = static_cast<f32>(v2 / texHeight);

    vertices[0] = ModelVertex(positions[0], Vector2f(u2n, v1n), normal);
    vertices[1] = ModelVertex(positions[1], Vector2f(u1n, v1n), normal);
    vertices[2] = ModelVertex(positions[2], Vector2f(u1n, v2n), normal);
    vertices[3] = ModelVertex(positions[3], Vector2f(u2n, v2n), normal);

    if (mirror) {
        std::swap(vertices[0], vertices[3]);
        std::swap(vertices[1], vertices[2]);
        normal.x = -normal.x;
        for (auto& vertex : vertices) {
            vertex.normal = normal;
        }
    }
}

// ============================================================================
// ModelBox
// ============================================================================

ModelBox::ModelBox(i32 texOffX,
    i32 texOffY,
    f64 x,
    f64 y,
    f64 z,
    f64 width,
    f64 height,
    f64 depth,
    f64 deltaX,
    f64 deltaY,
    f64 deltaZ,
    f64 texWidth,
    f64 texHeight,
    bool mirror)
{
    // 计算盒子边界
    posX1 = x;
    posY1 = y;
    posZ1 = z;
    posX2 = x + width;
    posY2 = y + height;
    posZ2 = z + depth;

    // 保存重算 UV 所需的原始参数
    m_texOffX = texOffX;
    m_texOffY = texOffY;
    m_boxWidth = width;
    m_boxHeight = height;
    m_boxDepth = depth;
    m_deltaX = deltaX;
    m_deltaY = deltaY;
    m_deltaZ = deltaZ;
    m_mirror = mirror;

    // 用初始纹理尺寸/偏移构建 6 面
    rebuildQuads(texWidth, texHeight, texOffX, texOffY);
}

void ModelBox::rebuildQuads(f64 texWidth, f64 texHeight, i32 texOffX, i32 texOffY)
{
    // 固化本次重算使用的纹理偏移
    m_texOffX = texOffX;
    m_texOffY = texOffY;

    // 应用膨胀（防止Z-fighting）
    f64 x1 = posX1 - m_deltaX;
    f64 y1 = posY1 - m_deltaY;
    f64 z1 = posZ1 - m_deltaZ;
    f64 x2 = posX2 + m_deltaX;
    f64 y2 = posY2 + m_deltaY;
    f64 z2 = posZ2 + m_deltaZ;

    // 如果镜像，交换X方向
    if (m_mirror) {
        std::swap(x1, x2);
    }

    // 创建8个顶点
    Vector3f v0(static_cast<f32>(x1), static_cast<f32>(y1), static_cast<f32>(z1)); // 左下后
    Vector3f v1(static_cast<f32>(x2), static_cast<f32>(y1), static_cast<f32>(z1)); // 右下后
    Vector3f v2(static_cast<f32>(x2), static_cast<f32>(y2), static_cast<f32>(z1)); // 右上后
    Vector3f v3(static_cast<f32>(x1), static_cast<f32>(y2), static_cast<f32>(z1)); // 左上后
    Vector3f v4(static_cast<f32>(x1), static_cast<f32>(y1), static_cast<f32>(z2)); // 左下前
    Vector3f v5(static_cast<f32>(x2), static_cast<f32>(y1), static_cast<f32>(z2)); // 右下前
    Vector3f v6(static_cast<f32>(x2), static_cast<f32>(y2), static_cast<f32>(z2)); // 右上前
    Vector3f v7(static_cast<f32>(x1), static_cast<f32>(y2), static_cast<f32>(z2)); // 左上前

    // 计算UV坐标
    // 布局说明：
    // - 西面(X-): depth x height
    // - 东面(X+): depth x height
    // - 北面(Z-): width x height
    // - 南面(Z+): width x height
    // - 下底面(Y-): width x depth
    // - 上顶面(Y+): width x depth

    f64 f4 = static_cast<f64>(texOffX);                                                     // 西面U起点
    f64 f5 = static_cast<f64>(texOffX + m_boxDepth);                                        // 下底面U起点
    f64 f6 = static_cast<f64>(texOffX + m_boxDepth + m_boxWidth);                           // 北面U起点
    f64 f7 = static_cast<f64>(texOffX + m_boxDepth + m_boxWidth + m_boxWidth);              // 上顶面U起点
    f64 f8 = static_cast<f64>(texOffX + m_boxDepth + m_boxWidth + m_boxDepth);              // 东面U起点
    f64 f9 = static_cast<f64>(texOffX + m_boxDepth + m_boxWidth + m_boxDepth + m_boxWidth); // 南面U起点

    f64 f10 = static_cast<f64>(texOffY);                            // 上部分V起点
    f64 f11 = static_cast<f64>(texOffY + m_boxDepth);               // 中部分V起点
    f64 f12 = static_cast<f64>(texOffY + m_boxDepth + m_boxHeight); // 下部分V起点

    // 定义法线方向
    Vector3f normalEast(1.0f, 0.0f, 0.0f);   // 东面 X+
    Vector3f normalWest(-1.0f, 0.0f, 0.0f);  // 西面 X-
    Vector3f normalNorth(0.0f, 0.0f, -1.0f); // 北面 Z-
    Vector3f normalSouth(0.0f, 0.0f, 1.0f);  // 南面 Z+
    Vector3f normalUp(0.0f, 1.0f, 0.0f);     // 上顶面 Y+
    Vector3f normalDown(0.0f, -1.0f, 0.0f);  // 下底面 Y-

    // 创建6个面
    // 注意：面的顶点顺序需要符合逆时针约定（从外部看）
    // 面索引：quads[0] = EAST, quads[1] = WEST, quads[2] = DOWN, quads[3] = UP, quads[4] = NORTH, quads[5] = SOUTH

    // 东面 (X+) - quads[0]
    quads[0] = TexturedQuad({v5, v1, v2, v6}, // 右下前, 右下后, 右上后, 右上前
        f6,
        f11,
        f8,
        f12,
        texWidth,
        texHeight,
        normalEast,
        m_mirror);

    // 西面 (X-) - quads[1]
    quads[1] = TexturedQuad({v0, v4, v7, v3}, // 左下后, 左下前, 左上前, 左上后
        f4,
        f11,
        f5,
        f12,
        texWidth,
        texHeight,
        normalWest,
        m_mirror);

    // 下底面 (Y-) - quads[2]
    quads[2] = TexturedQuad({v5, v4, v0, v1}, // 右下前, 左下前, 左下后, 右下后
        f5,
        f10,
        f6,
        f11,
        texWidth,
        texHeight,
        normalDown,
        m_mirror);

    // 上顶面 (Y+) - quads[3]
    quads[3] = TexturedQuad({v2, v3, v7, v6}, // 右上后, 左上后, 左上前, 右上前
        f6,
        f11,
        f7,
        f10,
        texWidth,
        texHeight,
        normalUp,
        m_mirror);

    // 北面 (Z-) - quads[4]
    quads[4] = TexturedQuad({v1, v0, v3, v2}, // 右下后, 左下后, 左上后, 右上后
        f5,
        f11,
        f6,
        f12,
        texWidth,
        texHeight,
        normalNorth,
        m_mirror);

    // 南面 (Z+) - quads[5]
    quads[5] = TexturedQuad({v4, v5, v6, v7}, // 左下前, 右下前, 右上前, 左上前
        f8,
        f11,
        f9,
        f12,
        texWidth,
        texHeight,
        normalSouth,
        m_mirror);
}

// ============================================================================
// ModelRenderer
// ============================================================================

ModelRenderer::ModelRenderer(const std::string& name)
    : m_name(name)
{}

void ModelRenderer::setTextureSize(i32 width, i32 height)
{
    m_textureWidth = static_cast<f64>(width);
    m_textureHeight = static_cast<f64>(height);

    // 回溯重算已有盒子的 UV：纹理尺寸变更后，原按旧尺寸归一化的 UV 已固化在
    // 顶点里，必须用新尺寸重新归一化。每个盒子沿用其构造时的纹理偏移。
    // 不递归子部件——子部件有自己的独立纹理尺寸（如 PlayerModel 的披风 64×32）。
    for (auto& box : m_boxes) {
        box.rebuildQuads(m_textureWidth, m_textureHeight, box.texOffX(), box.texOffY());
    }
}

ModelRenderer& ModelRenderer::setTextureOffset(i32 offsetX, i32 offsetY)
{
    m_textureOffsetX = offsetX;
    m_textureOffsetY = offsetY;

    // 回溯重算已有盒子的 UV：纹理偏移变更后，把所有已有盒子的偏移统一刷成
    // 新值并重算。仅影响"建完盒子后再调 setTextureOffset"的场景（如 PlayerModel
    // 左腿在基类建盒后改偏移到 (16,48)），这正是期望行为。
    for (auto& box : m_boxes) {
        box.rebuildQuads(m_textureWidth, m_textureHeight, offsetX, offsetY);
    }
    return *this;
}

ModelRenderer& ModelRenderer::addBox(f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth, f64 delta)
{
    m_boxes.emplace_back(m_textureOffsetX,
        m_textureOffsetY,
        x,
        y,
        z,
        width,
        height,
        depth,
        delta,
        delta,
        delta,
        m_textureWidth,
        m_textureHeight,
        m_mirror);
    return *this;
}

ModelRenderer& ModelRenderer::addBox(
    i32 textureOffsetX, i32 textureOffsetY, f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth, f64 delta)
{
    m_boxes.emplace_back(textureOffsetX,
        textureOffsetY,
        x,
        y,
        z,
        width,
        height,
        depth,
        delta,
        delta,
        delta,
        m_textureWidth,
        m_textureHeight,
        m_mirror);
    return *this;
}

ModelRenderer& ModelRenderer::addBox(f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth, bool mirror, f64 delta)
{
    bool oldMirror = m_mirror;
    m_mirror = mirror;
    m_boxes.emplace_back(m_textureOffsetX,
        m_textureOffsetY,
        x,
        y,
        z,
        width,
        height,
        depth,
        delta,
        delta,
        delta,
        m_textureWidth,
        m_textureHeight,
        m_mirror);
    m_mirror = oldMirror;
    return *this;
}

std::shared_ptr<ModelRenderer> ModelRenderer::createChild(const std::string& name)
{
    auto child = std::make_shared<ModelRenderer>(name);
    child->setTextureSize(static_cast<i32>(m_textureWidth), static_cast<i32>(m_textureHeight));
    addChild(child);
    return child;
}

void ModelRenderer::copyModelAngles(const ModelRenderer& other)
{
    m_rotateAngleX = other.m_rotateAngleX;
    m_rotateAngleY = other.m_rotateAngleY;
    m_rotateAngleZ = other.m_rotateAngleZ;
    m_rotationPointX = other.m_rotationPointX;
    m_rotationPointY = other.m_rotationPointY;
    m_rotationPointZ = other.m_rotationPointZ;
}

// ============================================================================
// 矩阵工具
// ============================================================================

std::array<f64, 16> ModelRenderer::_identityMatrix()
{
    return {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
}

std::array<f64, 16> ModelRenderer::_multiplyMatrices(const std::array<f64, 16>& a, const std::array<f64, 16>& b)
{
    std::array<f64, 16> result;
    for (i32 row = 0; row < 4; ++row) {
        for (i32 col = 0; col < 4; ++col) {
            const auto resultIndex = static_cast<std::size_t>(row * 4 + col);
            result[resultIndex] = 0.0;
            for (i32 k = 0; k < 4; ++k) {
                result[resultIndex] +=
                    a[static_cast<std::size_t>(row * 4 + k)] * b[static_cast<std::size_t>(k * 4 + col)];
            }
        }
    }
    return result;
}

std::array<f64, 16> ModelRenderer::_translationMatrix(f64 x, f64 y, f64 z)
{
    return {1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0};
}

std::array<f64, 16> ModelRenderer::_rotationXMatrix(f64 angle)
{
    f64 c = std::cos(angle);
    f64 s = std::sin(angle);
    return {1.0, 0.0, 0.0, 0.0, 0.0, c, -s, 0.0, 0.0, s, c, 0.0, 0.0, 0.0, 0.0, 1.0};
}

std::array<f64, 16> ModelRenderer::_rotationYMatrix(f64 angle)
{
    f64 c = std::cos(angle);
    f64 s = std::sin(angle);
    return {c, 0.0, s, 0.0, 0.0, 1.0, 0.0, 0.0, -s, 0.0, c, 0.0, 0.0, 0.0, 0.0, 1.0};
}

std::array<f64, 16> ModelRenderer::_rotationZMatrix(f64 angle)
{
    f64 c = std::cos(angle);
    f64 s = std::sin(angle);
    return {c, -s, 0.0, 0.0, s, c, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
}

std::array<f64, 16> ModelRenderer::_scaleMatrix(f64 x, f64 y, f64 z)
{
    return {x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0};
}

ModelVertex ModelRenderer::_transformVertex(const ModelVertex& vertex, const std::array<f64, 16>& matrix)
{
    const f64* m = matrix.data();
    ModelVertex result;

    // 变换位置 (齐次坐标)
    result.position.x =
        static_cast<f32>(m[0] * vertex.position.x + m[1] * vertex.position.y + m[2] * vertex.position.z + m[3]);
    result.position.y =
        static_cast<f32>(m[4] * vertex.position.x + m[5] * vertex.position.y + m[6] * vertex.position.z + m[7]);
    result.position.z =
        static_cast<f32>(m[8] * vertex.position.x + m[9] * vertex.position.y + m[10] * vertex.position.z + m[11]);

    // 变换法线：使用逆转置矩阵确保非均匀缩放时法线正确
    // 从行主序 f64[16] 矩阵提取左上 3x3 子矩阵构造 glm::dmat3
    // 行主序布局：m[0..3] 是第0行, m[4..7] 是第1行, m[8..11] 是第2行
    glm::dmat3 transform3x3(m[0],
        m[4],
        m[8], // 第0列
        m[1],
        m[5],
        m[9], // 第1列
        m[2],
        m[6],
        m[10] // 第2列
    );
    glm::dmat3 normalMatrix = glm::transpose(glm::inverse(transform3x3));
    glm::dvec3 normal(vertex.normal.x, vertex.normal.y, vertex.normal.z);
    normal = glm::normalize(normalMatrix * normal);

    result.normal.x = static_cast<f32>(normal.x);
    result.normal.y = static_cast<f32>(normal.y);
    result.normal.z = static_cast<f32>(normal.z);

    // 纹理坐标不变
    result.texCoord = vertex.texCoord;

    return result;
}

void ModelRenderer::generateMesh(std::vector<ModelVertex>& vertices, std::vector<u32>& indices, f64 scale) const
{
    auto matrix = _identityMatrix();
    generateMesh(vertices, indices, matrix, scale);
}

void ModelRenderer::generateMesh(std::vector<ModelVertex>& vertices,
    std::vector<u32>& indices,
    const std::array<f64, 16>& parentMatrix,
    f64 scale) const
{
    if (!m_visible) {
        return;
    }

    // 构建当前部件的变换矩阵

    // 1. 平移到旋转点 (rotationPoint / 16.0)
    auto translation = _translationMatrix(m_rotationPointX * scale, m_rotationPointY * scale, m_rotationPointZ * scale);

    // 2. 应用旋转 (顺序: Z -> Y -> X)
    auto rotZ = _rotationZMatrix(m_rotateAngleZ);
    auto rotY = _rotationYMatrix(m_rotateAngleY);
    auto rotX = _rotationXMatrix(m_rotateAngleX);

    // 3. 应用缩放
    auto scaleMat = _scaleMatrix(m_scaleX, m_scaleY, m_scaleZ);

    // 组合变换: parentMatrix * translation * rotZ * rotY * rotX * scale
    auto matrix = _multiplyMatrices(parentMatrix, translation);
    matrix = _multiplyMatrices(matrix, rotZ);
    matrix = _multiplyMatrices(matrix, rotY);
    matrix = _multiplyMatrices(matrix, rotX);
    matrix = _multiplyMatrices(matrix, scaleMat);

    // 渲染盒子
    for (const auto& box : m_boxes) {
        for (const auto& quad : box.quads) {
            // 当前基顶点索引
            u32 baseIndex = static_cast<u32>(vertices.size());

            // 添加4个顶点
            for (const auto& vertex : quad.vertices) {
                ModelVertex scaledVertex = vertex;
                scaledVertex.position.x = static_cast<f32>(static_cast<f64>(scaledVertex.position.x) * scale);
                scaledVertex.position.y = static_cast<f32>(static_cast<f64>(scaledVertex.position.y) * scale);
                scaledVertex.position.z = static_cast<f32>(static_cast<f64>(scaledVertex.position.z) * scale);
                ModelVertex transformed = _transformVertex(scaledVertex, matrix);
                vertices.push_back(transformed);
            }

            // 添加6个索引（2个三角形）
            // 逆时针顺序: 0-1-2, 0-2-3
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);

            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }
    }

    // 渲染子部件（递归）
    for (const auto& child : m_children) {
        if (child) {
            child->generateMesh(vertices, indices, matrix, scale);
        }
    }
}

void ModelRenderer::render(f64 scale)
{
    if (!m_visible) {
        return;
    }

    // TODO: 此废弃方法仅保留为遗留接口兼容，待所有调用方迁移到 generateMesh() 后移除
    (void)scale;

    // 渲染子部件
    for (auto& child : m_children) {
        if (child) {
            child->render(scale);
        }
    }
}

void ModelRenderer::interpolateRotation(const Vector3f& target, f64 speed)
{
    m_rotateAngleX += (target.x - m_rotateAngleX) * speed;
    m_rotateAngleY += (target.y - m_rotateAngleY) * speed;
    m_rotateAngleZ += (target.z - m_rotateAngleZ) * speed;
}

void ModelRenderer::getTransformMatrix(std::array<f64, 16>& outMatrix) const
{
    // 构建变换矩阵
    // 使用 1/16 作为默认缩放（模型单位到世界单位）

    // 1. 平移到旋转点
    auto translation = _translationMatrix(m_rotationPointX, m_rotationPointY, m_rotationPointZ);

    // 2. 应用旋转 (顺序: Z -> Y -> X)
    auto rotZ = _rotationZMatrix(m_rotateAngleZ);
    auto rotY = _rotationYMatrix(m_rotateAngleY);
    auto rotX = _rotationXMatrix(m_rotateAngleX);

    // 组合变换: translation * rotZ * rotY * rotX
    outMatrix = _multiplyMatrices(translation, rotZ);
    outMatrix = _multiplyMatrices(outMatrix, rotY);
    outMatrix = _multiplyMatrices(outMatrix, rotX);
}

} // namespace mc::client::renderer::entity::model
