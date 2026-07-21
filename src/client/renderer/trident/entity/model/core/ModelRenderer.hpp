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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <array>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model {

/**
 * @brief 模型顶点
 *
 * 包含位置、纹理坐标和法线信息
 */
struct ModelVertex {
    Vector3f position; // 顶点位置
    Vector2f texCoord; // UV坐标
    Vector3f normal;   // 法线

    ModelVertex() = default;
    ModelVertex(f64 x, f64 y, f64 z, f64 u, f64 v, f64 nx = 0, f64 ny = 0, f64 nz = 0)
        : position(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z))
        , texCoord(static_cast<f32>(u), static_cast<f32>(v))
        , normal(static_cast<f32>(nx), static_cast<f32>(ny), static_cast<f32>(nz))
    {}

    ModelVertex(const Vector3f& pos, const Vector2f& tex, const Vector3f& norm)
        : position(pos)
        , texCoord(tex)
        , normal(norm)
    {}
};

/**
 * @brief 纹理四边形
 *
 * 代表一个四边形面，包含4个顶点和法线
 */
struct TexturedQuad {
    std::array<ModelVertex, 4> vertices;
    Vector3f normal;

    TexturedQuad() = default;

    /**
     * @brief 构造纹理四边形
     * @param positions 4个顶点位置
     * @param u1, v1, u2, v2 纹理坐标范围
     * @param texWidth 纹理宽度
     * @param texHeight 纹理高度
     * @param normal 面法线
     * @param mirror 是否镜像（影响顶点顺序）
     */
    TexturedQuad(const std::array<Vector3f, 4>& positions,
        f64 u1,
        f64 v1,
        f64 u2,
        f64 v2,
        f64 texWidth,
        f64 texHeight,
        const Vector3f& normal,
        bool mirror = false);
};

/**
 * @brief 模型盒子
 *
 * 每个盒子有6个面，每面是一个TexturedQuad。
 * UV坐标根据纹理偏移自动计算。
 *
 * 构造时保存 texOff/width/height/depth/mirror 原始参数，使 setTextureSize /
 * setTextureOffset 改变纹理尺寸/偏移后能经 rebuildQuads 回溯重算 6 面 UV，
 * 修复"纹理尺寸变更不传播到已建盒子"的固化时序问题。
 */
struct ModelBox {
    f64 posX1, posY1, posZ1;           // 最小角
    f64 posX2, posY2, posZ2;           // 最大角
    std::array<TexturedQuad, 6> quads; // 6个面：东、西、北、下、上、南

    /**
     * @brief 构造模型盒子
     * @param texOffX 纹理偏移X
     * @param texOffY 纹理偏移Y
     * @param x 起始X
     * @param y 起始Y
     * @param z 起始Z
     * @param width 宽度（X方向）
     * @param height 高度（Y方向）
     * @param depth 深度（Z方向）
     * @param deltaX X方向膨胀（防止Z-fighting）
     * @param deltaY Y方向膨胀
     * @param deltaZ Z方向膨胀
     * @param texWidth 纹理宽度
     * @param texHeight 纹理高度
     * @param mirror 是否镜像
     */
    ModelBox(i32 texOffX,
        i32 texOffY,
        f64 x,
        f64 y,
        f64 z,
        f64 width,
        f64 height,
        f64 depth,
        f64 deltaX = 0.0,
        f64 deltaY = 0.0,
        f64 deltaZ = 0.0,
        f64 texWidth = 64.0,
        f64 texHeight = 32.0,
        bool mirror = false);

    /**
     * @brief 用新的纹理尺寸/偏移重算 6 面 UV
     *
     * 位置/法线不变，仅按新 texWidth/texHeight 重新归一化每个面四边形的 UV，
     * 并把新偏移固化为本盒子的纹理偏移。供 ModelRenderer::setTextureSize /
     * setTextureOffset 在改变字段后回溯重算已有盒子 UV。
     *
     * @param texWidth 新纹理宽度
     * @param texHeight 新纹理高度
     * @param texOffX 新纹理偏移X（覆盖构造时的偏移）
     * @param texOffY 新纹理偏移Y
     */
    void rebuildQuads(f64 texWidth, f64 texHeight, i32 texOffX, i32 texOffY);

    /**
     * @brief 获取构造/重算时固化的纹理偏移X
     */
    [[nodiscard]] i32 texOffX() const { return m_texOffX; }
    /**
     * @brief 获取构造/重算时固化的纹理偏移Y
     */
    [[nodiscard]] i32 texOffY() const { return m_texOffY; }

private:
    // 构造时固化的 UV 相关原始参数，供 rebuildQuads 重算使用
    i32 m_texOffX = 0;     // 纹理偏移X
    i32 m_texOffY = 0;     // 纹理偏移Y
    f64 m_boxWidth = 0.0;  // 盒子宽度（X方向，像素布局用）
    f64 m_boxHeight = 0.0; // 盒子高度（Y方向）
    f64 m_boxDepth = 0.0;  // 盒子深度（Z方向）
    f64 m_deltaX = 0.0;    // X方向膨胀（重算顶点位置用）
    f64 m_deltaY = 0.0;    // Y方向膨胀
    f64 m_deltaZ = 0.0;    // Z方向膨胀
    bool m_mirror = false; // 镜像
};

/**
 * @brief 模型部件
 *
 * 代表模型的一个部分（如头部、身体、腿等）。
 * 包含位置、旋转、缩放以及子部件。
 */
class ModelRenderer {
public:
    /**
     * @brief 构造函数
     * @param name 部件名称（用于调试）
     */
    explicit ModelRenderer(const std::string& name = "");
    ~ModelRenderer() = default;

    // ========== 纹理尺寸 ==========

    /**
     * @brief 设置纹理尺寸
     */
    void setTextureSize(i32 width, i32 height);

    /**
     * @brief 设置纹理偏移（下一个addBox使用）
     */
    ModelRenderer& setTextureOffset(i32 offsetX, i32 offsetY);

    // ========== 变换 ==========

    /**
     * @brief 设置位置偏移
     */
    void setOffset(f64 x, f64 y, f64 z)
    {
        m_offsetX = x;
        m_offsetY = y;
        m_offsetZ = z;
    }

    /**
     * @brief 设置旋转点
     */
    void setRotationPoint(f64 x, f64 y, f64 z)
    {
        m_rotationPointX = x;
        m_rotationPointY = y;
        m_rotationPointZ = z;
    }

    /**
     * @brief 设置旋转角度（弧度）
     */
    void setRotation(f64 x, f64 y, f64 z)
    {
        m_rotateAngleX = x;
        m_rotateAngleY = y;
        m_rotateAngleZ = z;
    }

    /**
     * @brief 设置缩放
     */
    void setScale(f64 x, f64 y, f64 z)
    {
        m_scaleX = x;
        m_scaleY = y;
        m_scaleZ = z;
    }

    // ========== 盒子（立方体） ==========

    /**
     * @brief 添加一个盒子
     * @param x 起始X
     * @param y 起始Y
     * @param z 起始Z
     * @param width 宽度
     * @param height 高度
     * @param depth 深度
     * @param delta 膨胀值（用于防止Z-fighting）
     * @return 本部件引用
     */
    ModelRenderer& addBox(f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth, f64 delta = 0.0);

    /**
     * @brief 添加一个盒子（带纹理偏移）
     * @param textureOffsetX 纹理偏移X
     * @param textureOffsetY 纹理偏移Y
     * @param x 起始X
     * @param y 起始Y
     * @param z 起始Z
     * @param width 宽度
     * @param height 高度
     * @param depth 深度
     * @param delta 膨胀值
     * @return 本部件引用
     */
    ModelRenderer& addBox(
        i32 textureOffsetX, i32 textureOffsetY, f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth, f64 delta = 0.0);

    /**
     * @brief 添加一个盒子（带镜像选项）
     */
    ModelRenderer& addBox(f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth, bool mirror, f64 delta = 0.0);

    /**
     * @brief 清除所有盒子
     */
    void clearBoxes() { m_boxes.clear(); }

    // ========== 镜像 ==========

    /**
     * @brief 设置镜像模式
     */
    void setMirror(bool mirror) { m_mirror = mirror; }
    [[nodiscard]] bool mirror() const { return m_mirror; }

    // ========== 子部件 ==========

    /**
     * @brief 添加子部件
     * @param child 子部件
     */
    void addChild(std::shared_ptr<ModelRenderer> child) { m_children.push_back(child); }

    /**
     * @brief 获取子部件列表
     */
    [[nodiscard]] const std::vector<std::shared_ptr<ModelRenderer>>& children() const { return m_children; }

    /**
     * @brief 创建并添加子部件
     * @param name 子部件名称
     * @return 创建的子部件
     */
    std::shared_ptr<ModelRenderer> createChild(const std::string& name = "");

    // ========== 网格生成 ==========

    /**
     * @brief 生成渲染网格
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param scale 缩放因子（默认 1/16）
     */
    void generateMesh(std::vector<ModelVertex>& vertices, std::vector<u32>& indices, f64 scale = 1.0 / 16.0) const;

    /**
     * @brief 生成渲染网格（带变换矩阵）
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param parentMatrix 父变换矩阵（4x4，行主序）
     * @param scale 缩放因子
     */
    void generateMesh(std::vector<ModelVertex>& vertices,
        std::vector<u32>& indices,
        const std::array<f64, 16>& parentMatrix,
        f64 scale = 1.0 / 16.0) const;

    // ========== 渲染（遗留接口，已废弃 - 使用 generateMesh 代替） ==========

    /**
     * @brief 渲染模型（已废弃）
     *
     * 此方法为遗留的 CPU 立即模式渲染接口，项目已改用 GPU 管线路径。
     * 请使用 generateMesh() 生成网格数据，然后通过 EntityPipeline 提交到 GPU。
     *
     * @param scale 缩放因子
     * @deprecated 使用 generateMesh() 代替
     */
    void render(f64 scale = 1.0 / 16.0);

    // ========== 动画 ==========

    /**
     * @brief 插值旋转
     * @param target 目标角度
     * @param speed 插值速度
     */
    void interpolateRotation(const Vector3f& target, f64 speed);

    // ========== 状态 ==========

    /**
     * @brief 获取部件名称
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief 是否可见
     *
     * 不可见的模型部件不会渲染。
     * 此属性会传递给所有子部件。
     */
    [[nodiscard]] bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    // ========== 变换矩阵 ==========

    /**
     * @brief 获取变换矩阵
     * @param outMatrix 输出矩阵（4x4，行主序）
     */
    void getTransformMatrix(std::array<f64, 16>& outMatrix) const;

    // ========== 旋转访问器 ==========

    [[nodiscard]] f64 rotateAngleX() const { return m_rotateAngleX; }
    [[nodiscard]] f64 rotateAngleY() const { return m_rotateAngleY; }
    [[nodiscard]] f64 rotateAngleZ() const { return m_rotateAngleZ; }

    // 兼容性别名
    [[nodiscard]] f64 getRotateAngleX() const { return m_rotateAngleX; }
    [[nodiscard]] f64 getRotateAngleY() const { return m_rotateAngleY; }
    [[nodiscard]] f64 getRotateAngleZ() const { return m_rotateAngleZ; }

    void setRotateAngleX(f64 angle) { m_rotateAngleX = angle; }
    void setRotateAngleY(f64 angle) { m_rotateAngleY = angle; }
    void setRotateAngleZ(f64 angle) { m_rotateAngleZ = angle; }

    // ========== 旋转点访问器 ==========

    [[nodiscard]] f64 rotationPointX() const { return m_rotationPointX; }
    [[nodiscard]] f64 rotationPointY() const { return m_rotationPointY; }
    [[nodiscard]] f64 rotationPointZ() const { return m_rotationPointZ; }

    // 兼容性别名
    [[nodiscard]] f64 getRotationPointX() const { return m_rotationPointX; }
    [[nodiscard]] f64 getRotationPointY() const { return m_rotationPointY; }
    [[nodiscard]] f64 getRotationPointZ() const { return m_rotationPointZ; }

    void setRotationPointX(f64 x) { m_rotationPointX = x; }
    void setRotationPointY(f64 y) { m_rotationPointY = y; }
    void setRotationPointZ(f64 z) { m_rotationPointZ = z; }

    // ========== 复制旋转 ==========

    /**
     * @brief 复制另一个部件的旋转角度和旋转点
     */
    void copyModelAngles(const ModelRenderer& other);

private:
    std::string m_name;

    // 变换
    f64 m_offsetX = 0.0;
    f64 m_offsetY = 0.0;
    f64 m_offsetZ = 0.0;
    f64 m_rotationPointX = 0.0;
    f64 m_rotationPointY = 0.0;
    f64 m_rotationPointZ = 0.0;
    f64 m_rotateAngleX = 0.0;
    f64 m_rotateAngleY = 0.0;
    f64 m_rotateAngleZ = 0.0;
    f64 m_scaleX = 1.0;
    f64 m_scaleY = 1.0;
    f64 m_scaleZ = 1.0;

    // 纹理
    f64 m_textureWidth = 64.0;
    f64 m_textureHeight = 32.0;
    i32 m_textureOffsetX = 0;
    i32 m_textureOffsetY = 0;

    // 镜像
    bool m_mirror = false;

    // 可见性
    bool m_visible = true;

    // 子部件
    std::vector<std::shared_ptr<ModelRenderer>> m_children;

    // 盒子数据
    std::vector<ModelBox> m_boxes;

    // ========== 矩阵工具 ==========

    /**
     * @brief 创建单位矩阵
     */
    static std::array<f64, 16> _identityMatrix();

    /**
     * @brief 矩阵乘法
     */
    static std::array<f64, 16> _multiplyMatrices(const std::array<f64, 16>& a, const std::array<f64, 16>& b);

    /**
     * @brief 创建平移矩阵
     */
    static std::array<f64, 16> _translationMatrix(f64 x, f64 y, f64 z);

    /**
     * @brief 创建绕X轴旋转矩阵
     */
    static std::array<f64, 16> _rotationXMatrix(f64 angle);

    /**
     * @brief 创建绕Y轴旋转矩阵
     */
    static std::array<f64, 16> _rotationYMatrix(f64 angle);

    /**
     * @brief 创建绕Z轴旋转矩阵
     */
    static std::array<f64, 16> _rotationZMatrix(f64 angle);

    /**
     * @brief 创建缩放矩阵
     */
    static std::array<f64, 16> _scaleMatrix(f64 x, f64 y, f64 z);

    /**
     * @brief 应用矩阵变换到顶点
     */
    static ModelVertex _transformVertex(const ModelVertex& vertex, const std::array<f64, 16>& matrix);
};

} // namespace mc::client::renderer::entity::model
