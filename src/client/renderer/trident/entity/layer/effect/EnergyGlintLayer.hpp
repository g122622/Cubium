/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../core/AnimationContext.hpp"
#include "../../model/core/ModelRenderer.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>
#include <memory>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

namespace mc {
class LivingEntity;
}

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::effect {

/**
 * @brief 附魔光效层渲染器
 *
 * 渲染附魔物品的紫色光效。使用滚动的光效纹理。
 *
 * 参考 MC 1.16.5 EnergyLayer
 *
 * @tparam TEntity 实体类型
 */
template <typename TEntity>
class EnergyGlintLayer : public core::LayerRenderer<TEntity> {
public:
    EnergyGlintLayer() = default;
    ~EnergyGlintLayer() override = default;

    /**
     * @brief 渲染附魔光效层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 渲染附魔光效层（CPU路径 - 已废弃）
     */
    void render(TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale) override;

    /**
     * @brief 检查是否应该渲染附魔光效
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

protected:
    /**
     * @brief 计算光效滚动偏移
     */
    [[nodiscard]] f32 calculateGlintOffset(f32 ageInTicks) const;

    /**
     * @brief 构建光效网格
     */
    void buildGlintMesh(f32 glintOffset, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    ResourceLocation m_glintTexture{"minecraft", "textures/misc/enchanted_item_glint.png"};

    // 光效网格缓存
    std::unordered_map<i32, pipeline::EntityMesh> m_glintMeshCache;
};

} // namespace mc::client::renderer::entity::layer::effect

namespace mc::client::renderer::entity::layer::effect {

template <typename TEntity>
void EnergyGlintLayer<TEntity>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    if (!shouldRender(entity)) {
        return;
    }

    // 计算光效滚动偏移
    f32 glintOffset = calculateGlintOffset(static_cast<f32>(context.ageInTicks));

    // 构建光效网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    buildGlintMesh(glintOffset, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // 创建临时网格
    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("EnergyGlintLayer: Failed to create glint mesh");
        return;
    }

    // 计算光效变换矩阵
    // 光效层覆盖整个实体
    std::array<f64, 16> glintTransform;
    glintTransform = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

    // 光效略微放大以避免 z-fighting
    const f32 glintScale = 1.01f;
    glintTransform[0] = glintScale;
    glintTransform[5] = glintScale;
    glintTransform[10] = glintScale;

    // 获取实体位置
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 使用紫色发光颜色
    // 参考 MC 1.16.5: 附魔光效使用紫色/蓝色光效 (0.5F, 0.0F, 1.0F, 0.5F)
    Vector4f overlayColor(0.5f, 0.0f, 1.0f, 0.5f);

    // 切换到叠加混合模式（用于附魔光效）
    // 参考 MC 1.16.5 EnergyLayer: 使用 RenderType.getEnergySwirl()
    // 混合公式: src * srcAlpha + dst * 1 (加法混合)
    pipeline.bind(cmd, pipeline::BlendMode::Additive);

    pipeline.drawMesh(cmd, result.value(), glintTransform, entityPos, 1.0, overlayColor, 0.0f, 0.0f);

    // 恢复 Alpha 混合模式
    pipeline.bind(cmd, pipeline::BlendMode::Alpha);

    // spdlog::trace("EnergyGlintLayer: Rendered glint effect on entity");

    (void)cmd;
}

template <typename TEntity>
void EnergyGlintLayer<TEntity>::render(TEntity& entity,
    f32 limbSwing,
    f32 limbSwingAmount,
    f32 partialTicks,
    f32 ageInTicks,
    f32 netHeadYaw,
    f32 headPitch,
    f32 scale)
{
    // CPU 路径已废弃
    (void)entity;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)partialTicks;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

template <typename TEntity>
bool EnergyGlintLayer<TEntity>::shouldRender(const TEntity& entity) const
{
    // 检查实体是否有附魔物品
    // 参考 MC 1.16.5: ItemStack.hasEffect() -> ItemStack.isEnchanted()
    // 检查所有装备槽位是否有附魔物品
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        using ::mc::EquipmentSlot;
        using ::mc::item::enchant::EnchantmentHelper;

        // 检查主手物品
        const auto& mainHand = entity.getEquipment(EquipmentSlot::MainHand);
        if (!mainHand.isEmpty() && EnchantmentHelper::hasEnchantments(mainHand)) {
            return true;
        }

        // 检查副手物品
        const auto& offHand = entity.getEquipment(EquipmentSlot::OffHand);
        if (!offHand.isEmpty() && EnchantmentHelper::hasEnchantments(offHand)) {
            return true;
        }

        // 检查头盔
        const auto& head = entity.getEquipment(EquipmentSlot::Head);
        if (!head.isEmpty() && EnchantmentHelper::hasEnchantments(head)) {
            return true;
        }

        // 检查胸甲
        const auto& chest = entity.getEquipment(EquipmentSlot::Chest);
        if (!chest.isEmpty() && EnchantmentHelper::hasEnchantments(chest)) {
            return true;
        }

        // 检查护腿
        const auto& legs = entity.getEquipment(EquipmentSlot::Legs);
        if (!legs.isEmpty() && EnchantmentHelper::hasEnchantments(legs)) {
            return true;
        }

        // 检查靴子
        const auto& feet = entity.getEquipment(EquipmentSlot::Feet);
        if (!feet.isEmpty() && EnchantmentHelper::hasEnchantments(feet)) {
            return true;
        }
    }
    return false;
}

template <typename TEntity>
f32 EnergyGlintLayer<TEntity>::calculateGlintOffset(f32 ageInTicks) const
{
    // 光效滚动速度
    // 参考 MC 1.16.5 的光效动画
    // 使用 std::fmod 计算偏移，并确保结果在 [0, 1) 范围内
    f32 offset = std::fmod(ageInTicks * 0.01f, 1.0f);
    // 处理负数情况（虽然 ageInTicks 不应该为负，但为了健壮性）
    if (offset < 0.0f) {
        offset += 1.0f;
    }
    return offset;
}

template <typename TEntity>
void EnergyGlintLayer<TEntity>::buildGlintMesh(
    f32 glintOffset, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 附魔光效是一个覆盖整个实体的半透明网格
    // 使用滚动的 UV 坐标来模拟光效流动
    vertices.clear();
    indices.clear();

    // 简化实现：创建一个包裹实体的立方体
    constexpr f32 SIZE = 1.0f;
    f32 half = SIZE / 2.0f;

    // UV 滚动偏移
    f32 uOffset = glintOffset;
    f32 vOffset = glintOffset * 0.5f;

    // 顶点格式: ModelVertex(x, y, z, u, v, nx, ny, nz)
    // 使用滚动的 UV 坐标

    // 前面
    vertices.push_back(model::ModelVertex(-half, -half, half, uOffset, vOffset, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(half, -half, half, uOffset + 1.0f, vOffset, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(half, half, half, uOffset + 1.0f, vOffset + 1.0f, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(-half, half, half, uOffset, vOffset + 1.0f, 0.0f, 0.0f, 1.0f));

    // 后面
    vertices.push_back(model::ModelVertex(half, -half, -half, uOffset, vOffset, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(-half, -half, -half, uOffset + 1.0f, vOffset, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(-half, half, -half, uOffset + 1.0f, vOffset + 1.0f, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(half, half, -half, uOffset, vOffset + 1.0f, 0.0f, 0.0f, -1.0f));

    // 顶面
    vertices.push_back(model::ModelVertex(-half, half, half, uOffset, vOffset, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(half, half, half, uOffset + 1.0f, vOffset, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(half, half, -half, uOffset + 1.0f, vOffset + 1.0f, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-half, half, -half, uOffset, vOffset + 1.0f, 0.0f, 1.0f, 0.0f));

    // 底面
    vertices.push_back(model::ModelVertex(-half, -half, -half, uOffset, vOffset, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(half, -half, -half, uOffset + 1.0f, vOffset, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(half, -half, half, uOffset + 1.0f, vOffset + 1.0f, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-half, -half, half, uOffset, vOffset + 1.0f, 0.0f, -1.0f, 0.0f));

    // 右面
    vertices.push_back(model::ModelVertex(half, -half, half, uOffset, vOffset, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(half, -half, -half, uOffset + 1.0f, vOffset, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(half, half, -half, uOffset + 1.0f, vOffset + 1.0f, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(half, half, half, uOffset, vOffset + 1.0f, 1.0f, 0.0f, 0.0f));

    // 左面
    vertices.push_back(model::ModelVertex(-half, -half, -half, uOffset, vOffset, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-half, -half, half, uOffset + 1.0f, vOffset, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-half, half, half, uOffset + 1.0f, vOffset + 1.0f, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(-half, half, -half, uOffset, vOffset + 1.0f, -1.0f, 0.0f, 0.0f));

    // 索引（每个面两个三角形）
    for (u32 face = 0; face < 6; ++face) {
        u32 base = face * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }
}

} // namespace mc::client::renderer::entity::layer::effect
