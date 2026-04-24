#include "WolfCollarLayer.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"

namespace mc::client::renderer::entity::layer::entity {

namespace {
    // 项圈颜色 RGB 值（MC 1.16.5 DyeColor）
    const Vector3f COLLAR_COLORS[16] = {
        Vector3f(1.0f, 1.0f, 1.0f),       // 白色 (0)
        Vector3f(0.85f, 0.5f, 0.2f),      // 橙色 (1)
        Vector3f(0.8f, 0.2f, 0.6f),       // 品红色 (2)
        Vector3f(0.2f, 0.6f, 0.9f),       // 淡蓝色 (3)
        Vector3f(0.9f, 0.9f, 0.2f),       // 黄色 (4)
        Vector3f(0.4f, 0.8f, 0.2f),       // 黄绿色 (5)
        Vector3f(1.0f, 0.5f, 0.7f),       // 粉红色 (6)
        Vector3f(0.3f, 0.3f, 0.3f),       // 灰色 (7)
        Vector3f(0.5f, 0.5f, 0.5f),       // 淡灰色 (8)
        Vector3f(0.2f, 0.4f, 0.6f),       // 青色 (9)
        Vector3f(0.5f, 0.2f, 0.8f),       // 紫色 (10)
        Vector3f(0.2f, 0.3f, 0.7f),       // 蓝色 (11)
        Vector3f(0.5f, 0.3f, 0.1f),       // 棕色 (12)
        Vector3f(0.2f, 0.5f, 0.2f),       // 绿色 (13)
        Vector3f(0.6f, 0.2f, 0.2f),       // 红色 (14)
        Vector3f(0.1f, 0.1f, 0.1f),       // 黑色 (15)
    };
}

void WolfCollarLayer::render(
    ::mc::WolfEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // 参考 MC 1.16.5 WolfCollarLayer.render()
    // TODO: 渲染项圈
    // 1. 获取项圈颜色
    // 2. 渲染项圈网格
    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

bool WolfCollarLayer::shouldRender(const ::mc::WolfEntity& entity) const {
    // 只有驯服的狼才显示项圈
    if constexpr (std::is_base_of_v<::mc::WolfEntity, ::mc::WolfEntity>) {
        // return entity.isTamed();
        (void)entity;
        return true; // 暂时返回 true
    }
    return false;
}

Vector3f WolfCollarLayer::getCollarColor(const ::mc::WolfEntity& entity) {
    // 获取狼的项圈颜色
    if constexpr (std::is_base_of_v<::mc::WolfEntity, ::mc::WolfEntity>) {
        // u8 colorIndex = entity.getCollarColor();
        // if (colorIndex < 16) {
        //     return COLLAR_COLORS[colorIndex];
        // }
        (void)entity;
    }
    return COLLAR_COLORS[14]; // 默认红色
}

} // namespace mc::client::renderer::entity::layer::entity
