#include "ChestRenderer.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/block/Block.hpp"
#include <ctime>
#include <cmath>

namespace mc::client::renderer::trident::blockentity {

ChestRenderer::ChestRenderer()
    : BlockEntityRenderer<mc::blockentity::ChestEntity>()
    , m_model()
{
}

bool ChestRenderer::isChristmas() {
    // MC 1.16.5: 12月24-26日使用圣诞节纹理
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    const int month = localTime.tm_mon + 1;  // tm_mon 是 0-11
    const int day = localTime.tm_mday;

    return month == 12 && day >= 24 && day <= 26;
}

mc::client::renderer::blockentity::model::ChestModel::ChestType ChestRenderer::determineChestType(
    const mc::blockentity::ChestEntity& entity) const
{
    // MC 1.16.5: 从方块状态获取 ChestType
    // 需要 entity.getBlockState() 获取 TYPE 属性
    // ChestType: SINGLE, LEFT, RIGHT
    // 当前默认返回单箱，完整实现需要 BlockState 访问
    (void)entity;
    return mc::client::renderer::blockentity::model::ChestModel::ChestType::Single;
}

void ChestRenderer::render(
    const mc::blockentity::ChestEntity& entity,
    f32 partialTick,
    u32 light)
{
    // 获取插值后的盖子角度
    const f32 lidAngle = entity.getInterpolatedLidAngle(partialTick);

    // 如果盖子关闭且没有打开计数，跳过渲染
    if (lidAngle <= 0.001f && entity.getOpenCount() == 0) {
        // 仍然需要渲染箱体
        m_model.setLidAngle(0.0f);
    } else {
        // 应用 MC 风格的缓动函数
        m_model.setLidAngle(lidAngle);
    }

    // 设置箱子类型（单箱/双箱左/右）
    const auto chestType = determineChestType(entity);
    m_model.setChestType(chestType);

    // 生成网格数据
    std::vector<entity::model::ModelVertex> vertices;
    std::vector<u32> indices;
    m_model.generateMesh(vertices, indices);

    // 渲染管线集成说明：
    // 该渲染器生成模型顶点数据后，由上层渲染系统（BlockEntityRenderDispatcher）
    // 负责将顶点数据提交到 GPU。完整的渲染需要：
    // 1. 从方块状态获取 FACING 属性确定朝向
    // 2. 根据 isChristmas() 选择纹理变体
    // 3. 计算双箱时的光照合并
    // 这些功能依赖外部系统（方块状态、纹理系统）的实现完善后集成。
    (void)light;
    (void)vertices;
    (void)indices;
}

} // namespace mc::client::renderer::trident::blockentity
