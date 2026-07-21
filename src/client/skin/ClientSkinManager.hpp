/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
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

#include "SkinTextureUploader.hpp"
#include "client/renderer/trident/entity/core/PlayerSkinRegionProvider.hpp"
#include "client/world/player/PlayerIdentityRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/manager/SkinManager.hpp"
#include <array>
#include <memory>
#include <vector>

namespace mc::client::skin {

/**
 * @brief 客户端皮肤管理器（薄 facade + PlayerSkinRegionProvider 实现）
 *
 * 职责聚合（每个职责由独立服务承担，本类仅编排）：
 * - 玩家档案/缓存/异步加载：common 层 SkinManager（m_skinManager）
 * - 默认皮肤像素/选型：common 层 DefaultSkinProvider（m_skinManager 持有）
 * - GPU 纹理上传到实体图集：SkinTextureUploader（m_uploader）
 * - UUID↔entityId 映射：PlayerIdentityRegistry（注入，m_identityRegistry）
 *
 * 关键架构修复：不再自建孤儿 EntityTextureAtlas。皮肤纹理统一上传到渲染器唯一的
 * 实体纹理图集（由 setTextureAtlas 注入），PlayerRenderer/AnimatedMeshCache 从同一
 * 图集采样，消除"两个图集"导致的彩色乱码。
 *
 * 实现 PlayerSkinRegionProvider：EntityRendererManager 的 UvRemapFunc 玩家分支调用
 * getSkinRegionForEntity(entityId)，本类按 entityId→UUID→皮肤区域解析：
 *   1. PlayerIdentityRegistry.uuidOf(entityId) 取 UUID
 *   2. 若 PlayerSkinInfo 有自定义皮肤像素 → SkinTextureUploader.getOrCreateRegion 懒上传
 *   3. 否则回退 SkinTextureUploader.getDefaultRegion(uuid)（loadDefaultSkins 时已注入）
 *   4. 非 playerId/皮肤未就绪 → nullptr（调用方回退默认实体纹理路径）
 */
class ClientSkinManager : public renderer::entity::core::PlayerSkinRegionProvider {
public:
    ClientSkinManager();
    ~ClientSkinManager() override;

    // 禁止拷贝
    ClientSkinManager(const ClientSkinManager&) = delete;
    ClientSkinManager& operator=(const ClientSkinManager&) = delete;

    /**
     * @brief 注入渲染器唯一的实体纹理图集
     *
     * 必须在 initialize() 之前调用。图集须已 build()。图集生命周期由 TridentEngine 管理。
     */
    void setTextureAtlas(renderer::entity::pipeline::EntityTextureAtlas* atlas) { m_uploader.setTextureAtlas(atlas); }

    /**
     * @brief 注入玩家身份注册表（UUID↔entityId 映射）
     *
     * 必须在 getSkinRegionForEntity 被调用前注入。生命周期由 ClientApplicationSession 管理。
     */
    void setIdentityRegistry(PlayerIdentityRegistry* registry) { m_identityRegistry = registry; }

    /**
     * @brief 初始化客户端皮肤管理器
     *
     * 重建底层 SkinManager（用正确 cacheDir），重新下发缓存的资源包/线程池，
     * 加载 18 种默认皮肤像素并上传到注入的实体图集动态区域。
     *
     * @param device Vulkan 设备（保留参数，未来基于 device 的纹理管理扩展用）
     * @param physicalDevice 物理设备（未使用，保留对称性）
     * @param commandPool 命令池（未使用，injectRegion 内部自管 staging）
     * @param graphicsQueue 图形队列（未使用）
     * @param cacheDir 皮肤缓存目录路径
     * @return 成功或错误
     */
    Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::string& cacheDir);

    /**
     * @brief 关闭客户端皮肤管理器
     */
    void shutdown();

    // ========== 玩家皮肤管理 ==========

    /**
     * @brief 注册玩家皮肤
     *
     * 获取或创建 PlayerSkinInfo，触发异步加载。自定义皮肤像素就绪后由
     * getSkinRegionForEntity 懒上传到图集。
     *
     * @param profile 玩家档案
     * @return 皮肤纹理 ResourceLocation（默认皮肤 location 或自定义 location）
     */
    Result<ResourceLocation> registerPlayerSkin(const ::mc::skin::GameProfile& profile);

    /**
     * @brief 获取玩家皮肤类型
     * @param uuid 玩家UUID
     * @return 皮肤类型
     */
    [[nodiscard]] ::mc::skin::SkinType getSkinType(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 获取玩家皮肤信息
     */
    [[nodiscard]] std::shared_ptr<::mc::skin::PlayerSkinInfo> getPlayerInfo(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 移除玩家皮肤信息（玩家离线）
     *
     * 联动 SkinTextureUploader.removeRegion，同步释放图集动态区域（修复旧实现
     * removePlayerInfo 不同步 region 的 bug）。
     */
    void removePlayerInfo(const std::array<u8, 16>& uuid);

    // ========== PlayerSkinRegionProvider 实现 ==========

    /**
     * @brief 按 entityId 查询玩家皮肤区域（渲染层调用入口）
     *
     * 解析链：entityId → UUID →（自定义皮肤像素懒上传 or 默认皮肤区域）。
     * 非玩家或皮肤未就绪返回 nullptr。
     */
    [[nodiscard]] const TextureRegion* getSkinRegionForEntity(EntityInstanceId entityId) const override;

    // ========== 资源包设置 ==========

    /**
     * @brief 设置资源包列表（用于加载默认皮肤及资源包内皮肤）
     *
     * 必须在 initialize() 之前调用。内部会缓存列表，因为 initialize() 会用
     * 正确的 cacheDir 重建底层 SkinManager，重建后会重新下发缓存的列表。
     * 列表顺序为添加顺序（低→高优先级），查找时反向遍历（后添加的优先）。
     */
    void setResourcePacks(std::vector<::mc::IResourcePack*> resourcePacks)
    {
        m_resourcePacks = std::move(resourcePacks);
        m_skinManager->setResourcePacks(m_resourcePacks);
    }

    /**
     * @brief 注入工作线程池用于异步皮肤加载
     *
     * 必须在 initialize() 之前调用，线程池由调用方拥有。同样会被缓存，
     * 在 initialize() 重建底层 SkinManager 后重新下发。
     *
     * @param workerPool 工作线程池指针（非所有权）
     */
    void setWorkerPool(::mc::util::ServerWorkerPool* workerPool)
    {
        m_workerPool = workerPool;
        m_skinManager->setWorkerPool(workerPool);
    }

private:
    /**
     * @brief 上传 18 种默认皮肤变体到图集动态区域
     */
    void _uploadDefaultSkins();

    std::unique_ptr<::mc::skin::SkinManager> m_skinManager;
    SkinTextureUploader m_uploader;
    PlayerIdentityRegistry* m_identityRegistry = nullptr;

    // initialize() 会用正确的 cacheDir 重建 m_skinManager，这里缓存注入的列表以便重建后重新下发
    std::vector<::mc::IResourcePack*> m_resourcePacks;
    ::mc::util::ServerWorkerPool* m_workerPool = nullptr;

    bool m_initialized = false;
};

} // namespace mc::client::skin
