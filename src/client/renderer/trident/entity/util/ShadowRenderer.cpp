#include "ShadowRenderer.hpp"
#include "../pipeline/EntityPipeline.hpp"
#include "../model/core/ModelRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::util {

// 静态成员初始化
bool ShadowRenderer::s_initialized = false;
u32 ShadowRenderer::s_segments = 16;
std::vector<f32> ShadowRenderer::s_shadowVertices;
std::vector<u32> ShadowRenderer::s_shadowIndices;
pipeline::EntityMesh* ShadowRenderer::s_shadowMesh = nullptr;

// 阴影常量（参考 MC 1.16.5 EntityRendererManager.java:260）
// 阴影透明度在距离 256 格时衰减为 0
static constexpr f64 MAX_SHADOW_DISTANCE = 256.0;
static constexpr f64 SHADOW_TEX_U = 0.0;
static constexpr f64 SHADOW_TEX_V = 0.0;
static constexpr f64 SHADOW_TEX_SIZE = 32.0 / 256.0;

bool ShadowRenderer::initialize(pipeline::EntityPipeline& pipeline) {
    if (s_initialized) {
        return true;
    }

    s_segments = 16;

    // 创建阴影网格
    if (!createShadowMesh(pipeline)) {
        spdlog::error("ShadowRenderer: Failed to create shadow mesh");
        return false;
    }

    s_initialized = true;
    spdlog::info("ShadowRenderer: Initialized successfully");
    return true;
}

void ShadowRenderer::cleanup() {
    s_shadowVertices.clear();
    s_shadowIndices.clear();

    // 网格由 EntityPipeline 管理，不需要手动删除
    s_shadowMesh = nullptr;

    s_initialized = false;
    spdlog::info("ShadowRenderer: Cleaned up");
}

bool ShadowRenderer::isInitialized() {
    return s_initialized;
}

void ShadowRenderer::renderShadow(
    Entity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 shadowAlpha)
{
    // CPU 路径 - 已废弃，仅保持向后兼容
    if (!s_initialized) {
        // 没有管线时无法初始化
        return;
    }

    // 计算透明度
    f64 alpha = computeShadowAlpha(entity, partialTicks, shadowRadius, shadowAlpha);
    if (alpha <= 0.0) {
        return;
    }

    // CPU 路径无法执行实际渲染
    // 需要使用 GPU 路径
    (void)partialTicks;
    (void)shadowRadius;
    (void)shadowAlpha;
}

void ShadowRenderer::renderShadow(
    VkCommandBuffer cmd,
    ClientEntity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 shadowAlpha,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized) {
        if (!initialize(pipeline)) {
            return;
        }
    }

    if (!s_shadowMesh || s_shadowMesh->indexCount == 0) {
        return;
    }

    // 计算透明度
    f64 alpha = computeShadowAlpha(entity, partialTicks, shadowRadius, shadowAlpha);
    if (alpha <= 0.0) {
        return;
    }

    // 获取插值位置
    Vector3 posInterp = entity.getInterpolatedPosition(static_cast<f32>(partialTicks));
    f64 interpX = posInterp.x;
    f64 interpY = posInterp.y;
    f64 interpZ = posInterp.z;

    // 默认地面高度为实体当前位置下方
    f64 groundY = interpY - entity.height();

    // 简化：使用实体高度作为地面检测
    // TODO: 实际射线检测获取精确地面高度
    // 当前假设实体站在地面上

    // 计算阴影高度差（用于透明度衰减）
    f64 heightAboveGround = interpY - groundY;
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return;  // 太高，不渲染阴影
    }

    // 计算缩放因子
    f64 scale = shadowRadius;

    // 模型矩阵：单位矩阵
    std::array<f64, 16> modelMatrix = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 位置：阴影在地面上
    Vector3f position(
        static_cast<f32>(interpX),
        static_cast<f32>(groundY + 0.01),  // 略高于地面避免 z-fighting
        static_cast<f32>(interpZ)
    );

    // 绘制阴影（使用透明度）
    // 阴影颜色为半透明黑色
    Vector4f overlayColor(0.0f, 0.0f, 0.0f, static_cast<f32>(alpha));
    pipeline.drawMesh(cmd, *s_shadowMesh, modelMatrix, position, static_cast<f32>(scale),
                      overlayColor, 0.0f, 0.0f);
}

void ShadowRenderer::renderShadow(
    VkCommandBuffer cmd,
    Entity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 shadowAlpha,
    pipeline::EntityPipeline& pipeline)
{
    if (!s_initialized) {
        if (!initialize(pipeline)) {
            return;
        }
    }

    if (!s_shadowMesh || s_shadowMesh->indexCount == 0) {
        return;
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

    // 获取插值位置
    f64 prevX = entity.prevX();
    f64 prevY = entity.prevY();
    f64 prevZ = entity.prevZ();
    f64 interpX = prevX + (x - prevX) * partialTicks;
    f64 interpY = prevY + (y - prevY) * partialTicks;
    f64 interpZ = prevZ + (z - prevZ) * partialTicks;

    // 尝试获取地面高度
    f64 groundY = interpY;  // 默认假设在地面上

    // 如果实体有世界引用，尝试获取实际地面高度
    auto* world = entity.world();
    if (world) {
        // 简化：假设地面就是实体所在高度的下方
        // TODO: 实际射线检测获取精确地面高度
        // 这里使用一个简化的方法：向下扫描几个方块
        for (int dy = 0; dy <= static_cast<int>(MAX_SHADOW_DISTANCE); ++dy) {
            i32 checkY = static_cast<i32>(interpY) - dy;
            auto blockState = world->getBlockState(
                static_cast<i32>(std::floor(interpX)),
                checkY,
                static_cast<i32>(std::floor(interpZ))
            );
            if (blockState && !blockState->isAir()) {
                groundY = static_cast<f64>(checkY + 1);  // 地面高度是方块上方
                break;
            }
        }
    }

    // 计算阴影高度差（用于透明度衰减）
    f64 heightAboveGround = interpY - groundY;
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return;  // 太高，不渲染阴影
    }

    // 计算缩放因子（距离越远阴影越大但越淡）
    f64 scale = shadowRadius;

    // 模型矩阵：单位矩阵，稍后在绘制时应用位置和缩放
    std::array<f64, 16> modelMatrix = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 位置：阴影在地面上
    Vector3f position(
        static_cast<f32>(interpX),
        static_cast<f32>(groundY + 0.01),  // 略高于地面避免 z-fighting
        static_cast<f32>(interpZ)
    );

    // 绘制阴影（使用透明度）
    // 阴影颜色为半透明黑色
    Vector4f overlayColor(0.0f, 0.0f, 0.0f, static_cast<f32>(alpha));
    pipeline.drawMesh(cmd, *s_shadowMesh, modelMatrix, position, static_cast<f32>(scale),
                      overlayColor, 0.0f, 0.0f);
}

bool ShadowRenderer::createShadowMesh(pipeline::EntityPipeline& pipeline) {
    // 生成阴影圆盘顶点
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;

    const f64 radius = 1.0;

    // 中心点
    model::ModelVertex centerVertex;
    centerVertex.position = Vector3f(0.0f, 0.0f, 0.0f);
    centerVertex.texCoord = Vector2f(0.5f, 0.5f);
    centerVertex.normal = Vector3f(0.0f, 1.0f, 0.0f);
    vertices.push_back(centerVertex);

    // 圆周顶点
    for (u32 i = 0; i <= s_segments; ++i) {
        f64 angle = static_cast<f64>(i) / static_cast<f64>(s_segments) * 2.0 * 3.14159265359;
        f64 x = std::cos(angle) * radius;
        f64 z = std::sin(angle) * radius;

        model::ModelVertex vertex;
        vertex.position = Vector3f(
            static_cast<f32>(x),
            0.0f,
            static_cast<f32>(z)
        );
        vertex.texCoord = Vector2f(
            static_cast<f32>(0.5 + 0.5 * std::cos(angle)),
            static_cast<f32>(0.5 + 0.5 * std::sin(angle))
        );
        vertex.normal = Vector3f(0.0f, 1.0f, 0.0f);
        vertices.push_back(vertex);
    }

    // 创建三角形索引（扇形）
    for (u32 i = 1; i <= s_segments; ++i) {
        indices.push_back(0);      // 中心点
        indices.push_back(i);      // 当前圆周点
        indices.push_back(i + 1);  // 下一个圆周点
    }

    // 创建网格
    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::error("ShadowRenderer: Failed to create shadow mesh");
        return false;
    }

    // 存储网格（注意：这里需要管理内存）
    // TODO: 使用适当的网格管理
    static pipeline::EntityMesh shadowMeshStorage = result.value();
    s_shadowMesh = &shadowMeshStorage;

    spdlog::debug("ShadowRenderer: Created shadow mesh with {} vertices, {} indices",
                  vertices.size(), indices.size());
    return true;
}

f64 ShadowRenderer::computeShadowAlpha(
    Entity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 baseAlpha)
{
    (void)partialTicks;
    (void)shadowRadius;

    // 获取实体到地面的距离
    f64 entityY = entity.y();
    f64 groundY = entityY;  // 默认假设在地面上
    IWorld* world = entity.world();

    // 如果实体有世界引用，尝试获取实际地面高度
    if (world) {
        // 简化：向下扫描获取地面高度
        for (int dy = 0; dy <= static_cast<int>(MAX_SHADOW_DISTANCE); ++dy) {
            i32 checkY = static_cast<i32>(entityY) - dy;
            auto blockState = world->getBlockState(
                static_cast<i32>(std::floor(entity.x())),
                checkY,
                static_cast<i32>(std::floor(entity.z()))
            );
            if (blockState && !blockState->isAir()) {
                groundY = static_cast<f64>(checkY + 1);
                break;
            }
        }
    }

    f64 heightAboveGround = entityY - groundY;

    // 如果实体太高，不渲染阴影
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return 0.0;
    }

    // 计算透明度衰减
    // 参考 MC 1.16.5 EntityRendererManager.java:260
    // (1.0 - distance / 256.0) * shadowOpaque
    f64 distanceFactor = 1.0 - (heightAboveGround / MAX_SHADOW_DISTANCE);
    distanceFactor = std::max(0.0, distanceFactor);

    // 幼体阴影减半
    // 参考 MC 1.16.5 EntityRendererManager.java:366-371
    f64 sizeMultiplier = 1.0;
    if (entity.isChild()) {
        sizeMultiplier = 0.5;
    }

    // 世界亮度查询（MC 1.16.5 EntityRendererManager:398）
    f64 brightness = 1.0;
    if (world) {
        BlockPos blockPos(
            static_cast<i32>(std::floor(entity.x())),
            static_cast<i32>(groundY),
            static_cast<i32>(std::floor(entity.z()))
        );

        // 获取光照值
        u8 blockLight = world->getBlockLight(blockPos);
        u8 skyLight = world->getSkyLight(blockPos);

        // 计算天空减暗（需要时间和天气信息）
        // IWorld 接口提供 dayTime(), isRaining(), isThundering()
        i32 skyDarkening = 0;
        i64 dayTime = static_cast<i64>(world->dayTime());
        bool isRaining = world->isRaining();
        bool isThundering = world->isThundering();
        skyDarkening = InternalLightUtils::calculateSkyDarkening(dayTime, isRaining, isThundering);

        // 使用 InternalLightUtils 计算实际亮度
        i32 rawBrightness = InternalLightUtils::calculateRawBrightness(blockLight, skyLight, skyDarkening);
        brightness = static_cast<f64>(rawBrightness) / 15.0;  // 归一化到 [0, 1]
    }

    return baseAlpha * distanceFactor * sizeMultiplier * brightness;
}

f64 ShadowRenderer::computeShadowAlpha(
    ClientEntity& entity,
    f64 partialTicks,
    f64 shadowRadius,
    f64 baseAlpha)
{
    (void)partialTicks;
    (void)shadowRadius;

    // 获取相机距离用于距离衰减
    // 参考 MC 1.16.5 EntityRendererManager.java:260

    // 获取实体高度（假设站在地面上）
    f64 entityHeight = static_cast<f64>(entity.height());
    f64 heightAboveGround = entityHeight;  // 假设实体站在地面上

    // 如果实体太高，不渲染阴影
    if (heightAboveGround > MAX_SHADOW_DISTANCE) {
        return 0.0;
    }

    // 计算透明度衰减
    // 参考 MC 1.16.5 EntityRendererManager.java:260
    f64 distanceFactor = 1.0 - (heightAboveGround / MAX_SHADOW_DISTANCE);
    if (distanceFactor < 0.0) {
        distanceFactor = 0.0;
    }

    // 幼体阴影减半
    f64 sizeMultiplier = 1.0;
    if (entity.isChild()) {
        sizeMultiplier = 0.5;
    }

    // 世界亮度因子（简化）
    f64 brightness = 1.0;

    return baseAlpha * distanceFactor * sizeMultiplier * brightness;
}

void ShadowRenderer::getShadowVertices(
    f64 radius,
    u32 segments,
    std::vector<f32>& vertices,
    std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

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
