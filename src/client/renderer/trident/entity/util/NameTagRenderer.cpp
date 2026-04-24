#include "NameTagRenderer.hpp"
#include "common/entity/core/Entity.hpp"
#include <cmath>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
f64 NameTagRenderer::s_maxDistance = DEFAULT_MAX_DISTANCE;
f64 NameTagRenderer::s_scale = DEFAULT_SCALE;
bool NameTagRenderer::s_showBackground = true;
u8 NameTagRenderer::s_bgColorR = 0;
u8 NameTagRenderer::s_bgColorG = 0;
u8 NameTagRenderer::s_bgColorB = 0;
u8 NameTagRenderer::s_bgColorA = 128;

void NameTagRenderer::renderNameTag(
    Entity& entity,
    const String& displayName,
    f64 partialTicks
) {
    if (displayName.empty()) {
        return;
    }

    // 计算名称标签位置
    Vector3d position = calculateNameTagPosition(entity, partialTicks);

    // 计算缩放
    // TODO: 需要相机位置来计算实际距离
    f64 scale = s_scale;

    // TODO: 实际渲染
    // 需要使用文本渲染器绘制名称
    // 1. 计算文本宽度
    // 2. 设置变换矩阵（位置、旋转面向相机、缩放）
    // 3. 绘制背景（如果启用）
    // 4. 绘制文本

    (void)position;
    (void)scale;
}

bool NameTagRenderer::shouldRenderNameTag(
    Entity& entity,
    f64 distanceToCamera
) {
    // 检查距离
    if (distanceToCamera > s_maxDistance) {
        return false;
    }

    // 检查实体是否有自定义名称
    // TODO: 检查实体是否有自定义名称或是否被命名
    (void)entity;
    return true;
}

void NameTagRenderer::setMaxDistance(f64 distance) {
    s_maxDistance = distance;
}

f64 NameTagRenderer::maxDistance() {
    return s_maxDistance;
}

void NameTagRenderer::setScale(f64 scale) {
    s_scale = scale;
}

void NameTagRenderer::setBackgroundColor(u8 r, u8 g, u8 b, u8 a) {
    s_bgColorR = r;
    s_bgColorG = g;
    s_bgColorB = b;
    s_bgColorA = a;
}

void NameTagRenderer::setShowBackground(bool show) {
    s_showBackground = show;
}

Vector3d NameTagRenderer::calculateNameTagPosition(
    Entity& entity,
    f64 partialTicks
) {
    // 获取插值位置
    // TODO: 使用 getInterpolatedPosition(partialTicks)
    f64 x = entity.x();
    f64 y = entity.y();
    f64 z = entity.z();

    // 在实体高度之上
    f64 height = static_cast<f64>(entity.height());
    f64 nameTagY = y + height + HEIGHT_OFFSET;

    return Vector3d(x, nameTagY, z);
}

f64 NameTagRenderer::calculateScale(f64 distanceToCamera) {
    // 名称标签随距离缩放
    // 参考 MC 1.16.5: 使用对数缩放
    if (distanceToCamera <= 1.0) {
        return s_scale;
    }

    // 远距离时稍微放大以保持可读性
    f64 distanceScale = 1.0 + std::log(distanceToCamera) * 0.1;
    return s_scale * distanceScale;
}

} // namespace mc::client::renderer::entity::util
