#include "FireEffect.hpp"
#include "common/entity/core/Entity.hpp"
#include <cmath>

namespace mc::client::renderer::entity::effect::fire {

bool FireEffect::s_initialized = false;

// 火焰纹理UV坐标（参考MC 1.16.5）
static constexpr f64 FIRE_TEX_U_OFFSET = 0.0;
static constexpr f64 FIRE_TEX_V_OFFSET = 0.0;
static constexpr f64 FIRE_TEX_WIDTH = 16.0 / 256.0;
static constexpr f64 FIRE_TEX_HEIGHT = 16.0 / 256.0;

void FireEffect::initialize() {
    if (s_initialized) {
        return;
    }

    // 参考 MC 1.16.5 火焰纹理初始化
    // 火焰纹理位于 textures/entity/fire_layer_X.png
    // 需要加载两层火焰纹理用于动画
    //
    // 当前等待纹理系统支持：
    // - 火焰纹理加载
    // - 动画纹理支持

    s_initialized = true;
}

void FireEffect::cleanup() {
    if (!s_initialized) {
        return;
    }

    // 清理火焰纹理和着色器资源
    // 当前无需清理，等待纹理系统支持后实现

    s_initialized = false;
}

bool FireEffect::isBurning(Entity& entity) {
    // 参考 MC 1.16.5 Entity.isBurning()
    // 检查实体是否有燃烧状态
    // 燃烧状态来源：
    // 1. 火焰效果 (StatusEffectType::FIRE)
    // 2. 岩浆接触
    // 3. 燃烧方块接触

    // TODO: 从实体获取燃烧状态
    // 需要 Entity::isOnFire() 方法
    (void)entity;
    return false;
}

void FireEffect::renderFire(Entity& entity, f64 partialTicks) {
    if (!isBurning(entity)) {
        return;
    }

    // 参考 MC 1.16.5 EntityRenderer.renderFire()
    // 火焰渲染步骤：
    // 1. 获取实体边界框
    // 2. 在底部放置一层火焰
    // 3. 在四周放置火焰（共5个火焰四边形）
    // 4. 使用动画纹理（根据时间切换纹理帧）
    // 5. 火焰向上飘动动画

    // 获取实体尺寸
    f64 width = static_cast<f64>(entity.width());
    f64 height = static_cast<f64>(entity.height());
    f64 x = entity.x();
    f64 y = entity.y();
    f64 z = entity.z();

    // 计算火焰尺寸
    f64 fireWidth = width * 1.4;
    f64 fireHeight = height * 1.4;

    // 火焰偏移（摇曳动画）
    f64 time = static_cast<f64>(entity.ticksExisted()) + partialTicks;
    f64 offsetX = computeFireOffset(time, x * 1000.0);
    f64 offsetZ = computeFireOffset(time, z * 1000.0);

    // 生成火焰四边形
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    // 底部火焰
    generateFireQuad(x + offsetX, y, z + offsetZ, fireWidth, fireHeight, vertices, indices);

    // 当前等待渲染管线支持：
    // - 火焰纹理绑定
    // - 透明混合模式
    // - 广告牌渲染（火焰面向相机）

    (void)partialTicks;
    (void)fireWidth;
    (void)fireHeight;
    (void)vertices;
    (void)indices;
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
    // 参考 MC 1.16.5 使用简单噪声函数
    return std::sin(time * 0.3 + seed) * 0.1;
}

} // namespace mc::client::renderer::entity::effect::fire
