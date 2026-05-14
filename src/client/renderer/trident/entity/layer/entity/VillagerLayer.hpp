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
// 村民类型名称映射 (MC 1.16.5 VillagerType)
// ============================================================================
namespace VillagerLayerDetail {
inline const char* VILLAGER_TYPE_NAMES[] = {
    "desert", // VillagerType::Desert = 0
    "jungle", // VillagerType::Jungle = 1
    "plains", // VillagerType::Plains = 2
    "savanna", // VillagerType::Savanna = 3
    "snow", // VillagerType::Snow = 4
    "swamp", // VillagerType::Swamp = 5
    "taiga" // VillagerType::Taiga = 6
};

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

inline const char* VILLAGER_LEVEL_NAMES[] = {
    "stone", // 等级 1 - 新手
    "iron", // 等级 2 - 学徒
    "gold", // 等级 3 - 老手
    "emerald", // 等级 4 - 专家
    "diamond" // 等级 5 - 大师
};

constexpr i32 VILLAGER_MIN_LEVEL = 1;
constexpr i32 VILLAGER_MAX_LEVEL = 5;
} // namespace VillagerLayerDetail

/**
 * @brief 村民多层纹理渲染器
 *
 * 渲染村民的类型层、职业层和等级徽章层。
 *
 * MC 1.16.5 村民纹理层叠顺序（从底到顶）：
 * 1. 基础纹理 (villager.png) - 由主渲染器渲染
 * 2. 类型层 (type/{type}.png) - 根据生物群系叠加
 * 3. 职业层 (profession/{profession}.png) - 根据职业叠加
 * 4. 等级徽章层 (profession_level/{badge}.png) - 根据等级叠加
 *
 * 参考 MC 1.16.5 VillagerLevelPendantLayer
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
        ::mc::entity::VillagerType type = data.type();
        ::mc::entity::VillagerProfession profession = data.profession();
        i32 level = std::clamp(data.level(), VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);

        // MC 1.16.5 VillagerLevelPendantLayer.render():
        // 1. 渲染类型层
        // 2. 渲染职业层（非 NONE 且非儿童）
        // 3. 渲染等级徽章层（非 NONE 且非 NITWIT 且非儿童）

        // 目前使用简化实现：仅记录日志
        // 完整实现需要纹理加载和多层渲染支持
        spdlog::trace("VillagerLayer: type={}, profession={}, level={}",
            static_cast<int>(type), static_cast<int>(profession), level);

        (void)cmd;
        (void)pipeline;
        (void)context;
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

    /**
     * @brief 检查是否应该渲染村民层
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override
    {
        // MC 1.16.5 VillagerLevelPendantLayer.shouldRender():
        // return !entity.isInvisible();

        // 检查是否隐身
        if constexpr (std::is_base_of_v<::mc::Entity, TEntity>) {
            if (entity.hasFlag(::mc::EntityFlags::Invisible)) {
                return false;
            }
        }

        return true;
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

    /**
     * @brief 获取类型层纹理
     * @param type 村民类型
     * @return 纹理位置
     */
    [[nodiscard]] ResourceLocation getTypeTexture(::mc::entity::VillagerType type) const
    {
        const i32 index = static_cast<i32>(type);
        if (index >= 0 && index < static_cast<i32>(std::size(VillagerLayerDetail::VILLAGER_TYPE_NAMES))) {
            return buildTexturePath(std::string("type/") + VillagerLayerDetail::VILLAGER_TYPE_NAMES[index]);
        }
        // 默认返回 plains 类型
        return buildTexturePath("type/plains");
    }

    /**
     * @brief 获取职业层纹理
     * @param profession 村民职业
     * @return 纹理位置
     */
    [[nodiscard]] ResourceLocation getProfessionTexture(::mc::entity::VillagerProfession profession) const
    {
        const i32 index = static_cast<i32>(profession);
        if (index >= 0 && index < static_cast<i32>(std::size(VillagerLayerDetail::VILLAGER_PROFESSION_NAMES))) {
            return buildTexturePath(std::string("profession/") + VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[index]);
        }
        // 默认返回 none
        return buildTexturePath("profession/none");
    }

    /**
     * @brief 获取等级徽章纹理
     * @param level 村民等级 (1-5)
     * @return 纹理位置
     */
    [[nodiscard]] ResourceLocation getLevelTexture(i32 level) const
    {
        const i32 clampedLevel = std::clamp(level, VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);
        const i32 index = clampedLevel - VillagerLayerDetail::VILLAGER_MIN_LEVEL; // 转换为 0-based 索引
        return buildTexturePath(std::string("profession_level/") + VillagerLayerDetail::VILLAGER_LEVEL_NAMES[index]);
    }

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

    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
    std::string m_texturePrefix; // "villager" 或 "zombie_villager"
};

} // namespace mc::client::renderer::entity::layer::entity
