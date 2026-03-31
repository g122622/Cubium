#pragma once

#include "EntityRenderer.hpp"
#include "EntityPipeline.hpp"
#include "model/EntityModel.hpp"
#include "../../../../common/core/Types.hpp"
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>

namespace mc {
class Entity;
class Item;
class ItemStack;
struct TextureRegion;
}

namespace mc::client {

// 前向声明
class ClientEntity;
class EntityTextureAtlas;
class ItemTextureAtlas;

namespace renderer {

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
    using RendererCreator = std::function<std::unique_ptr<EntityRenderer>()>;

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
    void registerRenderer(const String& typeId, RendererCreator creator);

    /**
     * @brief 获取渲染器
     * @param typeId 实体类型ID
     * @return 渲染器指针，如果未注册返回nullptr
     */
    [[nodiscard]] EntityRenderer* getRenderer(const String& typeId);

    // ========== 实体网格缓存 ==========

    /**
     * @brief 获取或创建实体网格
     * @param entity 客户端实体
     * @return 网格指针，如果实体类型无渲染器返回nullptr
     */
    [[nodiscard]] EntityMesh* getOrCreateMesh(ClientEntity& entity);

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

    // ========== 渲染 ==========

    /**
     * @brief 渲染实体
     * @param entity 要渲染的实体
     * @param partialTicks 部分tick
     * @deprecated 使用 renderWithPipeline 代替
     */
    void render(Entity& entity, f32 partialTicks);

    /**
     * @brief 使用管线渲染实体
     * @param cmd 命令缓冲区
     * @param entity 客户端实体
     * @param partialTicks 部分tick
     */
    void renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f32 partialTicks);

    // ========== 管线 ==========

    /**
     * @brief 设置实体渲染管线
     */
    void setPipeline(EntityPipeline* pipeline) { m_pipeline = pipeline; }

    /**
     * @brief 设置实体纹理图集（用于UV重映射）
     */
    void setTextureAtlas(const EntityTextureAtlas* textureAtlas);

    /**
     * @brief 设置物品纹理图集（用于 ItemEntity 渲染）
     */
    void setItemTextureAtlas(ItemTextureAtlas* itemAtlas) { m_itemTextureAtlas = itemAtlas; }

    /**
     * @brief 获取物品纹理图集
     */
    [[nodiscard]] ItemTextureAtlas* itemTextureAtlas() { return m_itemTextureAtlas; }

    /**
     * @brief 设置相机描述符集
     * @param descriptorSet 相机描述符集（set = 0）
     */
    void setCameraDescriptorSet(VkDescriptorSet descriptorSet) { m_cameraDescriptorSet = descriptorSet; }

    /**
     * @brief 获取实体渲染管线
     */
    [[nodiscard]] EntityPipeline* pipeline() { return m_pipeline; }

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
    std::unordered_map<String, std::unique_ptr<EntityRenderer>> m_renderers;
    std::unordered_map<String, RendererCreator> m_creators;

    // 实体网格缓存
    std::unordered_map<EntityId, EntityMesh> m_meshes;

    // 管线
    EntityPipeline* m_pipeline = nullptr;
    const EntityTextureAtlas* m_textureAtlas = nullptr;
    ItemTextureAtlas* m_itemTextureAtlas = nullptr;  // 用于 ItemEntity 渲染

    // 相机描述符集（set = 0）
    VkDescriptorSet m_cameraDescriptorSet = VK_NULL_HANDLE;

    bool m_renderShadows = true;
    bool m_renderNameTags = true;

    /**
     * @brief 创建或获取渲染器
     */
    [[nodiscard]] EntityRenderer* getOrCreateRenderer(const String& typeId);

    /**
     * @brief 生成实体模型网格
     * @param typeId 实体类型ID
     * @param vertices 输出顶点
     * @param indices 输出索引
     * @return 是否成功生成
     */
    bool generateModelMesh(const String& typeId,
                           std::vector<ModelVertex>& vertices,
                           std::vector<u32>& indices);

    /**
     * @brief 生成 ItemEntity 的网格
     *
     * ItemEntity 使用简单的四边形网格来显示物品图标
     *
     * @param vertices 输出顶点
     * @param indices 输出索引
     */
    void generateItemEntityMesh(std::vector<ModelVertex>& vertices,
                                std::vector<u32>& indices);

    /**
     * @brief 生成 ExperienceOrb 的网格
     *
     * ExperienceOrb 使用简单的四边形网格（billboard 方式）
     *
     * @param vertices 输出顶点
     * @param indices 输出索引
     */
    void generateExperienceOrbMesh(std::vector<ModelVertex>& vertices,
                                   std::vector<u32>& indices);

    /**
     * @brief 将 ItemEntity 的 UV 映射到物品纹理图集
     *
     * 根据 ItemStack 中的物品获取纹理区域，重映射 UV 坐标
     *
     * @param entity 客户端实体
     * @param vertices 顶点数据（会被修改）
     */
    void remapItemEntityUv(ClientEntity& entity, std::vector<ModelVertex>& vertices);

    /**
     * @brief 将模型局部UV映射到图集区域
     */
    void remapUvToAtlasRegion(const String& normalizedTypeId,
                              std::vector<ModelVertex>& vertices) const;

    /**
     * @brief 计算 ItemEntity 浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f32 calculateItemBobOffset(u32 ticksExisted, f32 partialTick) const;

    /**
     * @brief 计算 ItemEntity 旋转角度
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return 旋转角度（度）
     */
    [[nodiscard]] f32 calculateItemRotation(u32 ticksExisted, f32 partialTick) const;

    /**
     * @brief 计算 ExperienceOrb 浮动偏移
     * @param ticksExisted 实体存活时间
     * @param partialTick 部分 tick
     * @return Y 轴偏移
     */
    [[nodiscard]] f32 calculateExperienceOrbBobOffset(u32 ticksExisted, f32 partialTick) const;
};

} // namespace renderer
} // namespace mc::client
