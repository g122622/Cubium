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

#include "client/renderer/trident/entity/core/AnimatedMeshCache.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace mc {
class Entity;
class Item;
class ItemStack;
struct TextureRegion;
} // namespace mc

namespace mc::client {
class ClientEntity;
}

namespace mc::client::renderer::entity {

/**
 * @brief 实体渲染器管理器
 *
 * 管理所有实体渲染器，根据实体类型分派渲染。
 * 集成EntityPipeline进行Vulkan渲染。
 *
 * 参考 MC 1.16.5 EntityRendererManager
 */
class EntityRendererManager {
public:
    using RendererCreator = std::function<std::unique_ptr<core::EntityRenderer>()>;

    EntityRendererManager();
    ~EntityRendererManager();

    // 禁止拷贝
    EntityRendererManager(const EntityRendererManager&) = delete;
    EntityRendererManager& operator=(const EntityRendererManager&) = delete;

    // ========== 渲染器管理 ==========

    /**
     * @brief 注册渲染器
     * @param typeId 实体类型ID
     * @param creator 渲染器创建函数
     */
    void registerRenderer(const std::string& typeId, RendererCreator creator);

    /**
     * @brief 获取渲染器
     * @param typeId 实体类型ID
     * @return 渲染器指针，如果未注册返回nullptr
     */
    [[nodiscard]] core::EntityRenderer* getRenderer(const std::string& typeId);

    // ========== 实体网格缓存 ==========

    /**
     * @brief 获取或创建实体网格（静态网格）
     * @param entity 客户端实体
     * @return 网格指针，如果实体类型无渲染器返回nullptr
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateMesh(ClientEntity& entity);

    /**
     * @brief 获取或创建动画实体网格
     *
     * 使用动画缓存，根据动画状态决定是否需要重新生成网格。
     * 这是主要的动画渲染路径。
     *
     * @param entity 客户端实体
     * @param model 已设置动画角度的模型
     * @param context 动画上下文
     * @return 网格指针，如果实体类型无渲染器返回nullptr
     */
    [[nodiscard]] pipeline::EntityMesh* getOrCreateAnimatedMesh(
        ClientEntity& entity, model::EntityModel& model, const core::AnimationContext& context);

    /**
     * @brief 更新实体网格
     *
     * 当实体动画变化时调用，重新生成网格。
     *
     * @param entity 客户端实体
     */
    void updateMesh(ClientEntity& entity);

    /**
     * @brief 移除实体网格
     * @param entityId 实体ID
     */
    void removeMesh(EntityId entityId);

    /**
     * @brief 清除所有实体网格
     */
    void clearMeshes();

    /**
     * @brief 清除动画网格缓存
     */
    void clearAnimatedMeshes();

    // ========== 渲染 ==========

    /**
     * @brief 渲染实体
     * @param entity 要渲染的实体
     * @param partialTicks 部分tick
     * @deprecated 使用 renderWithPipeline 代替
     */
    void render(Entity& entity, f64 partialTicks);

    /**
     * @brief 使用管线渲染实体
     * @param cmd 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     */
    void renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks);

    /**
     * @brief 使用管线渲染实体（带视锥剔除）
     *
     * 首先检查实体的包围盒是否在视锥内，如果不在则跳过渲染。
     *
     * @param cmd 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     * @param frustum 视锥体（用于剔除）
     * @return true 如果实体被渲染，false 如果被剔除
     */
    bool renderWithPipeline(
        VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks, const mc::math::frustum::Frustum& frustum);

    // ========== 管线 ==========

    /**
     * @brief 设置实体渲染管线
     */
    void setPipeline(pipeline::EntityPipeline* pipeline) { m_pipeline = pipeline; }

    /**
     * @brief 设置实体纹理图集（用于UV重映射）
     */
    void setTextureAtlas(const pipeline::EntityTextureAtlas* textureAtlas);

    /**
     * @brief 设置物品纹理图集（用于 ItemEntity 渲染）
     */
    void setItemTextureAtlas(pipeline::EntityTextureAtlas* itemAtlas) { m_itemTextureAtlas = itemAtlas; }

    /**
     * @brief 获取物品纹理图集
     */
    [[nodiscard]] pipeline::EntityTextureAtlas* itemTextureAtlas() { return m_itemTextureAtlas; }

    /**
     * @brief 设置相机描述符集
     * @param descriptorSet 相机描述符集（set = 0）
     */
    void setCameraDescriptorSet(VkDescriptorSet descriptorSet) { m_cameraDescriptorSet = descriptorSet; }

    /**
     * @brief 设置相机信息（用于名称标签渲染）
     *
     * 必须在每帧渲染实体前调用，以便名称标签渲染器进行视锥剔除和背面剔除。
     *
     * @param position 相机世界位置
     * @param viewMatrix 视图矩阵
     * @param frustum 视锥体
     */
    void setCameraInfo(
        const glm::dvec3& position, const glm::mat4& viewMatrix, const mc::math::frustum::Frustum& frustum);

    /**
     * @brief 获取实体渲染管线
     */
    [[nodiscard]] pipeline::EntityPipeline* pipeline() { return m_pipeline; }

    // ========== 渲染设置 ==========

    /**
     * @brief 设置是否渲染阴影
     */
    void setRenderShadows(bool render) { m_renderShadows = render; }

    /**
     * @brief 获取是否渲染阴影
     */
    [[nodiscard]] bool renderShadows() const { return m_renderShadows; }

    /**
     * @brief 设置是否渲染名称标签
     */
    void setRenderNameTags(bool render) { m_renderNameTags = render; }

    /**
     * @brief 获取是否渲染名称标签
     */
    [[nodiscard]] bool renderNameTags() const { return m_renderNameTags; }

    // ========== 初始化 ==========

    /**
     * @brief 初始化默认渲染器
     */
    void initializeDefaults();

private:
    std::unordered_map<std::string, std::unique_ptr<core::EntityRenderer>> m_renderers;
    std::unordered_map<std::string, RendererCreator> m_creators;

    // 静态实体网格缓存（用于非动画实体，如 ItemEntity、ExperienceOrb）
    std::unordered_map<EntityId, pipeline::EntityMesh> m_meshes;

    // 动画实体网格缓存（用于动画实体）
    std::unique_ptr<core::AnimatedMeshCache> m_animatedMeshCache;

    // 管线
    pipeline::EntityPipeline* m_pipeline = nullptr;
    const pipeline::EntityTextureAtlas* m_textureAtlas = nullptr;
    pipeline::EntityTextureAtlas* m_itemTextureAtlas = nullptr; // 用于 ItemEntity 渲染

    // 相机描述符集（set = 0）
    VkDescriptorSet m_cameraDescriptorSet = VK_NULL_HANDLE;

    // 相机信息（用于名称标签渲染）
    glm::dvec3 m_cameraPosition{0.0, 0.0, 0.0};
    glm::mat4 m_viewMatrix{1.0f};
    mc::math::frustum::Frustum m_frustum;
    bool m_hasCameraInfo = false;

    bool m_renderShadows = true;
    bool m_renderNameTags = true;

    /**
     * @brief 创建或获取渲染器
     */
    [[nodiscard]] core::EntityRenderer* getOrCreateRenderer(const std::string& typeId);

    /**
     * @brief 生成实体模型网格
     * @param typeId 实体类型ID
     * @param vertices 输出顶点
     * @param indices 输出索引
     * @return 是否成功生成
     */
    bool generateModelMesh(
        const std::string& typeId, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 生成 ItemEntity 的网格
     *
     * ItemEntity 使用简单的四边形网格来显示物品图标
     *
     * @param vertices 输出顶点
     * @param indices 输出索引
     */
    void generateItemEntityMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 生成 ExperienceOrb 的网格
     *
     * ExperienceOrb 使用简单的四边形网格（billboard 方式）
     *
     * @param vertices 输出顶点
     * @param indices 输出索引
     */
    void generateExperienceOrbMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 将 ItemEntity 的 UV 映射到物品纹理图集
     *
     * 根据 ItemStack 中的物品获取纹理区域，重映射 UV 坐标
     *
     * @param entity 客户端实体
     * @param vertices 顶点数据（会被修改）
     */
    void remapItemEntityUv(ClientEntity& entity, std::vector<model::ModelVertex>& vertices);

    /**
     * @brief 将模型局部UV映射到图集区域
     */
    void remapUvToAtlasRegion(const std::string& normalizedTypeId, std::vector<model::ModelVertex>& vertices) const;

    /**
     * @brief 计算 ItemEntity 浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f64 calculateItemBobOffset(const ClientEntity& entity, f64 partialTick) const;

    /**
     * @brief 计算 ItemEntity 旋转角度
     * @param entity 客户端物品实体
     * @param partialTick 部分 tick
     * @return 旋转角度（度）
     */
    [[nodiscard]] f64 calculateItemRotation(const ClientEntity& entity, f64 partialTick) const;

    /**
     * @brief 计算 ExperienceOrb 浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f64 calculateExperienceOrbBobOffset(u32 ticksExisted, f64 partialTick) const;

    /**
     * @brief 判断实体是否使用动画网格
     *
     * ItemEntity 和 ExperienceOrb 使用静态网格，
     * 其他实体使用动画网格。
     */
    [[nodiscard]] bool usesAnimatedMesh(const std::string& normalizedTypeId) const;

    /**
     * @brief 为实体创建模型并设置动画
     *
     * 根据实体类型创建模型，设置动画参数，返回模型引用。
     * 调用者需要在使用后销毁模型。
     *
     * @param entity 客户端实体
     * @param context 动画上下文（输出）
     * @return 模型指针，如果实体类型无模型返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<model::EntityModel> createModelForEntity(
        ClientEntity& entity, core::AnimationContext& context);
};

} // namespace mc::client::renderer::entity
