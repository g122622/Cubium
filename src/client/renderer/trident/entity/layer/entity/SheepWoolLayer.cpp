#include "SheepWoolLayer.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"

namespace mc::client::renderer::entity::layer::entity {

namespace {
    // 羊毛颜色 RGB 值（MC 1.16.5 DyeColor）
    const Vector3f WOOL_COLORS[16] = {
        Vector3f(1.0f, 1.0f, 1.0f),       // 白色 (0)
        Vector3f(0.85f, 0.85f, 0.85f),    // 橙色 (1)
        Vector3f(0.8f, 0.6f, 1.0f),       // 品红色 (2)
        Vector3f(0.6f, 0.8f, 1.0f),       // 淡蓝色 (3)
        Vector3f(1.0f, 1.0f, 0.5f),       // 黄色 (4)
        Vector3f(0.5f, 1.0f, 0.5f),       // 黄绿色 (5)
        Vector3f(1.0f, 0.6f, 0.6f),       // 粉红色 (6)
        Vector3f(0.5f, 0.5f, 0.5f),       // 灰色 (7)
        Vector3f(0.3f, 0.3f, 0.3f),       // 淡灰色 (8)
        Vector3f(0.4f, 0.3f, 0.2f),       // 青色 (9)
        Vector3f(0.3f, 0.3f, 0.6f),       // 紫色 (10)
        Vector3f(0.2f, 0.3f, 0.5f),       // 蓝色 (11)
        Vector3f(0.4f, 0.3f, 0.2f),       // 棕色 (12)
        Vector3f(0.2f, 0.4f, 0.2f),       // 绿色 (13)
        Vector3f(0.6f, 0.2f, 0.2f),       // 红色 (14)
        Vector3f(0.1f, 0.1f, 0.1f),       // 黑色 (15)
    };
}

template<typename TEntity>
void SheepWoolLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // 参考 MC 1.16.5 SheepWoolLayer.render()
    // 1. 检查是否有羊毛
    // 2. 获取羊毛颜色
    // 3. 渲染羊毛模型层

    // TODO: 实际渲染羊毛网格
    // 需要获取父模型的部件并渲染羊毛层
    // 目前只实现基础逻辑框架
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
    (void)entity;
}

template<typename TEntity>
bool SheepWoolLayer<TEntity>::shouldRender(const TEntity& entity) const {
    return checkHasWool(entity);
}

template<typename TEntity>
Vector3f SheepWoolLayer<TEntity>::getWoolColor(const TEntity& entity) {
    // 尝试将 entity 转换为 SheepEntity 以获取颜色
    // 如果不是 SheepEntity，返回默认白色
    if constexpr (std::is_base_of_v<::mc::SheepEntity, TEntity>) {
        u8 colorIndex = entity.getWoolColor();
        if (colorIndex < 16) {
            return WOOL_COLORS[colorIndex];
        }
    }
    return Vector3f(1.0f, 1.0f, 1.0f);  // 默认白色
}

template<typename TEntity>
bool SheepWoolLayer<TEntity>::checkHasWool(const TEntity& entity) {
    // 尝试将 entity 转换为 SheepEntity 以检查羊毛状态
    if constexpr (std::is_base_of_v<::mc::SheepEntity, TEntity>) {
        return entity.hasWool();
    }
    return false;
}

// 显式实例化
template class SheepWoolLayer<::mc::LivingEntity>;
template class SheepWoolLayer<::mc::SheepEntity>;

} // namespace mc::client::renderer::entity::layer::entity
