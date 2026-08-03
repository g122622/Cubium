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

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/IEntityRenderer.hpp"
#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/animal/VillagerModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

// ============================================================================
// 村民纹理名称映射
// ============================================================================
namespace VillagerLayerDetail {
/// 村民类型名称（生物群系），对应 VillagerType 枚举
inline const char* VILLAGER_TYPE_NAMES[] = {
    "desert",  // VillagerType::Desert = 0
    "jungle",  // VillagerType::Jungle = 1
    "plains",  // VillagerType::Plains = 2
    "savanna", // VillagerType::Savanna = 3
    "snow",    // VillagerType::Snow = 4
    "swamp",   // VillagerType::Swamp = 5
    "taiga"    // VillagerType::Taiga = 6
};

/// 村民职业名称，对应 VillagerProfession 枚举
inline const char* VILLAGER_PROFESSION_NAMES[] = {
    "none",          // VillagerProfession::None = 0
    "armorer",       // VillagerProfession::Armorer = 1
    "butcher",       // VillagerProfession::Butcher = 2
    "cartographer",  // VillagerProfession::Cartographer = 3
    "cleric",        // VillagerProfession::Cleric = 4
    "farmer",        // VillagerProfession::Farmer = 5
    "fisherman",     // VillagerProfession::Fisherman = 6
    "fletcher",      // VillagerProfession::Fletcher = 7
    "leatherworker", // VillagerProfession::Leatherworker = 8
    "librarian",     // VillagerProfession::Librarian = 9
    "mason",         // VillagerProfession::Mason = 10
    "nitwit",        // VillagerProfession::Nitwit = 11
    "shepherd",      // VillagerProfession::Shepherd = 12
    "toolsmith",     // VillagerProfession::Toolsmith = 13
    "weaponsmith"    // VillagerProfession::Weaponsmith = 14
};

/// 村民等级徽章名称
inline const char* VILLAGER_LEVEL_NAMES[] = {
    "stone",   // 等级 1 - 新手 (Novice)
    "iron",    // 等级 2 - 学徒 (Apprentice)
    "gold",    // 等级 3 - 老手 (Journeyman)
    "emerald", // 等级 4 - 专家 (Expert)
    "diamond"  // 等级 5 - 大师 (Master)
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
 * 村民纹理由多层叠加组成：
 * 1. 基础纹理 (villager.png) - 由主渲染器渲染，包含身体和头部基础
 * 2. 类型层 (type/{type}.png) - 根据生物群系叠加不同外观
 * 3. 职业层 (profession/{profession}.png) - 根据职业叠加装备和服饰
 * 4. 等级徽章层 (profession_level/{badge}.png) - 显示交易等级徽章
 *
 * 渲染规则：
 * - 类型层：始终渲染（非隐身时）
 * - 职业层：职业 != NONE 且 非儿童 时渲染
 * - 等级徽章层：职业 != NONE 且 职业 != NITWIT 且 非儿童 时渲染
 *
 * 帽子可见性规则：
 * - 类型层帽子类型和职业层帽子类型共同决定是否显示基础帽子
 * - 如果职业帽子为 NONE 或 PARTIAL 且类型帽子为 FULL，则隐藏基础帽子
 *
 * 实现说明：
 * - 使用静态网格缓存，按纹理路径索引
 * - 支持运行时UV重映射到纹理图集区域
 * - 线程安全的网格缓存
 *
 * @tparam TEntity 村民实体类型 (VillagerEntity 或 ZombieVillagerEntity)
 * @tparam TModel 村民模型类型
 */
template <typename TEntity = ::mc::entity::VillagerEntity,
    typename TModel = ::mc::client::renderer::entity::model::animal::VillagerModel>
class VillagerLayer : public layer::core::LayerRenderer<TEntity> {
public:
    /**
     * @brief 构造函数
     * @param renderer 关联的渲染器
     * @param texturePrefix 纹理路径前缀 ("villager" 或 "zombie_villager")
     */
    explicit VillagerLayer(mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>& renderer,
        const std::string& texturePrefix = "villager")
        : m_renderer(&renderer)
        , m_texturePrefix(texturePrefix)
    {}

    ~VillagerLayer() override = default;

    /**
     * @brief 设置纹理图集
     *
     * 必须在渲染前调用，用于获取纹理UV区域。
     *
     * @param atlas 纹理图集指针
     */
    void setTextureAtlas(const pipeline::EntityTextureAtlas* atlas) { m_textureAtlas = atlas; }

    /**
     * @brief 渲染村民层（GPU管线路径）
     *
     * 实现多层纹理渲染：
     * 1. 类型层 - 始终渲染（生物群系外观）
     * 2. 职业层 - 职业不为NONE且非儿童时渲染
     * 3. 等级徽章层 - 职业不为NONE且不为NITWIT且非儿童时渲染
     *
     * 每层纹理通过UV重映射实现不同的纹理覆盖。
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
        const i32 level =
            std::clamp(data.level(), VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);

        // 判断渲染条件
        const bool isChild = _isChildEntity(entity);
        const bool shouldRenderProfession = (profession != ::mc::entity::VillagerProfession::None) && !isChild;
        const bool shouldRenderLevel =
            shouldRenderProfession && (profession != ::mc::entity::VillagerProfession::Nitwit);

        // 获取模型
        TModel* model = getParentModel();
        if (!model) {
            return;
        }

        // 计算模型矩阵
        std::array<f64, 16> modelMatrix = _computeModelMatrix(entity);

        // 获取实体位置
        Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

        // 获取受伤时间
        f32 hurtTime = 0.0f;
        f32 deathTime = 0.0f;
        if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
            hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;
            deathTime = static_cast<f32>(entity.deathTime());
        }

        Vector4f overlayColor(1.0f, 1.0f, 1.0f, 1.0f);
        const f64 scale = 1.0 / 16.0;

        // 渲染类型层（始终渲染）
        ResourceLocation typeTexture = getTypeTexture(type);
        if (pipeline::EntityMesh* typeMesh = _getOrCreateMeshForTexture(pipeline, *model, typeTexture)) {
            pipeline.drawMesh(cmd, *typeMesh, modelMatrix, entityPos, scale, overlayColor, hurtTime, deathTime);
        }

        // 渲染职业层
        if (shouldRenderProfession) {
            ResourceLocation professionTexture = getProfessionTexture(profession);
            if (pipeline::EntityMesh* professionMesh =
                    _getOrCreateMeshForTexture(pipeline, *model, professionTexture)) {
                pipeline.drawMesh(
                    cmd, *professionMesh, modelMatrix, entityPos, scale, overlayColor, hurtTime, deathTime);
            }
        }

        // 渲染等级徽章层
        if (shouldRenderLevel) {
            ResourceLocation levelTexture = getLevelTexture(level);
            if (pipeline::EntityMesh* levelMesh = _getOrCreateMeshForTexture(pipeline, *model, levelTexture)) {
                pipeline.drawMesh(cmd, *levelMesh, modelMatrix, entityPos, scale, overlayColor, hurtTime, deathTime);
            }
        }
    }

    /**
     * @brief 检查是否应该渲染村民层
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
            return _buildTexturePath(std::string("type/") + VillagerLayerDetail::VILLAGER_TYPE_NAMES[index]);
        }
        return _buildTexturePath("type/plains");
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
            return _buildTexturePath(
                std::string("profession/") + VillagerLayerDetail::VILLAGER_PROFESSION_NAMES[index]);
        }
        // MC 1.21+ 资源包中没有 none.png，无职业时不会调用此方法
        return _buildTexturePath("profession/nitwit");
    }

    /**
     * @brief 获取等级徽章纹理路径
     * @param level 村民等级 (1-5)
     * @return 纹理位置
     */
    [[nodiscard]] ResourceLocation getLevelTexture(i32 level) const
    {
        const i32 clampedLevel =
            std::clamp(level, VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);
        const i32 index = clampedLevel - VillagerLayerDetail::VILLAGER_MIN_LEVEL;
        return _buildTexturePath(std::string("profession_level/") + VillagerLayerDetail::VILLAGER_LEVEL_NAMES[index]);
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
        const i32 clampedLevel =
            std::clamp(level, VillagerLayerDetail::VILLAGER_MIN_LEVEL, VillagerLayerDetail::VILLAGER_MAX_LEVEL);
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
    [[nodiscard]] ResourceLocation _buildTexturePath(const std::string& subpath) const
    {
        return ResourceLocation("minecraft", "textures/entity/" + m_texturePrefix + "/" + subpath + ".png");
    }

    /**
     * @brief 检查实体是否为儿童
     */
    [[nodiscard]] bool _isChildEntity(const TEntity& entity) const
    {
        if constexpr (std::is_base_of_v<::mc::AgeableEntity, TEntity>) {
            return entity.isChild();
        }
        return false;
    }

    /**
     * @brief 计算模型矩阵
     */
    [[nodiscard]] std::array<f64, 16> _computeModelMatrix(const TEntity& entity) const
    {
        std::array<f64, 16> modelMatrix = {
            1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

        // 应用 Y 轴旋转（yaw）
        // 使用 entity.yaw() 方法（Entity 基类方法）
        f64 yaw = static_cast<f64>(entity.yaw());
        f64 yawRad = yaw * static_cast<f64>(math::DEG_TO_RAD);
        f64 cosYaw = std::cos(yawRad);
        f64 sinYaw = std::sin(yawRad);
        modelMatrix[0] = cosYaw;
        modelMatrix[2] = sinYaw;
        modelMatrix[8] = -sinYaw;
        modelMatrix[10] = cosYaw;

        // scale(-1, -1, 1) - X和Y都取反
        for (i32 i = 0; i < 4; ++i) {
            const auto rowOffset = static_cast<std::size_t>(i * 4);
            modelMatrix[rowOffset] *= -1.0;
            modelMatrix[rowOffset + 1] *= -1.0;
        }

        // Y偏移
        modelMatrix[13] = 1.501;

        return modelMatrix;
    }

    /**
     * @brief 获取或创建纹理对应的网格
     *
     * 从静态缓存获取网格，如果不存在则创建。
     * 网格按纹理路径缓存，支持多个实体共享同一纹理的网格。
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateMeshForTexture(
        pipeline::EntityPipeline& pipeline, TModel& model, const ResourceLocation& textureLoc)
    {
        const std::string textureKey = textureLoc.toString();

        // 先尝试从缓存读取（读锁）
        {
            std::shared_lock<std::shared_mutex> lock(s_meshCacheMutex);
            auto it = s_meshCache.find(textureKey);
            if (it != s_meshCache.end() && it->second && it->second->indexCount > 0) {
                return it->second.get();
            }
        }

        // 缓存未命中，需要创建（写锁）
        std::unique_lock<std::shared_mutex> lock(s_meshCacheMutex);

        // 双重检查
        auto it = s_meshCache.find(textureKey);
        if (it != s_meshCache.end() && it->second && it->second->indexCount > 0) {
            return it->second.get();
        }

        // 生成网格
        std::vector<model::ModelVertex> vertices;
        std::vector<u32> indices;
        model.generateMesh(vertices, indices, 1.0 / 16.0);

        if (vertices.empty() || indices.empty()) {
            spdlog::warn("VillagerLayer: Failed to generate mesh for texture {}", textureKey);
            return nullptr;
        }

        // 应用UV重映射
        if (m_textureAtlas && m_textureAtlas->isBuilt()) {
            const TextureRegion* region = m_textureAtlas->getRegion(textureLoc);
            if (region) {
                _remapUVs(vertices, *region);
            }
        }

        // 创建GPU网格
        auto result = pipeline.createMesh(vertices, indices);
        if (!result.success()) {
            spdlog::warn("VillagerLayer: Failed to create GPU mesh for texture {}", textureKey);
            return nullptr;
        }

        s_meshCache[textureKey] = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
        return s_meshCache[textureKey].get();
    }

    /**
     * @brief 重映射UV坐标到纹理图集区域
     *
     * 将模型局部UV坐标（0-1范围）映射到纹理图集中的目标区域。
     */
    static void _remapUVs(std::vector<model::ModelVertex>& vertices, const TextureRegion& region)
    {
        const f64 du = region.u1 - region.u0;
        const f64 dv = region.v1 - region.v0;

        for (auto& vertex : vertices) {
            // 假设原始UV在0-1范围内
            // 映射到纹理图集区域
            vertex.texCoord.x = static_cast<f32>(region.u0 + vertex.texCoord.x * du);
            vertex.texCoord.y = static_cast<f32>(region.v0 + vertex.texCoord.y * dv);
        }
    }

    mc::client::renderer::entity::core::IEntityRenderer<TEntity, TModel>* m_renderer = nullptr;
    std::string m_texturePrefix;                                  ///< "villager" 或 "zombie_villager"
    const pipeline::EntityTextureAtlas* m_textureAtlas = nullptr; ///< 纹理图集指针

    /// 静态网格缓存（按纹理路径索引）
    /// 使用 shared_ptr 以支持线程安全访问
    static std::unordered_map<std::string, std::unique_ptr<pipeline::EntityMesh>> s_meshCache;
    static std::shared_mutex s_meshCacheMutex;
};

// 静态成员定义
template <typename TEntity, typename TModel>
std::unordered_map<std::string, std::unique_ptr<pipeline::EntityMesh>> VillagerLayer<TEntity, TModel>::s_meshCache;

template <typename TEntity, typename TModel>
std::shared_mutex VillagerLayer<TEntity, TModel>::s_meshCacheMutex;

} // namespace mc::client::renderer::entity::layer::entity
