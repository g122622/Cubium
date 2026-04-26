#include "HeldItemLayer.hpp"
#include "../../core/AnimationContext.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../../../item/ItemMeshBuilder.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/math/Vector4.hpp"
#include "common/util/math/MathConstants.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include <cmath>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::equipment {

// 静态网格缓存（用于手持物品）
static std::unordered_map<u32, pipeline::EntityMesh> s_heldItemMeshCache;
static bool s_cacheInitialized = false;

template<typename TEntity>
void HeldItemLayer<TEntity>::renderPipeline(
    TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // MC 1.16.5 HeldItemLayer:24-26
    // 首先根据主手判断哪只手渲染哪个物品
    // boolean flag = entity.getPrimaryHand() == HandSide.RIGHT;
    // ItemStack offhand = flag ? entity.getHeldItemOffhand() : entity.getHeldItemMainhand();
    // ItemStack mainhand = flag ? entity.getHeldItemMainhand() : entity.getHeldItemOffhand();

    bool isRightHanded = true;  // 默认右手为主手
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        isRightHanded = entity.isRightHanded();
    }

    // MC 1.16.5 逻辑：
    // 如果右手为主手：右手渲染主手物品，左手渲染副手物品
    // 如果左手为主手：右手渲染副手物品，左手渲染主手物品
    mc::Hand rightHandSlot = isRightHanded ? mc::Hand::MainHand : mc::Hand::OffHand;
    mc::Hand leftHandSlot = isRightHanded ? mc::Hand::OffHand : mc::Hand::MainHand;

    // 渲染右手物品（使用 ThirdPersonRightHand 变换）
    renderHandItemPipeline(entity, rightHandSlot, mc::HandSide::Right, cmd, context, pipeline);

    // 渲染左手物品（使用 ThirdPersonLeftHand 变换）
    renderHandItemPipeline(entity, leftHandSlot, mc::HandSide::Left, cmd, context, pipeline);
}

template<typename TEntity>
void HeldItemLayer<TEntity>::render(
    TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // CPU 路径 - 已废弃，仅调用 GPU 路径的空实现
    // 这个方法保留用于向后兼容
    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
    // 注意：CPU 路径不再实现，需要通过 GPU 管线渲染
}

template<typename TEntity>
bool HeldItemLayer<TEntity>::shouldRender(const TEntity& entity) const {
    // 检查是否有手持物品
    const ItemStack* mainHand = getHeldItem(entity, mc::Hand::MainHand);
    const ItemStack* offHand = getHeldItem(entity, mc::Hand::OffHand);

    if (mainHand && !mainHand->isEmpty()) return true;
    if (offHand && !offHand->isEmpty()) return true;

    return false;
}

template<typename TEntity>
void HeldItemLayer<TEntity>::renderHandItemPipeline(
    TEntity& entity,
    mc::Hand hand,
    mc::HandSide handSide,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    const ItemStack* item = getHeldItem(entity, hand);
    if (!item || item->isEmpty()) {
        return;
    }

    // 计算手持物品的变换矩阵
    std::array<f64, 16> itemTransform;
    computeItemTransform(
        handSide,
        static_cast<f32>(context.limbSwing),
        static_cast<f32>(context.limbSwingAmount),
        static_cast<f32>(context.swingProgress),
        itemTransform
    );

    // 获取物品ID用于缓存
    u32 itemId = static_cast<u32>(item->getItem()->itemId());

    // 获取或创建物品网格
    auto it = s_heldItemMeshCache.find(itemId);
    if (it == s_heldItemMeshCache.end()) {
        // 构建物品网格
        auto transformType = (handSide == mc::HandSide::Right)
            ? item::ItemTransformType::ThirdPersonRightHand
            : item::ItemTransformType::ThirdPersonLeftHand;

        auto [vertices, indices] = item::ItemMeshBuilder::buildHeldItemMesh(*item, transformType);

        if (vertices.empty() || indices.empty()) {
            return;
        }

        // 创建 GPU 网格
        auto result = pipeline.createMesh(vertices, indices);
        if (!result.success()) {
            spdlog::warn("HeldItemLayer: Failed to create mesh for item {}",
                         item->getItem()->itemLocation().toString());
            return;
        }

        s_heldItemMeshCache[itemId] = std::move(result.value());
        it = s_heldItemMeshCache.find(itemId);
    }

    if (it == s_heldItemMeshCache.end() || it->second.indexCount == 0) {
        return;
    }

    // 获取实体的世界位置
    Vector3f entityPos(
        static_cast<f32>(entity.x()),
        static_cast<f32>(entity.y()),
        static_cast<f32>(entity.z())
    );

    // 结合实体位置和物品变换
    // 物品变换是相对于实体身体坐标系的
    std::array<f64, 16> worldTransform = itemTransform;

    // 绘制物品网格
    // 使用实体的 hurtTime 和 deathTime 来传递受伤效果
    f32 hurtTime = 0.0f;
    f32 deathTime = 0.0f;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
        deathTime = static_cast<f32>(entity.deathTime());
    }

    pipeline.drawMesh(cmd, it->second, worldTransform, entityPos, 1.0,
                      Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);

    spdlog::trace("HeldItemLayer: Rendered item '{}' in {} hand (handSlot={})",
                  item->getItem()->itemLocation().toString(),
                  handSide == mc::HandSide::Right ? "right" : "left",
                  hand == mc::Hand::MainHand ? "main" : "off");
}

template<typename TEntity>
void HeldItemLayer<TEntity>::renderHandItem(
    TEntity& entity,
    mc::Hand hand,
    mc::HandSide handSide,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 scale)
{
    // CPU 路径 - 已废弃
    const ItemStack* item = getHeldItem(entity, hand);
    if (!item || item->isEmpty()) {
        return;
    }

    // 仅作为占位符
    (void)handSide;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)scale;
}

template<typename TEntity>
const ItemStack* HeldItemLayer<TEntity>::getHeldItem(
    const TEntity& entity,
    mc::Hand hand) const
{
    // 从实体获取手持物品
    // LivingEntity 有 getEquipment 方法
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        using EquipmentSlot = ::mc::EquipmentSlot;
        auto slot = (hand == mc::Hand::MainHand) ? EquipmentSlot::MainHand : EquipmentSlot::OffHand;
        return &entity.getEquipment(slot);
    }
    return nullptr;
}

template<typename TEntity>
void HeldItemLayer<TEntity>::computeItemTransform(
    mc::HandSide handSide,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 swingProgress,
    std::array<f64, 16>& outMatrix)
{
    // 初始化为单位矩阵
    outMatrix = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // 参考 MC 1.16.5 HeldItemLayer.java:45-48
    // matrixStackIn.rotate(Vector3f.XP.rotationDegrees(-90.0F));
    // matrixStackIn.rotate(Vector3f.YP.rotationDegrees(180.0F));
    // boolean flag = p_229135_4_ == HandSide.LEFT;
    // matrixStackIn.translate((double)((float)(flag ? -1 : 1) / 16.0F), 0.125D, -0.625D);

    // 首先应用 X 轴 -90° 旋转
    // rotateX(-90°) 后 Y 轴变为 -Z 轴
    f64 cosX = std::cos(-mc::math::PI_DOUBLE * 0.5);  // cos(-90°)
    f64 sinX = std::sin(-mc::math::PI_DOUBLE * 0.5);  // sin(-90°)

    // 然后应用 Y 轴 180° 旋转
    f64 cosY = std::cos(mc::math::PI_DOUBLE);  // cos(180°)
    f64 sinY = std::sin(mc::math::PI_DOUBLE);  // sin(180°)

    // 组合旋转矩阵：先 X 旋转再 Y 旋转
    // R = Ry(180°) * Rx(-90°)
    // 简化后：
    // cosY=-1, sinY=0, cosX=0, sinX=-1
    // 结果矩阵：
    // [0, 0, 1, 0]
    // [0, -1, 0, 0]
    // [1, 0, 0, 0]
    // [0, 0, 0, 1]
    outMatrix[0] = 0.0;
    outMatrix[1] = 0.0;
    outMatrix[2] = 1.0;
    outMatrix[4] = 0.0;
    outMatrix[5] = -1.0;
    outMatrix[6] = 0.0;
    outMatrix[8] = 1.0;
    outMatrix[9] = 0.0;
    outMatrix[10] = 0.0;

    // MC 1.16.5: translate((float)(flag ? -1 : 1) / 16.0F, 0.125D, -0.625D)
    // flag = (handSide == HandSide.LEFT) in MC terms
    // 所以 Right 时 x = +1/16, Left 时 x = -1/16
    bool isLeftHand = (handSide == mc::HandSide::Left);
    f64 xOffset = isLeftHand ? -1.0 / 16.0 : 1.0 / 16.0;
    f64 yOffset = 0.125;
    f64 zOffset = -0.625;

    // 应用位置偏移（在旋转后的坐标系中）
    outMatrix[3] = xOffset;
    outMatrix[7] = yOffset;
    outMatrix[11] = zOffset;

    // 步态动画影响
    if (limbSwingAmount > 0.001f) {
        f64 armSwing = std::sin(static_cast<f64>(limbSwing) * 0.5) * static_cast<f64>(limbSwingAmount) * 0.5;
        // 手臂摆动时物品跟随移动
        // 这里简化处理，实际应该根据手臂骨骼位置计算
        (void)armSwing;  // TODO: 应用摆动动画
    }

    // 挥动手臂动画
    if (swingProgress > 0.0f) {
        // 攻击动画：物品向外挥动
        f64 swingAngle = static_cast<f64>(swingProgress) * mc::math::PI_DOUBLE;  // 0 到 π
        f64 swingFactor = std::sin(swingAngle);

        // 在原有旋转基础上添加挥动效果
        // 挥动时物品沿 Z 轴旋转
        f64 cosSwing = std::cos(swingFactor * 0.5);
        f64 sinSwing = std::sin(swingFactor * 0.5);

        // 修改旋转矩阵以包含挥动效果
        // 简化：直接调整位置
        outMatrix[7] += sinSwing * 0.1;  // Y 方向偏移
        outMatrix[11] += cosSwing * 0.1; // Z 方向偏移
    }

    // 避免未使用变量警告
    (void)cosX;
    (void)sinX;
    (void)cosY;
    (void)sinY;
}

// 显式实例化常用类型
template class HeldItemLayer<::mc::LivingEntity>;
template class HeldItemLayer<::mc::Player>;

} // namespace mc::client::renderer::entity::layer::equipment
