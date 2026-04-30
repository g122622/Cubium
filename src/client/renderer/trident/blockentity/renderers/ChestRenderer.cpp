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
    const std::tm* localTime = std::localtime(&now);
    if (localTime == nullptr) {
        return false;
    }

    const int month = localTime->tm_mon + 1;  // tm_mon 是 0-11
    const int day = localTime->tm_mday;

    return month == 12 && day >= 24 && day <= 26;
}

mc::client::renderer::blockentity::model::ChestModel::ChestType ChestRenderer::determineChestType(
    const mc::blockentity::ChestEntity& entity) const
{
    // TODO: 从方块状态获取 ChestType
    // 目前返回单箱
    // 需要访问 entity.getBlockState() 并获取 TYPE 属性
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

    // TODO: 实现完整的渲染流程
    // 1. 获取箱子方块状态
    // 2. 确定朝向（FACING 属性）
    // 3. 应用旋转变换
    // 4. 根据是否圣诞节选择纹理
    // 5. 获取双箱时的光照合并
    // 6. 生成网格并提交到渲染管线

    // 当前占位实现 - 模型已准备好
    // 需要集成：
    // - BlockModelCache 获取模型
    // - MatrixStack 应用变换
    // - EntityPipeline 提交网格
    // - 纹理图集获取材质

    (void)light;  // 将用于光照计算
}

} // namespace mc::client::renderer::trident::blockentity
