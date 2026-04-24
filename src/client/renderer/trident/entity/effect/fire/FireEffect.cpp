#include "FireEffect.hpp"
#include "common/entity/core/Entity.hpp"
#include <cmath>

namespace mc::client::renderer::entity::effect::fire {

bool FireEffect::s_initialized = false;

void FireEffect::initialize() {
    if (s_initialized) {
        return;
    }

    // 初始化火焰纹理和着色器
    // TODO: 加载火焰纹理资源

    s_initialized = true;
}

void FireEffect::cleanup() {
    if (!s_initialized) {
        return;
    }

    // 清理火焰纹理和着色器资源
    // TODO: 释放资源

    s_initialized = false;
}

bool FireEffect::isBurning(Entity& entity) {
    // 参考 MC 1.16.5 Entity.isBurning()
    // 检查实体是否有燃烧状态
    // TODO: 从实体获取燃烧状态
    (void)entity;
    return false;
}

void FireEffect::renderFire(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 EntityRenderer.renderFire()
    // 火焰渲染步骤：
    // 1. 获取实体边界框
    // 2. 在底部和两侧放置火焰四边形
    // 3. 使用动画纹理

    // TODO: 实现火焰渲染
    (void)entity;
    (void)partialTicks;
}

void FireEffect::generateFireQuad(
    f64 x, f64 y, f64 z,
    f64 width, f64 height,
    std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices
) {
    // 参考 MC 1.16.5 火焰四边形生成
    // 生成一个火焰四边形（两个三角形）
    // 火焰四边形始终面向摄像机（广告牌效果）

    u32 baseIndex = static_cast<u32>(vertices.size());

    // 四个顶点
    // 火焰纹理 UV 根据时间动画
    vertices.emplace_back(model::ModelVertex(
        x - width / 2, y, z,
        0.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    ));
    vertices.emplace_back(model::ModelVertex(
        x - width / 2, y + height, z,
        0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    ));
    vertices.emplace_back(model::ModelVertex(
        x + width / 2, y + height, z,
        1.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    ));
    vertices.emplace_back(model::ModelVertex(
        x + width / 2, y, z,
        1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    ));

    // 两个三角形
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);

    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

f64 FireEffect::computeFireOffset(f64 time, f64 seed) {
    // 计算火焰动画偏移
    // 使用正弦波创建火焰摇曳效果
    return std::sin(time * 10.0 + seed) * 0.1;
}

} // namespace mc::client::renderer::entity::effect::fire
