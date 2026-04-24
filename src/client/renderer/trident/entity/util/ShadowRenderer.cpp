#include "ShadowRenderer.hpp"
#include "common/entity/core/Entity.hpp"
#include <cmath>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
bool ShadowRenderer::s_initialized = false;
u32 ShadowRenderer::s_segments = 16;
std::vector<f32> ShadowRenderer::s_shadowVertices;
std::vector<u32> ShadowRenderer::s_shadowIndices;

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
    f64 y = entity.y();  // TODO: 获取地面高度
    f64 z = entity.z();

    // TODO: 实际渲染
    // 需要绑定阴影着色器、设置变换矩阵、绘制阴影圆盘
    (void)x;
    (void)y;
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

    // 阴影透明度随高度衰减
    // 参考 MC 1.16.5: alpha = baseAlpha * (1 - height / maxShadowDistance)
    // maxShadowDistance 通常为 16

    // TODO: 获取实体到地面的实际距离
    // 暂时使用实体高度
    f64 entityHeight = static_cast<f64>(entity.height());
    f64 maxShadowDistance = 16.0;

    // 如果实体太高，不渲染阴影
    if (entityHeight > maxShadowDistance) {
        return 0.0;
    }

    // 计算透明度
    f64 distanceFactor = 1.0 - (entityHeight / maxShadowDistance);
    return baseAlpha * distanceFactor * shadowRadius;
}

void ShadowRenderer::getShadowVertices(
    f64 radius,
    u32 segments,
    std::vector<f32>& vertices,
    std::vector<u32>& indices
) {
    vertices.clear();
    indices.clear();

    // 创建圆盘顶点
    // 中心点
    vertices.push_back(0.0f);  // x
    vertices.push_back(0.0f);  // y
    vertices.push_back(0.0f);  // z
    vertices.push_back(0.5f);  // u
    vertices.push_back(0.5f);  // v
    vertices.push_back(0.0f);  // nx
    vertices.push_back(1.0f);  // ny
    vertices.push_back(0.0f);  // nz

    // 圆周顶点
    for (u32 i = 0; i <= segments; ++i) {
        f64 angle = static_cast<f64>(i) / static_cast<f64>(segments) * 2.0 * 3.14159265359;
        f64 x = std::cos(angle) * radius;
        f64 z = std::sin(angle) * radius;

        vertices.push_back(static_cast<f32>(x));   // x
        vertices.push_back(0.0f);                    // y
        vertices.push_back(static_cast<f32>(z));   // z
        vertices.push_back(static_cast<f32>(0.5 + 0.5 * std::cos(angle)));  // u
        vertices.push_back(static_cast<f32>(0.5 + 0.5 * std::sin(angle)));  // v
        vertices.push_back(0.0f);  // nx
        vertices.push_back(1.0f);  // ny
        vertices.push_back(0.0f);  // nz
    }

    // 创建三角形索引
    for (u32 i = 1; i <= segments; ++i) {
        indices.push_back(0);      // 中心点
        indices.push_back(i);      // 当前圆周点
        indices.push_back(i + 1);  // 下一个圆周点
    }
}

} // namespace mc::client::renderer::entity::util
