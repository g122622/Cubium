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

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/IEntityRenderer.hpp"
#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/item/ItemMeshBuilder.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <memory>
#include <optional>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace mc {
class LivingEntity;
}

namespace mc::item::items {
class DyeableArmorItem;
}

namespace mc::client::renderer::entity::layer::equipment {

/**
 * @brief 盔甲层渲染器
 *
 * 在实体模型上渲染盔甲层。支持不同部位的盔甲：
 * - 头盔
 * - 胸甲
 * - 护腿
 * - 靴子
 *
 * 支持皮革染色的盔甲。
 *
 * @tparam TEntity 实体类型
 * @tparam TModel 模型类型
 */
template <typename TEntity, typename TModel>
class ArmorLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 盔甲部位
     */
    enum class ArmorSlot : u8 {
        Head = 0,  // 头盔
        Chest = 1, // 胸甲
        Legs = 2,  // 护腿
        Feet = 3   // 靴子
    };

    /**
     * @brief 默认构造函数
     */
    ArmorLayer() = default;

    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     */
    explicit ArmorLayer(entity::core::IEntityRenderer<TEntity, TModel>& renderer)
        : m_renderer(&renderer)
    {}

    ~ArmorLayer() override = default;

    /**
     * @brief 渲染盔甲层（GPU管线路径）
     *
     * 渲染顺序:
     * 1. Chest (胸甲) - layer_1
     * 2. Legs (护腿) - layer_2
     * 3. Feet (靴子) - layer_1
     * 4. Head (头盔) - layer_1
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override
    {
        renderArmorPartPipeline(entity, ArmorSlot::Chest, cmd, context, pipeline);
        renderArmorPartPipeline(entity, ArmorSlot::Legs, cmd, context, pipeline);
        renderArmorPartPipeline(entity, ArmorSlot::Feet, cmd, context, pipeline);
        renderArmorPartPipeline(entity, ArmorSlot::Head, cmd, context, pipeline);
    }

    /**
     * @brief 检查是否应该渲染盔甲层
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override
    {
        // 检查是否穿戴了任何盔甲
        if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
            using EquipmentSlot = ::mc::EquipmentSlot;
            const auto& head = entity.getEquipment(EquipmentSlot::Head);
            const auto& chest = entity.getEquipment(EquipmentSlot::Chest);
            const auto& legs = entity.getEquipment(EquipmentSlot::Legs);
            const auto& feet = entity.getEquipment(EquipmentSlot::Feet);

            return (!head.isEmpty()) || (!chest.isEmpty()) || (!legs.isEmpty()) || (!feet.isEmpty());
        }
        return false;
    }

protected:
    /**
     * @brief 渲染特定部位的盔甲（GPU管线路径）
     *
     * TODO: 当前使用 ItemMeshBuilder::buildArmorMesh 构建网格，未根据盔甲材质切换纹理。
     * getArmorTexture() 和 getArmorOverlayTexture() 已实现动态纹理路径选择，
     * 但尚未接入渲染管线。需要将这两个方法的返回值传递给 EntityPipeline，
     * 在 drawMesh 时绑定正确的盔甲纹理图集（而非实体通用纹理图集）。
     * 皮革盔甲还需分两遍渲染：底色层（可染色）+ 覆盖层（不可染色）。
     */
    virtual void renderArmorPartPipeline(TEntity& entity,
        ArmorSlot slot,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline)
    {
        // 获取盔甲物品
        const ::mc::ItemStack* armorItem = getArmorItem(entity, slot);
        if (!armorItem || armorItem->isEmpty()) {
            // 物品为空，清除缓存
            _clearSlotCache(slot);
            return;
        }

        // 获取父模型并复制动画状态到盔甲模型
        TModel* parentModel = getParentModel();
        TModel& armorModel = getArmorModel(slot);
        if (parentModel) {
            armorModel.copyAnglesTo(parentModel);
        }

        setModelSlotVisible(armorModel, slot);

        // 获取身体部件变换矩阵
        std::array<f64, 16> bodyPartTransform;
        getBodyPartTransform(slot, context, bodyPartTransform);

        // 获取或创建网格缓存
        auto& meshCache = _getMeshCache(slot);
        u32 itemId = armorItem->getItem() ? armorItem->getItem()->itemId() : 0;

        // 检查是否需要更新网格（物品变化或首次渲染）
        bool needsUpdate = !meshCache.mesh.has_value() || meshCache.lastItemId != itemId ||
            meshCache.lastSlot != static_cast<u32>(slot);

        if (needsUpdate) {
            // 构建新的盔甲网格
            auto [vertices, indices] =
                item::ItemMeshBuilder::buildArmorMesh(*armorItem, static_cast<u32>(slot), bodyPartTransform);

            if (vertices.empty() || indices.empty()) {
                _clearSlotCache(slot);
                return;
            }

            if (!meshCache.mesh.has_value()) {
                // 创建新网格
                auto result = pipeline.createMesh(vertices, indices);
                if (!result.success()) {
                    return;
                }
                meshCache.mesh = std::move(result.value());
            } else {
                // 更新现有网格
                auto result = pipeline.updateMesh(meshCache.mesh.value(), vertices, indices);
                if (!result.success()) {
                    return;
                }
            }

            meshCache.lastItemId = itemId;
            meshCache.lastSlot = static_cast<u32>(slot);
        }

        // 获取实体位置
        Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

        // 获取盔甲颜色（染色盔甲）
        Vector4f overlayColor(0.0f, 0.0f, 0.0f, 0.0f);
        if (isDyeableArmor(*armorItem)) {
            Vector3f dyeColor = getDyeColor(*armorItem);
            overlayColor = Vector4f(dyeColor.x, dyeColor.y, dyeColor.z, 1.0f);
        }

        // 使用实体的 hurtTime 和 deathTime
        f32 hurtTime = 0.0f;
        f32 deathTime = 0.0f;
        if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
            hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
            deathTime = static_cast<f32>(entity.deathTime());
        }

        pipeline.drawMesh(
            cmd, meshCache.mesh.value(), bodyPartTransform, entityPos, 1.0, overlayColor, hurtTime, deathTime);
    }

    /**
     * @brief 渲染特定部位的盔甲（CPU路径 - 已废弃）
     * TODO: 待确认是否完全移除CPU渲染路径
     */
    virtual void renderArmorPart(TEntity& entity,
        ArmorSlot slot,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale)
    {
        (void)entity;
        (void)slot;
        (void)limbSwing;
        (void)limbSwingAmount;
        (void)partialTicks;
        (void)ageInTicks;
        (void)netHeadYaw;
        (void)headPitch;
        (void)scale;
    }

    /**
     * @brief 获取盔甲物品
     */
    [[nodiscard]] virtual const ::mc::ItemStack* getArmorItem(const TEntity& entity, ArmorSlot slot) const
    {
        if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
            using EquipmentSlot = ::mc::EquipmentSlot;
            switch (slot) {
                case ArmorSlot::Head:
                    return &entity.getEquipment(EquipmentSlot::Head);
                case ArmorSlot::Chest:
                    return &entity.getEquipment(EquipmentSlot::Chest);
                case ArmorSlot::Legs:
                    return &entity.getEquipment(EquipmentSlot::Legs);
                case ArmorSlot::Feet:
                    return &entity.getEquipment(EquipmentSlot::Feet);
            }
        }
        return nullptr;
    }

    /**
     * @brief 获取身体部件变换矩阵
     *
     * TODO: 当前仅设置简单的Y偏移，未利用AnimationContext中的骨骼动画数据，
     * 需要实现完整的骨骼动画变换
     */
    virtual void getBodyPartTransform(ArmorSlot slot,
        const mc::client::renderer::entity::core::AnimationContext& context,
        std::array<f64, 16>& outMatrix)
    {
        // 初始化为单位矩阵
        outMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

        // 根据部位应用不同的变换
        switch (slot) {
            case ArmorSlot::Head:
                // 头盔位置
                outMatrix[7] = 0.5; // Y 偏移
                break;
            case ArmorSlot::Chest:
                // 胸甲位置（身体）
                outMatrix[7] = 0.0;
                break;
            case ArmorSlot::Legs:
                // 护腿位置
                outMatrix[7] = -0.5; // Y 偏移
                break;
            case ArmorSlot::Feet:
                // 靴子位置
                outMatrix[7] = -1.0; // Y 偏移
                break;
        }

        (void)context;
    }

    /**
     * @brief 设置模型部件可见性
     *
     * 根据盔甲槽位设置哪些模型部件应该可见
     */
    virtual void setModelSlotVisible(TModel& model, ArmorSlot slot)
    {
        // 默认隐藏所有部件
        model.setAllVisible(false);

        // 根据槽位显示对应部件
        switch (slot) {
            case ArmorSlot::Head:
                // 头盔：显示头部和帽子层
                if (auto head = model.getModelHead()) {
                    head->setVisible(true);
                }
                if (auto headwear = model.getModelHeadwear()) {
                    headwear->setVisible(true);
                }
                break;
            case ArmorSlot::Chest:
                // 胸甲：显示身体、左臂、右臂
                if (auto body = model.getModelBody()) {
                    body->setVisible(true);
                }
                if (auto leftArm = model.getLeftArm()) {
                    leftArm->setVisible(true);
                }
                if (auto rightArm = model.getRightArm()) {
                    rightArm->setVisible(true);
                }
                break;
            case ArmorSlot::Legs:
                // 护腿：显示身体、左腿、右腿
                if (auto body = model.getModelBody()) {
                    body->setVisible(true);
                }
                if (auto leftLeg = model.getLeftLeg()) {
                    leftLeg->setVisible(true);
                }
                if (auto rightLeg = model.getRightLeg()) {
                    rightLeg->setVisible(true);
                }
                break;
            case ArmorSlot::Feet:
                // 靴子：显示左腿、右腿
                if (auto leftLeg = model.getLeftLeg()) {
                    leftLeg->setVisible(true);
                }
                if (auto rightLeg = model.getRightLeg()) {
                    rightLeg->setVisible(true);
                }
                break;
        }
    }

    /**
     * @brief 检查物品是否为可染色盔甲
     */
    [[nodiscard]] bool isDyeableArmor(const ::mc::ItemStack& stack) const
    {
        const ::mc::Item* item = stack.getItem();
        if (!item) {
            return false;
        }
        // 检查是否为 DyeableArmorItem 实例
        // 皮革盔甲是可染色的
        return dynamic_cast<const ::mc::item::items::DyeableArmorItem*>(item) != nullptr;
    }

    /**
     * @brief 获取染色盔甲的颜色
     * @param stack 盔甲物品堆
     * @return RGB 颜色值（归一化到 [0, 1]）
     */
    [[nodiscard]] Vector3f getDyeColor(const ::mc::ItemStack& stack) const
    {
        const ::mc::Item* item = stack.getItem();
        const auto* dyeableItem = dynamic_cast<const ::mc::item::items::DyeableArmorItem*>(item);
        if (dyeableItem) {
            u32 color = dyeableItem->getColor(stack);
            // 从 ARGB 提取 RGB 并归一化
            f32 r = static_cast<f32>((color >> 16) & 0xFF) / 255.0f;
            f32 g = static_cast<f32>((color >> 8) & 0xFF) / 255.0f;
            f32 b = static_cast<f32>(color & 0xFF) / 255.0f;
            return Vector3f(r, g, b);
        }
        return Vector3f(1.0f, 1.0f, 1.0f); // 默认白色
    }

    /**
     * @brief 获取盔甲模型
     *
     * 使用延迟初始化的模式缓存盔甲模型，避免每帧创建。
     */
    [[nodiscard]] virtual TModel& getArmorModel(ArmorSlot slot)
    {
        // 返回对应的盔甲模型（已缓存）
        switch (slot) {
            case ArmorSlot::Head:
                if (!m_headArmorModel) m_headArmorModel = std::make_unique<TModel>(0.0f);
                return *m_headArmorModel;
            case ArmorSlot::Chest:
                if (!m_chestArmorModel) m_chestArmorModel = std::make_unique<TModel>(0.0f);
                return *m_chestArmorModel;
            case ArmorSlot::Legs:
                if (!m_legsArmorModel) m_legsArmorModel = std::make_unique<TModel>(0.0f);
                return *m_legsArmorModel;
            case ArmorSlot::Feet:
            default:
                if (!m_feetArmorModel) m_feetArmorModel = std::make_unique<TModel>(0.0f);
                return *m_feetArmorModel;
        }
    }

    /**
     * @brief 获取盔甲纹理路径
     *
     * 根据盔甲材质和装备槽位返回对应的纹理路径。
     * 使用 MC 1.21+ 的 equipment 纹理路径格式：
     * - 头盔/胸甲/靴子: textures/entity/equipment/humanoid/<assetId>.png
     * - 护腿: textures/entity/equipment/humanoid_leggings/<assetId>.png
     *
     * TODO: 此方法尚未被 renderArmorPartPipeline 调用，纹理路径返回值
     * 需要在渲染管线中绑定到对应的纹理图集后方可生效。
     *
     * @param entity 实体引用（用于子类自定义纹理）
     * @param slot 盔甲槽位
     * @return 纹理资源路径
     */
    [[nodiscard]] virtual ResourceLocation getArmorTexture(const TEntity& entity, ArmorSlot slot)
    {
        const ::mc::ItemStack* armorItem = getArmorItem(entity, slot);
        if (!armorItem || armorItem->isEmpty()) {
            return ::mc::item::armor::ArmorMaterial::getArmorTexturePath("iron", slot);
        }

        const ::mc::Item* item = armorItem->getItem();
        const auto* armor = dynamic_cast<const ::mc::item::items::ArmorItem*>(item);
        if (!armor) {
            // 鞘翅等非盔甲物品走默认纹理
            return ::mc::item::armor::ArmorMaterial::getArmorTexturePath("iron", slot);
        }

        return ::mc::item::armor::ArmorMaterial::getArmorTexturePath(armor->getMaterial().getAssetId(), slot);
    }

    /**
     * @brief 获取皮革盔甲覆盖层纹理路径
     *
     * 皮革盔甲有两层纹理：底色层（可染色）和覆盖层（不可染色，显示细节图案）。
     * 覆盖层纹理路径格式：
     * - 头盔/胸甲/靴子: textures/entity/equipment/humanoid/leather_overlay.png
     * - 护腿: textures/entity/equipment/humanoid_leggings/leather_overlay.png
     *
     * TODO: 此方法尚未被 renderArmorPartPipeline 调用，覆盖层纹理需要
     * 在底色层渲染之后以第二遍渲染方式叠加，方可显示皮革盔甲的细节图案。
     *
     * @param entity 实体引用
     * @param slot 盔甲槽位
     * @return 覆盖层纹理路径，若非皮革盔甲则返回空
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getArmorOverlayTexture(const TEntity& entity, ArmorSlot slot)
    {
        const ::mc::ItemStack* armorItem = getArmorItem(entity, slot);
        if (!armorItem || armorItem->isEmpty()) {
            return std::nullopt;
        }

        // 仅皮革盔甲有覆盖层
        const ::mc::Item* item = armorItem->getItem();
        if (!dynamic_cast<const ::mc::item::items::DyeableArmorItem*>(item)) {
            return std::nullopt;
        }

        return ::mc::item::armor::ArmorMaterial::getLeatherOverlayTexturePath(slot);
    }

    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] entity::core::IEntityRenderer<TEntity, TModel>* getRenderer() { return m_renderer; }

    /**
     * @brief 获取关联的模型
     */
    [[nodiscard]] TModel* getParentModel() { return m_renderer ? &m_renderer->getModel() : nullptr; }

private:
    entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;

    // 盔甲模型（按部位）- 延迟初始化缓存
    std::unique_ptr<TModel> m_headArmorModel;
    std::unique_ptr<TModel> m_chestArmorModel;
    std::unique_ptr<TModel> m_legsArmorModel;
    std::unique_ptr<TModel> m_feetArmorModel;

    /**
     * @brief 盔甲网格缓存条目
     *
     * 缓存已创建的网格，避免每帧重新创建。
     * 当装备的物品变化时更新缓存。
     */
    struct ArmorMeshCache {
        std::optional<pipeline::EntityMesh> mesh; ///< 网格数据
        u32 lastItemId = 0;                       ///< 上次渲染的物品ID
        u32 lastSlot = 0;                         ///< 上次渲染的槽位
    };

    // 每个槽位的网格缓存
    ArmorMeshCache m_headCache;
    ArmorMeshCache m_chestCache;
    ArmorMeshCache m_legsCache;
    ArmorMeshCache m_feetCache;

    /**
     * @brief 获取指定槽位的网格缓存
     */
    ArmorMeshCache& _getMeshCache(ArmorSlot slot)
    {
        switch (slot) {
            case ArmorSlot::Head:
                return m_headCache;
            case ArmorSlot::Chest:
                return m_chestCache;
            case ArmorSlot::Legs:
                return m_legsCache;
            case ArmorSlot::Feet:
            default:
                return m_feetCache;
        }
    }

    /**
     * @brief 清除指定槽位的网格缓存
     */
    void _clearSlotCache(ArmorSlot slot)
    {
        auto& cache = _getMeshCache(slot);
        cache.mesh.reset();
        cache.lastItemId = 0;
        cache.lastSlot = 0;
    }
};

} // namespace mc::client::renderer::entity::layer::equipment
