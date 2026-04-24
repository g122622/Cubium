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

// 相机位置（需要从渲染系统设置）
static Vector3d s_cameraPosition(0.0, 0.0, 0.0);

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

    // 计算到相机的距离
    Vector3d toCamera = s_cameraPosition - position;
    f64 distanceToCamera = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);

    // 检查是否应该渲染
    if (!shouldRenderNameTag(entity, distanceToCamera)) {
        return;
    }

    // 计算缩放
    f64 scale = calculateScale(distanceToCamera);

    // 参考 MC 1.16.5 NameTagRenderer.renderNameTag()
    // 渲染名称标签需要：
    // 1. 创建面向相机的billboard变换矩阵
    // 2. 渲染文本（使用FontRenderer）
    // 3. 可选：渲染背景面板

    // 当前暂不执行实际渲染，等待文本渲染系统支持
    // 需要的功能：
    // - FontRenderer支持3D空间中的文本渲染
    // - billboard变换（始终面向相机）
    // - 深度测试配置

    (void)position;
    (void)scale;
}

bool NameTagRenderer::shouldRenderNameTag(
    Entity& entity,
    f64 distanceToCamera
) {
    // 检查距离
    if (distanceToCamera > s_maxDistance * s_maxDistance) {
        return false;
    }

    // 检查实体是否有自定义名称或是否被命名
    // 参考 MC 1.16.5 EntityRenderer.canRenderName()
    const String& customName = entity.customName();
    bool hasCustomName = !customName.empty();
    bool isCustomNameVisible = entity.isCustomNameVisible();

    // 如果有自定义名称且设置为可见，总是渲染
    if (hasCustomName && isCustomNameVisible) {
        return true;
    }

    // 其他情况下，检查实体是否被玩家瞄准
    // TODO: 实现瞄准检测

    return hasCustomName;
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
    // 参考 MC 1.16.5 EntityRenderer.getRenderOffset()
    // 名称标签位置在实体上方

    // 获取位置
    f64 x = entity.x();
    f64 y = entity.y();
    f64 z = entity.z();

    // 在实体高度之上
    f64 height = static_cast<f64>(entity.height());
    f64 nameTagY = y + height + HEIGHT_OFFSET;

    // 如果实体正在蹲伏，调整高度
    // TODO: 从实体获取蹲伏状态
    // if (entity.isCrouching()) {
    //     nameTagY -= 0.25;
    // }

    (void)partialTicks;
    return Vector3d(x, nameTagY, z);
}

f64 NameTagRenderer::calculateScale(f64 distanceToCamera) {
    // 参考 MC 1.16.5: 名称标签使用固定缩放
    // MC 1.16.5 不随距离缩放名称标签

    // 如果距离太近，稍微放大
    if (distanceToCamera < 1.0) {
        return s_scale * 1.5;
    }

    return s_scale;
}

} // namespace mc::client::renderer::entity::util
