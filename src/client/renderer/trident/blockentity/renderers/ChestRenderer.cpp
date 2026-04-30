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

    // 后续渲染管线集成步骤：
    // 1. 获取箱子方块状态确定朝向（FACING 属性）
    // 2. 应用旋转变换（绕 Y 轴）
    // 3. 根据是否圣诞节选择纹理
    // 4. 获取双箱时的光照合并
    // 5. 提交顶点数据到渲染管线
    // 注意：完整渲染管线集成需要在 EntityPipeline 或专用 BlockEntityPipeline 中实现

    (void)light;
    (void)vertices;
    (void)indices;
}

} // namespace mc::client::renderer::trident::blockentity
