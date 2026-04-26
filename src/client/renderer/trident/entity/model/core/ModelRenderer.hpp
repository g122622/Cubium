#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <vector>
#include <memory>
#include <array>

namespace mc::client::renderer::entity::model {

/**
 * @brief 模型顶点（参考MC PositionTextureVertex）
 *
 * 包含位置、纹理坐标和法线信息
 */
struct ModelVertex {
    Vector3f position;   // 顶点位置
    Vector2f texCoord;   // UV坐标
    Vector3f normal;     // 法线

    ModelVertex() = default;
    ModelVertex(f64 x, f64 y, f64 z, f64 u, f64 v, f64 nx = 0, f64 ny = 0, f64 nz = 0)
        : position(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z))
        , texCoord(static_cast<f32>(u), static_cast<f32>(v))
        , normal(static_cast<f32>(nx), static_cast<f32>(ny), static_cast<f32>(nz)) {}

    ModelVertex(const Vector3f& pos, const Vector2f& tex, const Vector3f& norm)
        : position(pos), texCoord(tex), normal(norm) {}
};

/**
 * @brief 纹理四边形（参考MC TexturedQuad）
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
                 f64 u1, f64 v1, f64 u2, f64 v2,
                 f64 texWidth, f64 texHeight,
                 const Vector3f& normal,
                 bool mirror = false);
};

/**
 * @brief 模型盒子（参考MC ModelBox）
 *
 * 每个盒子有6个面，每面是一个TexturedQuad。
 * UV坐标根据纹理偏移自动计算。
 */
struct ModelBox {
    f64 posX1, posY1, posZ1;  // 最小角
    f64 posX2, posY2, posZ2;  // 最大角
    std::array<TexturedQuad, 6> quads;  // 6个面：东、西、北、下、上、南

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
    ModelBox(i32 texOffX, i32 texOffY,
             f64 x, f64 y, f64 z,
             f64 width, f64 height, f64 depth,
             f64 deltaX = 0.0f, f64 deltaY = 0.0f, f64 deltaZ = 0.0f,
             f64 texWidth = 64.0f, f64 texHeight = 32.0f,
             bool mirror = false);
};

/**
 * @brief 模型部件
 *
 * 代表模型的一个部分（如头部、身体、腿等）。
 * 包含位置、旋转、缩放以及子部件。
 *
 * 参考 MC 1.16.5 ModelRenderer
 */
class ModelRenderer {
public:
    /**
     * @brief 构造函数
     * @param name 部件名称（用于调试）
     */
    explicit ModelRenderer(const String& name = "");
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
    void setOffset(f64 x, f64 y, f64 z) {
        m_offsetX = x;
        m_offsetY = y;
        m_offsetZ = z;
    }

    /**
     * @brief 设置旋转点
     */
    void setRotationPoint(f64 x, f64 y, f64 z) {
        m_rotationPointX = x;
        m_rotationPointY = y;
        m_rotationPointZ = z;
    }

    /**
     * @brief 设置旋转角度（弧度）
     */
    void setRotation(f64 x, f64 y, f64 z) {
        m_rotateAngleX = x;
        m_rotateAngleY = y;
        m_rotateAngleZ = z;
    }

    /**
     * @brief 设置缩放
     */
    void setScale(f64 x, f64 y, f64 z) {
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
    ModelRenderer& addBox(f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth, f64 delta = 0.0f);

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
    ModelRenderer& addBox(i32 textureOffsetX, i32 textureOffsetY,
                          f64 x, f64 y, f64 z,
                          f64 width, f64 height, f64 depth,
                          f64 delta = 0.0f);

    /**
     * @brief 添加一个盒子（带镜像选项）
     */
    ModelRenderer& addBox(f64 x, f64 y, f64 z,
                          f64 width, f64 height, f64 depth,
                          bool mirror, f64 delta = 0.0f);

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
    void addChild(std::shared_ptr<ModelRenderer> child) {
        m_children.push_back(child);
    }

    /**
     * @brief 创建并添加子部件
     * @param name 子部件名称
     * @return 创建的子部件
     */
    std::shared_ptr<ModelRenderer> createChild(const String& name = "");

    // ========== 网格生成 ==========

    /**
     * @brief 生成渲染网格
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param scale 缩放因子（默认 1/16）
     */
    void generateMesh(std::vector<ModelVertex>& vertices,
                      std::vector<u32>& indices,
                      f64 scale = 1.0f / 16.0f) const;

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
                      f64 scale = 1.0f / 16.0f) const;

    // ========== 渲染（遗留接口，未来移除） ==========

    /**
     * @brief 渲染模型
     * @param scale 缩放因子
     */
    void render(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 渲染时不进行旋转
     * @param scale 缩放因子
     */
    void renderNoRotate(f64 scale = 1.0f / 16.0f);

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
    [[nodiscard]] const String& name() const { return m_name; }

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
    String m_name;

    // 变换
    f64 m_offsetX = 0.0f;
    f64 m_offsetY = 0.0f;
    f64 m_offsetZ = 0.0f;
    f64 m_rotationPointX = 0.0f;
    f64 m_rotationPointY = 0.0f;
    f64 m_rotationPointZ = 0.0f;
    f64 m_rotateAngleX = 0.0f;
    f64 m_rotateAngleY = 0.0f;
    f64 m_rotateAngleZ = 0.0f;
    f64 m_scaleX = 1.0f;
    f64 m_scaleY = 1.0f;
    f64 m_scaleZ = 1.0f;

    // 纹理
    f64 m_textureWidth = 64.0f;
    f64 m_textureHeight = 32.0f;
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
    static std::array<f64, 16> identityMatrix();

    /**
     * @brief 矩阵乘法
     */
    static std::array<f64, 16> multiplyMatrices(const std::array<f64, 16>& a, const std::array<f64, 16>& b);

    /**
     * @brief 创建平移矩阵
     */
    static std::array<f64, 16> translationMatrix(f64 x, f64 y, f64 z);

    /**
     * @brief 创建绕X轴旋转矩阵
     */
    static std::array<f64, 16> rotationXMatrix(f64 angle);

    /**
     * @brief 创建绕Y轴旋转矩阵
     */
    static std::array<f64, 16> rotationYMatrix(f64 angle);

    /**
     * @brief 创建绕Z轴旋转矩阵
     */
    static std::array<f64, 16> rotationZMatrix(f64 angle);

    /**
     * @brief 创建缩放矩阵
     */
    static std::array<f64, 16> scaleMatrix(f64 x, f64 y, f64 z);

    /**
     * @brief 应用矩阵变换到顶点
     */
    static ModelVertex transformVertex(const ModelVertex& vertex, const std::array<f64, 16>& matrix);
};

} // namespace mc::client::renderer::entity::model
