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

#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/manager/SkinManager.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace mc::client::skin {

/**
 * @brief 客户端皮肤管理器
 *
 * 扩展 SkinManager，添加：
 * - GPU 纹理管理
 * - EntityTextureAtlas 集成
 * - PlayerRenderer 纹理绑定
 *
 * 使用示例：
 * @code
 * ClientSkinManager skinManager;
 * skinManager.initialize(device, physicalDevice, commandPool, graphicsQueue);
 *
 * // 注册玩家皮肤
 * auto result = skinManager.registerPlayerSkin(profile);
 *
 * // 获取纹理区域
 * auto* region = skinManager.getSkinRegion(uuid);
 * renderer.setSkinTexture(region);
 * @endcode
 */
class ClientSkinManager {
public:
    ClientSkinManager();
    ~ClientSkinManager();

    // 禁止拷贝
    ClientSkinManager(const ClientSkinManager&) = delete;
    ClientSkinManager& operator=(const ClientSkinManager&) = delete;

    /**
     * @brief 初始化客户端皮肤管理器
     *
     * @param device Vulkan 设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @param cacheDir 缓存目录路径（如 ~/minecraft_reborn/cache/skins）
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
     * 加载皮肤并上传到 GPU。
     *
     * @param profile 玩家档案
     * @return 皮肤纹理 ResourceLocation
     */
    Result<ResourceLocation> registerPlayerSkin(const ::mc::skin::GameProfile& profile);

    /**
     * @brief 获取玩家皮肤纹理区域
     * @param uuid 玩家UUID
     * @return 纹理区域，不存在返回默认皮肤区域
     */
    [[nodiscard]] const TextureRegion* getSkinRegion(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 获取玩家披风纹理区域
     * @param uuid 玩家UUID
     * @return 纹理区域，无披风返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getCapeRegion(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 获取玩家鞘翅纹理区域
     * @param uuid 玩家UUID
     * @return 纹理区域，无鞘翅返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getElytraRegion(const std::array<u8, 16>& uuid) const;

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

    // ========== 默认皮肤 ==========

    /**
     * @brief 获取 Steve 皮肤纹理区域
     */
    [[nodiscard]] const TextureRegion* getSteveSkinRegion() const { return m_steveRegion; }

    /**
     * @brief 获取 Alex 皮肤纹理区域
     */
    [[nodiscard]] const TextureRegion* getAlexSkinRegion() const { return m_alexRegion; }

    // ========== 纹理图集 ==========

    /**
     * @brief 获取皮肤纹理图集
     */
    [[nodiscard]] const renderer::entity::pipeline::EntityTextureAtlas& textureAtlas() const { return *m_textureAtlas; }

    /**
     * @brief 获取可修改的纹理图集引用
     */
    renderer::entity::pipeline::EntityTextureAtlas& textureAtlas() { return *m_textureAtlas; }

    /**
     * @brief 检查纹理图集是否需要重建
     */
    [[nodiscard]] bool needsAtlasRebuild() const { return m_textureAtlas->needsRebuild(); }

    /**
     * @brief 重建纹理图集
     *
     * 在新皮肤添加后调用，重新打包所有皮肤纹理。
     * 注意：此操作可能较慢，应在合适的时机调用。
     *
     * @return 成功或错误
     */
    Result<void> rebuildAtlas();

    // ========== 底层管理器访问 ==========

    /**
     * @brief 获取底层皮肤管理器
     */
    [[nodiscard]] ::mc::skin::SkinManager& skinManager() { return *m_skinManager; }
    [[nodiscard]] const ::mc::skin::SkinManager& skinManager() const { return *m_skinManager; }

    // ========== 资源包设置 ==========

    /**
     * @brief 设置资源包（用于加载默认皮肤）
     */
    void setResourcePack(IResourcePack* resourcePack) { m_skinManager->setResourcePack(resourcePack); }

private:
    /**
     * @brief 加载默认皮肤到图集
     */
    Result<void> loadDefaultSkins();

    /**
     * @brief 上传皮肤 PNG 数据到图集
     */
    Result<ResourceLocation> uploadSkinToAtlas(
        const std::vector<u8>& pngData, const ResourceLocation& preferredLocation);

    /**
     * @brief UUID 转字符串键
     */
    [[nodiscard]] static std::string uuidToKey(const std::array<u8, 16>& uuid);

    std::unique_ptr<::mc::skin::SkinManager> m_skinManager;
    std::unique_ptr<renderer::entity::pipeline::EntityTextureAtlas> m_textureAtlas;

    // 默认皮肤区域
    const TextureRegion* m_steveRegion = nullptr;
    const TextureRegion* m_alexRegion = nullptr;

    // UUID -> 纹理区域映射
    mutable std::mutex m_regionMutex;
    std::unordered_map<std::string, const TextureRegion*> m_skinRegions;
    std::unordered_map<std::string, const TextureRegion*> m_capeRegions;
    std::unordered_map<std::string, const TextureRegion*> m_elytraRegions;

    // 待重建的皮肤数据
    std::vector<std::pair<ResourceLocation, std::vector<u8>>> m_pendingSkins;
    mutable std::mutex m_pendingMutex;

    VkDevice m_device = VK_NULL_HANDLE;
    bool m_initialized = false;
};

} // namespace mc::client::skin
