#include "ShadowRenderer.hpp"
#include "common/entity/core/Entity.hpp"
#include <cmath>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
bool ShadowRenderer::s_initialized = false;
u32 ShadowRenderer::s_segments = 16;
std::vector<f32> ShadowRenderer::s_shadowVertices;
std::vector<u32> ShadowRenderer::s_shadowIndices;

// 阴影纹理UV坐标（参考MC 1.16.5）
static constexpr f64 SHADOW_TEX_U = 0.0;
static constexpr f64 SHADOW_TEX_V = 0.0;
static constexpr f64 SHADOW_TEX_SIZE = 32.0 / 256.0;  // 阴影纹理在图集中的大小

void ShadowRenderer::initialize(u32 segments) {
    if (s_initialized) {
        return;
    }

    s_segments = segments;
    getShadowVertices(1.0, s_segments, s_shadowVertices, s_shadowIndices);
    s_initialized = true;
}

void ShadowRenderer::cleanup() {
    s_shadowVertices.clear();
    s_shadowIndices.clear();
    s_initialized = false;
}

bool ShadowRenderer::isInitialized() {
    return s_initialized;
}

void ShadowRenderer::renderShadow(
    Entity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 shadowAlpha
) {
    if (!s_initialized) {
        initialize();
    }

    // 计算透明度
    f64 alpha = computeShadowAlpha(entity, partialTicks, shadowRadius, shadowAlpha);
    if (alpha <= 0.0) {
        return;
    }

    // 获取实体位置
    f64 x = entity.x();
    f64 y = entity.y();
    f64 z = entity.z();

    // 参考 MC 1.16.5 EntityRenderer.renderShadow()
    // 阴影需要渲染在地面上
    // TODO: 使用射线检测获取实际地面高度
    // 当前简化实现：假设实体在地面上
    f64 groundY = y;

    // 参考 MC 1.16.5：阴影渲染需要：
    // 1. 绑定阴影纹理（在实体纹理图集中）
    // 2. 设置变换矩阵（位置、缩放）
    // 3. 绘制圆盘网格
    // 4. 应用透明度混合

    // 当前暂不执行实际渲染，等待渲染管线支持
    // 需要的渲染管线功能：
    // - 实体渲染管线支持透明材质
    // - 阴影纹理绑定
    // - 实例化渲染支持

    (void)x;
    (void)groundY;
    (void)z;
    (void)shadowRadius;
    (void)alpha;
}

f64 ShadowRenderer::computeShadowAlpha(
    Entity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 baseAlpha
) {
    (void)partialTicks;

    // 参考 MC 1.16.5 EntityRenderer.getShadowOpacity()
    // 阴影透明度随高度衰减
    // 基础透明度 * (1 - height / maxDistance)

    // 获取实体到地面的距离
    // TODO: 实际射线检测
    // 当前使用简化计算
    f64 entityHeight = static_cast<f64>(entity.height());

    // 最大阴影距离（MC 1.16.5 默认值）
    static constexpr f64 MAX_SHADOW_DISTANCE = 16.0;

    // 如果实体太高，不渲染阴影
    if (entityHeight > MAX_SHADOW_DISTANCE) {
        return 0.0;
    }

    // 计算透明度衰减
    f64 heightFactor = 1.0 - (entityHeight / MAX_SHADOW_DISTANCE);
    if (heightFactor < 0.0) {
        heightFactor = 0.0;
    }

    // 考虑阴影半径的影响
    // MC 1.16.5: 更大的阴影半径会有更平滑的边缘
    f64 radiusFactor = shadowRadius > 0.0 ? std::min(shadowRadius / 0.5, 1.0) : 1.0;

    return baseAlpha * heightFactor * radiusFactor;
}

void ShadowRenderer::getShadowVertices(
    f64 radius,
    u32 segments,
    std::vector<f32>& vertices,
    std::vector<u32>& indices
) {
    vertices.clear();
    indices.clear();

    // 参考 MC 1.16.5: 阴影是一个水平圆盘
    // 顶点格式：位置(3) + 纹理坐标(2) + 法线(3)

    // 中心点
    vertices.push_back(0.0f);  // x
    vertices.push_back(0.0f);  // y
    vertices.push_back(0.0f);  // z
    vertices.push_back(0.5f);  // u（纹理中心）
    vertices.push_back(0.5f);  // v
    vertices.push_back(0.0f);  // nx
    vertices.push_back(1.0f);  // ny（向上）
    vertices.push_back(0.0f);  // nz

    // 圆周顶点
    for (u32 i = 0; i <= segments; ++i) {
        f64 angle = static_cast<f64>(i) / static_cast<f64>(segments) * 2.0 * 3.14159265359;
        f64 x = std::cos(angle) * radius;
        f64 z = std::sin(angle) * radius;

        vertices.push_back(static_cast<f32>(x));   // x
        vertices.push_back(0.0f);                    // y
        vertices.push_back(static_cast<f32>(z));   // z
        // UV坐标映射到阴影纹理
        vertices.push_back(static_cast<f32>(0.5 + 0.5 * std::cos(angle)));  // u
        vertices.push_back(static_cast<f32>(0.5 + 0.5 * std::sin(angle)));  // v
        vertices.push_back(0.0f);  // nx
        vertices.push_back(1.0f);  // ny
        vertices.push_back(0.0f);  // nz
    }

    // 创建三角形索引（扇形）
    for (u32 i = 1; i <= segments; ++i) {
        indices.push_back(0);      // 中心点
        indices.push_back(i);      // 当前圆周点
        indices.push_back(i + 1);  // 下一个圆周点
    }
}

} // namespace mc::client::renderer::entity::util
