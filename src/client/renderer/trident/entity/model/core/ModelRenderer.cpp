#include "ModelRenderer.hpp"
#include <cmath>
#include <algorithm>

namespace mc::client::renderer::entity::model {

// ============================================================================
// TexturedQuad
// ============================================================================

TexturedQuad::TexturedQuad(const std::array<Vector3f, 4>& positions,
                           f64 u1, f64 v1, f64 u2, f64 v2,
                           f64 texWidth, f64 texHeight,
                           const Vector3f& normal,
                           bool mirror)
    : normal(normal)
{
    // 计算UV坐标（归一化到0-1范围）
    f32 u1n = static_cast<f32>(u1 / texWidth);
    f32 v1n = static_cast<f32>(v1 / texHeight);
    f32 u2n = static_cast<f32>(u2 / texWidth);
    f32 v2n = static_cast<f32>(v2 / texHeight);

    // 设置顶点UV坐标
    // 顶点顺序：右下 -> 左下 -> 左上 -> 右上（逆时针，符合OpenGL约定）
    if (mirror) {
        // 镜像：交换顶点顺序
        vertices[0] = ModelVertex(positions[0], Vector2f(u1n, v1n), normal);
        vertices[1] = ModelVertex(positions[1], Vector2f(u2n, v1n), normal);
        vertices[2] = ModelVertex(positions[2], Vector2f(u2n, v2n), normal);
        vertices[3] = ModelVertex(positions[3], Vector2f(u1n, v2n), normal);
    } else {
        // 正常顺序
        vertices[0] = ModelVertex(positions[0], Vector2f(u2n, v1n), normal);
        vertices[1] = ModelVertex(positions[1], Vector2f(u1n, v1n), normal);
        vertices[2] = ModelVertex(positions[2], Vector2f(u1n, v2n), normal);
        vertices[3] = ModelVertex(positions[3], Vector2f(u2n, v2n), normal);
    }
}

// ============================================================================
// ModelBox
// ============================================================================

ModelBox::ModelBox(i32 texOffX, i32 texOffY,
                   f64 x, f64 y, f64 z,
                   f64 width, f64 height, f64 depth,
                   f64 deltaX, f64 deltaY, f64 deltaZ,
                   f64 texWidth, f64 texHeight,
                   bool mirror)
{
    // 计算盒子边界
    posX1 = x;
    posY1 = y;
    posZ1 = z;
    posX2 = x + width;
    posY2 = y + height;
    posZ2 = z + depth;

    // 应用膨胀（防止Z-fighting）
    f64 x1 = x - deltaX;
    f64 y1 = y - deltaY;
    f64 z1 = z - deltaZ;
    f64 x2 = posX2 + deltaX;
    f64 y2 = posY2 + deltaY;
    f64 z2 = posZ2 + deltaZ;

    // 如果镜像，交换X方向
    if (mirror) {
        std::swap(x1, x2);
    }

    // 创建8个顶点
    // 参考 MC 1.16.5 ModelBox 构造函数
    Vector3f v0(static_cast<f32>(x1), static_cast<f32>(y1), static_cast<f32>(z1));  // 左下后
    Vector3f v1(static_cast<f32>(x2), static_cast<f32>(y1), static_cast<f32>(z1));  // 右下后
    Vector3f v2(static_cast<f32>(x2), static_cast<f32>(y2), static_cast<f32>(z1));  // 右上后
    Vector3f v3(static_cast<f32>(x1), static_cast<f32>(y2), static_cast<f32>(z1));  // 左上后
    Vector3f v4(static_cast<f32>(x1), static_cast<f32>(y1), static_cast<f32>(z2));  // 左下前
    Vector3f v5(static_cast<f32>(x2), static_cast<f32>(y1), static_cast<f32>(z2));  // 右下前
    Vector3f v6(static_cast<f32>(x2), static_cast<f32>(y2), static_cast<f32>(z2));  // 右上前
    Vector3f v7(static_cast<f32>(x1), static_cast<f32>(y2), static_cast<f32>(z2));  // 左上前

    // 计算UV坐标
    // 参考 MC 1.16.5 纹理布局
    // 布局说明：
    // - 西面(X-): depth x height
    // - 东面(X+): depth x height
    // - 北面(Z-): width x height
    // - 南面(Z+): width x height
    // - 下底面(Y-): width x depth
    // - 上顶面(Y+): width x depth

    f64 f4 = static_cast<f64>(texOffX);                          // 西面U起点
    f64 f5 = static_cast<f64>(texOffX + depth);                  // 下底面U起点
    f64 f6 = static_cast<f64>(texOffX + depth + width);          // 北面U起点
    f64 f7 = static_cast<f64>(texOffX + depth + width + width);  // 上顶面U起点
    f64 f8 = static_cast<f64>(texOffX + depth + width + depth);  // 东面U起点
    f64 f9 = static_cast<f64>(texOffX + depth + width + depth + width);  // 南面U起点

    f64 f10 = static_cast<f64>(texOffY);          // 上部分V起点
    f64 f11 = static_cast<f64>(texOffY + depth);  // 中部分V起点
    f64 f12 = static_cast<f64>(texOffY + depth + height);  // 下部分V起点

    // 定义法线方向
    Vector3f normalEast(1.0f, 0.0f, 0.0f);   // 东面 X+
    Vector3f normalWest(-1.0f, 0.0f, 0.0f);  // 西面 X-
    Vector3f normalNorth(0.0f, 0.0f, -1.0f); // 北面 Z-
    Vector3f normalSouth(0.0f, 0.0f, 1.0f);  // 南面 Z+
    Vector3f normalUp(0.0f, 1.0f, 0.0f);     // 上顶面 Y+
    Vector3f normalDown(0.0f, -1.0f, 0.0f);  // 下底面 Y-

    // 创建6个面
    // 注意：面的顶点顺序需要符合逆时针约定（从外部看）

    // 东面 (X+) - indices: 0,1,2,3 -> v5,v1,v2,v6
    quads[0] = TexturedQuad(
        {v5, v1, v2, v6},
        f6, f11, f8, f12,
        texWidth, texHeight,
        normalEast, mirror
    );

    // 西面 (X-) - indices: 4,5,6,7 -> v4,v0,v3,v7
    quads[1] = TexturedQuad(
        {v4, v0, v3, v7},
        f4, f11, f5, f12,
        texWidth, texHeight,
        normalWest, mirror
    );

    // 北面 (Z-) - indices: 8,9,10,11 -> v1,v0,v3,v2
    quads[2] = TexturedQuad(
        {v1, v0, v3, v2},
        f5, f11, f6, f12,
        texWidth, texHeight,
        normalNorth, mirror
    );

    // 下底面 (Y-) - indices: 12,13,14,15 -> v5,v4,v0,v1
    quads[3] = TexturedQuad(
        {v5, v4, v0, v1},
        f5, f10, f6, f11,
        texWidth, texHeight,
        normalDown, mirror
    );

    // 上顶面 (Y+) - indices: 16,17,18,19 -> v2,v3,v7,v6
    quads[4] = TexturedQuad(
        {v2, v3, v7, v6},
        f6, f11, f7, f10,
        texWidth, texHeight,
        normalUp, mirror
    );

    // 南面 (Z+) - indices: 20,21,22,23 -> v4,v5,v6,v7
    quads[5] = TexturedQuad(
        {v4, v5, v6, v7},
        f8, f11, f9, f12,
        texWidth, texHeight,
        normalSouth, mirror
    );
}

// ============================================================================
// ModelRenderer
// ============================================================================

ModelRenderer::ModelRenderer(const String& name)
    : m_name(name)
{
}

void ModelRenderer::setTextureSize(i32 width, i32 height) {
    m_textureWidth = static_cast<f64>(width);
    m_textureHeight = static_cast<f64>(height);
}

ModelRenderer& ModelRenderer::setTextureOffset(i32 offsetX, i32 offsetY) {
    m_textureOffsetX = offsetX;
    m_textureOffsetY = offsetY;
    return *this;
}

ModelRenderer& ModelRenderer::addBox(f64 x, f64 y, f64 z,
                                      f64 width, f64 height, f64 depth,
                                      f64 delta) {
    m_boxes.emplace_back(
        m_textureOffsetX, m_textureOffsetY,
        x, y, z, width, height, depth,
        delta, delta, delta,
        m_textureWidth, m_textureHeight,
        m_mirror
    );
    return *this;
}

ModelRenderer& ModelRenderer::addBox(i32 textureOffsetX, i32 textureOffsetY,
                                      f64 x, f64 y, f64 z,
                                      f64 width, f64 height, f64 depth,
                                      f64 delta) {
    m_boxes.emplace_back(
        textureOffsetX, textureOffsetY,
        x, y, z, width, height, depth,
        delta, delta, delta,
        m_textureWidth, m_textureHeight,
        m_mirror
    );
    return *this;
}

ModelRenderer& ModelRenderer::addBox(f64 x, f64 y, f64 z,
                                      f64 width, f64 height, f64 depth,
                                      bool mirror, f64 delta) {
    bool oldMirror = m_mirror;
    m_mirror = mirror;
    m_boxes.emplace_back(
        m_textureOffsetX, m_textureOffsetY,
        x, y, z, width, height, depth,
        delta, delta, delta,
        m_textureWidth, m_textureHeight,
        m_mirror
    );
    m_mirror = oldMirror;
    return *this;
}

std::shared_ptr<ModelRenderer> ModelRenderer::createChild(const String& name) {
    auto child = std::make_shared<ModelRenderer>(name);
    child->setTextureSize(static_cast<i32>(m_textureWidth), static_cast<i32>(m_textureHeight));
    addChild(child);
    return child;
}

void ModelRenderer::copyModelAngles(const ModelRenderer& other) {
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

std::array<f64, 16> ModelRenderer::identityMatrix() {
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

std::array<f64, 16> ModelRenderer::multiplyMatrices(const std::array<f64, 16>& a, const std::array<f64, 16>& b) {
    std::array<f64, 16> result;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result[row * 4 + col] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result[row * 4 + col] += a[row * 4 + k] * b[k * 4 + col];
            }
        }
    }
    return result;
}

std::array<f64, 16> ModelRenderer::translationMatrix(f64 x, f64 y, f64 z) {
    return {
        1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

std::array<f64, 16> ModelRenderer::rotationXMatrix(f64 angle) {
    f64 c = std::cos(angle);
    f64 s = std::sin(angle);
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c,    -s,   0.0f,
        0.0f, s,    c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

std::array<f64, 16> ModelRenderer::rotationYMatrix(f64 angle) {
    f64 c = std::cos(angle);
    f64 s = std::sin(angle);
    return {
        c,    0.0f, s,    0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s,   0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

std::array<f64, 16> ModelRenderer::rotationZMatrix(f64 angle) {
    f64 c = std::cos(angle);
    f64 s = std::sin(angle);
    return {
        c,    -s,   0.0f, 0.0f,
        s,    c,    0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

std::array<f64, 16> ModelRenderer::scaleMatrix(f64 x, f64 y, f64 z) {
    return {
        x,    0.0f, 0.0f, 0.0f,
        0.0f, y,    0.0f, 0.0f,
        0.0f, 0.0f, z,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

ModelVertex ModelRenderer::transformVertex(const ModelVertex& vertex, const std::array<f64, 16>& matrix) {
    const f64* m = matrix.data();
    ModelVertex result;

    // 变换位置 (齐次坐标)
    result.position.x = m[0] * vertex.position.x + m[1] * vertex.position.y + m[2] * vertex.position.z + m[3];
    result.position.y = m[4] * vertex.position.x + m[5] * vertex.position.y + m[6] * vertex.position.z + m[7];
    result.position.z = m[8] * vertex.position.x + m[9] * vertex.position.y + m[10] * vertex.position.z + m[11];

    // 变换法线 (只使用旋转部分，假设矩阵是正交的)
    // 对于包含缩放的矩阵，需要使用逆转置矩阵，但这里简化处理
    result.normal.x = m[0] * vertex.normal.x + m[1] * vertex.normal.y + m[2] * vertex.normal.z;
    result.normal.y = m[4] * vertex.normal.x + m[5] * vertex.normal.y + m[6] * vertex.normal.z;
    result.normal.z = m[8] * vertex.normal.x + m[9] * vertex.normal.y + m[10] * vertex.normal.z;

    // 归一化法线
    f64 len = std::sqrt(result.normal.x * result.normal.x +
                        result.normal.y * result.normal.y +
                        result.normal.z * result.normal.z);
    if (len > 0.0f) {
        result.normal.x /= len;
        result.normal.y /= len;
        result.normal.z /= len;
    }

    // 纹理坐标不变
    result.texCoord = vertex.texCoord;

    return result;
}

void ModelRenderer::generateMesh(std::vector<ModelVertex>& vertices,
                                  std::vector<u32>& indices,
                                  f64 scale) const {
    auto matrix = identityMatrix();
    generateMesh(vertices, indices, matrix, scale);
}

void ModelRenderer::generateMesh(std::vector<ModelVertex>& vertices,
                                  std::vector<u32>& indices,
                                  const std::array<f64, 16>& parentMatrix,
                                  f64 scale) const {
    if (!m_visible) {
        return;
    }

    // 构建当前部件的变换矩阵
    // 参考 MC 1.16.5 ModelRenderer.translateRotate()

    // 1. 平移到旋转点 (rotationPoint / 16.0f)
    auto translation = translationMatrix(
        m_rotationPointX * scale,
        m_rotationPointY * scale,
        m_rotationPointZ * scale
    );

    // 2. 应用旋转 (顺序: Z -> Y -> X，与MC一致)
    auto rotZ = rotationZMatrix(m_rotateAngleZ);
    auto rotY = rotationYMatrix(m_rotateAngleY);
    auto rotX = rotationXMatrix(m_rotateAngleX);

    // 3. 应用缩放
    auto scaleMat = scaleMatrix(m_scaleX, m_scaleY, m_scaleZ);

    // 组合变换: parentMatrix * translation * rotZ * rotY * rotX * scale
    auto matrix = multiplyMatrices(parentMatrix, translation);
    matrix = multiplyMatrices(matrix, rotZ);
    matrix = multiplyMatrices(matrix, rotY);
    matrix = multiplyMatrices(matrix, rotX);
    matrix = multiplyMatrices(matrix, scaleMat);

    // 渲染盒子
    for (const auto& box : m_boxes) {
        for (const auto& quad : box.quads) {
            // 当前基顶点索引
            u32 baseIndex = static_cast<u32>(vertices.size());

            // 添加4个顶点
            for (const auto& vertex : quad.vertices) {
                ModelVertex transformed = transformVertex(vertex, matrix);
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

void ModelRenderer::render(f64 scale) {
    if (!m_visible) {
        return;
    }

    // TODO: 实际渲染逻辑
    // 现在使用 generateMesh 来生成网格数据

    (void)scale;

    // 渲染子部件
    for (auto& child : m_children) {
        if (child) {
            child->render(scale);
        }
    }
}

void ModelRenderer::renderNoRotate(f64 scale) {
    if (!m_visible) {
        return;
    }

    // TODO: 渲染盒子（不应用旋转）

    (void)scale;

    // 渲染子部件
    for (auto& child : m_children) {
        if (child) {
            child->renderNoRotate(scale);
        }
    }
}

void ModelRenderer::interpolateRotation(const Vector3f& target, f64 speed) {
    m_rotateAngleX += (target.x - m_rotateAngleX) * speed;
    m_rotateAngleY += (target.y - m_rotateAngleY) * speed;
    m_rotateAngleZ += (target.z - m_rotateAngleZ) * speed;
}

} // namespace mc::client::renderer::entity::model
