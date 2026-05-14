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
#include "../../core/IEntityRenderer.hpp"
#include "../../model/animal/VillagerModel.hpp"
#include "../../pipeline/EntityPipeline.hpp"
#include "../core/LayerRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <algorithm>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vulkan/vulkan.h>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

// ============================================================================
// 村民纹理名称映射 (MC 1.16.5)
// ============================================================================
namespace VillagerLayerDetail {
/// 村民类型名称（生物群系），对应 VillagerType 枚举
inline const char* VILLAGER_TYPE_NAMES[] = {
    "desert", // VillagerType::Desert = 0
    "jungle", // VillagerType::Jungle = 1
    "plains", // VillagerType::Plains = 2
    "savanna", // VillagerType::Savanna = 3
    "snow", // VillagerType::Snow = 4
    "swamp", // VillagerType::Swamp = 5
    "taiga" // VillagerType::Taiga = 6
};

/// 村民职业名称，对应 VillagerProfession 枚举
inline const char* VILLAGER_PROFESSION_NAMES[] = {
    "none", // VillagerProfession::None = 0
    "armorer", // VillagerProfession::Armorer = 1
    "butcher", // VillagerProfession::Butcher = 2
    "cartographer", // VillagerProfession::Cartographer = 3
    "cleric", // VillagerProfession::Cleric = 4
    "farmer", // VillagerProfession::Farmer = 5
    "fisherman", // VillagerProfession::Fisherman = 6
    "fletcher", // VillagerProfession::Fletcher = 7
    "leatherworker", // VillagerProfession::Leatherworker = 8
    "librarian", // VillagerProfession::Librarian = 9
    "mason", // VillagerProfession::Mason = 10
    "nitwit", // VillagerProfession::Nitwit = 11
    "shepherd", // VillagerProfession::Shepherd = 12
    "toolsmith", // VillagerProfession::Toolsmith = 13
    "weaponsmith" // VillagerProfession::Weaponsmith = 14
};

/// 村民等级徽章名称
inline const char* VILLAGER_LEVEL_NAMES[] = {
    "stone", // 等级 1 - 新手 (Novice)
    "iron", // 等级 2 - 学徒 (Apprentice)
    "gold", // 等级 3 - 老手 (Journeyman)
    "emerald", // 等级 4 - 专家 (Expert)
    "diamond" // 等级 5 - 大师 (Master)
};

constexpr i32 VILLAGER_TYPE_COUNT = 7;
constexpr i32 VILLAGER_PROFESSION_COUNT = 15;
constexpr i32 VILLAGER_LEVEL_COUNT = 5;
constexpr i32 VILLAGER_MIN_LEVEL = 1;
constexpr i32 VILLAGER_MAX_LEVEL = 5;
} // namespace VillagerLayerDetail

/**
 * @brief 村民多层纹理渲染器
 *
 * 参考 MC 1.16.5 VillagerLevelPendantLayer
 *
 * 村民纹理由多层叠加组成：
 * 1. 基础纹理 (villager.png) - 由主渲染器渲染，包含身体和头部基础
 * 2. 类型层 (type/{type}.png) - 根据生物群系叠加不同外观
 * 3. 职业层 (profession/{profession}.png) - 根据职业叠加装备和服饰
 * 4. 等级徽章层 (profession_level/{badge}.png) - 显示交易等级徽章
 *
 * 渲染规则（MC 1.16.5）：
 * - 类型层：始终渲染（非隐身时）
 * - 职业层：职业 != NONE 且 非儿童 时渲染
 * - 等级徽章层：职业 != NONE 且 职业 != NITWIT 且 非儿童 时渲染
 *
 * 帽子可见性规则：
 * - 类型层帽子类型和职业层帽子类型共同决定是否显示基础帽子
 * - 如果职业帽子为 NONE 或 PARTIAL 且类型帽子为 FULL，则隐藏基础帽子
 *
 * @tparam TEntity 村民实体类型 (VillagerEntity 或 ZombieVillagerEntity)
 * @tparam TModel 村民模型类型
 */
template <typename TEntity = ::mc::entity::VillagerEntity, typename TModel = ::mc::client::renderer::entity::model::animal::VillagerModel>
class VillagerLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     * @param texturePrefix 纹理路径前缀 ("villager" 或 "zombie_villager")
     */
    explicit VillagerLayer(
        mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer,
        const std::string& texturePrefix = "villager")
        : m_renderer(&renderer)
        , m_texturePrefix(texturePrefix)
    {}

    ~VillagerLayer() override = default;

    /**
     * @brief 渲染村民层（GPU管线路径）
     *
     * 当前实现状态：
     * - 已实现纹理路径计算
     * - 已实现渲染条件判断
     * - TODO: 需要纹理图集UV重映射支持才能实现真正的多层渲染
     *
     * 架构限制说明：
     * EntityPipeline.drawMesh() 不支持运行时纹理切换。
     * 当前架构使用纹理图集，UV坐标在网格生成时已固定。
     * 要实现多层纹理需要：
     * 1. 为每层生成独立网格
     * 2. 对每层网格进行UV重映射到对应纹理区域
     * 3. 顺序渲染每层
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override
    {
        if (!shouldRender(entity)) {
            return;
        }

        // 获取村民数据
        const ::mc::entity::VillagerData& data = entity.villagerData();
        const ::mc::entity::VillagerType type = data.type();
        const ::mc::entity::VillagerProfession profession = data.profession();
        const i32 level = std::clamp(data.level(), VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);

        // 计算各层纹理路径
        const ResourceLocation typeTexture = getTypeTexture(type);
        const ResourceLocation professionTexture = getProfessionTexture(profession);
        const ResourceLocation levelTexture = getLevelTexture(level);

        // 判断渲染条件
        const bool isChild = isChildEntity(entity);
        const bool shouldRenderProfession = (profession != ::mc::entity::VillagerProfession::None) && !isChild;
        const bool shouldRenderLevel = shouldRenderProfession && (profession != ::mc::entity::VillagerProfession::Nitwit);

        // MC 1.16.5 渲染顺序：
        // 1. 渲染类型层
        // 2. 渲染职业层（如果条件满足）
        // 3. 渲染等级徽章层（如果条件满足）

        // 当前架构限制：无法动态切换纹理
        // 已添加纹理路径到 EntityTextureLoader::ADDITIONAL_TEXTURES
        // 完整实现需要扩展 EntityPipeline 支持运行时UV重映射

        spdlog::trace("VillagerLayer: type={} ({}), profession={} ({}), level={} ({}), renderProf={}, renderLevel={}",
            static_cast<int>(type), typeTexture.toString(),
            static_cast<int>(profession), professionTexture.toString(),
            level, levelTexture.toString(),
            shouldRenderProfession, shouldRenderLevel);

        (void)cmd;
        (void)pipeline;
        (void)context;
        (void)typeTexture;
        (void)professionTexture;
        (void)levelTexture;
        (void)shouldRenderLevel;
    }

    /**
     * @brief 渲染村民层（CPU路径 - 已废弃）
     */
    void render(TEntity& entity,
        f32 limbSwing,
        f32 limbSwingAmount,
        f32 partialTicks,
        f32 ageInTicks,
        f32 netHeadYaw,
        f32 headPitch,
        f32 scale) override
    {
        (void)entity;
        (void)limbSwing;
        (void)limbSwingAmount;
        (void)partialTicks;
        (void)ageInTicks;
        (void)netHeadYaw;
        (void)headPitch;
        (void)scale;
    }

    /**
     * @brief 检查是否应该渲染村民层
     *
     * MC 1.16.5 VillagerLevelPendantLayer.shouldRender():
     * return !entity.isInvisible();
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override
    {
        if constexpr (std::is_base_of_v<::mc::Entity, TEntity>) {
            if (entity.hasFlag(::mc::EntityFlags::Invisible)) {
                return false;
            }
        }
        return true;
    }

    // ========== 纹理路径获取方法 ==========

    /**
     * @brief 获取类型层纹理路径
     * @param type 村民类型（生物群系）
     * @return 纹理位置
     */
    [[nodiscard]] ResourceLocation getTypeTexture(::mc::entity::VillagerType type) const
    {
        const i32 index = static_cast<i32>(type);
        if (index >= 0 && index < VillagerLayerDetail::VILLAGER_TYPE_COUNT) {
            return buildTexturePath(std::string("type/") + VillagerLayerDetail::VILLAGER_TYPE_NAMES[index]);
        }
        return buildTexturePath("type/plains");
    }

    /**
     * @brief 获取职业层纹理路径
     * @param profession 村民职业
     * @return 纹理位置
     */
    [[nodiscard]] ResourceLocation getProfessionTexture(::mc::entity::VillagerProfession profession) const
    {
        const i32 index = static_cast<i32>(profession);
        if (index >= 0 && index < VillagerLayerDetail::VILLAGER_PROFESSION_COUNT) {
            return buildTexturePath(std::string("profession/") + VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[index]);
        }
        return buildTexturePath("profession/none");
    }

    /**
     * @brief 获取等级徽章纹理路径
     * @param level 村民等级 (1-5)
     * @return 纹理位置
     */
    [[nodiscard]] ResourceLocation getLevelTexture(i32 level) const
    {
        const i32 clampedLevel = std::clamp(level, VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);
        const i32 index = clampedLevel - VillagerLayerDetail::VILLAGER_MIN_LEVEL;
        return buildTexturePath(std::string("profession_level/") + VillagerLayerDetail::VILLAGER_LEVEL_NAMES[index]);
    }

    /**
     * @brief 获取职业名称字符串
     */
    [[nodiscard]] static const char* getProfessionName(::mc::entity::VillagerProfession profession)
    {
        const i32 index = static_cast<i32>(profession);
        if (index >= 0 && index < VillagerLayerDetail::VILLAGER_PROFESSION_COUNT) {
            return VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[index];
        }
        return "none";
    }

    /**
     * @brief 获取类型名称字符串
     */
    [[nodiscard]] static const char* getTypeName(::mc::entity::VillagerType type)
    {
        const i32 index = static_cast<i32>(type);
        if (index >= 0 && index < VillagerLayerDetail::VILLAGER_TYPE_COUNT) {
            return VillagerLayerDetail::VILLAGER_TYPE_NAMES[index];
        }
        return "plains";
    }

    /**
     * @brief 获取等级徽章名称字符串
     */
    [[nodiscard]] static const char* getLevelName(i32 level)
    {
        const i32 clampedLevel = std::clamp(level, VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);
        const i32 index = clampedLevel - VillagerLayerDetail::VILLAGER_MIN_LEVEL;
        return VillagerLayerDetail::VILLAGER_LEVEL_NAMES[index];
    }

protected:
    /**
     * @brief 获取关联的渲染器
     */
    [[nodiscard]] mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* getRenderer()
    {
        return m_renderer;
    }

    /**
     * @brief 获取父模型
     */
    [[nodiscard]] TModel* getParentModel() { return m_renderer ? &m_renderer->getModel() : nullptr; }

private:
    /**
     * @brief 构建纹理路径
     * @param subpath 子路径 (如 "type/desert")
     * @return 完整纹理位置
     */
    [[nodiscard]] ResourceLocation buildTexturePath(const std::string& subpath) const
    {
        // MC 1.16.5 纹理路径格式: textures/entity/{villager/zombie_villager}/{subpath}.png
        return ResourceLocation("minecraft", "textures/entity/" + m_texturePrefix + "/" + subpath + ".png");
    }

    /**
     * @brief 检查实体是否为儿童
     */
    [[nodiscard]] bool isChildEntity(const TEntity& entity) const
    {
        if constexpr (std::is_base_of_v<::mc::AgeableEntity, TEntity>) {
            return entity.isChild();
        }
        return false;
    }

    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
    std::string m_texturePrefix; ///< "villager" 或 "zombie_villager"
};

} // namespace mc::client::renderer::entity::layer::entity
